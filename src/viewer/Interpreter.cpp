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
    else if ( "options" == cmd ){
        interpreter->createMap( am ) || ERROR << "cannot create map.'";
    }
    else if ( "image" == cmd ){
        interpreter->loadImage( am ) || ERROR << "cannot load image.";
    }
    else if ( "model" == cmd ){
        interpreter->loadModel( am ) || ERROR << "cannot load model.";
    }
    else if ( "elevation" == cmd ){
        interpreter->loadElevation( am ) || ERROR << "cannot load elevation.";
    }
    else if ( "unload" == cmd ){
        interpreter->unload( am ) || ERROR << "cannot unload layer.";
    }
    else if ( "show" == cmd ){
        interpreter->show( am ) || ERROR << "cannot show layer.";
    }
    else if ( "hide" == cmd ){
        interpreter->hide( am ) || ERROR << "cannot hide layer.";
    }
    else{
        ERROR << "unknown command '" << cmd << "'.";
    }
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

bool Interpreter::loadImage(const AttributeMap & )
{
    DEBUG_TRACE
    if (!_viewer){
        ERROR << "map has not been created yet."; 
        return false;
    }
    //osgEarth::Config conf;
    //std::istringstream iss(xml);
    //if ( !conf.fromXML( iss ) ) return false;
    //osgEarth::Config confOpt;
    //conf.getObjIfSet( "image",      confOpt ); 
    //osg::ref_ptr<osgEarth::ImageLayer> layer = new osgEarth::ImageLayer( osgEarth::ImageLayerOptions( confOpt ) );

    //return _viewer->addLayer( layer.get() );
    ERROR << "not implemented";
    return false;

}

bool Interpreter::unload( const AttributeMap& am )
{
    return _viewer->removeLayer( am["name"] );
}

bool Interpreter::setVisible( const AttributeMap& am, bool visible )
{
    return _viewer->setVisible( am["name"], visible );
}

bool Interpreter::loadModel(const AttributeMap & )
{
    DEBUG_TRACE
    //osgEarth::Config conf;
    //std::istringstream iss(xml);
    //if ( !conf.fromXML( iss ) ) return false;
    //osgEarth::Config confOpt;
    //conf.getObjIfSet( "model",      confOpt ); 
    //osg::ref_ptr<osgEarth::ModelLayer> layer = new osgEarth::ModelLayer( osgEarth::ModelLayerOptions( confOpt ) );

    //return _viewer->addLayer( layer.get() );
    ERROR << "not implemented";
    return false;
}

bool Interpreter::loadElevation(const AttributeMap & )
{
    DEBUG_TRACE
    //osgEarth::Config conf;
    //std::istringstream iss(xml);
    //if ( !conf.fromXML( iss ) ) return false;
    //osgEarth::Config confOpt;
    //conf.getObjIfSet( "elevation",      confOpt ); 
    //osg::ref_ptr<osgEarth::ElevationLayer> layer = new osgEarth::ElevationLayer( osgEarth::ElevationLayerOptions( confOpt ) );

    //return _viewer->addLayer( layer.get() );
    ERROR << "not implemented";
    return false;
}

bool Interpreter::createMap(const AttributeMap & )
{
    DEBUG_TRACE
    //osgEarth::Config conf;
    //std::istringstream iss(xml);
    //if ( !conf.fromXML( iss ) ) return false;

    //osgEarth::Config confOpt;
    //conf.getObjIfSet( "options",      confOpt ); 
    //confOpt.add( "name", "map" ) ;
    //confOpt.add( "type","projected" );
   
    //osgEarth::MapOptions mapOpt( confOpt );
    //if ( !mapOpt.profile().isSet() ){
    //    ERROR << "did not find profile in options.\n";
    //    return false;
    //}

    //osg::ref_ptr<osgEarth::Map> map = new osgEarth::Map( mapOpt  );
    //return _viewer->addMap( new osgEarth::MapNode( map.get() ) );
    ERROR << "not implemented";
    return false;
}


}
}
