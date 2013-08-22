#ifndef STACK3D_VIEWER_INTERPRETER_H
#define STACK3D_VIEWER_INTERPRETER_H

#include "ViewerWidget.h"
#include "Log.h"

#include <osg/Node>

#include <string>
#include <sstream>

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
            if ( found == end() ) {
                ERROR << "cannot find attribute '" << key << "'";
                return "";
            }
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
    bool help() const {
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
        return true;
    }
    //bool list() const;

    bool loadVectorPostgis( const AttributeMap & );
    bool loadRasterGDAL( const AttributeMap & );
    bool loadElevation( const AttributeMap & );
    bool unloadLayer( const AttributeMap & );
    bool showLayer(const AttributeMap & am);
    bool hideLayer(const AttributeMap & am);
    bool setSymbology( const AttributeMap & );
    bool setFullExtent( const AttributeMap & );

private:

    // volatile to use only the thread safe interface
    // see http://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766
    volatile ViewerWidget * _viewer; 

    const std::string _inputFile;
};


}
}

#endif
