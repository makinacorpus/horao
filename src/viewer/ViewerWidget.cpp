#include "ViewerWidget.h"
#include "Log.h"

#include <osg/CullFace>
#include <osg/Material>
#include <osgGA/TrackballManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/StateSetManipulator>
#include <osgDB/ReadFile>
#include <osgText/Text>
#include <osg/io_utils>

#include <osgPPU/Processor.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitText.h>
#include <osgPPU/ShaderAttribute.h>

#include <cassert>
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
namespace Stack3d {
namespace Viewer {

static const char* vertSource =
{
    "uniform vec3 lightPosition;\n"
    "varying vec3 normal, eyeVec, lightDir;\n"
    "void main()\n"
    "{\n"
        "vec4 vertexInEye = gl_ModelViewMatrix * gl_Vertex;\n"
        "eyeVec = -vertexInEye.xyz;\n"
        "lightDir = vec3(lightPosition - vertexInEye.xyz);\n"
        "normal = gl_NormalMatrix * gl_Normal;\n"
        "gl_Position = ftransform();\n"
    "}\n"
};

static const char* fragSource =
{
    "uniform vec4 lightDiffuse;\n"
    "uniform vec4 lightSpecular;\n"
    "uniform float shininess;\n"
    "varying vec3 normal, eyeVec, lightDir;\n"
    "void main (void)\n"
    "{\n"
        "vec4 finalColor = gl_FrontLightModelProduct.sceneColor;\n"
        "vec3 N = normalize(normal);\n"
        "vec3 L = normalize(lightDir);\n"
        "float lambert = dot(N,L);\n"
        "if (lambert > 0.0)\n"
        "{\n"
            "finalColor += lightDiffuse * lambert;\n"
            "vec3 E = normalize(eyeVec);\n"
            "vec3 R = reflect(-L, N);\n"
            "float specular = pow(max(dot(R, E), 0.0), shininess);\n"
            "finalColor += lightSpecular * specular;\n"
        "}\n"
        "gl_FragColor = finalColor;\n"
    "}\n"
};

class LightPosCallback : public osg::Uniform::Callback
{
public:
    virtual void operator()( osg::Uniform* uniform, osg::NodeVisitor* nv )
    {
        const osg::FrameStamp* fs = nv->getFrameStamp();
        if ( !fs ) return;
        float angle = osg::inDegrees( (float)fs->getFrameNumber() );
        uniform->set( osg::Vec3(20.0f * cosf(angle), 20.0f * sinf(angle), 1.0f) );
    }
};



ViewerWidget::ViewerWidget():
   osgViewer::Viewer()
{
    osg::setNotifyLevel(osg::NOTICE);

    //osg::DisplaySettings::instance()->setNumMultiSamples( 4 );

    setThreadingModel( osgViewer::Viewer::SingleThreaded );

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    {
        osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
        traits->windowName = "Simple Viewer";
        traits->windowDecoration = true;
        traits->x = 0;
        traits->y = 0;
        traits->width = WINDOW_HEIGHT;
        traits->height = WINDOW_HEIGHT;
        traits->doubleBuffer = true;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
    }

    setUpViewInWindow(0, 0, traits->width, traits->height );

    {
        osg::Camera* camera = getCamera();

        camera->setClearColor( osg::Vec4( 204.0/255, 204.0/255, 204.0/255, 1 ) );
        //camera->setViewport( new osg::Viewport( 0, 0, traits->width, traits->height ) );
        //camera->setProjectionMatrixAsPerspective( 30.0f, double( traits->width )/double( traits->height ), 1.0f, 10000.0f );
        //camera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        //camera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);

        _root = new osg::Group;
        setSceneData( _root.get() );


        // create sunlight
        //setLightingMode( osg::View::SKY_LIGHT );
        //getLight()->setPosition(osg::Vec4(1000,2000,3000,0));
        //getLight()->setDirection(osg::Vec3(-1,-2,-3));
        //getLight()->setAmbient(osg::Vec4( 0.6,0.6,0.6,1 ));
        //getLight()->setDiffuse(osg::Vec4( 0.6,0.6,0.6,1 ));
        //getLight()->setSpecular(osg::Vec4( 0.9,0.9,0.9,1 ));

        addEventHandler( new osgViewer::StatsHandler );
        //setCameraManipulator( new osgGA::TerrainManipulator );
        setCameraManipulator( new osgGA::TrackballManipulator );
        
        addEventHandler(new osgViewer::WindowSizeHandler);
        addEventHandler(new osgViewer::StatsHandler);
        addEventHandler( new osgGA::StateSetManipulator( getCamera()->getOrCreateStateSet()) );
        addEventHandler(new osgViewer::ScreenCaptureHandler);

        setFrameStamp( new osg::FrameStamp );

    }

    // back alpha blending for "transparency"
    {
        osg::StateSet* ss = _root->getOrCreateStateSet();
        ss->setAttributeAndModes( new osg::CullFace( osg::CullFace::BACK ), osg::StateAttribute::ON );
        ss->setMode(GL_BLEND, osg::StateAttribute::ON);
    }

    // per pixel lighting
    if(1)
    {
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader( new osg::Shader(osg::Shader::VERTEX, vertSource) );
        program->addShader( new osg::Shader(osg::Shader::FRAGMENT, fragSource) );
        osg::StateSet* stateset = _root->getOrCreateStateSet();
        stateset->setAttributeAndModes( program.get() );
        stateset->addUniform( new osg::Uniform("lightDiffuse", osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)) );
        stateset->addUniform( new osg::Uniform("lightSpecular", osg::Vec4(1.0f, 1.0f, 0.4f, 1.0f)) );
        stateset->addUniform( new osg::Uniform("shininess", 64.0f) );

        osg::ref_ptr<osg::Uniform> lightPos = new osg::Uniform( "lightPosition", osg::Vec3(1000,2000,3000) );
        stateset->addUniform( lightPos.get() );
    }

    realize();
}

void ViewerWidget::frame(double time)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mutex );
    osgViewer::Viewer::frame(time);
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

