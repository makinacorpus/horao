#include "ViewerWidget.h"

#include <osg/CullFace>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgText/Text>
#include <osg/io_utils>
#include <osgEarth/Map>
#include <cassert>

#define DEBUG_TRACE std::cerr << __PRETTY_FUNCTION__ << "\n";

namespace Stack3d {
namespace Viewer {

ViewerWidget::ViewerWidget():
   osgViewer::Viewer()
{
    osg::DisplaySettings::instance()->setNumMultiSamples( 8 );

    setUpViewInWindow(0, 0, 800, 800 );
    //setThreadingModel( osgViewer::CompositeViewer::SingleThreaded );

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

        osg::ref_ptr<osg::Group> root = new osg::Group;
        setSceneData( root.get() );

        osg::StateSet* ss = root->getOrCreateStateSet();
        osg::CullFace* cf = new osg::CullFace( osg::CullFace::BACK );
        ss->setAttribute( cf );

        // create sunlight
        setLightingMode( osg::View::SKY_LIGHT );
        getLight()->setPosition(osg::Vec4(1000,0,1000,0));
        //getLight()->setDirection(osg::Vec3(-1,0,-1));
        getLight()->setAmbient(osg::Vec4( 0.8,0.8,0.8,1 ));
        getLight()->setDiffuse(osg::Vec4( 0.8,0.8,0.8,1 ));

        addEventHandler( new osgViewer::StatsHandler );
        setCameraManipulator( new osgGA::TrackballManipulator );
        
        addEventHandler(new osgViewer::WindowSizeHandler);
        addEventHandler(new osgViewer::StatsHandler);
        addEventHandler( new osgGA::StateSetManipulator( getCamera()->getOrCreateStateSet()) );
        addEventHandler(new osgViewer::ScreenCaptureHandler);


    }
    realize();
}

/*
void ViewerWidget::resizeEvent( QResizeEvent* e)
{
    if( _ppuout.get() )
    {
        const double r = _ppuout->getViewport()->aspectRatio();
        const int h = e->size().height();
        const int w = e->size().width();
        if (w / r <  h ) {
            _ppuout->getViewport()->width() = h * r;
            _ppuout->getViewport()->height() = h;
        }
        else {
            _ppuout->getViewport()->width() = w;
            _ppuout->getViewport()->height() = w / r;
        }
    }
}
*/

void ViewerWidget::frame()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mutex );
    osgViewer::Viewer::frame();
    if (done()) DEBUG_TRACE;
}

void ViewerWidget::setDone( bool flag ) volatile
{
    DEBUG_TRACE
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );
    that->osgViewer::Viewer::setDone( flag );
}

bool ViewerWidget::addMap( osgEarth::MapNode * map ) volatile 
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );
    
    that->_mapNode = map;

    that->getSceneData()->asGroup()->addChild( map );
    return true;
}

bool ViewerWidget::addLayer( osgEarth::Layer * layer ) volatile 
{
    assert(layer);

    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );
    
    if (!that->_mapNode.get()){
        std::cerr << "error: trying to add layer without map.\n";
        return false;
    }

#define CAST_ADD_RETURN( LayerType ) \
    if ( osgEarth::LayerType * l = dynamic_cast<osgEarth::LayerType *>(layer) ) {\
        that->_mapNode->getMap()->add##LayerType( l ); \
        return true;\
    }

    CAST_ADD_RETURN( ImageLayer )
    CAST_ADD_RETURN( ElevationLayer )
    CAST_ADD_RETURN( ModelLayer )
    if ( osgEarth::MaskLayer * l = dynamic_cast<osgEarth::MaskLayer *>(layer) ) {
        that->_mapNode->getMap()->addTerrainMaskLayer( l );
        return true;
    }

    assert(false && bool("unhandled layer type"));
#undef CAST_ADD_RETURN

    return false;
}

bool ViewerWidget::removeLayer( osgEarth::Layer * layer ) volatile
{
    assert(layer);

    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );

    if (!that->_mapNode.get()){
        std::cerr << "error: trying to add layer without map.\n";
        return false;
    }

#define CAST_REM_RETURN( LayerType ) \
    if ( osgEarth::LayerType * l = dynamic_cast<osgEarth::LayerType *>(layer) ) {\
        that->_mapNode->getMap()->remove##LayerType( l ); \
        return true;\
    }

    CAST_REM_RETURN( ImageLayer )
    CAST_REM_RETURN( ElevationLayer )
    CAST_REM_RETURN( ModelLayer )
    if ( osgEarth::MaskLayer * l = dynamic_cast<osgEarth::MaskLayer *>(layer) ) {
        that->_mapNode->getMap()->removeTerrainMaskLayer( l );
        return true;
    }

    assert(false && bool("unhandled layer type"));
#undef CAST_REM_RETURN

    return false;
}

bool ViewerWidget::removeLayer( const std::string& layerId ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );

    if (!that->_mapNode.get()){
        std::cerr << "error: trying to add layer without map.\n";
        return false;
    }

    if ( osgEarth::ModelLayer* l = that->_mapNode->getMap()->getModelLayerByName( layerId ) ) {
	that->_mapNode->getMap()->removeModelLayer( l );
	return true;
    }
    if ( osgEarth::ImageLayer* l = that->_mapNode->getMap()->getImageLayerByName( layerId ) ) {
	that->_mapNode->getMap()->removeImageLayer( l );
	return true;
    }
    if ( osgEarth::ElevationLayer* l = that->_mapNode->getMap()->getElevationLayerByName( layerId ) ) {
	that->_mapNode->getMap()->removeElevationLayer( l );
	return true;
    }
    assert(false && bool("unhandled layer type"));
    return false;
}

bool ViewerWidget::setVisible( const std::string& layerId, bool visible ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );

    if (!that->_mapNode.get()){
        std::cerr << "error: trying to add layer without map.\n";
        return false;
    }

    if ( osgEarth::ModelLayer* l = that->_mapNode->getMap()->getModelLayerByName( layerId ) ) {
	l->setVisible( visible );
	return true;
    }
    assert(false && bool("unhandled layer type"));
    return false;
}

}
}

