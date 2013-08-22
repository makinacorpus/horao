#ifndef STACK3D_VIEWER_INTERPRETER_H
#define STACK3D_VIEWER_INTERPRETER_H

#include "ViewerWidget.h"

#include <osg/Node>

#include <string>
#include <sstream>
#include <iostream>

// little trick to close error message tags, note that character '"' is not allowed en error message
struct EndErr { ~EndErr(){std::cerr << "\"/>"<< std::endl; } };
#define DEBUG_TRACE std::cerr << __PRETTY_FUNCTION__ << "\n";
#define ERROR   (EndErr(), (std::cerr << "<error   msg=\"" << __FILE__ << ":" << __LINE__ << " " ))
#define WARNING (EndErr(), (std::cerr << "<warning msg=\"" << __FILE__ << ":" << __LINE__ << " " ))

namespace Stack3d {
namespace Viewer {

struct Interpreter: public OpenThreads::Thread
{
    Interpreter( volatile ViewerWidget * , const std::string & fileName = "" );

    struct AttributeMap: std::map< std::string, std::string >
    {
        const std::string operator[]( const std::string & key) const
        {
            const const_iterator found = find( key );
            if ( found == end() ) {
                ERROR << "cannot find attribute '" << key << "'";
                return "";
            }
            return found->second;
        }

        // we hide this one, so we need to redirect the call
        std::string & operator[]( const std::string & key )
        {
           return std::map< std::string, std::string >::operator[]( key );
        }

    };

    // The interpreter uses xml definitions of osgearth properties and create layers.
    // Several commands (like <unload name="layerName"/> or <list/>) have been added
    // to allow to interactively modify the map.
    // The first thing to do is to create the map with an <option> section
    void run();
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

    bool loadImage(const AttributeMap & );
    bool loadModel(const AttributeMap & );
    bool loadElevation(const AttributeMap & );
    bool unload(const AttributeMap & );
    bool show(const AttributeMap & am){ return setVisible(am, true); }
    bool hide(const AttributeMap & am){ return setVisible(am, false); }
    bool createMap(const AttributeMap & );

private:
    bool setVisible( const AttributeMap &, bool visible);
    volatile ViewerWidget * _viewer; // volatile to use only the thread safe interface
                                     // see http://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766
    const std::string _inputFile;
};


}
}

#endif
