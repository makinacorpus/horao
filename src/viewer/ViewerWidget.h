#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgEarth/MapNode>

#include <queue>

namespace Stack3d {
namespace Viewer {

struct ViewerWidget: osgViewer::Viewer
{
    ViewerWidget();
    // should be called before any other layer operation
    bool addMap( osgEarth::MapNode * ) volatile;
    bool addLayer( osgEarth::Layer * ) volatile;
    bool removeLayer( osgEarth::Layer * ) volatile;
    bool removeLayer( const std::string& ) volatile;
    bool setVisible( const std::string&, bool ) volatile;
    void setDone( bool flag ) volatile;

private:
    OpenThreads::Mutex _mutex;
    osg::ref_ptr< osgEarth::MapNode > _mapNode;

    void frame();
};

}
}
#endif
