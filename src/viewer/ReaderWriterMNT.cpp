
#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
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

#define DEBUG_OUT if (0) std::cout
#define ERROR (std::cerr << "error: ")

struct ReaderWriterMNT : osgDB::ReaderWriter
{
    ReaderWriterMNT()
    {
        supportsExtension( "mnt", "MNT tif loader" );
        supportsExtension( "mntd", "MNT tif loader" );
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
            DEBUG_OUT << "key=\"" << key << "\" value=\"" << value << "\"\n";
            am.insert( std::make_pair( key, value ) );
        }

        // define transfo  layerToWord
        osg::Matrixd layerToWord;
        {
            osg::Vec3d origin;
            if ( !( std::stringstream( am["origin"] ) >> origin.x() >> origin.y() >> origin.z() ) ){
                ERROR << "failed to obtain origin=\"" << am["origin"] <<"\"\n";
                return ReadResult::ERROR_IN_READING_FILE;
            }
            layerToWord.makeTranslate( -origin );
        }

        double xmin, ymin, xmax, ymax;
        std::stringstream ext( am["extent"] );
        std::string l;
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            ERROR << "cannot parse extent=\"" << am["extent"] << "\"\n";;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        double meshSize;
        if ( !(std::istringstream( am["mesh_size"] ) >> meshSize ) ){
            ERROR << "cannot parse mesh_size=\"" << am["mesh_size"] << "\"\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }

        GDALAllRegister();
        GDALDataset * raster = (GDALDataset *) GDALOpen( am["file"].c_str(), GA_ReadOnly );
        if ( ! raster ) {
            ERROR << "Problem opening the dataset\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }
        if ( raster->GetRasterCount() < 1 ) {
            ERROR << "Invalid number of bands\n";
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

        // compute the position of the tile
        int x= ( xmin - originX ) * pixelPerMetreX;
        int y= ( originY - ymax ) * pixelPerMetreY ;
        int w= ( xmax - xmin ) * pixelPerMetreX ;
        int h= ( ymax - ymin ) * pixelPerMetreY ;

        DEBUG_OUT << std::setprecision(8) << " xmin=" << xmin << " ymin=" << ymin << " xmax=" << xmax << " ymax=" << ymax << "\n"; 
        DEBUG_OUT << " originX=" << originX << " originY=" << originY << " pixelWidth=" << pixelWidth << " pixelHeight=" << pixelHeight 
            << " pixelPerMetreX=" << pixelPerMetreX 
            << " pixelPerMetreY=" << pixelPerMetreY
            << "\n"; 
        DEBUG_OUT << " x=" << x << " y=" << y << " w=" << w << " h=" << h << "\n"; 

        if ( x<0 || y<0 || (x + w) > pixelWidth || (y + h) > pixelHeight ){
            ERROR << "specified extent=\"" << am["extent"] 
                << "\" is not covered by file=\"" << am["file"] 
                << "\" (file extend=\"" 
                << std::setprecision(8)
                << originX << " " << originY << "," 
                << originX + pixelWidth * transform[1] << " "
                << originY + pixelHeight * transform[5] << "\")\n ";
            return ReadResult::ERROR_IN_READING_FILE;
        }

        assert( x >= 0 && x + w <= pixelWidth );
        assert( y >= 0 && y + h <= pixelHeight );
        assert( h >= 0 && w >= 0 );

        osg::ref_ptr<osg::HeightField> hf( new osg::HeightField() );

        const int Lx = std::max( 1, int(meshSize * pixelPerMetreX) ) ;
        const int Ly = std::max( 1, int(meshSize * pixelPerMetreY) ) ;
        w = w / Lx;
        h = h / Ly;
        hf->allocate( w, h );
        hf->setXInterval( pixelPerMetreX * Lx );
        hf->setYInterval( pixelPerMetreY * Ly );

        GDALRasterBand * band = raster->GetRasterBand( 1 );
        GDALDataType dType = band->GetRasterDataType();
        int dSizeBits = GDALGetDataTypeSize( dType );
        // vector is automatically deleted, and data are contiguous
        std::vector<char> buffer( w * h * dSizeBits / 8  );
        char* blockData = &buffer[0];

        band->RasterIO( GF_Read, x, y, w * Lx, h * Ly, blockData, w, h, dType, 0, 0 ); 

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

        for ( int y = 0; y < h; ++y ) {
            for ( int x = 0; x < w; ++x ) {
                hf->setHeight( x, y, float( (SRCVAL(blockData, dType, y*w+x) * dataScale)  + dataOffset ) );
            }
        }

        hf->setSkirtHeight(10);

        DEBUG_OUT << "loaded in " << timer.time_s() << "sec\n";

        osg::Geode * geode = new osg::Geode;
        geode->addDrawable( new osg::ShapeDrawable(hf.get()) );
        return geode;
    }
};

REGISTER_OSGPLUGIN(postgis, ReaderWriterMNT)


