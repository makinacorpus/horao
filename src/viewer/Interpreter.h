#ifndef STACK3D_VIEWER_INTERPRETER_H
#define STACK3D_VIEWER_INTERPRETER_H

#include "ViewerWidget.h"

#include <osg/Node>

#include <string>
#include <sstream>

namespace Stack3d {
namespace Viewer {

struct Interpreter
{
    Interpreter( volatile ViewerWidget * viewer, const std::string & fileName = "" ) : _viewer( viewer ), _inputFile( fileName ) {}

    // The interpreter uses xml definitions of osgearth properties and create layers.
    // Several commands (like <unload name="layerName"/> or <list/>) have been added
    // to allow to interactively modify the map.
    void operator()();
    bool help() const {
        std::cout 
            << "    <help/>: display this.\n"
            << "    <list/>: list all layers.\n"
            << "    <image name=\"layerName\">...</image>: load image layer.\n"
            << "    <elevation name=\"layerName\">...</elevation>: load elevation layer.\n" 
            << "    <unload name=\"layerName\">: unload layer.\n"
            ;
        return true;
    }
    //bool list() const;
    bool loadImage(const std::string & xml);
    //bool unload(const std::string & xml);
private:
    volatile ViewerWidget * _viewer;
    const std::string _inputFile;
};


}
}

#endif
