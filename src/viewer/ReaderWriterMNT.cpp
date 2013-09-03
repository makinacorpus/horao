#include "StringUtils.h"

#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgUtil/Optimizer>
#include <osgTerrain/Terrain>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <map>
#include <cassert>

#include <gdal/gdal_priv.h>
#include <gdal/cpl_conv.h>

#define DEBUG_OUT if (1) std::cout
#define ERROR (std::cerr << "error: ")

void MyErrorHandler(CPLErr , int /*err_no*/, const char *msg)
{
    ERROR << "from GDAL:" << msg << "\n";
}

struct ReaderWriterMNT : osgDB::ReaderWriter
{

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

    ReaderWriterMNT()
    {
        GDALAllRegister();
        CPLSetErrorHandler( MyErrorHandler );	

        supportsExtension( "mnt", "MNT tif loader" );
        supportsExtension( "mntd", "MNT tif loader" );
        DEBUG_OUT << "ctor of ReaderWriterMNT\n";
    }

    const char* className() const
    {
        return "ReaderWriterMNT";
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

        DEBUG_OUT << "loaded plugin mnt for [" << file_name << "]\n";

        osg::Timer timer;

        DEBUG_OUT << "loading...\n";
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

        // define transfo  layerToWord
        //osg::Matrixd layerToWord;
        //{
        osg::Vec3d origin;
        if ( !( std::stringstream( am["origin"] ) >> origin.x() >> origin.y() >> origin.z() ) ){
            ERROR << "failed to obtain origin=\"" << am["origin"] <<"\"\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }
        //layerToWord.makeTranslate( -origin );
        //}

        double xmin, ymin, xmax, ymax;
        std::stringstream ext( am["extent"] );
        std::string l;
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            ERROR << "cannot parse extent=\"" << am["extent"] << "\"\n";;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        if ( xmin > xmax || ymin > ymax ){
            ERROR << "cannot parse extent=\"" << am["extent"] << "\" xmin must be inferior to xmax and ymin to ymax in extend=\"min ymin,xmax ymx\"\n";;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        double meshSize;
        if ( !(std::istringstream( am["mesh_size"] ) >> meshSize ) ){
            ERROR << "cannot parse mesh_size=\"" << am["mesh_size"] << "\"\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }

        Dataset raster( am["file"].c_str() );

        if ( ! raster ) {
            ERROR << "cannot open dataset from file=\"" << am["file"] << "\"\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }
        if ( raster->GetRasterCount() < 1 ) {
            ERROR << "invalid number of bands\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }

        const int pixelWidth = raster->GetRasterXSize();
        const int pixelHeight = raster->GetRasterYSize();

        double transform[6];
        raster->GetGeoTransform( transform );

        // assume square pixels
        assert( std::abs(transform[4]) < FLT_EPSILON );
        assert( std::abs(transform[2]) < FLT_EPSILON );

        const double originX = transform[0];
        const double originY = transform[3];
        
        const double pixelPerMetreX =  1.f/transform[1];
        const double pixelPerMetreY = -1.f/transform[5]; // image is top->bottom

        assert( pixelPerMetreX > 0 && pixelPerMetreY > 0 );

        // compute the position of the tile
        int x= std::floor(( xmin - originX ) * pixelPerMetreX) ;
        int y= std::floor(( originY - ymax ) * pixelPerMetreY) ;
        int w= std::ceil(( xmax - xmin ) * pixelPerMetreX) ;
        int h= std::ceil(( ymax - ymin ) * pixelPerMetreY) ;

        // resize to fit data (avoid out of bound)
        if ( y < 0 ){
            h = std::max(0, h+y);
            y=0;
        }
        if ( y + h > pixelHeight ){
            h = std::max(0, pixelHeight - y);
        }

        if ( x < 0 ){
            w = std::max(0, w+x);
            x=0;
        }
        if ( x + w > pixelWidth ){
            w = std::max(0, pixelWidth - x);
        }


        DEBUG_OUT << std::setprecision(8) << " xmin=" << xmin << " ymin=" << ymin << " xmax=" << xmax << " ymax=" << ymax << "\n"; 
        DEBUG_OUT << " originX=" << originX << " originY=" << originY << " pixelWidth=" << pixelWidth << " pixelHeight=" << pixelHeight 
            << " pixelPerMetreX=" << pixelPerMetreX 
            << " pixelPerMetreY=" << pixelPerMetreY
            << "\n"; 
        DEBUG_OUT << " x=" << x << " y=" << y << " w=" << w << " h=" << h << "\n"; 

        //if ( x<0 || y<0 || (x + w) > pixelWidth || (y + h) > pixelHeight ){
        //    ERROR << "specified extent=\"" << am["extent"] 
        //        << "\" is not covered by file=\"" << am["file"] 
        //        << "\" (file extend=\"" 
        //        << std::setprecision(16)
        //        << originX << " " << originY + pixelHeight * transform[5] << "," 
        //        << originX + pixelWidth * transform[1] << " "
        //        <<  originY << "\")\n ";
        //    return ReadResult::ERROR_IN_READING_FILE;
        //}

        //assert( x >= 0 && x + w <= pixelWidth );
        //assert( y >= 0 && y + h <= pixelHeight );
        assert( h >= 0 && w >= 0 );

        osg::ref_ptr<osg::HeightField> hf( new osg::HeightField() );

        const int Lx = std::max( 1, int(meshSize * pixelPerMetreX) ) ;
        const int Ly = std::max( 1, int(meshSize * pixelPerMetreY) ) ;
        w = w / Lx;
        h = h / Ly;
        hf->allocate( w, h );
        hf->setXInterval( Lx / pixelPerMetreX );
        hf->setYInterval( Ly / pixelPerMetreY );
        hf->setOrigin( osg::Vec3(xmin, ymin, 0) - origin );

        GDALRasterBand * band = raster->GetRasterBand( 1 );
        GDALDataType dType = band->GetRasterDataType();
        int dSizeBits = GDALGetDataTypeSize( dType );
        // vector is automatically deleted, and data are contiguous
        std::vector<char> buffer( w * h * dSizeBits / 8  );
        char* blockData = &buffer[0];

        if (buffer.size()){
           band->RasterIO( GF_Read, x, y, w * Lx, h * Ly, blockData, w, h, dType, 0, 0 ); 
        }

        double dataOffset;
        double dataScale;
        int ok;
        dataOffset = band->GetOffset( &ok );
        if ( ! ok ) {
            dataOffset = 0.0;
        }
        dataScale = band->GetScale( &ok );
        if ( ! ok ) {
            ERROR << "cannot get scale\n";
            dataScale = 1.0;
        }

        float zMax = 0;
        for ( int i = 0; i < h; ++i ) {
            for ( int j = 0; j < w; ++j ) {
                const float z = float( (SRCVAL(blockData, dType, i*w+j) * dataScale)  + dataOffset );
                hf->setHeight( j, h-1-i, z );
                zMax = std::max( z, zMax );

            }
        }
        DEBUG_OUT << "zMax=" << zMax << "\n";

        hf->setSkirtHeight(10);

        DEBUG_OUT << "loaded in " << timer.time_s() << "sec\n";

        osg::Geode * geode = new osg::Geode;
        geode->addDrawable( new osg::ShapeDrawable( hf.get() ) );
        return geode;
    }
};

REGISTER_OSGPLUGIN(postgis, ReaderWriterMNT)


