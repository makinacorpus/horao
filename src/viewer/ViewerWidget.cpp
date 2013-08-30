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
#include <cassert>
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
namespace Stack3d {
namespace Viewer {


static osg::Program* loadProgram( const std::string& programName )
{
	osg::Program* program = new osg::Program();

	std::string vShaderFile( programName + ".vert");
	std::string fShaderFile( programName + ".frag");

	osg::Shader* vShader = osgDB::readShaderFile( osg::Shader::VERTEX, vShaderFile.c_str() );
	osg::Shader* fShader = osgDB::readShaderFile( osg::Shader::FRAGMENT, fShaderFile.c_str() );

	program->addShader(vShader);
	program->addShader(fShader);

	return program;
}

static osg::Texture2D* createRenderTexture(int width, int height)
{
    osg::Texture2D* texture = new osg::Texture2D();
    texture->setTextureSize(width,height);

    texture->setInternalFormat(GL_RGBA);

    texture->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::CLAMP_TO_EDGE);
	texture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
	texture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

    return texture;
}

static osg::Texture2D* createDepthTexture(int width, int height)
{
	osg::Texture2D* texture = new osg::Texture2D();
	texture->setTextureSize(width,height);

	texture->setSourceFormat(GL_DEPTH_COMPONENT);
	texture->setSourceType(GL_FLOAT);
	texture->setInternalFormat(GL_DEPTH_COMPONENT32F);
	
	texture->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::CLAMP_TO_EDGE);
	texture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
	texture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

	return texture;
}

//  =========================================================================================

//!
//!	Utility.
//!	Create a plane geode
//!
static osg::Geode* createPlaneGeode( float x, float y, float width, float height)
{
	osg::Geode* newPlaneGeode = new osg::Geode();
	//	Create plane's geometry
	osg::Geometry* quadGeometry = new osg::Geometry();
	 //	Vertices
    osg::Vec3Array* vertices = new osg::Vec3Array();
    vertices->push_back( osg::Vec3f( x,			y,			0	) );	//	First triangle
    vertices->push_back( osg::Vec3f( x,			y+height,	0	) );
    vertices->push_back( osg::Vec3f( x+width,	y,			0	) );

    vertices->push_back( osg::Vec3f( x,			y+height,	0	) );	//	Second triangle
    vertices->push_back( osg::Vec3f( x+width,	y+height,	0	) );
    vertices->push_back( osg::Vec3f( x+width,	y		,	0	) );
	//	Normals
	osg::Vec3Array* normals = new osg::Vec3Array();
	normals->push_back( osg::Vec3f( 0.0f, 0.0f, -1.0f ) );	//	First triangle
	normals->push_back( osg::Vec3f( 0.0f, 0.0f, -1.0f ) );
	normals->push_back( osg::Vec3f( 0.0f, 0.0f, -1.0f ) );
	normals->push_back( osg::Vec3f( 0.0f, 0.0f, -1.0f ) );	//	Second triangle
	normals->push_back( osg::Vec3f( 0.0f, 0.0f, -1.0f ) );
	normals->push_back( osg::Vec3f( 0.0f, 0.0f, -1.0f ) );
	//	Tex Coords
	osg::Vec3Array* texCoords = new osg::Vec3Array();
	texCoords->push_back( osg::Vec3f( 0.0f, 0.0f, 0.0f ) );	//	First triangle
	texCoords->push_back( osg::Vec3f( 0.0f, 1.0f, 0.0f ) );
	texCoords->push_back( osg::Vec3f( 1.0f, 0.0f, 0.0f ) );
	texCoords->push_back( osg::Vec3f( 0.0f, 1.0f, 0.0f ) );	//	Second triangle
	texCoords->push_back( osg::Vec3f( 1.0f, 1.0f, 0.0f ) );
	texCoords->push_back( osg::Vec3f( 1.0f, 0.0f, 0.0f ) );
	//	Create quad primitive
	osg::DrawElementsUInt* quadPrimitive = new osg::DrawElementsUInt( osg::PrimitiveSet::TRIANGLES );
	quadPrimitive->push_back(0);
	quadPrimitive->push_back(1);
	quadPrimitive->push_back(2);
	quadPrimitive->push_back(3);
	quadPrimitive->push_back(4);
	quadPrimitive->push_back(5);
	//	Attach vertices to geometry
	quadGeometry->setVertexArray(vertices);
	//	Attach vertex normals to geometry
	quadGeometry->setNormalArray(normals);
	quadGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
	//	Attach texcoords to geometry
	quadGeometry->setTexCoordArray(0,texCoords);
	//	Attach primitive to geometry
	quadGeometry->addPrimitiveSet(quadPrimitive);
	//	Set geometry to geode
	newPlaneGeode->addDrawable(quadGeometry);

	//	Return the geode
	return newPlaneGeode;
}

