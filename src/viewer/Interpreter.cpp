#include "Interpreter.h"

#include "StringUtils.h"
#include "SkyBox.h"

#include <osgDB/ReadFile>
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
        ERROR << "cannot open '" << _inputFile << "'";
        std::cout << "<error msg=\""<< Log::instance().str() << "\"/>\n";\
        Log::instance().str("");\
    }
    std::string line;
    std::string physicalLine;
    while ( std::getline( ifs, physicalLine ) || std::getline( std::cin, physicalLine ) ) {
        if ( physicalLine.empty() || '#' == physicalLine[0] ) continue; // empty line

        if ( physicalLine[ physicalLine.length() -1 ] == '\\' ) {
            std::cout << physicalLine << "\n";
            line += physicalLine.substr(0, physicalLine.length() - 1) ;
            continue;
        }
        else if (!line.empty()){
            std::cout << physicalLine << "\n";
            line += physicalLine.substr(0, physicalLine.length() - 1) ;
        }
        else{
            line = physicalLine;
        }

        std::stringstream ls(line);
        line = "";
        std::string cmd;
        std::getline( ls, cmd, ' ' );

        AttributeMap am;
        std::string key, value;
        while (    std::getline( ls, key, '=' ) 
                && std::getline( ls, value, '"' ) 
                && std::getline( ls, value, '"' )){
            // remove spaces in key
            key.erase( remove_if(key.begin(), key.end(), isspace ), key.end());
            am[ key ] = unescapeXMLString( value );
        }

        if ( "help" == cmd ){
            help();
        }
#define COMMAND( CMD )                                                  \
        else if ( #CMD == cmd  ){                                       \
            if ( !CMD( am ) ){                                          \
                ERROR << "cannot " << #CMD;                             \
                std::cout << "<error msg=\""<< Log::instance().str() << "\"/>\n"; \
            }                                                           \
            else {                                                      \
                std::cout << "<ok/>\n";                                 \
            }                                                           \
            Log::instance().str("");                                    \
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
        else{
            ERROR << "unknown command '" << cmd << "'.";
            std::cout << "<error msg=\"" << Log::instance().str() << "\"/>\n";
            Log::instance().str("");
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

bool Interpreter::lookAt( const AttributeMap & )
{
    osg::Matrix m = osg::Matrix::lookAt(osg::Vec3(0, 200, 0), osg::Vec3(200, 200, 0), osg::Vec3(0, 0, 1));
    return _viewer->setLookAt( m );
}

bool Interpreter::loadFile( const AttributeMap & am )
{
    if ( am.value("id").empty() || am.value("file").empty() || am.value("origin").empty()  ) return false;

    osg::Vec3d origin;
    if ( !( std::stringstream( am.value("origin") ) >> origin.x() >> origin.y() >> origin.z() ) ){
        ERROR << "cannot parse origin";
        return false;
    }

    osg::ref_ptr< osg::Node > node = osgDB::readNodeFile( am.value("file") );

    if (!node.get()) return false;

    osg::ref_ptr< osg::PositionAttitudeTransform > tr = new osg::PositionAttitudeTransform;
    tr->setPosition( -origin );
    tr->addChild( node.get() );

    return _viewer->addNode( am.value("id"), tr.get() );

}

bool Interpreter::addSky( const AttributeMap & am )
{
    if ( am.value("id").empty() || am.value("image").empty() || am.value("radius").empty() ) return false;

    double radius;
    if (!(std::stringstream( am.value("radius") ) >> radius ) ){
        ERROR << "cannot parse radius=\"" << am.value("radius") << "\"\n";
        return false;
    }

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable( new osg::ShapeDrawable( new osg::Sphere(osg::Vec3(), radius)) );
 
    osg::Image* posX = osgDB::readImageFile( "posX_"+am.value("image"));
    osg::Image* negX = osgDB::readImageFile( "negX_"+am.value("image"));
    osg::Image* posY = osgDB::readImageFile( "posY_"+am.value("image"));
    osg::Image* negY = osgDB::readImageFile( "negY_"+am.value("image"));
    osg::Image* posZ = osgDB::readImageFile( "posZ_"+am.value("image"));
    osg::Image* negZ = osgDB::readImageFile( "negZ_"+am.value("image"));
    if ( !( posX && negX && posY && negY && posZ && negZ ) ){
        ERROR << "cannot find image=\"" << am.value("image") << "\"\n";
        return false;
    }
    
    osg::ref_ptr<SkyBox> skybox = new SkyBox;
    skybox->getOrCreateStateSet()->setTextureAttributeAndModes( 0, new osg::TexGen );
    skybox->setEnvironmentMap( 0, posX, negX, posY, negY, posZ, negZ );
    skybox->addChild( geode.get() );

    return _viewer->addNode( am.value("id"), skybox.get() );
}

bool Interpreter::addPlane( const AttributeMap & am )
{
    if ( am.value("id").empty() || am.value("extent").empty() || am.value("origin").empty() ) return false;

    double xmin, ymin, xmax, ymax;
    std::stringstream ext( am.value("extent") );
    std::string l;
    if ( !(ext >> xmin >> ymin)
        || !std::getline(ext, l, ',')
        || !(ext >> xmax >> ymax) ) {
        ERROR << "cannot parse extent";
        return false;
    }

    osg::Vec3d origin;
    if ( !( std::stringstream( am.value("origin") ) >> origin.x() >> origin.y() >> origin.z() ) ){
        ERROR << "cannot parse origin";
        return false;
    }

    osg::Box* unitCube = new osg::Box( osg::Vec3(xmin, ymin, 0) + osg::Vec3(xmax-xmin, ymax-ymin, 0)*.5f - origin, xmax-xmin, ymax-ymin, 0);
    osg::ShapeDrawable* plane = new osg::ShapeDrawable(unitCube);
    osg::Geode* geode = new osg::Geode();
    geode->addDrawable( plane );
    return _viewer->addNode( am.value("id"), geode );
}

bool Interpreter::loadVectorPostgis(const AttributeMap & am )
{
    if ( am.value("id").empty() 
      || am.value("conn_info").empty() 
      || am.value("origin").empty()
      ) return false;

    // with LOD
    if ( ! am.optionalValue("lod").empty() ){
        std::vector<  double > lodDistance;
        if ( am.value("extent").empty() ||  am.value("tile_size").empty() ) return false;
        std::stringstream levels(am.optionalValue("lod"));
        std::string l;
        while ( std::getline( levels, l, ' ' ) ){
           lodDistance.push_back( atof(l.c_str() ) );
           const int idx = lodDistance.size()-2;
           if (idx < 0) continue;
           const std::string lodIdx = intToString( idx );
           if ( am.value("query_"+lodIdx ).empty() 
                   || !isQueryValid( am.value("query_"+lodIdx ) ) ) return false;
        }
        
        float xmin, ymin, xmax, ymax;
        std::stringstream ext( am.value("extent") );
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            ERROR << "cannot parse extent";
            return false;
        }

        float tileSize;
        if (!(std::stringstream(am.value("tile_size")) >> tileSize) || tileSize <= 0 ){
            ERROR << "cannot parse tile_size";
            return false;
        }

        osg::Vec3 origin(0,0,0);
        if (!(std::stringstream(am.value("origin")) >> origin.x() >> origin.y() ) ){
            ERROR << "cannot parse origin";
            return false;
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
                    if (query.empty()) return false;
                    const std::string pseudoFile = "conn_info=\"" + escapeXMLString(am.value("conn_info"))       + "\" "
                        + "origin=\""    + escapeXMLString(am.value("origin"))          + "\" "
                        + "query=\""     + escapeXMLString(query) + "\"" + POSTGIS_EXTENSION;

                    pagedLod->setFileName( lodDistance.size()-2-ilod,  pseudoFile );
                    pagedLod->setRange( lodDistance.size()-2-ilod, lodDistance[ilod], lodDistance[ilod+1] );
                }
                pagedLod->setCenter( osg::Vec3( xm+.5*tileSize, ym+.5*tileSize ,0) - origin );
                pagedLod->setRadius( .5*tileSize*std::sqrt(2.0) );
                group->addChild( pagedLod.get() );
            }
        }
        if (!_viewer->addNode( am.value("id"), group.get() )) return false;

            
    }
    // without LOD
    else{
      if ( am.value("query").empty() || !isQueryValid( am.value("query") ) ) return false;
      const std::string pseudoFile = "conn_info=\""       + escapeXMLString(am.value("conn_info"))       + "\" "
          + "origin=\""          + escapeXMLString(am.value("origin"))          + "\" "
          + "query=\""           + escapeXMLString(am.value("query"))           + "\"" + POSTGIS_EXTENSION;
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( pseudoFile );
        if (!node.get() ){
            ERROR << "cannot create layer";
            return false;
        }
        if (!_viewer->addNode( am.value("id"), node.get() )) return false;
    }

    return true;
}

