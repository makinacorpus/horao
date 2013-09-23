#ifndef STACK3D_VIEWER_INTERPRETER_H
#define STACK3D_VIEWER_INTERPRETER_H

#include "ViewerWidget.h"
#include "Log.h"

#include <osg/Node>

#include <string>
#include <sstream>
#include <cassert>

namespace Stack3d {
namespace Viewer {

struct Interpreter: public OpenThreads::Thread
{
    Interpreter( volatile ViewerWidget * , const std::string & fileName = "" );

    struct AttributeMap: std::map< std::string, std::string >
    {
        const std::string value( const std::string & key ) const
        {
            const const_iterator found = find( key );
            if ( found == end() ) throw Exception("cannot find attribute '" + key + "'");

            return found->second;
        }

        const std::string optionalValue( const std::string & key ) const
        {
            const const_iterator found = find( key );
            return found == end() ? "" : found->second;
        }
    };

    void run(); // virtual in OpenThreads::Thread

    // The interpreter uses xml definitions of osgearth properties and create layers.
    // Several commands (like <unload name="layerName"/> or <list/>) have been added
    // to allow to interactively modify the map.
    // The first thing to do is to create the map with an <option> section
    void help() const {
        std::cout 
            << "    <help/>: display this.\n"
            << "    <options>...</options>: create the map (first thing to do).\n"
            //<< "    <list/>: list all layers.\n"
            << "    <image name=\"layerName\">...</image>: load image layer.\n"
            << "    <elevation name=\"layerName\">...</elevation>: load elevation layer.\n" 
            << "    <unload name=\"layerName\">: unload layer.\n"
            << "    <show name=\"layerName\">: show layer.\n"
            << "    <hide name=\"layerName\">: hide layer.\n"
            ;
    }
    //bool list() const;

    void loadVectorPostgis( const AttributeMap & );
    void loadRasterGDAL( const AttributeMap & );
    void loadElevation( const AttributeMap & );
    void loadFile( const AttributeMap & );
    void unloadLayer( const AttributeMap & );
    void showLayer(const AttributeMap & am);
    void hideLayer(const AttributeMap & am);
    void setSymbology( const AttributeMap & );
    void setFullExtent( const AttributeMap & );
    void addPlane( const AttributeMap & );
    void addSky( const AttributeMap & );
    void lookAt( const AttributeMap & );
    void writeFile( const AttributeMap & );

private:

    // volatile to use only the thread safe interface
    // see http://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766
    volatile ViewerWidget * _viewer; 

    const std::string _inputFile;
};

inline
const std::string 
tileQuery( std::string query, float xmin, float ymin, float xmax, float ymax )
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
                throw Exception("unended comment in query");
            }
            query.replace (end, 2, "");
            
            std::stringstream bbox;
            bbox << "ST_MakeEnvelope(" << xmin << "," << ymin << "," << xmax << "," << ymax << ")";
            const size_t tile = query.find("TILE", where);
            assert( tile != std::string::npos );
            query.replace( tile, 4, bbox.str().c_str() ); 
        }
    }

    if (!foundSpatialMetaComment) throw ("did not found spatial meta comment in query (necessary for tiling)");

    return query;
}

}
}

#endif
