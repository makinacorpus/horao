#include <stdexcept>

#include <osg/Shape>

#include <osgViewer/Viewer>

#include <osgDB/WriteFile>

#include <osgTerrain/Terrain>
#include <osgTerrain/TerrainTile>
#include <osgTerrain/Layer>

#include <gdal/gdal_priv.h>
#include <gdal/cpl_conv.h>

//
// Build a HeightField from a raster band
// x,y : origin pixel point
// w,h : width x height (in raster pixels)
// level : 0 => destination size = w,h
//         1 => destination size : w/2,h/2
osg::HeightField* getTerrainTile( GDALRasterBand* band, int x, int y, int w, int h, int level = 0 )
{
    std::cout << "getTile " << x << ", " << y << ", " << w << ", " << h << ", " << level << std::endl;
    osg::ref_ptr<osg::HeightField> hf( new osg::HeightField() );

    int L = 1 << level;
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

    return hf.release();
}

class TerrainGDALImporter
{
public:
    TerrainGDALImporter( const std::string& filename )
    {
        // GDAL init
        GDALAllRegister();
        
        _raster = (GDALDataset *) GDALOpen( filename.c_str(), GA_ReadOnly );
        if ( ! _raster ) {
            throw std::runtime_error( "Problem opening the dataset" );
        }
        if ( _raster->GetRasterCount() < 1 ) {
            throw std::runtime_error( "Invalid number of bands" );
        }

        _pixelWidth = _raster->GetRasterXSize();
        _pixelHeight = _raster->GetRasterYSize();

        _band = _raster->GetRasterBand( 1 );

        _raster->GetGeoTransform( _transform );
        
        _pixelSizeX = _transform[1];
        _pixelSizeY = _transform[5];

        std::cout << "px: " << _pixelSizeX << " py: " << _pixelSizeY << std::endl;
        
        // Pixel/Line to Xp,Yp
        //    Xp = padfTransform[0] + P*padfTransform[1] + L*padfTransform[2];
        //    Yp = padfTransform[3] + P*padfTransform[4] + L*padfTransform[5];
        _originX = _transform[0];
        _originY = _transform[3];
        
        _width = _pixelWidth * _transform[1] + _pixelHeight * _transform[2];
        _height = _pixelWidth * _transform[4] + _pixelHeight * _transform[5];

        _terrain = new osgTerrain::Terrain;
    }

    osg::LOD* getQuadTile( int x,
                           int y,
                           int w,
                           int h,
                           int level,
                           int nLevels
                           )
    {
        std::cout << "x: " << x << " y: " << y << " w: " << w << " h: " << h << " level: " << level << std::endl;
        int L = 1 << level;
        
        // width and height adjustment
        int correctWidth = w;
        if ( x + w > _pixelWidth ) {
            correctWidth = _pixelWidth - x;
            if ( correctWidth <= 0 ) {
                return 0;
            }
        }
        int correctHeight = h;
        if ( y + h > _pixelHeight ) {
            correctHeight = _pixelHeight - y;
            if ( correctHeight <= 0 ) {
                return 0;
            }
        }
        std::cout << x << " , " << y << std::endl;
        std::cout << correctWidth << "x" << correctHeight << std::endl;
        osg::ref_ptr< osg::HeightField > hf ( getTerrainTile( _band,
                                                              x,
                                                              y,
                                                              correctWidth,
                                                              correctHeight,
                                                              level ) );
        
        hf->setXInterval( _pixelSizeX * L );
        hf->setYInterval( _pixelSizeY * L );
        double tileWidth = w * fabs(_pixelSizeX * L);
        double tileHeight = h * fabs(_pixelSizeY * L);
        hf->setOrigin( osg::Vec3( _originX + tileWidth * x, _originY + tileHeight * y, 0.0 ) );
        hf->setSkirtHeight(500);
        
        osg::ref_ptr< osgTerrain::TerrainTile > tile( new osgTerrain::TerrainTile );
        
        // locator
        osg::ref_ptr< osgTerrain::Locator > locator( new osgTerrain::Locator );
        locator->setCoordinateSystemType( osgTerrain::Locator::PROJECTED );
        osg::Matrixd tr;
        
        tr.makeIdentity();
        tr( 0, 0 ) = fabs(_width);
        tr( 1, 1 ) = fabs(_height);
        tr( 3, 0 ) = _originX;
        tr( 3, 1 ) = _originY;
        
        locator->setTransform( tr );
        
        // the heightfield of this tile
        osg::ref_ptr< osgTerrain::HeightFieldLayer > hfl( new osgTerrain::HeightFieldLayer( hf.release() ) );
        // TODO: how to set the locator globally on the tile
        hfl->setLocator( locator );
        
        tile->setElevationLayer( hfl );
        
        osg::LOD* lod( new osg::LOD );
        lod->setRangeMode( osg::LOD::DISTANCE_FROM_EYE_POINT );
        
#if 1
        if ( level == nLevels ) {
            lod->addChild( tile.get(), (level+1) * 10000, FLT_MAX );
        }
        else {
            lod->addChild( tile.get(), (level+1) * 10000, (level+2) * 10000 );
        }
#endif
        lod->addChild( tile.get(), 0, FLT_MAX );
        tile->setTerrain( _terrain );
        
#if 1
        if ( level ) {
            osg::LOD* l1 = getQuadTile( x, y,
                                        w / 2, h / 2,
                                        level - 1, nLevels );
            osg::LOD* l2 = getQuadTile( x + w / 2, y,
                                        w / 2, h / 2,
                                        level - 1, nLevels );
            osg::LOD* l3 = getQuadTile( x, y + h / 2,
                                        w / 2, h / 2,
                                        level - 1, nLevels );
            osg::LOD* l4 = getQuadTile( x + w / 2, y + h / 2,
                                        w / 2, h / 2,
                                        level - 1, nLevels );
            
            if ( l1 ) {
                lod->addChild( l1, level*1000, (level+1) * 1000);
            }
            if ( l2 ) {
                lod->addChild( l2, level*1000, (level+1) * 1000);
            }
            if ( l3 ) {
                lod->addChild( l3, level*1000, (level+1) * 1000);
            }
            if ( l4 ) {
                lod->addChild( l4, level*1000, (level+1) * 1000);
            }
        }
#endif
        
        return lod;
    }

