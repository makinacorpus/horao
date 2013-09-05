#include "PostGisUtils.h"
#include "StringUtils.h"

#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgUtil/Optimizer>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>

#include <gdal/gdal_priv.h>
#include <gdal/cpl_conv.h>

#define DEBUG_OUT if (0) std::cerr

// for GDAL RAII
struct Dataset
{
    Dataset( const std::string & file )
       : _raster( (GDALDataset *) GDALOpen( file.c_str(), GA_ReadOnly ) )
    {}

    GDALDataset * operator->(){ return _raster; }
    operator bool(){ return _raster;}

    ~Dataset()
    {
        if (_raster) GDALClose( _raster );
    }
private:
    GDALDataset * _raster;
};

// for RAII of connection
struct PostgisConnection
{

    PostgisConnection( const std::string & connInfo )
        : _conn( PQconnectdb(connInfo.c_str()) )
    {}

    operator bool(){ return CONNECTION_OK == PQstatus( _conn );  }

    ~PostgisConnection()
    {
        if (_conn) PQfinish( _conn );
    }

    // for RAII ok query results
    struct QueryResult
    {
        QueryResult( PostgisConnection & conn, const std::string & query )
            : _res( PQexec( conn._conn, query.c_str() ) )
            , _error( PQresultErrorMessage(_res) )
        {}

        ~QueryResult() 
        { 
            PQclear(_res);
        }

        operator bool() const { return _error.empty(); }

        PGresult * get(){ return _res; }

        const std::string & error() const { return _error; }

    private:
        PGresult * _res;
        const std::string _error;
        // non copyable
        QueryResult( const QueryResult & );
        QueryResult operator=( const QueryResult &); 
    };

private:
   PGconn * _conn;
};

void MyErrorHandler(CPLErr , int /*err_no*/, const char *msg)
{
    ERROR << "from GDAL:" << msg << "\n";
}

struct ReaderWriterPOSTGIS : osgDB::ReaderWriter
{
    ReaderWriterPOSTGIS()
    {
        GDALAllRegister();
        CPLSetErrorHandler( MyErrorHandler );	
        supportsExtension( "postgis", "PostGIS feature loader" );
        supportsExtension( "postgisd", "PostGIS feature loader" );
    }

    const char* className() const
    {
        return "ReaderWriterPOSTGIS";
    }

    ReadResult readNode(std::istream&, const Options*) const
    {
        return ReadResult::NOT_IMPLEMENTED;
    }

