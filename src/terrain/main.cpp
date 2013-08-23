#include <osg/Shape>

#include <osgDB/WriteFile>

#include <osgTerrain/Terrain>
#include <osgTerrain/TerrainTile>
#include <osgTerrain/Layer>

#include <gdal/gdal_priv.h>
#include <gdal/cpl_conv.h>

//
// Build a TerrainTile from a raster band
// x,y : origin pixel point
// w,h : width x height (in raster pixels)
osg::HeightField* getTerrainTile( GDALRasterBand* band, int x, int y, int w, int h )
{
    osg::ref_ptr<osg::HeightField> hf( new osg::HeightField() );

    hf->allocate( w, h );

    GDALDataType dType = band->GetRasterDataType();
    int dSizeBits = GDALGetDataTypeSize( dType );
    // vector is automatically deleted, and data are contiguous
    std::vector<char> buffer( w * h * dSizeBits * 8  );
    char* blockData = &buffer[0];

    band->RasterIO( GF_Read, x, y, w, h, blockData, w, h, dType, 0, 0 ); 

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

int main( int, char *argv[] )
{
    // arg: raster file
    const std::string fileName = argv[1];

    // GDAL init
    GDALAllRegister();

    GDALDataset* raster = (GDALDataset *) GDALOpen( fileName.c_str(), GA_ReadOnly );
    if ( ! raster ) {

        std::cerr << "Problem opening the dataset" << std::endl;
        return 1;
    }
    if ( raster->GetRasterCount() < 1 ) {
        std::cerr << "Invalid number of bands" << std::endl;
        return 1;
    }

    int pixelWidth, pixelHeight;
    pixelWidth = raster->GetRasterXSize();
    pixelHeight = raster->GetRasterYSize();
    std::cout << pixelWidth << "x" << pixelHeight << std::endl;

    GDALRasterBand* band = raster->GetRasterBand( 1 );

    double transform[6];

    raster->GetGeoTransform( transform );
    std::cout << transform[0] << " " << transform[1] << " " << transform[2] << std::endl;
    std::cout << transform[3] << " " << transform[4] << " " << transform[5] << std::endl;

    double pixelSizeX = transform[1];
    double pixelSizeY = transform[5];

    // Pixel/Line to Xp,Yp
    //    Xp = padfTransform[0] + P*padfTransform[1] + L*padfTransform[2];
    //    Yp = padfTransform[3] + P*padfTransform[4] + L*padfTransform[5];
    double originX = transform[0];
    double originY = transform[3];

    double width = pixelWidth * transform[1] + pixelHeight * transform[2];
    double height = pixelWidth * transform[4] + pixelHeight * transform[5];

    std::cout << width << "x" << height << std::endl;

    // the main terrain object
    osgTerrain::Terrain* terrain( new osgTerrain::Terrain );

    //    const double reqTileWidth = 2000;
    //    const double reqTileHeight = 2000;
    const double reqTileWidth = fabs(width);
    const double reqTileHeight = fabs(height);

    double tileWidth = int(ceilf(reqTileWidth / fabs(pixelSizeX))) * fabs(pixelSizeX);
    double tileHeight = int(ceilf(reqTileHeight / fabs(pixelSizeY))) * fabs(pixelSizeY);

    int tilePixelWidth = int(tileWidth / fabs(pixelSizeX));
    int tilePixelHeight = int(tileHeight / fabs(pixelSizeY));

    int nXTiles = int(ceilf(fabs(width) / tileWidth));
    int nYTiles = int(ceilf(fabs(height) / tileHeight));

    std::cout << "tileWidth: " << tileWidth << " tileHeight: " << tileHeight << std::endl;
    std::cout << "pixelWidth: " << pixelWidth << " pixelHeight: " << pixelHeight << std::endl;
    std::cout << "tilePixelWidth: " << tilePixelWidth << " tilePixelHeight: " << tilePixelHeight << std::endl;
    std::cout << "nXTiles: " << nXTiles << " nYTiles: " << nYTiles << std::endl;

    // tile generation
    for ( int y = 0; y < nYTiles; ++y ) {
        for ( int x = 0; x < nXTiles; ++x ) {

            osg::ref_ptr< osg::HeightField > hf( getTerrainTile( band, tilePixelWidth * x, tilePixelHeight * y, tilePixelWidth, tilePixelHeight ) );
            
            hf->setXInterval( pixelSizeX );
            hf->setYInterval( pixelSizeY );
            hf->setOrigin( osg::Vec3( originX + tileWidth * x, originY + tileWidth * y, 0.0 ) );
            
            osg::ref_ptr< osgTerrain::TerrainTile > tile( new osgTerrain::TerrainTile );
            
            // locator
            osg::ref_ptr< osgTerrain::Locator > locator( new osgTerrain::Locator );
            locator->setCoordinateSystemType( osgTerrain::Locator::PROJECTED );
            osg::Matrixd tr;
            
            tr.makeIdentity();
            // FIXME: there is something wrong here (sign?)
            tr( 0, 0 ) = fabs(tileWidth);
            tr( 1, 1 ) = fabs(tileHeight);
            tr( 3, 0 ) = originX + tileWidth * x;
            tr( 3, 1 ) = originY + tileWidth * y;
            
            locator->setTransform( tr );
            
            // the heightfield of this tile
            osg::ref_ptr< osgTerrain::HeightFieldLayer > hfl( new osgTerrain::HeightFieldLayer( hf.release() ) );
            // TODO: how to set the locator globally on the tile
            hfl->setLocator( locator );
            
            tile->setElevationLayer( hfl );
            
            tile->setTerrain( terrain );
            terrain->addChild( tile );
        }
    }

    osgDB::writeNodeFile( *terrain, "terrain.osg" );

    return 0;
}
