#include "Interpreter.h"

#include <osgDB/ReadFile>
#include <osg/Material>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/XmlUtils>
#include <osgEarthDrivers/tms/TMSOptions>
#include <osgEarthDrivers/gdal/GDALOptions>

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include <iostream>
#include <cassert>

#include <expat.h>

#define DEBUG_TRACE std::cerr << __PRETTY_FUNCTION__ << "\n";

namespace Stack3d {
namespace Viewer {

struct XmlUserData {
    explicit XmlUserData( Interpreter * i ) : depth(0), interpreter( i ) {}
    std::string elemName;
    std::string elemContend;
    int depth;
    Interpreter * interpreter;
};

inline
void startElement(void *userData, const XML_Char *name, const XML_Char **atts){
    XmlUserData * that = reinterpret_cast<XmlUserData * >(userData);
    
    if ( that->depth == 1 ){
        that->elemName = name;
    }

    if ( that->depth >=1 ){
        that->elemContend += "<" + std::string(name);
        while ( *atts ){
            const std::string key(*(atts++));
            const std::string value(*(atts++));
            that->elemContend += " " + key + "=\"" + value + "\"";
        }
        that->elemContend += ">";
    }
    ++that->depth;
}

inline
void endElement(void *userData, const XML_Char *name){
    XmlUserData * that = reinterpret_cast<XmlUserData * >(userData);

    --that->depth;
    if ( that->depth >= 1 ){
        that->elemContend += "</" + std::string(name) + ">\n";
    }

    if ( that->depth == 1 ){
        if ( "help" == that->elemName ){
            that->interpreter->help();
        }
        else if ( "options" == that->elemName ){
            if ( !that->interpreter->createMap( that->elemContend ) ){
                std::cerr << "error: cannot create map.'\n";
            }
        }
        else if ( "image" == that->elemName ){
            if ( !that->interpreter->loadImage( that->elemContend ) ){
                std::cerr << "error: cannot load image.\n";
            }
        }
        else if ( "model" == that->elemName ){
            if ( !that->interpreter->loadModel( that->elemContend ) ){
                std::cerr << "error: cannot load model.\n";
            }
        }
        else{
            std::cerr << "error: unknown command '" << name << "'.\n";
        }
        that->elemContend = "";
    }
}

inline
void characterDataHandler(void *userData, const XML_Char *s, int len){
    XmlUserData * that = reinterpret_cast<XmlUserData * >(userData);
   
    if ( that->depth >= 1 ){
        that->elemContend += std::string(s, s+len);
    }
}

Interpreter::Interpreter( volatile ViewerWidget * viewer, const std::string & fileName )
    : _viewer( viewer )
    , _inputFile( fileName ) 
{}

void Interpreter::operator()()
{
    std::ifstream ifs( _inputFile.c_str() );
    if ( !_inputFile.empty() && !ifs ) std::cout << "error: cannot open '" << _inputFile <<"'\n";

    XML_Parser parser = XML_ParserCreate(NULL);
    XmlUserData userData(this);
    XML_SetUserData(parser, &userData);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, characterDataHandler);

    std::string line("<map>");
    XML_Parse(parser, line.c_str(), line.length(), false);
    while ( std::getline( ifs, line ) || std::getline( std::cin, line ) ) {
        if (!XML_Parse(parser, line.c_str(), line.length(), false)) {
            std::cerr << "error: " << XML_ErrorString( XML_GetErrorCode(parser) ) << "\n";
            userData = XmlUserData(this);
        }
    }

    line = "</map>";
    if (!XML_Parse(parser, line.c_str(), line.length(), true)) {
        std::cerr << "error: " << XML_ErrorString( XML_GetErrorCode(parser) ) << "\n";
        userData = XmlUserData(this);
    }
    XML_ParserFree(parser);


    if (QApplication::instance()) QApplication::instance()->quit();
}

bool Interpreter::loadImage(const std::string & xml)
{
    DEBUG_TRACE
    if (!_viewer){
        std::cerr << "error: map has not been created yet.\n"; 
        return false;
    }
    osgEarth::Config conf;
    std::istringstream iss(xml);
    if ( !conf.fromXML( iss ) ) return false;
    osgEarth::Config confOpt;
    conf.getObjIfSet( "image",      confOpt ); 
    osg::ref_ptr<osgEarth::ImageLayer> layer = new osgEarth::ImageLayer( osgEarth::ImageLayerOptions( confOpt ) );

    return _viewer->addLayer( layer.get() );
}

bool Interpreter::loadModel(const std::string & xml)
{
    DEBUG_TRACE
    osgEarth::Config conf;
    std::istringstream iss(xml);
    if ( !conf.fromXML( iss ) ) return false;
    osgEarth::Config confOpt;
    conf.getObjIfSet( "model",      confOpt ); 
    osg::ref_ptr<osgEarth::ModelLayer> layer = new osgEarth::ModelLayer( osgEarth::ModelLayerOptions( confOpt ) );

    return _viewer->addLayer( layer.get() );
}


bool Interpreter::createMap(const std::string & xml)
{
    DEBUG_TRACE
    osgEarth::Config conf;
    std::istringstream iss(xml);
    if ( !conf.fromXML( iss ) ) return false;

    osgEarth::Config confOpt;
    conf.getObjIfSet( "options",      confOpt ); 
    confOpt.add( "name", "map" ) ;
    confOpt.add( "type","projected" );
   
    osgEarth::MapOptions mapOpt( confOpt );
    if ( !mapOpt.profile().isSet() ){
        std::cerr << "error: did not find profile in options.\n";
        return false;
    }

    osg::ref_ptr<osgEarth::Map> map = new osgEarth::Map( mapOpt  );
    return _viewer->addMap( new osgEarth::MapNode( map.get() ) );
}


}
}
