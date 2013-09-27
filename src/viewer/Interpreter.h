/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef STACK3D_VIEWER_INTERPRETER_H
#define STACK3D_VIEWER_INTERPRETER_H

#include "ViewerWidget.h"
#include <osgGIS/StringUtils.h>

#include <osg/Node>

#include <string>
#include <sstream>
#include <cassert>

namespace Stack3d {
namespace Viewer {

struct Interpreter: public OpenThreads::Thread {
    Interpreter( volatile ViewerWidget* , const std::string& fileName = "" );

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

    void loadVectorPostgis( const AttributeMap& );
    void loadRasterGDAL( const AttributeMap& );
    void loadElevation( const AttributeMap& );
    void loadFile( const AttributeMap& );
    void unloadLayer( const AttributeMap& );
    void showLayer( const AttributeMap& am );
    void hideLayer( const AttributeMap& am );
    void setSymbology( const AttributeMap& );
    void setFullExtent( const AttributeMap& );
    void addPlane( const AttributeMap& );
    void addSky( const AttributeMap& );
    void lookAt( const AttributeMap& );
    void writeFile( const AttributeMap& );

private:

    // volatile to use only the thread safe interface
    // see http://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766
    volatile ViewerWidget* _viewer;

    const std::string _inputFile;
};

const std::string tileQuery( std::string query, float xmin, float ymin, float xmax, float ymax );

}
}

#endif
