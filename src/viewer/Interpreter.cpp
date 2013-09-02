#include "Interpreter.h"
#include <osgDB/ReadFile>
#include <osg/Material>
#include <osg/Geode>
#include <osg/ShapeDrawable>

#include <iostream>
#include <cassert>

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
    while ( std::getline( ifs, line ) || std::getline( std::cin, line ) ) {
        if ( line.empty() || '#' == line[0] ) continue; // empty line

        std::stringstream ls(line);
        std::string cmd;
        std::getline( ls, cmd, ' ' );

        AttributeMap am;
        std::string key, value;
        while (    std::getline( ls, key, '=' ) 
                && std::getline( ls, value, '"' ) 
                && std::getline( ls, value, '"' )){
            // remove spaces in key
            key.erase( remove_if(key.begin(), key.end(), isspace ), key.end());
            am[ key ] = value;
        }

        if ( "help" == cmd ){
            help();
        }
#define COMMAND( CMD ) \
        else if ( #CMD == cmd  ){\
            if ( !CMD( am ) ){\
               ERROR << "cannot " << #CMD;\
               std::cout << "<error msg=\""<< Log::instance().str() << "\"/>\n";\
               Log::instance().str("");\
            }\
        }
        COMMAND(loadVectorPostgis)
        COMMAND(loadRasterGDAL)
        COMMAND(loadElevation)
        COMMAND(unloadLayer)
        COMMAND(showLayer)
        COMMAND(hideLayer)
        COMMAND(setSymbology)
        COMMAND(setFullExtent)
        COMMAND(addPlane)
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

bool Interpreter::addPlane( const AttributeMap & am )
{
    if ( am.value("id").empty() || am.value("extend").empty() ) return false;

    float xmin, ymin, xmax, ymax;
    std::stringstream ext( am.value("extend") );
    std::string l;
    if ( !(ext >> xmin >> ymin)
        || !std::getline(ext, l, ',')
        || !(ext >> xmax >> ymax) ) {
        ERROR << "cannot parse extend";
        return false;
    }

    osg::Box* unitCube = new osg::Box( osg::Vec3(xmax-xmin, ymax-ymin,-2)/2, xmax-xmin, ymax-ymin, 0);
    osg::ShapeDrawable* plane = new osg::ShapeDrawable(unitCube);
    osg::Geode* geode = new osg::Geode();
    geode->addDrawable( plane );
    return _viewer->addNode( am.value("id"), geode );
}

bool Interpreter::loadVectorPostgis(const AttributeMap & am )
{
    if ( am.value("id").empty() 
      || am.value("conn_info").empty() 
      || am.value("center").empty()
      ) return false;

    // with LOD
    if ( ! am.optionalValue("lod").empty() ){
        std::vector<  double > lodDistance;
        if ( am.value("extend").empty() ||  am.value("tile_size").empty() ) return false;
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
        std::stringstream ext( am.value("extend") );
        if ( !(ext >> xmin >> ymin)
            || !std::getline(ext, l, ',')
            || !(ext >> xmax >> ymax) ) {
            ERROR << "cannot parse extend";
            return false;
        }

        float tileSize;
        if (!(std::stringstream(am.value("tile_size")) >> tileSize) || tileSize <= 0 ){
            ERROR << "cannot parse tile_size";
            return false;
        }

        osg::Vec3 center(0,0,0);
        if (!(std::stringstream(am.value("center")) >> center.x() >> center.y() ) ){
            ERROR << "cannot parse center";
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
                    const std::string pseudoFile = "conn_info=\"" + am.value("conn_info")       + "\" "
                                                 + "center=\""    + am.value("center")          + "\" "
                                                 + "query=\""     + query + "\".postgis";

                    pagedLod->setFileName( lodDistance.size()-2-ilod,  pseudoFile );
                    pagedLod->setRange( lodDistance.size()-2-ilod, lodDistance[ilod], lodDistance[ilod+1] );
                }
                pagedLod->setCenter( osg::Vec3( xm+.5*tileSize, ym+.5*tileSize ,0) - center );
                pagedLod->setRadius( .5*tileSize*std::sqrt(2.0) );
                group->addChild( pagedLod.get() );
            }
        }
        if (!_viewer->addNode( am.value("id"), group.get() )) return false;

            
    }
    // without LOD
    else{
      if ( am.value("query").empty() || !isQueryValid( am.value("query") ) ) return false;
        const std::string pseudoFile = "conn_info=\""       + am.value("conn_info")       + "\" "
                                     + "center=\""          + am.value("center")          + "\" "
                                     + "query=\""           + am.value("query")           + "\".postgisd";
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

bool Interpreter::loadElevation(const AttributeMap & )
{
    ERROR << "not implemented";
    return false;
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
