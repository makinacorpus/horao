#include "ViewerWidget.h"
#include "SimpleSSAO.h"

#include <osg/CullFace>
#include <osgGA/TrackballManipulator>
#include <osgText/Text>
#include <osg/io_utils>

namespace Stack3d {
namespace Viewer {
// class to handle events with a pick
class PickHandler : public osgGA::GUIEventHandler {
public:

    PickHandler(osgText::Text* updateText):
        _updateText(updateText) {}

    ~PickHandler() {}

    bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa);

    void pick(osgViewer::View* view, const osgGA::GUIEventAdapter& ea);

    void setLabel(const std::string& name)
    {
        if (_updateText.get()) _updateText->setText(name);
    }

protected:

    osg::ref_ptr<osgText::Text>  _updateText;
};

bool PickHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa)
{
    switch(ea.getEventType())
    {
        case(osgGA::GUIEventAdapter::PUSH):
        {
            osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
            if (view) pick(view,ea);
            return false;
        }
        case(osgGA::GUIEventAdapter::KEYDOWN):
        {
            if (ea.getKey()=='c')
            {
                osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
                osg::ref_ptr<osgGA::GUIEventAdapter> event = new osgGA::GUIEventAdapter(ea);
                event->setX((ea.getXmin()+ea.getXmax())*0.5);
                event->setY((ea.getYmin()+ea.getYmax())*0.5);
                if (view) pick(view,*event);
            }
            return false;
        }
        default:
            return false;
    }
}

void PickHandler::pick(osgViewer::View* view, const osgGA::GUIEventAdapter& ea)
{
    osgUtil::LineSegmentIntersector::Intersections intersections;

    std::string gdlist="";

    if (view->computeIntersections(ea,intersections))
    {
        for(osgUtil::LineSegmentIntersector::Intersections::iterator hitr = intersections.begin();
            hitr != intersections.end();
            ++hitr)
        {
            std::ostringstream os;
            if (!hitr->nodePath.empty() && !(hitr->nodePath.back()->getName().empty()))
            {
                // the geodes are identified by name.
                os<<"Object \""<<hitr->nodePath.back()->getName()<<"\""<<std::endl;
            }
            else if (hitr->drawable.valid())
            {
                os<<"Object \""<<hitr->drawable->className()<<"\""<<std::endl;
            }

            os<<"        local coords vertex("<< hitr->getLocalIntersectPoint()<<")"<<"  normal("<<hitr->getLocalIntersectNormal()<<")"<<std::endl;
            os<<"        world coords vertex("<< hitr->getWorldIntersectPoint()<<")"<<"  normal("<<hitr->getWorldIntersectNormal()<<")"<<std::endl;
            const osgUtil::LineSegmentIntersector::Intersection::IndexList& vil = hitr->indexList;
            for(unsigned int i=0;i<vil.size();++i)
            {
                os<<"        vertex indices ["<<i<<"] = "<<vil[i]<<std::endl;
            }

            gdlist += os.str();
        }
    }
    setLabel(gdlist);
}

osg::Node* createHUD(osgText::Text* updateText)
{

    // create the hud. derived from osgHud.cpp
    // adds a set of quads, each in a separate Geode - which can be picked individually
    // eg to be used as a menuing/help system!
    // Can pick texts too!

    osg::Camera* hudCamera = new osg::Camera;
    hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudCamera->setProjectionMatrixAsOrtho2D(0,1600,0,900);
    hudCamera->setViewMatrix(osg::Matrix::identity());
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);

    std::string timesFont("fonts/times.ttf");

    // turn lighting off for the text and disable depth test to ensure its always ontop.
    osg::Vec3 position(150.0f,800.0f,0.0f);
    osg::Vec3 delta(0.0f,-60.0f,0.0f);

    {
        osg::Geode* geode = new osg::Geode();
        osg::StateSet* stateset = geode->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
        stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
        geode->setName("simple");
        hudCamera->addChild(geode);

        position += delta;
    }


    for (int i=0; i<5; i++) {
        osg::Vec3 dy(0.0f,-30.0f,0.0f);
        osg::Vec3 dx(120.0f,0.0f,0.0f);
        osg::Geode* geode = new osg::Geode();
        osg::StateSet* stateset = geode->getOrCreateStateSet();
        const char *opts[]={"One", "Two", "Three", "January", "Feb", "2003"};
        osg::Geometry *quad=new osg::Geometry;
        stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
        stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
        std::string name="subOption";
        name += " ";
        name += std::string(opts[i]);
        geode->setName(name);
        osg::Vec3Array* vertices = new osg::Vec3Array(4); // 1 quad
        osg::Vec4Array* colors = new osg::Vec4Array;
        colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(0.8-0.1*i,0.1*i,0.2*i, 1.0));
        quad->setColorArray(colors, osg::Array::BIND_OVERALL);
        (*vertices)[0]=position;
        (*vertices)[1]=position+dx;
        (*vertices)[2]=position+dx+dy;
        (*vertices)[3]=position+dy;
        quad->setVertexArray(vertices);
        quad->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));
        geode->addDrawable(quad);
        hudCamera->addChild(geode);

        position += delta;
    }



    { // this displays what has been selected
        osg::Geode* geode = new osg::Geode();
        osg::StateSet* stateset = geode->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
        stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
        geode->setName("The text label");
        geode->addDrawable( updateText );
        hudCamera->addChild(geode);

        updateText->setCharacterSize(20.0f);
        updateText->setFont(timesFont);
        updateText->setColor(osg::Vec4(1.0f,1.0f,0.0f,1.0f));
        updateText->setText("");
        updateText->setPosition(position);
        updateText->setDataVariance(osg::Object::DYNAMIC);

        position += delta;
    }

    return hudCamera;

}


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

        osg::Group* root = new osg::Group;
        view->setSceneData( root );

        osg::StateSet* ss = root->getOrCreateStateSet();
        osg::CullFace* cf = new osg::CullFace( osg::CullFace::BACK );
        ss->setAttribute( cf );
        ss->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON |osg::StateAttribute::OVERRIDE );

        //ssao
        if(1)
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

        osg::ref_ptr<osgText::Text> updateText = new osgText::Text;
        root->addChild(createHUD(updateText.get()));

        // create sunlight
        view->setLightingMode( osg::View::SKY_LIGHT );
        view->getLight()->setPosition(osg::Vec4(1000,0,1000,0));
        //view->getLight()->setDirection(osg::Vec3(-1,0,-1));
        view->getLight()->setAmbient(osg::Vec4( 0.8,0.8,0.8,1 ));
        view->getLight()->setDiffuse(osg::Vec4( 0.8,0.8,0.8,1 ));

        view->addEventHandler( new osgViewer::StatsHandler );
        view->addEventHandler(new PickHandler(updateText.get()));
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

