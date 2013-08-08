#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgQt/GraphicsWindowQt>
#include <osgPPU/UnitOut.h>
#include <osgEarth/MapNode>

#include <QGridLayout>
#include <QTimer>

#include <boost/thread.hpp>

#include <queue>

namespace Stack3d {
namespace Viewer {

struct ViewerWidget : QWidget, osgViewer::CompositeViewer
{
    ViewerWidget();
    // should be called before any other layer operation
    bool addMap( osgEarth::MapNode * ) volatile;
    bool addLayer( osgEarth::Layer * ) volatile;
    bool removeLayer( osgEarth::Layer * ) volatile;

    void paintEvent( QPaintEvent* );
    void resizeEvent( QResizeEvent* );
private:
    QTimer _timer;
    boost::mutex _mutex;
    osg::ref_ptr< osgPPU::UnitOut > _ppuout;
    osg::ref_ptr< osgEarth::MapNode > _mapNode;
};

}
}
#endif
