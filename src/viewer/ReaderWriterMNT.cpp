
#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgUtil/Optimizer>
#include <osgTerrain/Terrain>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>

#include <gdal/gdal_priv.h>
#include <gdal/cpl_conv.h>

#define DEBUG_OUT if (1) std::cout

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
            osg::Vec3d center(0,0,0);
            if ( !( std::stringstream( am["center"] ) >> center.x() >> center.y() ) ){
                std::cerr << "failed to obtain center=\""<< am["center"] <<"\"\n";
                return ReadResult::ERROR_IN_READING_FILE;
            }
            layerToWord.makeTranslate( -center );
        }

        GDALAllRegister();
        GDALDataset * raster = (GDALDataset *) GDALOpen( am["file"].c_str(), GA_ReadOnly );
        if ( ! raster ) {
            std::cerr << "Problem opening the dataset\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }
        if ( raster->GetRasterCount() < 1 ) {
            std::cerr << "Invalid number of bands\n";
            return ReadResult::ERROR_IN_READING_FILE;
        }

        //int pixelWidth = raster->GetRasterXSize();
        //int pixelHeight = raster->GetRasterYSize();

        double transform[6];
        raster->GetGeoTransform( transform );

        GDALRasterBand * band = raster->GetRasterBand( 1 );
        
        int x=0;
        int y=0;
        int w=100;
        int h=100;


        osg::ref_ptr<osg::HeightField> hf( new osg::HeightField() );

        int L = 1 << atoi(am["level"].c_str());
        w = w / L;
        h = h / L;
        hf->allocate( w, h );

        GDALDataType dType = band->GetRasterDataType();
        int dSizeBits = GDALGetDataTypeSize( dType );
        // vector is automatically deleted, and data are contiguous
        std::vector<char> buffer( w * h * dSizeBits / 8  );
        char* blockData = &buffer[0];

        band->RasterIO( GF_Read, x, y, w * L, h * L, blockData, w, h, dType, 0, 0 ); 

        double dataOffset;
        double dataScale;
        int ok;
        dataOffset = band->GetOffset( &ok );
        if ( ! ok ) {
            dataOffset = 0.0;
        }
        dataScale = band->GetScale( &ok );
        if ( ! ok ) {
            dataScale = 1.0;
        }

        for ( int y = 0; y < h; ++y ) {
            for ( int x = 0; x < w; ++x ) {
                hf->setHeight( x, y, float( (SRCVAL(blockData, dType, y*w+x) * dataScale)  + dataOffset ) );
            }
        }

        //raster->GetGeoTransform( _transform );


        hf->setSkirtHeight(10);

        DEBUG_OUT << "loaded in " << timer.time_s() << "sec\n";

        osg::Geode * geode = new osg::Geode;
        geode->addDrawable( new osg::ShapeDrawable(hf.get()) );
        return geode;
    }
};

REGISTER_OSGPLUGIN(postgis, ReaderWriterMNT)


