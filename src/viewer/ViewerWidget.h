#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <queue>

namespace Stack3d {
namespace Viewer {

struct ViewerWidget: osgViewer::Viewer
{
    ViewerWidget();
    bool addNode( const std::string& nodeId, osg::Node * ) volatile;
    bool removeNode( const std::string& nodeId) volatile;
    bool setVisible( const std::string& nodeId, bool visible) volatile;
    void setDone( bool flag ) volatile;

private:
    OpenThreads::Mutex _mutex;
    osg::ref_ptr<osg::Group> _root;
    typedef std::map< std::string, osg::ref_ptr<osg::Node> > NodeMap;
    NodeMap _nodeMap;
    void frame();
};

}
}
#endif
