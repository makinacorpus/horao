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
    bool list( std::stringstream & ) const;
    bool unload( std::stringstream & ss );
    bool load( std::stringstream & ss );
public:
    volatile ViewerWidget * _viewer;
    std::map< std::string, osg::ref_ptr< osg::Node > > _nodeMap;
    const std::string _inputFile;
};

}
}

#endif