//  =========================================================================================

osg::Camera* createRenderTargetCamera(osg::Texture2D* rttDepthTexture,
									  osg::Texture2D* rttColorTexture01,
									  osg::Texture2D* rttColorTexture02,
									  osg::Texture2D* rttColorTexture03)
{
    osg::Camera* camera = new osg::Camera();
	//	Set the camera to render before the main camera.
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
	//	Tell the camera to use OpenGL frame buffer object where supported.
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	//	Attack RTT textures to Camera
	camera->attach(osg::Camera::DEPTH_BUFFER, rttDepthTexture  );
	camera->attach(osg::Camera::COLOR_BUFFER0,rttColorTexture01);
	camera->attach(osg::Camera::COLOR_BUFFER1,rttColorTexture02);
	camera->attach(osg::Camera::COLOR_BUFFER2,rttColorTexture03);
	//	Set up the background color and clear mask.
	camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	//	Set viewport
    camera->setViewport(0,0,512,512);
	//	Set up projection.
	camera->setProjectionMatrixAsPerspective(45.0, 1.0, 10.0, 100.0);
	//	Set view
	camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	camera->setViewMatrixAsLookAt(osg::Vec3(0.0f,-30.0f,0.0f),osg::Vec3(0,0,0),osg::Vec3(0.0f,0.0f,1.0f));
	
	//	Camera hints
	camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    return camera;
}