    //! @note stupid key="value" parser, value must not contain '"'  
    ReadResult readNode(const std::string& file_name, const Options* ) const
    {
        if ( !acceptsExtension(osgDB::getLowerCaseFileExtension( file_name )))
            return ReadResult::FILE_NOT_HANDLED;

        DEBUG_OUT << "loaded plugin postgis for [" << file_name << "]\n";

        osg::Timer timer;

        DEBUG_OUT << "connecting to postgis...\n";
        timer.setStartTick();

        typedef std::map< std::string, std::string > AttributeMap;
        AttributeMap am;
        std::stringstream line(file_name);
        std::string key, value;
        while (    std::getline( line, key, '=' ) 
                && std::getline( line, value, '"' ) 
                && std::getline( line, value, '"' )){
            // remove spaces in key
            key.erase( remove_if(key.begin(), key.end(), isspace ), key.end());
            value = unescapeXMLString(value);
            DEBUG_OUT << "key=\"" << key << "\" value=\"" << value << "\"\n";
            am.insert( std::make_pair( key, value ) );
        }

        PostgisConnection conn( am["conn_info"] );
        if (!conn){
            std::cerr << "failed to open database with conn_info=\"" << am["conn_info"] << "\"\n";
            return ReadResult::FILE_NOT_FOUND;
        }

        DEBUG_OUT << "connected in " <<  timer.time_s() << "sec\n";

        DEBUG_OUT << "execute request...\n";
        timer.setStartTick();

        PostgisConnection::QueryResult res( conn, am["query"].c_str() );
        if (!res){
            std::cerr << "failed to execute query=\"" <<  am["query"] << "\" : " << res.error() << "\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }

        const int numFeatures = PQntuples( res.get() );
        DEBUG_OUT << "got " << numFeatures << " features in " << timer.time_s() << "sec\n";

        DEBUG_OUT << "converting " << numFeatures << " features from postgis...\n";
        timer.setStartTick();

        // define transfo  layerToWord
        osg::Matrixd layerToWord;
        osg::Vec3d origin;
        {
            if ( !( std::stringstream( am["origin"] ) >> origin.x() >> origin.y() >> origin.z() ) ){
                std::cerr << "failed to obtain origin=\""<< am["origin"] <<"\"\n";
                return ReadResult::ERROR_IN_READING_FILE;
            }
            layerToWord.makeTranslate( -origin );
        }

        osg::ref_ptr<osg::Geode> group = new osg::Geode();

        std::string geocolumn( "geom" );
        if ( am.find("geocolumn") != am.end() ) {
            geocolumn = am["geocolumn"];
        }

        const int geomIdx   = PQfnumber(res.get(),  geocolumn.c_str() );

        const int posIdx    = PQfnumber(res.get(),  "pos" );
        const int heightIdx = PQfnumber(res.get(),  "height" );
        const int widthIdx  = PQfnumber(res.get(),  "width");

        Stack3d::Viewer::TriangleMesh mesh( layerToWord );

        if (geomIdx >= 0){ // we have a geom column, we create the model from it 
            for( int i=0; i<numFeatures; i++ ) {
                const char * wkb = PQgetvalue( res.get(), i, geomIdx );
                Stack3d::Viewer::Lwgeom lwgeom( wkb, Stack3d::Viewer::Lwgeom::WKB() );
                assert( lwgeom.get() );
                mesh.push_back( lwgeom.get() );
            }
        }
        else if ( posIdx >= 0 && heightIdx >= 0 && widthIdx >=0 ){ // we draw bars instead of geom
            for( int i=0; i<numFeatures; i++ ) {
                const char * wkb = PQgetvalue( res.get(), i, posIdx );
                Stack3d::Viewer::Lwgeom lwgeom( wkb, Stack3d::Viewer::Lwgeom::WKB() );
                assert( lwgeom.get() );
                LWPOINT * lwpoint = lwgeom_as_lwpoint( lwgeom.get() );
                if( !lwpoint ){
                    std::cerr << "failed to get points from column 'pos'\n";
                    return ReadResult::ERROR_IN_READING_FILE;
                }

                const POINT3DZ p = getPoint3dz( lwpoint->point, 0 );
                const float h = atof( PQgetvalue( res.get(), i, heightIdx ) );
                const float w = atof( PQgetvalue( res.get(), i, widthIdx ) );

                mesh.addBar( osg::Vec3(p.x, p.y, p.z + h/2), w, w, h);
            }

/*
            osg::Node * cube = osgDB::readNodeFile("cube.obj");
            assert(cube->asGroup());
            for( int i=0; i<numFeatures; i++ ) {
                const char * wkb = PQgetvalue( res.get(), i, posIdx );
                Stack3d::Viewer::Lwgeom lwgeom( wkb, Stack3d::Viewer::Lwgeom::WKB() );
                assert( lwgeom.get() );
                LWPOINT * lwpoint = lwgeom_as_lwpoint( lwgeom.get() );
                if( !lwpoint ){
                    std::cerr << "failed to get points from column 'pos'\n";
                    return ReadResult::ERROR_IN_READING_FILE;
                }

                const POINT3DZ p = getPoint3dz( lwpoint->point, 0 );
                const float h = atof( PQgetvalue( res.get(), i, heightIdx ) );
                //const float w = atof( PQgetvalue( res.get(), i, widthIdx ) );


                osg::Matrix move;
                move.makeTranslate( osg::Vec3(p.x, p.y, p.z + h)*layerToWord );
            }
*/
        } 
        else {
            std::cerr << "cannot find either 'geom' column or 'height','width' columns\n"; 
            return ReadResult::ERROR_IN_READING_FILE;
        }

        if (!am["elevation"].empty()) {
            Dataset raster( am["elevation"].c_str() );
            double transform[6];
            raster->GetGeoTransform( transform );
            
            // assume square pixels
            assert( std::abs(transform[4]) < FLT_EPSILON );
            assert( std::abs(transform[2]) < FLT_EPSILON );

            const double originX = transform[0];
            const double originY = transform[3];
            
            const double pixelPerMetreX =  1.f/transform[1];
            const double pixelPerMetreY = -1.f/transform[5]; // image is top->bottom
            GDALRasterBand * band = raster->GetRasterBand( 1 );
            GDALDataType dType = band->GetRasterDataType();
            const int dSizeBits = GDALGetDataTypeSize( dType );
            std::vector<char> buffer( dSizeBits / 8  );
            char* blockData = &buffer[0];

            double dataOffset;
            int ok;
            dataOffset = band->GetOffset( &ok );
            if ( ! ok ) {
                dataOffset = 0.0;
            }
            const double dataScale = band->GetScale( &ok );
            assert(ok);
            for ( std::vector<osg::Vec3>::iterator v = mesh.begin(); v!=mesh.end(); v++){
               band->RasterIO( GF_Read, int( ( v->x() + origin.x() - originX )*pixelPerMetreX), 
                                        int( ( originY - v->y() - origin.y() )*pixelPerMetreY), 
                                        1, 1, blockData, 1, 1, dType, 0, 0 ); 
               v->z() = float( (SRCVAL(blockData, dType, 0) * dataScale)  + dataOffset ) - origin.z();
            }

            
        }


        group->addDrawable(mesh.createGeometry());



        DEBUG_OUT << "converted " << numFeatures << " features in " << timer.time_s() << "sec\n";

        return group.release();
    }
};

REGISTER_OSGPLUGIN(postgis, ReaderWriterPOSTGIS)


