#ifndef STACK3D_VIEWER_INTERPRETER_H
#define STACK3D_VIEWER_INTERPRETER_H

#include "ViewerWidget.h"

#include <osg/Node>

#include <string>
#include <sstream>
#include <iostream>

namespace Stack3d {
namespace Viewer {

struct Interpreter
{
    Interpreter( volatile ViewerWidget * , const std::string & fileName = "" );

    // The interpreter uses xml definitions of osgearth properties and create layers.
    // Several commands (like <unload name="layerName"/> or <list/>) have been added
    // to allow to interactively modify the map.
    // The first thing to do is to create the map with an <option> section
    void operator()();
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
    bool loadImage(const std::string & xml);
    bool loadModel(const std::string & xml);
    bool loadElevation(const std::string & xml);
    bool unload(const std::string& name);
    bool setVisible( const std::string& name, bool visible);
    bool createMap(const std::string & xml);

private:
    volatile ViewerWidget * _viewer; // volatile to use only the thread safe interface
                                     // see http://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766
    const std::string _inputFile;
};


}
}

#endif
