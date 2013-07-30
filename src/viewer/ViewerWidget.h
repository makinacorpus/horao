#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgQt/GraphicsWindowQt>
#include <osgPPU/UnitOut.h>

#include <QGridLayout>
#include <QTimer>

#include <boost/thread.hpp>

#include <queue>

namespace Stack3d {
namespace Viewer {

struct ViewerWidget : QWidget, osgViewer::CompositeViewer
{
    ViewerWidget();
    void addNode( osg::Node * node ) volatile;
    void removeNode( osg::Node * node ) volatile;
    void addSSAO() volatile;

    void paintEvent( QPaintEvent* );
    void resizeEvent( QResizeEvent* );
private:
    QTimer _timer;
    boost::mutex _queueMutex;
    std::queue< osg::ref_ptr< osg::Node > > _addNodeQueue; 
    std::queue< osg::ref_ptr< osg::Node > > _removeNodeQueue;
    osg::ref_ptr< osgPPU::UnitOut > _ppuout;
};

}
}
#endif