osg::Camera* createNormalCamera(void)
{
	osg::Camera* camera = new osg::Camera();
	//	Set clear color and mask
	camera->setClearColor(osg::Vec4( 204.0/255, 204.0/255, 204.0/255, 1 ));
	camera->setClearMask(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	//	Set viewport
	camera->setViewport(0,0,WINDOW_WIDTH/2,WINDOW_HEIGHT/2);
	//	Set projection
	camera->setProjectionMatrixAsOrtho2D(0,1024,0,1024);
	//	Set view
	camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	camera->setViewMatrix(osg::Matrix::identity());
	//	Camera hints
	camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

	return camera;
}



ViewerWidget::ViewerWidget():
   osgViewer::Viewer()
{
    osg::setNotifyLevel(osg::NOTICE);

    //osg::DisplaySettings::instance()->setNumMultiSamples( 4 );

    setUpViewInWindow(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT );
    //setThreadingModel( osgViewer::Viewer::SingleThreaded );

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    {
        osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
        traits->windowName = "Simple Viewer";
        traits->windowDecoration = true;
        traits->x = 0;
        traits->y = 0;
        traits->width = 800;
        traits->height = 800;
        traits->doubleBuffer = true;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
    }

    {
        //osg::Camera* camera = getCamera();

        //camera->setClearColor( osg::Vec4( 204.0/255, 204.0/255, 204.0/255, 1 ) );
        //camera->setViewport( new osg::Viewport( 0, 0, traits->width, traits->height ) );
        //camera->setProjectionMatrixAsPerspective( 30.0f, double( traits->width )/double( traits->height ), 1.0f, 10000.0f );
        //camera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        //camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

        _root = new osg::Group;
        setSceneData( _root.get() );


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

    // back alpha blending for "transparency"
    {
        osg::StateSet* ss = _root->getOrCreateStateSet();
        osg::CullFace* cf = new osg::CullFace( osg::CullFace::BACK );
        ss->setAttributeAndModes( cf, osg::StateAttribute::ON );
        ss->setMode(GL_BLEND, osg::StateAttribute::ON);
    }

    // fragment shader
    if(0)
    {
        osg::Program* program = new osg::Program;
        osg::Shader* fShader = osgDB::readShaderFile(osg::Shader::FRAGMENT, "shaders/test.frag" );
        osg::Shader* vShader = osgDB::readShaderFile(osg::Shader::VERTEX, "shaders/test.vert" );

        if (!fShader || !vShader){
            std::cerr << "error: cannot read shader\n";
        }
        program->addShader( vShader );
        program->addShader( fShader );
        osg::StateSet* ss = _root->getOrCreateStateSet();
        ss->setAttributeAndModes(program, osg::StateAttribute::ON);
    }

    if (0)
    {
        //	Create RT textures
        osg::Texture2D* rttDepthTexture   = createDepthTexture (512,512);
        osg::Texture2D* rttColorTexture01 = createRenderTexture(512,512);
        osg::Texture2D* rttColorTexture02 = createRenderTexture(512,512);
        osg::Texture2D* rttColorTexture03 = createRenderTexture(512,512);
        assert( rttDepthTexture && rttColorTexture01 && rttColorTexture02 && rttColorTexture03 );
        //	Create RTT camera
        osg::Camera* rttCamera = createRenderTargetCamera(rttDepthTexture,rttColorTexture01,rttColorTexture02,rttColorTexture03);

        assert( rttCamera );
        _root->addChild( rttCamera );

        //	Add 4 x ortho quads to the screen/scene

        //	Create 4 quads each one is a quarter of the screen.
        osg::Geode* fullScreenQuadGeode01 = createPlaneGeode( 0,	128, 128, 128);
        osg::Geode* fullScreenQuadGeode02 = createPlaneGeode( 128,	128, 128, 128);
        osg::Geode* fullScreenQuadGeode03 = createPlaneGeode( 0,	0,   128, 128);
        osg::Geode* fullScreenQuadGeode04 = createPlaneGeode( 128,	0,   128, 128);

        //	Create an orthographic camera and attach the 4 quads to it.
        osg::Camera* normalCamera = createNormalCamera();
        normalCamera->addChild(fullScreenQuadGeode01);
        normalCamera->addChild(fullScreenQuadGeode02);
        normalCamera->addChild(fullScreenQuadGeode03);
        normalCamera->addChild(fullScreenQuadGeode04);

        ////	quad 01 (Depth)
        //{
        //    osg::Program* program = loadProgram("../src/shaders/depth");
        //    osg::Uniform* uniformTex01 = new osg::Uniform("uTexture01",0);
        //    osg::StateSet* ss = new osg::StateSet();
        //    ss->setTextureAttributeAndModes(0,rttDepthTexture,osg::StateAttribute::ON);
        //    ss->setAttribute(program,osg::StateAttribute::ON);
        //    ss->addUniform(uniformTex01);
        //    fullScreenQuadGeode01->setStateSet(ss);
        //}

        ////	quad 02 (Color)
        //{
        //    osg::Program* program = loadProgram("../src/shaders/test");
        //    osg::Uniform* uniformTex01 = new osg::Uniform("uTexture01",0);
        //    osg::StateSet* ss = new osg::StateSet();
        //    ss->setTextureAttributeAndModes(0,rttColorTexture01,osg::StateAttribute::ON);
        //    ss->setAttribute(program,osg::StateAttribute::ON);
        //    ss->addUniform(uniformTex01);
        //    fullScreenQuadGeode02->setStateSet(ss);
        //}

        //	quad 03 G-Buffer
        {
            osg::Program* program = loadProgram("../src/shaders/g-buffer");
            osg::Uniform* uniformTex01 = new osg::Uniform("uTexture01",0);
            osg::StateSet* ss = new osg::StateSet();
            ss->setTextureAttributeAndModes(0,rttColorTexture02,osg::StateAttribute::ON);
            ss->setAttribute(program,osg::StateAttribute::ON);
            ss->addUniform(uniformTex01);
            fullScreenQuadGeode03->setStateSet(ss);
        }

        //	quad 04
        //{
        //    osg::Program* program = loadProgram("../src/shaders/test");
        //    osg::Uniform* uniformTexDepth  = new osg::Uniform("uDepthTex", 0);
        //    osg::Uniform* uniformTexColor  = new osg::Uniform("uColorTex", 1);
        //    osg::Uniform* uniformTexNormal = new osg::Uniform("uNormalTex",2);
        //    osg::StateSet* ss = new osg::StateSet();
        //    ss->setTextureAttributeAndModes(0,rttDepthTexture,osg::StateAttribute::ON);
        //    ss->setTextureAttributeAndModes(1,rttColorTexture01,osg::StateAttribute::ON);
        //    ss->setTextureAttributeAndModes(2,rttColorTexture02,osg::StateAttribute::ON);
        //    ss->setAttribute(program,osg::StateAttribute::ON);
        //    ss->addUniform(uniformTexDepth);
        //    ss->addUniform(uniformTexColor);
        //    ss->addUniform(uniformTexNormal);
        //    fullScreenQuadGeode04->setStateSet(ss);
        //}
        _root->addChild(normalCamera);

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

