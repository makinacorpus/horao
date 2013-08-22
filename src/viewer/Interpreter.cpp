#include "Interpreter.h"

#include <osgDB/ReadFile>
#include <osg/Material>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/XmlUtils>
#include <osgEarthDrivers/tms/TMSOptions>
#include <osgEarthDrivers/gdal/GDALOptions>

#include <iostream>
#include <cassert>

#include <libxml/parser.h>



namespace Stack3d {
namespace Viewer {

inline
void startElement(void * user_data, const xmlChar * name, const xmlChar ** attrs)
{
    Interpreter * interpreter = reinterpret_cast<Interpreter *>(user_data);
    
    Interpreter::AttributeMap am;
    while( attrs && *attrs ){
        const std::string key( reinterpret_cast<const char *>( *(attrs++) ) );
        const std::string value( reinterpret_cast<const char *>( *(attrs++) ) );
        am[ key ] = value;
    }

    const std::string cmd( reinterpret_cast<const char *>(name) );
    if ( "help" == cmd ){
        interpreter->help();
    }
#define COMMAND( CMD ) \
    else if ( #CMD == cmd && !interpreter->CMD( am ) ){\
        ERROR << "cannot " << #CMD;\
        std::cout << "<error msg=\""<< Log::instance().str() << "\"/>\n";\
        Log::instance().clear();\
    }
    COMMAND(loadVectorPostgis)
    COMMAND(loadRasterGDAL)
    COMMAND(loadElevation)
    COMMAND(unloadLayer)
    COMMAND(showLayer)
    COMMAND(hideLayer)
    COMMAND(setSymbology)
    COMMAND(setFullExtent)
    else{
        ERROR << "unknown command '" << cmd << "'.";
        std::cout << "<error msg=\"" << Log::instance().str() << "\"/>\n";
        Log::instance().clear();
    }
#undef COMMAND
}

inline
static void
warning(void *user_data, const char *msg, ...) 
{
    (void) user_data;
    WARNING << msg;
}

inline
static void
error(void *user_data, const char *msg, ...) 
{
    (void) user_data;
    va_list args;
    va_start(args, msg);
    ERROR << va_arg(args, const char *);
    va_end(args);
}

inline
static void
fatalError(void *user_data, const char *msg, ...) 
{
    (void) user_data;
    va_list args;
    va_start(args, msg);
    ERROR << va_arg(args, const char *);
    va_end(args);
}

Interpreter::Interpreter(volatile ViewerWidget * vw, const std::string & fileName )
    : _viewer( vw )
    , _inputFile( fileName )
{}

void Interpreter::run()
{
    xmlSAXHandler handler;
    handler.startElement = startElement;
    handler.warning = warning;
    handler.error = error;
    handler.fatalError = fatalError;

    std::ifstream ifs( _inputFile.c_str() );
    _inputFile.empty() || ifs || ERROR << "cannot open '" << _inputFile << "'";
    std::string line;
    while ( std::getline( ifs, line ) || std::getline( std::cin, line ) ) {
        if ( line.empty() ) continue; // empty line
        if (xmlSAXUserParseMemory( &handler, this, line.c_str(), line.size() ) ){
            ERROR << " cannot parse line.";
        }
    }
    _viewer->setDone(true);
}

bool Interpreter::loadVectorPostgis(const AttributeMap & )
{
    ERROR << "not implemented";
    return false;
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
    return _viewer->removeNode( am.value("name") );
}

bool Interpreter::showLayer( const AttributeMap& am )
{
    return _viewer->setVisible( am.value("name"), true );
}

bool Interpreter::hideLayer( const AttributeMap& am )
{
    return _viewer->setVisible( am.value("name"), false );
}

bool Interpreter::setSymbology(const AttributeMap & )
{
    ERROR << "not implemented";
    return false;
}

bool Interpreter::setFullExtent(const AttributeMap & )
{
    ERROR << "not implemented";
    return false;
}


}
}
