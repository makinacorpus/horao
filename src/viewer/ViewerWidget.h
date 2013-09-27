#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/Viewer>
#include <osgDB/WriteFile>
#include <osgViewer/ViewerEventHandlers>

#include <queue>

namespace Stack3d {
namespace Viewer {

struct ViewerWidget: osgViewer::Viewer {
    ViewerWidget();
    void addNode( const std::string& nodeId, osg::Node* ) volatile;
    void removeNode( const std::string& nodeId ) volatile;
    void setVisible( const std::string& nodeId, bool visible ) volatile;
    void setDone( bool flag ) volatile;
    void setStateSet( const std::string& nodeId, osg::StateSet* ) volatile;
    void setLookAt( const osg::Vec3& eye, const osg::Vec3& center, const osg::Vec3& up ) volatile;
    void lookAtExtent( double xmin, double ymin, double xmax, double ymax ) volatile;
    void writeFile( const std::string& filename ) volatile;

private:

    osgGA::CameraManipulator* getCurrentManipulator();
    OpenThreads::Mutex _mutex;
    osg::ref_ptr<osg::Group> _root;
    typedef std::map< std::string, osg::ref_ptr<osg::Node> > NodeMap;
    NodeMap _nodeMap;
    void frame( double time );
};

}
}
#endif
