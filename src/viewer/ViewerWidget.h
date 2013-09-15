#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/Viewer>
#include <osgDB/WriteFile>
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
    bool setStateSet( const std::string& nodeId, osg::StateSet * ) volatile;
    bool setLookAt( const osg::Vec3 & eye, const osg::Vec3 & center, const osg::Vec3 & up ) volatile;
    bool lookAtExtent( double xmin, double ymin, double xmax, double ymax ) volatile;
    bool writeFile( const std::string & filename) volatile {
        ViewerWidget * that = const_cast< ViewerWidget * >(this);
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );
        return osgDB::writeNodeFile( *that->_root, filename );
    }


private:

    osgGA::CameraManipulator* getCurrentManipulator();
    OpenThreads::Mutex _mutex;
    osg::ref_ptr<osg::Group> _root;
    typedef std::map< std::string, osg::ref_ptr<osg::Node> > NodeMap;
    NodeMap _nodeMap;
    void frame(double time);
};

}
}
#endif
