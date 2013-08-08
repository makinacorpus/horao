#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgPPU/UnitOut.h>
#include <osgEarth/MapNode>

#include <boost/thread.hpp>

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
    void setDone( bool flag ) volatile;

private:
    boost::mutex _mutex;
    osg::ref_ptr< osgPPU::UnitOut > _ppuout;
    osg::ref_ptr< osgEarth::MapNode > _mapNode;

    void frame();
};

}
}
#endif
