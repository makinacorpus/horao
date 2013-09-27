#include "Interpreter.h"

#include <osgGIS/StringUtils.h>
#include "SkyBox.h"

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/Material>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>

#include <iostream>
#include <iomanip>
#include <cassert>

#define POSTGIS_EXTENSION ".postgis"
#define MNT_EXTENSION ".mnt"

namespace Stack3d {
namespace Viewer {
    
Interpreter::Interpreter(volatile ViewerWidget * vw, const std::string & fileName )
    : _viewer( vw )
    , _inputFile( fileName )
{}

void Interpreter::run()
{
    std::ifstream ifs( _inputFile.c_str() );
    if ( !_inputFile.empty() && !ifs ){
        const std::string msg = "cannot open '" + _inputFile + "'";
        std::cout << "<error msg=\""<< escapeXMLString(msg) << "\"/>\n";
    }
    std::string line;
    std::string physicalLine;
    while ( std::getline( ifs, physicalLine ) || std::getline( std::cin, physicalLine ) ) {
        if ( physicalLine.empty() || '#' == physicalLine[0] ) continue; // empty line

        if ( physicalLine[ physicalLine.length() -1 ] == '\\' ) {
            line += physicalLine.substr(0, physicalLine.length() - 1) ;
            continue;
        }
        else if (!line.empty()){
            line += physicalLine.substr(0, physicalLine.length() - 1) ;
        }
        else{
            line = physicalLine;
        }

        std::stringstream ls(line);
        line = "";
        std::string cmd;
        std::getline( ls, cmd, ' ' );

        AttributeMap am(ls);

        if ( "help" == cmd ){
            help();
        }
#define COMMAND( CMD )\
        else if ( #CMD == cmd  ){\
            try{\
                CMD( am );\
                std::cout << "<ok/>\n";\
            }\
            catch (std::exception & e){\
                std::cout << "<error msg=\""<< escapeXMLString(e.what()) << "\"/>\n";\
            }\
        }
        COMMAND(loadVectorPostgis)
        COMMAND(loadRasterGDAL)
        COMMAND(loadElevation)
        COMMAND(loadFile)
        COMMAND(unloadLayer)
        COMMAND(showLayer)
        COMMAND(hideLayer)
        COMMAND(setSymbology)
        COMMAND(setFullExtent)
        COMMAND(addPlane)
        COMMAND(lookAt)
        COMMAND(addSky)
        COMMAND(writeFile)
        else{
            const std::string msg = "unknown command '" + cmd + "'";
            std::cout << "<error msg=\"" << escapeXMLString(msg) << "\"/>\n";
        }
#undef COMMAND
    }
    _viewer->setDone(true);
}

inline
const std::string intToString( int i )
{
    std::stringstream s;
    s << i;
    return s.str();
}

void Interpreter::writeFile( const AttributeMap & am )
{
    _viewer->writeFile( am.value("file") );
}

void Interpreter::lookAt( const AttributeMap & am )
{
    if ( am.optionalValue("extent").empty() ){
        osg::Vec3d eye, center, up;
        if ( !( std::stringstream( am.value("eye") ) >> eye.x() >> eye.y() >> eye.z() ) 
           ||!( std::stringstream( am.value("center") ) >> center.x() >> center.y() >> center.z() )
           ||!( std::stringstream( am.value("up") ) >> up.x() >> up.y() >> up.z() )){
            throw std::runtime_error("cannot parse eye, center or up");
        }

        _viewer->setLookAt( eye, center, up );
    }
    else {
        osg::Vec3d origin;
        if ( !( std::stringstream( am.value("origin") ) >> origin.x() >> origin.y() >> origin.z() ) ){
            throw std::runtime_error("cannot parse origin");
        }

        float xmin, ymin, xmax, ymax;
        std::stringstream ext( am.value("extent") );
        std::string l;
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            throw std::runtime_error("cannot parse extent");
        }
        _viewer->lookAtExtent( xmin - origin.x(), 
                               ymin - origin.y(), 
                               xmax - origin.x(), 
                               ymax - origin.y());
    }
}

void Interpreter::loadFile( const AttributeMap & am )
{
    osg::Vec3d origin;
    if ( !( std::stringstream( am.value("origin") ) >> origin.x() >> origin.y() >> origin.z() ) ){
        throw std::runtime_error("cannot parse origin");
    }

    osg::ref_ptr< osg::Node > node = osgDB::readNodeFile( am.value("file") );

    if (!node.get()) throw std::runtime_error("cannot create geometry from '" + am.value("file") + "'");

    osg::ref_ptr< osg::PositionAttitudeTransform > tr = new osg::PositionAttitudeTransform;
    tr->setPosition( -origin );
    tr->addChild( node.get() );

    _viewer->addNode( am.value("id"), tr.get() );

}

void Interpreter::addSky( const AttributeMap & am )
{
    double radius;
    if (!(std::stringstream( am.value("radius") ) >> radius ) ){
        throw std::runtime_error("cannot parse radius=\"" + am.value("radius") + "\"");
    }

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable( new osg::ShapeDrawable( new osg::Sphere(osg::Vec3(), radius)) );

#define OPEN_IMAGE_OR_THROW( prefix )\
    osg::Image* prefix = osgDB::readImageFile( std::string(#prefix) + "_" + am.value("image"));\
    if (!prefix) throw std::runtime_error("cannot reade image file '" + std::string(#prefix) + "_" + am.value("image") + "'");

    OPEN_IMAGE_OR_THROW(posX)
    OPEN_IMAGE_OR_THROW(negX)
    OPEN_IMAGE_OR_THROW(posY)
    OPEN_IMAGE_OR_THROW(negY)
    OPEN_IMAGE_OR_THROW(posZ)
    OPEN_IMAGE_OR_THROW(negZ)
#undef OPEN_IMAGE_OR_THROW
    
    osg::ref_ptr<SkyBox> skybox = new SkyBox;
    skybox->getOrCreateStateSet()->setTextureAttributeAndModes( 0, new osg::TexGen );
    skybox->setEnvironmentMap( 0, posX, negX, posY, negY, posZ, negZ );
    skybox->addChild( geode.get() );

    _viewer->addNode( am.value("id"), skybox.get() );
}

void Interpreter::addPlane( const AttributeMap & am )
{
    double xmin, ymin, xmax, ymax;
    std::stringstream ext( am.value("extent") );
    std::string l;
    if ( !(ext >> xmin >> ymin)
        || !std::getline(ext, l, ',')
        || !(ext >> xmax >> ymax) ) {
        throw std::runtime_error("cannot parse extent");
    }

    osg::Vec3d origin;
    if ( !( std::stringstream( am.value("origin") ) >> origin.x() >> origin.y() >> origin.z() ) ){
        throw std::runtime_error("cannot parse origin");
    }

    osg::ref_ptr<osg::Box> unitCube = new osg::Box( osg::Vec3(xmin, ymin, 0) + osg::Vec3(xmax-xmin, ymax-ymin, 0)*.5f - origin, xmax-xmin, ymax-ymin, 0);
    osg::ref_ptr<osg::ShapeDrawable> plane = new osg::ShapeDrawable(unitCube.get());
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable( plane.get() );
    _viewer->addNode( am.value("id"), geode );
}

void Interpreter::loadVectorPostgis(const AttributeMap & am )
{
    std::string geocolumn = "geom";
    if ( ! am.optionalValue("geocolumn").empty() ) {
        geocolumn = am.optionalValue("geocolumn");
    }
    // with LOD
    if ( ! am.optionalValue("lod").empty() ){
        std::vector<  double > lodDistance;
        std::stringstream levels(am.optionalValue("lod"));
        std::string l;
        while ( std::getline( levels, l, ' ' ) ){
           lodDistance.push_back( atof(l.c_str() ) );
           const int idx = lodDistance.size()-2;
           if (idx < 0) continue;
           const std::string lodIdx = intToString( idx );
        }
        
        float xmin, ymin, xmax, ymax;
        std::stringstream ext( am.value("extent") );
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            throw std::runtime_error("cannot parse extent");
        }

        float tileSize;
        if (!(std::stringstream(am.value("tile_size")) >> tileSize) || tileSize <= 0 ){
            throw std::runtime_error("cannot parse tile_size");
        }

        osg::Vec3 origin(0,0,0);
        if (!(std::stringstream(am.value("origin")) >> origin.x() >> origin.y() ) ){
            throw std::runtime_error("cannot parse origin");
        }


        const size_t numTilesX = (xmax-xmin)/tileSize + 1;
        const size_t numTilesY = (ymax-ymin)/tileSize + 1;

        osg::ref_ptr<osg::Group> group = new osg::Group;
        for (size_t ix=0; ix<numTilesX; ix++){
            for (size_t iy=0; iy<numTilesY; iy++){
                osg::ref_ptr<osg::PagedLOD> pagedLod = new osg::PagedLOD;
                const float xm = xmin + ix*tileSize;
                const float ym = ymin + iy*tileSize;
                for (size_t ilod = 0; ilod < lodDistance.size()-1; ilod++){
                    const std::string lodIdx = intToString( ilod );
                    const std::string query = tileQuery( am.value("query_"+lodIdx ), xm, ym, xm+tileSize, ym+tileSize );
                    const std::string pseudoFile = "conn_info=\"" + escapeXMLString(am.value("conn_info"))       + "\" "
                        + "origin=\""    + escapeXMLString(am.value("origin"))          + "\" "
                        + "geocolumn=\"" + geocolumn + "\" "
                        + "query=\""     + escapeXMLString(query) + "\""
                        + (am.optionalValue("elevation").empty() ? "" : "elevation=\"" +  escapeXMLString(am.optionalValue("elevation")) + "\"" )
                        + POSTGIS_EXTENSION;

                    pagedLod->setFileName( ilod,  pseudoFile );
                    pagedLod->setRange( ilod, lodDistance[ilod+1], lodDistance[ilod] );
                }
                pagedLod->setCenter( osg::Vec3( xm+.5*tileSize, ym+.5*tileSize ,0) - origin );
                pagedLod->setRadius( .5*tileSize*std::sqrt(2.0) );
                group->addChild( pagedLod.get() );
            }
        }
        _viewer->addNode( am.value("id"), group.get() );

            
    }
    // without LOD
    else{
      const std::string pseudoFile = "conn_info=\""       + escapeXMLString(am.value("conn_info"))       + "\" "
          + "origin=\""          + escapeXMLString(am.value("origin"))          + "\" "
          + "geocolumn=\"" + escapeXMLString(geocolumn) + "\" "
          + "query=\""           + escapeXMLString(am.value("query"))           + "\"" 
          + (am.optionalValue("elevation").empty() ? "" : "elevation=\"" +  escapeXMLString(am.optionalValue("elevation")) + "\"" )
          + POSTGIS_EXTENSION;
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( pseudoFile );
        if (!node.get() ){
            throw std::runtime_error("cannot create layer");
        }
        _viewer->addNode( am.value("id"), node.get() );
    }
}

void Interpreter::loadRasterGDAL(const AttributeMap & )
{
    throw std::runtime_error("not implemented");
}

void Interpreter::loadElevation(const AttributeMap & am)
{
    // test if an terain db exist (.ive)
    std::string ive(am.value("file"));
    if ( ive.length() <= 4){
        throw std::runtime_error( "file=\"" + ive + "\" filename is too short (missing extension ?)");
    }
    ive.replace( ive.length() - 4, std::string::npos, ".ive" );
    if ( !osgDB::findDataFile(ive).empty() ){
        AttributeMap amIve( am );
        amIve.setValue("file", ive);
        loadFile(amIve);
        return;
    }

    // with LOD
    if ( ! am.optionalValue("lod").empty() ){
        std::vector<  double > lodDistance;
        std::stringstream levels(am.optionalValue("lod"));
        std::string l;
        while ( std::getline( levels, l, ' ' ) ){
           lodDistance.push_back( atof(l.c_str() ) );
           const int idx = lodDistance.size()-2;
           if (idx < 0) continue;
           const std::string lodIdx = intToString( idx );
        }
        
        float xmin, ymin, xmax, ymax;
        std::stringstream ext( am.value("extent") );
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            throw std::runtime_error("cannot parse extent");
        }

        float tileSize;
        if (!(std::stringstream(am.value("tile_size")) >> tileSize) || tileSize <= 0 ){
            throw std::runtime_error("cannot parse tile_size");
        }

        osg::Vec3 origin(0,0,0);
        if (!(std::stringstream(am.value("origin")) >> origin.x() >> origin.y() ) ){
            throw std::runtime_error("cannot parse origin");
        }

        const size_t numTilesX = (xmax-xmin)/tileSize + 1;
        const size_t numTilesY = (ymax-ymin)/tileSize + 1;

        osg::ref_ptr<osg::Group> group = new osg::Group;
        for (size_t ix=0; ix<numTilesX; ix++){
            for (size_t iy=0; iy<numTilesY; iy++){
                osg::ref_ptr<osg::PagedLOD> pagedLod = new osg::PagedLOD;
                const float xm = xmin + ix*tileSize;
                const float ym = ymin + iy*tileSize;
                for (size_t ilod = 0; ilod < lodDistance.size()-1; ilod++){
                    const std::string lodIdx = intToString( ilod );
                    std::stringstream extent;
                    extent << std::setprecision(16) 
                        << xm << " " << ym << "," << xm+tileSize << " " << ym+tileSize;
                    const std::string pseudoFile = 
                          "file=\""      + escapeXMLString(am.value("file"))              + "\" "
                        + "origin=\""    + escapeXMLString(am.value("origin"))            + "\" "
                        + "mesh_size=\"" + escapeXMLString(am.value("mesh_size_"+lodIdx)) + "\" "
                        + "extent=\""    + extent.str()                                   + "\" " + MNT_EXTENSION;

                    pagedLod->setFileName( ilod,  pseudoFile );
                    pagedLod->setRange( ilod, lodDistance[ilod+1], lodDistance[ilod] );
                }
                pagedLod->setCenter( osg::Vec3( xm+.5*tileSize, ym+.5*tileSize ,0) - origin );
                pagedLod->setRadius( .5*tileSize*std::sqrt(2.0) );
                group->addChild( pagedLod.get() );
            }
        }
        _viewer->addNode( am.value("id"), group.get() );
    }
    // without LOD
    else{
        const std::string pseudoFile = 
              "file=\""      + escapeXMLString(am.value("file"))              + "\" "
            + "origin=\""    + escapeXMLString(am.value("origin"))            + "\" "
            + "mesh_size=\"" + escapeXMLString(am.value("mesh_size"))         + "\" "
            + "extent=\""    + escapeXMLString(am.value("extent"))            + "\" " + MNT_EXTENSION;
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( pseudoFile );
        if (!node.get() ){
            throw std::runtime_error("cannot create layer");
        }
        _viewer->addNode( am.value("id"), node.get() );
    }
}

void Interpreter::unloadLayer( const AttributeMap& am )
{
    _viewer->removeNode( am.value("id") );
}

void Interpreter::showLayer( const AttributeMap& am )
{
    _viewer->setVisible( am.value("id"), true );
}

void Interpreter::hideLayer( const AttributeMap& am )
{
    _viewer->setVisible( am.value("id"), false );
}


// copied from osgearth
inline
const osg::Vec4 htmlColor( const std::string & html )
{
    std::string t = html;
    std::transform( t.begin(), t.end(), t.begin(), ::tolower );
    osg::Vec4ub c(0,0,0,255);
    if ( t.length() >= 7 ) {
        c.r() |= t[1]<='9' ? (t[1]-'0')<<4 : (10+(t[1]-'a'))<<4;
        c.r() |= t[2]<='9' ? (t[2]-'0')    : (10+(t[2]-'a'));
        c.g() |= t[3]<='9' ? (t[3]-'0')<<4 : (10+(t[3]-'a'))<<4;
        c.g() |= t[4]<='9' ? (t[4]-'0')    : (10+(t[4]-'a'));
        c.b() |= t[5]<='9' ? (t[5]-'0')<<4 : (10+(t[5]-'a'))<<4;
        c.b() |= t[6]<='9' ? (t[6]-'0')    : (10+(t[6]-'a'));
        if ( t.length() == 9 ) {
            c.a() = 0;
            c.a() |= t[7]<='9' ? (t[7]-'0')<<4 : (10+(t[7]-'a'))<<4;
            c.a() |= t[8]<='9' ? (t[8]-'0')    : (10+(t[8]-'a'));
        }
    }
    float w = ((float)c.r())/255.0f;
    float x = ((float)c.g())/255.0f;
    float y = ((float)c.b())/255.0f;
    float z = ((float)c.a())/255.0f;

    return osg::Vec4( w, x, y, z );
}

void Interpreter::setSymbology(const AttributeMap & am)
{
    osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;

    osg::ref_ptr<osg::Material> material = new osg::Material;
    if ( ! am.optionalValue("fill_color_ambient").empty() )
        material->setAmbient(osg::Material::FRONT, htmlColor(am.optionalValue("fill_color_ambient")) );
    if ( ! am.optionalValue("fill_color_diffuse").empty() )
        material->setDiffuse(osg::Material::FRONT, htmlColor(am.optionalValue("fill_color_diffuse")) );
    if ( ! am.optionalValue("fill_color_specular").empty() )
        material->setSpecular(osg::Material::FRONT, htmlColor(am.optionalValue("fill_color_specular")) );
    if ( ! am.optionalValue("fill_color_shininess").empty() )
        material->setShininess(osg::Material::FRONT, atof(am.optionalValue("fill_color_shininess").c_str()) );

    stateset->setAttribute(material,osg::StateAttribute::ON);
    //stateset->setMode( GL_LIGHTING, osg::StateAttribute::ON );
    stateset->setAttribute(material,osg::StateAttribute::OVERRIDE);

    _viewer->setStateSet( am.value("id"), stateset.get() );
}

void Interpreter::setFullExtent(const AttributeMap & )
{
    throw std::runtime_error("not implemented");
}

const std::string tileQuery( std::string query, float xmin, float ymin, float xmax, float ymax )
{ 
    const char * spacialMetaComments[] = {"/**WHERE TILE &&", "/**AND TILE &&"};

    bool foundSpatialMetaComment = false;
    for ( size_t i = 0; i < sizeof(spacialMetaComments)/sizeof(char *); i++ ){
        const size_t where = query.find(spacialMetaComments[i]);
        if ( where != std::string::npos )
        {
            foundSpatialMetaComment = true;
            query.replace (where, 3, "");
            const size_t end = query.find("*/", where);
            if ( end == std::string::npos ){
                throw std::runtime_error("unended comment in query");
            }
            query.replace (end, 2, "");
            
            std::stringstream bbox;
            bbox << "ST_MakeEnvelope(" << xmin << "," << ymin << "," << xmax << "," << ymax << ")";
            const size_t tile = query.find("TILE", where);
            assert( tile != std::string::npos );
            query.replace( tile, 4, bbox.str().c_str() ); 
        }
    }

    if (!foundSpatialMetaComment) throw std::runtime_error("did not found spatial meta comment in query (necessary for tiling)");

    return query;
}

}
}
