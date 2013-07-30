#include "ViewerWidget.h"
#include "SimpleSSAO.h"

#include <osg/CullFace>
#include <osgGA/TrackballManipulator>

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
        traits->width = 800;
        traits->height = 600;
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

        camera->setClearColor( osg::Vec4( 0.2, 0.2, 0.6, 1.0 ) );
        camera->setViewport( new osg::Viewport( 0, 0, traits->width, traits->height ) );
        camera->setProjectionMatrixAsPerspective( 30.0f, double( traits->width )/double( traits->height ), 1.0f, 100000.0f );

        osg::Group* root = new osg::Group;
        view->setSceneData( root );

        osg::StateSet* ss = root->getOrCreateStateSet();
        osg::CullFace* cf = new osg::CullFace( osg::CullFace::BACK );
        ss->setAttribute( cf );

        //ssao
        if(1)
        {
            const unsigned texWidth = traits->width;
            const unsigned texHeight = traits->height;
            const bool showAOMap = false;
            osgPPU::Unit* lastUnit = NULL;
            osgPPU::Processor* ppu = SimpleSSAO::createPipeline( texWidth, texHeight, view->getCamera(), lastUnit, showAOMap );

            _ppuout = new osgPPU::UnitOut();
            _ppuout->setName( "PipelineResult" );
            _ppuout->setInputTextureIndexForViewportReference( -1 ); // need this here to get viewport from camera
            _ppuout->setViewport( new osg::Viewport( 0,0,traits->width, traits->height ) );
            lastUnit->addChild( _ppuout.get() );

            root->addChild( ppu );
        }

        // create sunlight
        view->setLightingMode( osg::View::SKY_LIGHT );
        view->getLight()->setPosition(osg::Vec4(10000,0,10000,0));
        //view->getLight()->setDirection(osg::Vec3(-1,0,-1));
        view->getLight()->setAmbient(osg::Vec4( 0.8,0.8,0.8,1 ));
        view->getLight()->setDiffuse(osg::Vec4( 0.9,0.9,0.9,1 ));

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

void ViewerWidget::paintEvent( QPaintEvent* )
{
    // not, the lock is blocking refresh, do something smarter
    if ( _addNodeQueue.size() || _removeNodeQueue.size() )
    {
        boost::lock_guard<boost::mutex> lock( _queueMutex );
        osg::ref_ptr<osg::Group> scene( getView(0)->getSceneData()->asGroup() ); 
        while( _addNodeQueue.size() ){
           scene->addChild(  _addNodeQueue.front().get() );
           _addNodeQueue.pop();
        }
        while( _removeNodeQueue.size() ){
           scene->removeChild(  _removeNodeQueue.front().get() );
           _removeNodeQueue.pop();
        }
    }
    frame();
}

void ViewerWidget::addNode( osg::Node * node ) volatile 
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    boost::lock_guard<boost::mutex> lock( that->_queueMutex );
    that->_addNodeQueue.push( node );
}

void ViewerWidget::removeNode( osg::Node * node ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    boost::lock_guard<boost::mutex> lock( that->_queueMutex );
    that->_removeNodeQueue.push( node );
}

}
}

