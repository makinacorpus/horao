#include "ViewerWidget.h"
#include "Log.h"

#include <osg/CullFace>
#include <osg/Material>
#include <osgGA/TrackballManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/StateSetManipulator>
#include <osgText/Text>
#include <osg/io_utils>
#include <cassert>

namespace Stack3d {
namespace Viewer {

ViewerWidget::ViewerWidget():
   osgViewer::Viewer()
{
    //osg::DisplaySettings::instance()->setNumMultiSamples( 8 );

    setUpViewInWindow(0, 0, 800, 800 );
    setThreadingModel( osgViewer::Viewer::SingleThreaded );

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    {
        osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
        traits->windowName = "Simple Viewer";
        traits->windowDecoration = true;
        traits->x = 0;
        traits->y = 0;
        traits->width = 1000;
        traits->height = 800;
        traits->doubleBuffer = true;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
    }

    {
        osg::Camera* camera = getCamera();

        camera->setClearColor( osg::Vec4( 204.0/255, 204.0/255, 204.0/255, 1 ) );
        camera->setViewport( new osg::Viewport( 0, 0, traits->width, traits->height ) );
        camera->setProjectionMatrixAsPerspective( 30.0f, double( traits->width )/double( traits->height ), 1.0f, 10000.0f );
        //camera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        //camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

        _root = new osg::Group;
        setSceneData( _root.get() );

        osg::StateSet* ss = _root->getOrCreateStateSet();
        osg::CullFace* cf = new osg::CullFace( osg::CullFace::BACK );
        ss->setAttributeAndModes( cf, osg::StateAttribute::ON );

        ss->setMode(GL_BLEND, osg::StateAttribute::ON);

        // create sunlight
        setLightingMode( osg::View::SKY_LIGHT );
        getLight()->setPosition(osg::Vec4(1000,0,1000,0));
        //getLight()->setDirection(osg::Vec3(-1,0,-1));
        getLight()->setAmbient(osg::Vec4( 0.6,0.6,0.6,1 ));
        getLight()->setDiffuse(osg::Vec4( 0.6,0.6,0.6,1 ));

        addEventHandler( new osgViewer::StatsHandler );
        setCameraManipulator( new osgGA::TerrainManipulator );
        
        addEventHandler(new osgViewer::WindowSizeHandler);
        addEventHandler(new osgViewer::StatsHandler);
        addEventHandler( new osgGA::StateSetManipulator( getCamera()->getOrCreateStateSet()) );
        addEventHandler(new osgViewer::ScreenCaptureHandler);

        setFrameStamp( new osg::FrameStamp );

    }
    realize();
}

void ViewerWidget::frame()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mutex );
    osgViewer::Viewer::frame();
}

void ViewerWidget::setDone( bool flag ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );
    that->osgViewer::Viewer::setDone( flag );
}


bool ViewerWidget::setStateSet( const std::string& nodeId, osg::StateSet * stateset) volatile
{

    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );

    const NodeMap::const_iterator found = that->_nodeMap.find( nodeId );
    if ( found == that->_nodeMap.end() ){
        ERROR << "cannot find node '" << nodeId << "'";
        return false;
    }
    found->second->setStateSet( stateset );
    return true;
}


bool ViewerWidget::addNode( const std::string& nodeId, osg::Node * node ) volatile 
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );

    if ( that->_nodeMap.find( nodeId ) != that->_nodeMap.end() ){
        ERROR << "node '" << nodeId << "' already exists";
        return false;
    }

    that->_root->addChild( node );
    that->_nodeMap.insert( std::make_pair( nodeId, node ) );
    return true;
}

bool ViewerWidget::removeNode(  const std::string& nodeId ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );

    const NodeMap::const_iterator found = that->_nodeMap.find( nodeId );
    if ( found == that->_nodeMap.end() ){
        ERROR << "cannot find node '" << nodeId << "'";
        return false;
    }
    that->_root->removeChild( found->second.get() );
    return true;
}

bool ViewerWidget::setVisible( const std::string& nodeId, bool visible ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );

    const NodeMap::const_iterator found = that->_nodeMap.find( nodeId );
    if ( found == that->_nodeMap.end() ){
        ERROR << "cannot find node '" << nodeId << "'";
        return false;
    }
    found->second->setNodeMask(visible ? 0xffffffff : 0x0);
    return true;
}

}
}