bool Interpreter::loadRasterGDAL(const AttributeMap & )
{
    ERROR << "not implemented";
    return false;
}

bool Interpreter::loadElevation(const AttributeMap & am)
{
    if ( am.value("id").empty() 
      || am.value("origin").empty()
      || am.value("extent").empty()
      || am.value("file").empty()
      ) return false;


    // test if an terain db exist (.ive)
    std::string ive(am.value("file"));
    if ( ive.length() <= 4){
        ERROR << "file=\""<< ive << "\" filename is too short (missing exetension ?)\n";
        return false;
    }
    ive.replace( ive.length() - 4, std::string::npos, ".ive" );
    AttributeMap amIve( am );
    amIve["file"] = ive;
    if ( loadFile(amIve) ) return true;
    else std::cout << "did not find \"" << ive << "\"\n";

    // with LOD
    if ( ! am.optionalValue("lod").empty() ){
        std::vector<  double > lodDistance;
        if ( am.value("tile_size").empty() ) return false;
        std::stringstream levels(am.optionalValue("lod"));
        std::string l;
        while ( std::getline( levels, l, ' ' ) ){
           lodDistance.push_back( atof(l.c_str() ) );
           const int idx = lodDistance.size()-2;
           if (idx < 0) continue;
           const std::string lodIdx = intToString( idx );
           if ( am.value("mesh_size_"+lodIdx ).empty() ) return false;
        }
        
        float xmin, ymin, xmax, ymax;
        std::stringstream ext( am.value("extent") );
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            ERROR << "cannot parse extent";
            return false;
        }

        float tileSize;
        if (!(std::stringstream(am.value("tile_size")) >> tileSize) || tileSize <= 0 ){
            ERROR << "cannot parse tile_size";
            return false;
        }

        osg::Vec3 origin(0,0,0);
        if (!(std::stringstream(am.value("origin")) >> origin.x() >> origin.y() ) ){
            ERROR << "cannot parse origin";
            return false;
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

                    pagedLod->setFileName( lodDistance.size()-2-ilod,  pseudoFile );
                    pagedLod->setRange( lodDistance.size()-2-ilod, lodDistance[ilod], lodDistance[ilod+1] );
                }
                pagedLod->setCenter( osg::Vec3( xm+.5*tileSize, ym+.5*tileSize ,0) - origin );
                pagedLod->setRadius( .5*tileSize*std::sqrt(2.0) );
                group->addChild( pagedLod.get() );
            }
        }
        if (!_viewer->addNode( am.value("id"), group.get() )) return false;
    }
    // without LOD
    else{
      if ( am.value("mesh_size").empty() ) return false;
        const std::string pseudoFile = 
              "file=\""      + escapeXMLString(am.value("file"))              + "\" "
            + "origin=\""    + escapeXMLString(am.value("origin"))            + "\" "
            + "mesh_size=\"" + escapeXMLString(am.value("mesh_size"))         + "\" "
            + "extent=\""    + escapeXMLString(am.value("extent"))            + "\" " + MNT_EXTENSION;
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( pseudoFile );
        if (!node.get() ){
            ERROR << "cannot create layer";
            return false;
        }
        if (!_viewer->addNode( am.value("id"), node.get() )) return false;
    }

    return true;
}

bool Interpreter::unloadLayer( const AttributeMap& am )
{
    return am.value("id").empty() ? false : _viewer->removeNode( am.value("id") );
}

bool Interpreter::showLayer( const AttributeMap& am )
{
    return am.value("id").empty() ? false : _viewer->setVisible( am.value("id"), true );
}

bool Interpreter::hideLayer( const AttributeMap& am )
{
    return am.value("id").empty() ? false : _viewer->setVisible( am.value("id"), false );
}


// copied from osgearth
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


bool Interpreter::setSymbology(const AttributeMap & am)
{
    if ( am.value("id").empty() ) return false;
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

    return _viewer->setStateSet( am.value("id"), stateset.get() );
}

bool Interpreter::setFullExtent(const AttributeMap & )
{
    ERROR << "not implemented";
    return false;
}


}
}
