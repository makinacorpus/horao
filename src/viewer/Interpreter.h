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
    void operator()();
    bool help( std::stringstream & ) const {
        std::cout 
            << "    help: display this.\n"
            << "    list: list all layers.\n"
            << "    load <layerName> file <fileName>: load model from file.\n"
            << "    load <layerName> postgis <host> <dbname> <table> <user> <passwd>: load model from database.\n"
            << "    unload <layerName>: unload layer.\n"
            ;
        return true;
    }
    bool list( std::stringstream & ) const;
    bool load( std::stringstream & ss );
    bool unload( std::stringstream & ss );
private:
    volatile ViewerWidget * _viewer;
    std::map< std::string, osg::ref_ptr< osg::Node > > _nodeMap;
    const std::string _inputFile;

    osg::ref_ptr< osg::Node > loadFile( std::stringstream & ss );
    osg::ref_ptr< osg::Node > loadPostgis( std::stringstream & ss );

};

}
}

#endif
