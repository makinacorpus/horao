#include "ViewerWidget.h"
#include "SimpleSSAO.h"

#include <osg/CullFace>
#include <osgGA/TrackballManipulator>
#include <osgText/Text>
#include <osg/io_utils>
#include <osgEarth/Map>

#define SSAO 0

namespace Stack3d {
namespace Viewer {

ViewerWidget::ViewerWidget(): 
    QWidget() 
{
    setThreadingModel( osgViewer::CompositeViewer::SingleThreaded );

    // disable the default setting of viewer.done() by pressing Escape.
    setKeyEventSetsDone( 0 );
     
    osgQt::GraphicsWindowQt* gw; 
    {
        osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->windowName = "Simple Viewer";
        traits->windowDecoration = true;
        traits->x = 0;
        traits->y = 0;
        traits->width = 1000;
        traits->height = 800;
        traits->doubleBuffer = true;
        traits->samples = 4;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();

        gw = new osgQt::GraphicsWindowQt( traits.get() );
        setGeometry( 0, 0, traits->width, traits->height );
    }


    {
        osgViewer::View* view = new osgViewer::View;
        addView( view );

        osg::Camera* camera = view->getCamera();
        camera->setGraphicsContext( gw );

        const osg::GraphicsContext::Traits* traits = gw->getTraits();

        camera->setClearColor( osg::Vec4( 204.0/255, 204.0/255, 204.0/255, 1 ) );
        camera->setViewport( new osg::Viewport( 0, 0, traits->width, traits->height ) );
        camera->setProjectionMatrixAsPerspective( 30.0f, double( traits->width )/double( traits->height ), 1.0f, 10000.0f );
        //camera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        //camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

        osg::ref_ptr<osg::Group> root = new osg::Group;
        view->setSceneData( root.get() );

        osgEarth::MapOptions opt( osgEarth::Config("type","projected") );
        osg::ref_ptr<osgEarth::Map> map = new osgEarth::Map( opt );
        _mapNode = new osgEarth::MapNode( map ); 
        root->addChild( _mapNode.get() );

        osg::StateSet* ss = root->getOrCreateStateSet();
        osg::CullFace* cf = new osg::CullFace( osg::CullFace::BACK );
        ss->setAttribute( cf );
        ss->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON |osg::StateAttribute::OVERRIDE );

#if SSAO
        {
            const bool showAOMap = false;
            osgPPU::Unit* lastUnit = NULL;
            osgPPU::Processor* ppu = SimpleSSAO::createPipeline( traits->width, traits->height, view->getCamera(), lastUnit, showAOMap );

            _ppuout = new osgPPU::UnitOut();
            _ppuout->setName( "PipelineResult" );
            _ppuout->setInputTextureIndexForViewportReference( -1 ); // need this here to get viewport from camera
            _ppuout->setViewport( new osg::Viewport( 0,0,traits->width, traits->height ) );
            lastUnit->addChild( _ppuout.get() );

            root->addChild( ppu );
        }
#endif

        // create sunlight
        view->setLightingMode( osg::View::SKY_LIGHT );
        view->getLight()->setPosition(osg::Vec4(1000,0,1000,0));
        //view->getLight()->setDirection(osg::Vec3(-1,0,-1));
        view->getLight()->setAmbient(osg::Vec4( 0.8,0.8,0.8,1 ));
        view->getLight()->setDiffuse(osg::Vec4( 0.8,0.8,0.8,1 ));

        view->addEventHandler( new osgViewer::StatsHandler );
        view->setCameraManipulator( new osgGA::TrackballManipulator );
    }
    




    QGridLayout* grid = new QGridLayout;
    grid->addWidget( gw->getGLWidget(), 0, 0 );
    setLayout( grid );

    connect( &_timer, SIGNAL( timeout() ), this, SLOT( update() ) );
    _timer.start( 10 );
}

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

void ViewerWidget::paintEvent( QPaintEvent* )
{
    if ( _addLayerQueue.size() || _removeLayerQueue.size() )
    {
        boost::lock_guard<boost::mutex> lock( _queueMutex );
        while( _addLayerQueue.size() ){
           addLayer( _addLayerQueue.front().get() );
           _addLayerQueue.pop();
        }
        while( _removeLayerQueue.size() ){
           removeLayer( _removeLayerQueue.front().get() );
           _removeLayerQueue.pop();
        }
    }
    frame();
}

void ViewerWidget::addLayer( osgEarth::Layer * layer ) volatile 
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    boost::lock_guard<boost::mutex> lock( that->_queueMutex );
    that->_addLayerQueue.push( layer );
}

void ViewerWidget::removeLayer( osgEarth::Layer * layer ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    boost::lock_guard<boost::mutex> lock( that->_queueMutex );
    that->_removeLayerQueue.push( layer );
}


void ViewerWidget::addLayer( osgEarth::Layer * layer )
{
#define CAST_ADD_RETURN( LayerType ) \
    if ( osgEarth::LayerType * l = dynamic_cast<osgEarth::LayerType *>(layer) ) {\
        _mapNode->getMap()->add##LayerType( l ); \
        return;\
    }

    CAST_ADD_RETURN( ImageLayer )
    CAST_ADD_RETURN( ElevationLayer )
    CAST_ADD_RETURN( ModelLayer )
    if ( osgEarth::MaskLayer * l = dynamic_cast<osgEarth::MaskLayer *>(layer) ) {
        _mapNode->getMap()->addTerrainMaskLayer( l );
        return;
    }

    assert(false && bool("unhandled layer type"));
#undef CAST_ADD_RETURN
}

void ViewerWidget::removeLayer( osgEarth::Layer * layer )
{
#define CAST_REM_RETURN( LayerType ) \
    if ( osgEarth::LayerType * l = dynamic_cast<osgEarth::LayerType *>(layer) ) {\
        _mapNode->getMap()->remove##LayerType( l ); \
        return;\
    }

    CAST_REM_RETURN( ImageLayer )
    CAST_REM_RETURN( ElevationLayer )
    CAST_REM_RETURN( ModelLayer )
    if ( osgEarth::MaskLayer * l = dynamic_cast<osgEarth::MaskLayer *>(layer) ) {
        _mapNode->getMap()->removeTerrainMaskLayer( l );
        return;
    }

    assert(false && bool("unhandled layer type"));
#undef CAST_REM_RETURN
}

}
}

