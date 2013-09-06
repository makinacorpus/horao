#include "ViewerWidget.h"
#include "Log.h"

#include <osg/CullFace>
#include <osg/Material>
#include <osgGA/TrackballManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/OrbitManipulator>
#include <osgGA/StateSetManipulator>
#include <osgDB/ReadFile>
#include <osgText/Text>
#include <osg/io_utils>
#include <osg/Texture2D>

#include <cassert>
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
namespace Stack3d {
namespace Viewer {

static const char* vertSource =
{
    "uniform vec3 lightPosition;\n"
    "varying vec3 normal, eyeVec, lightDir;\n"
    "varying vec4 vertColor;\n"
    "void main()\n"
    "{\n"
        "vec4 vertexInEye = gl_ModelViewMatrix * gl_Vertex;\n"
        "eyeVec = -vertexInEye.xyz;\n"
        "lightDir = vec3(lightPosition - vertexInEye.xyz);\n"
        "normal = gl_NormalMatrix * gl_Normal;\n"
        "vertColor = gl_Color;\n"
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

/*
static const char* fragSourceFXAA =
{
"    // FXAA shader, GLSL code adapted from:\n"
"    // http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA\n"
"    // Whitepaper describing the technique:\n"
"    // http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf\n"
"\n"
"    precision mediump float;\n"
"    precision mediump int;\n"
"\n"
"    uniform sampler2D textureSampler;\n"
"\n"
"    // The inverse of the texture dimensions along X and Y\n"
"    uniform vec2 texcoordOffset;\n"
"\n"
"    varying vec4 vertColor;\n"
"    varying vec4 vertTexcoord;\n"
"\n"
"    void main() {\n"
"        // The parameters are hardcoded for now, but could be\n"
"        // made into uniforms to control fromt he program.\n"
"        float FXAA_SPAN_MAX = 8.0;\n"
"        float FXAA_REDUCE_MUL = 1.0/8.0;\n"
"        float FXAA_REDUCE_MIN = (1.0/128.0);\n"
"\n"
"        vec3 rgbNW = vec3(1,1,1);//texture2D(textureSampler, vertTexcoord.xy + (vec2(-1.0, -1.0) * texcoordOffset)).xyz;\n"
"        vec3 rgbNE = vec3(1,1,1);//texture2D(textureSampler, vertTexcoord.xy + (vec2(+1.0, -1.0) * texcoordOffset)).xyz;\n"
"        vec3 rgbSW = vec3(1,1,1);//texture2D(textureSampler, vertTexcoord.xy + (vec2(-1.0, +1.0) * texcoordOffset)).xyz;\n"
"        vec3 rgbSE = vec3(1,1,1);//texture2D(textureSampler, vertTexcoord.xy + (vec2(+1.0, +1.0) * texcoordOffset)).xyz;\n"
"        vec3 rgbM =  vec3(1,1,1);//texture2D(textureSampler, vertTexcoord.xy).xyz;\n"
"\n"
"        vec3 luma = vec3(0.299, 0.587, 0.114);\n"
"        float lumaNW = dot(rgbNW, luma);\n"
"        float lumaNE = dot(rgbNE, luma);\n"
"        float lumaSW = dot(rgbSW, luma);\n"
"        float lumaSE = dot(rgbSE, luma);\n"
"        float lumaM = dot( rgbM, luma);\n"
"\n"
"        float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"
"        float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n"
"\n"
"        vec2 dir;\n"
"        dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"
"        dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
"\n"
"        float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);\n"
"\n"
"        float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
"\n"
"        dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),\n"
"        max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * texcoordOffset;\n"
"\n"
"        //vec3 rgbA = (1.0/2.0) * (\n"
"        //    texture2D(textureSampler, vertTexcoord.xy + dir * (1.0/3.0 - 0.5)).xyz +\n"
"        //    texture2D(textureSampler, vertTexcoord.xy + dir * (2.0/3.0 - 0.5)).xyz);\n"
"        //vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (\n"
"        //    texture2D(textureSampler, vertTexcoord.xy + dir * (0.0/3.0 - 0.5)).xyz +\n"
"        //    texture2D(textureSampler, vertTexcoord.xy + dir * (3.0/3.0 - 0.5)).xyz);\n"
"        vec3 rgbA = (1.0/2.0) * vec3(1,1,1);\n"
"        vec3 rgbB = (1.0/2.0) + (1.0/4.0) * vec3(1,1,1);\n"
"        float lumaB = dot(rgbB, luma);\n"
"\n"
"        gl_FragColor.xyz= ((lumaB < lumaMin) || (lumaB > lumaMax)) ? rgbA : rgbB;\n"
"        //gl_FragColor.a = 1.0;\n"
"\n"
"        gl_FragColor *= vertColor;\n"
"    }\n"
};
*/

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
        //camera->setProjectionMatrixAsPerspective( 30.0f, double( traits->width )/double( traits->height ), 1.0f, 100000.0f );
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
        setCameraManipulator( new osgGA::OrbitManipulator );
        
        addEventHandler(new osgViewer::WindowSizeHandler);
        addEventHandler(new osgViewer::StatsHandler);
        addEventHandler(new osgGA::StateSetManipulator( getCamera()->getOrCreateStateSet()) );
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
    if(0)
    {
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader( new osg::Shader(osg::Shader::VERTEX, vertSource) );
        program->addShader( new osg::Shader(osg::Shader::FRAGMENT, fragSource) );
        osg::StateSet* stateset = _root->getOrCreateStateSet();
        stateset->setAttributeAndModes( program.get() );
        stateset->addUniform( new osg::Uniform("lightDiffuse", osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)) );
        stateset->addUniform( new osg::Uniform("lightSpecular", osg::Vec4(0.1f, 0.1f, 0.1f, 1.0f)) );
        stateset->addUniform( new osg::Uniform("shininess", 64.f) );
        //getLight()->setAmbient(osg::Vec4( 1,1,1,1 ));

        osg::ref_ptr<osg::Uniform> lightPos = new osg::Uniform( "lightPosition", osg::Vec3(10000,20000,3000) );
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

bool ViewerWidget::setLookAt( const osg::Vec3 & eye, const osg::Vec3 & center, const osg::Vec3 & up ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );
    that->getCameraManipulator()->setHomePosition(eye, center, up); 
    that->getCameraManipulator()->home(0); 
    return true;
}


bool ViewerWidget::lookAtExtent( double xmin, double ymin, double xmax, double ymax ) volatile
{
    ViewerWidget * that = const_cast< ViewerWidget * >(this);
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock( that->_mutex );
    double fovy, aspectRatio, zNear, zFar;
    if (!that->getCamera()->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar) )
        return false;

    // compute distance from fovy
    const double fovRad = fovy * M_PI / 180;
    const double altitude = .5*(ymax-ymin) / std::tan( .5*fovRad );

    const osg::Vec3 up(0,1,0);
    const osg::Vec3 center(xmin+.5*(xmax-xmin), ymin+.5*(ymax-ymin), 0 );
    const osg::Vec3 eye(center.x(), center.y(), altitude);

    that->getCameraManipulator()->setHomePosition(eye, center, up); 
    that->getCameraManipulator()->home(0); 
    return true;
}

}
}