    osgTerrain::Terrain* getTerrain( double reqTileWidth, double reqTileHeight )
    {
        double tileWidth = int(ceilf(reqTileWidth / fabs(_pixelSizeX))) * fabs(_pixelSizeX);
        double tileHeight = int(ceilf(reqTileHeight / fabs(_pixelSizeY))) * fabs(_pixelSizeY);
        
        int tilePixelWidth = int(tileWidth / fabs(_pixelSizeX));
        int tilePixelHeight = int(tileHeight / fabs(_pixelSizeY));
        
        int nXTiles = int(ceilf(fabs(_width) / tileWidth));
        int nYTiles = int(ceilf(fabs(_height) / tileHeight));
        
        std::cout << "tileWidth: " << tileWidth << " tileHeight: " << tileHeight << std::endl;
        std::cout << "tilePixelWidth: " << tilePixelWidth << " tilePixelHeight: " << tilePixelHeight << std::endl;
        std::cout << "nXTiles: " << nXTiles << " nYTiles: " << nYTiles << std::endl;
        
        int xLevels = 0, yLevels = 0;
        int nx = nXTiles, ny = nYTiles;
        while ( nx >= 1 ) {
            nx /= 2;
            xLevels++;
        }
        while ( ny >= 1 ) {
            ny /= 2;
            yLevels++;
        }
        int nLevels = std::max( xLevels, yLevels );
        std::cout << "nLevels: " << nLevels << std::endl;
        nXTiles = 1 << nLevels;
        nYTiles = 1 << nLevels;
        std::cout << nXTiles << std::endl;
        
        osg::ref_ptr< osg::LOD > tile( getQuadTile( 0, 0,
                                                    tilePixelWidth * nXTiles,
                                                    tilePixelHeight * nYTiles,
                                                    // only 1 level for now
                                                    1,
                                                    nLevels ) );

        _terrain->addChild( tile.get() );

        return _terrain.get();
    }

private:
    GDALDataset *_raster;
    GDALRasterBand *_band;

    // width, height in pixels
    int _pixelWidth, _pixelHeight;

    // total width, height in native coordinates
    double _width, _height;

    double _pixelSizeX, _pixelSizeY;

    double _transform[6];

    double _originX, _originY;

    osg::ref_ptr< osgTerrain::Terrain > _terrain;
};

int main( int, char *argv[] )
{
    // arg: raster file
    const std::string fileName = argv[1];

    TerrainGDALImporter importer( fileName );

    // ask for a quad-tree terrain with LOD0 tile size of 2000x2000
    osg::ref_ptr< osgTerrain::Terrain > terrain( importer.getTerrain( 2000, 2000 ) );

    osgDB::writeNodeFile( *terrain, "terrain.osg" );
#if 0
    osgViewer::Viewer viewer;
    viewer.setSceneData( terrain );

    viewer.run();
#endif

    return 0;
}
