#include "Interpreter.h"

#include <osgDB/ReadFile>
#include <osg/Material>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
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
        else if ( "image" == that->elemName ){
            if ( !that->interpreter->loadImage( that->elemContend ) ){
                std::cerr << "error: cannot load '" << name << "'\n";
            }
        }
        else{
            std::cerr << "error: unknown command '" << name << "'\n";
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

void Interpreter::operator()()
{
    std::ifstream ifs( _inputFile.c_str() );
    if ( !_inputFile.empty() && !ifs ) std::cout << "error: cannot open '" << _inputFile <<"'\n";

    XML_Parser parser = XML_ParserCreate(NULL);
    XmlUserData userData(this);
    XML_SetUserData(parser, &userData);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, characterDataHandler);

    std::string line;
    bool done = false;
    while (!done) {
        done = !( std::getline( ifs, line ) || !std::getline( std::cin, line ) );
        if (!XML_Parse(parser, line.c_str(), line.length(), done)) {
            std::cerr << "error: " << XML_ErrorString( XML_GetErrorCode(parser) ) << "\n";
            userData = XmlUserData(this);
        }
    }
    XML_ParserFree(parser);


    if (QApplication::instance()) QApplication::instance()->quit();
}

bool Interpreter::loadImage(const std::string & xml)
{
    using namespace osgEarth;
    using namespace osgEarth::Drivers;

    Config conf;
    std::istringstream iss(xml);
    if ( !conf.fromXML( iss ) ) return false;
    osg::ref_ptr<ImageLayer> layer = new ImageLayer( ImageLayerOptions( conf ) );
    _viewer->addLayer( layer.get() );
    return true;
}


}
}
