#include <QTimer>
#include <QApplication>
#include <QGridLayout>

#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>

#include <osgDB/ReadFile>

#include <osgQt/GraphicsWindowQt>

#include <iostream>
#include <cassert>
#include <sstream>
#include <queue>

// ssao

#include <osg/Material>
#include <osg/Texture2D>
#include <osg/CullFace>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>

#include <osgPPU/Processor.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitText.h>
#include <osgPPU/ShaderAttribute.h>

#include <boost/thread.hpp>

namespace SimpleSSAO {

//-----------------------------------------------------------------------------------------
// Default variables desfribing the algorithm parameters
//-----------------------------------------------------------------------------------------
float gBlurSigma = 5;
float gBlurRadius = 15;
float gIntensity = 20;

//-----------------------------------------------------------------------------------------
// Create RTT texture
//-----------------------------------------------------------------------------------------
osg::Texture* createRenderTexture( int tex_width, int tex_height, bool depth )
{
    // create simple 2D texture
    osg::Texture2D* texture2D = new osg::Texture2D;
    texture2D->setTextureSize( tex_width, tex_height );
    texture2D->setInternalFormat( GL_RGBA );
    texture2D->setFilter( osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR );
    texture2D->setFilter( osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR );

    if ( !depth ) {
        texture2D->setInternalFormat( GL_RGBA16F_ARB );
        texture2D->setSourceFormat( GL_RGBA );
        texture2D->setSourceType( GL_FLOAT );
    }
    else {
        texture2D->setInternalFormat( GL_DEPTH_COMPONENT );
    }

    return texture2D;
}


//---------------------------------------------------------------------------------
// Setup pipeline
// There will be valid textures attached to the camera.
// Also the camera will be attached to the processor.
// Set widht and height to define internal size of the pipeline textures.
//---------------------------------------------------------------------------------
osgPPU::Processor* createPipeline( int width, int height, osg::Camera* camera, osgPPU::Unit*& lastUnit, bool showOnlyAOMap = false )
{
    using namespace osgPPU;

    //---------------------------------------------------------------------------------
    // presetup the camera
    //---------------------------------------------------------------------------------
    // set up the background color, clear mask and viewport
    camera->setClearColor( osg::Vec4( 0.1f,0.2f,0.3f,1.0f ) );
    camera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    camera->setViewport( new osg::Viewport( 0,0,width,height ) );

    // tell the camera to use OpenGL frame buffer object where supported.
    camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );

    // create texture to render to
    osg::Texture* textureColor = createRenderTexture( width, height, false );
    osg::Texture* textureDepth = createRenderTexture( width, height, true );

    // attach the texture and use it as the color buffer.
    camera->attach( osg::Camera::COLOR_BUFFER0, textureColor );
    camera->attach( osg::Camera::DEPTH_BUFFER, textureDepth );

    //---------------------------------------------------------------------------------
    // Setup processor and first two units, which will bypass the camera textures
    //---------------------------------------------------------------------------------
    // create processor
    Processor* processor = new Processor;
    processor->setCamera( camera );

    // first unit does get the color input from the camera into the pipeline
    UnitCameraAttachmentBypass* colorBypass = new UnitCameraAttachmentBypass;
    colorBypass->setBufferComponent( osg::Camera::COLOR_BUFFER0 );
    processor->addChild( colorBypass );

    // create unit which will bypass the depth texture into the pipleine
    UnitCameraAttachmentBypass* depthBypass = new UnitCameraAttachmentBypass;
    depthBypass->setBufferComponent( osg::Camera::DEPTH_BUFFER );
    processor->addChild( depthBypass );


    //---------------------------------------------------------------------------------
    // Setup a unit, which will read the depth texture clamp it accordingly and
    // process the output to a color texture, so that the value can be used further
    //---------------------------------------------------------------------------------
    /*UnitInOut* processDepth = new UnitInOut;
    {
        // create a shader which will process the depth values
        osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT,
            "uniform sampler2D depthTexture;\n"\
            "void main() {\n"\
            "   float depth = texture2D(depthTexture, gl_TexCoord[0].xy).x;\n"\
            "   gl_FragData[0].xyzw = depth;\n"\
            "}\n"
        );

        // create shader attribute and setup one input texture
        ShaderAttribute* depthShader = new ShaderAttribute;
        depthShader->addShader(fpShader);
        depthShader->setName("DepthProcessShader");
        depthShader->add("depthTexture", osg::Uniform::SAMPLER_2D);
        depthShader->set("depthTexture", 0);

        // create the unit and attach the shader to it
        processDepth->getOrCreateStateSet()->setAttributeAndModes(depthShader);
        depthBypass->addChild(processDepth);
        processDepth->setName("ProcessDepth");
    }*/


    //---------------------------------------------------------------------------------
    // Create units which will apply gaussian blur on the input textures
    //---------------------------------------------------------------------------------
    UnitInOut* blurx = new UnitInOut();
    UnitInOut* blury = new UnitInOut();
    {
        // set name and indicies
        blurx->setName( "BlurHorizontal" );
        blury->setName( "BlurVertical" );

        // read shaders from file
        osg::ref_ptr<osgDB::ReaderWriter::Options> fragmentOptions = new osgDB::ReaderWriter::Options( "fragment" );
        osg::ref_ptr<osgDB::ReaderWriter::Options> vertexOptions = new osgDB::ReaderWriter::Options( "vertex" );
        osg::Shader* vshader = osgDB::readShaderFile( "/home/mora/osgPPU/Data/glsl/gauss_convolution_vp.glsl", vertexOptions.get() );
        osg::Shader* fhshader = osgDB::readShaderFile( "/home/mora/osgPPU/Data/glsl/gauss_convolution_1Dx_fp.glsl", fragmentOptions.get() );
        osg::Shader* fvshader = osgDB::readShaderFile( "/home/mora/osgPPU/Data/glsl/gauss_convolution_1Dy_fp.glsl", fragmentOptions.get() );
        assert( vshader && fhshader && fvshader );

        // setup horizontal blur shaders
        osgPPU::ShaderAttribute* gaussx = new osgPPU::ShaderAttribute();
        gaussx->addShader( vshader );
        gaussx->addShader( fhshader );
        gaussx->setName( "BlurHorizontalShader" );

        gaussx->add( "sigma", osg::Uniform::FLOAT );
        gaussx->add( "radius", osg::Uniform::FLOAT );
        gaussx->add( "texUnit0", osg::Uniform::SAMPLER_2D );

        gaussx->set( "sigma", gBlurSigma );
        gaussx->set( "radius", gBlurRadius );
        gaussx->set( "texUnit0", 0 );

        blurx->getOrCreateStateSet()->setAttributeAndModes( gaussx );

        // setup vertical blur shaders
        osgPPU::ShaderAttribute* gaussy = new osgPPU::ShaderAttribute();
        gaussy->addShader( vshader );
        gaussy->addShader( fvshader );
        gaussy->setName( "BlurVerticalShader" );

        gaussy->add( "sigma", osg::Uniform::FLOAT );
        gaussy->add( "radius", osg::Uniform::FLOAT );
        gaussy->add( "texUnit0", osg::Uniform::SAMPLER_2D );

        gaussy->set( "sigma", gBlurSigma );
        gaussy->set( "radius", gBlurRadius );
        gaussy->set( "texUnit0", 0 );

        blury->getOrCreateStateSet()->setAttributeAndModes( gaussy );

        // connect the gaussian blur ppus
        depthBypass->addChild( blurx );
        blurx->addChild( blury );
    }

    //---------------------------------------------------------------------------------
    // Now we want to substract blurred from non-blurred depth and to compute the
    // resulting AO image
    //---------------------------------------------------------------------------------
    UnitInOut* aoUnit = new UnitInOut;
    {
        osg::Shader* fpShader = new osg::Shader( osg::Shader::FRAGMENT );

        // create a shader which will process the depth values
        if ( showOnlyAOMap == false ) {
            fpShader->setShaderSource(
                "uniform float intensity;\n"\
                "uniform sampler2D blurredDepthTexture;\n"\
                "uniform sampler2D originalDepthTexture;\n"\
                "uniform sampler2D colorTexture;\n"\
                "void main() {\n"\
                "   float blurred = texture2D(blurredDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   float original = texture2D(originalDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   vec4 color = texture2D(colorTexture, gl_TexCoord[0].xy);\n"\
                "   vec4 result = color - vec4(intensity * clamp((original - blurred), 0.0, 1.0));\n"\
                "   gl_FragData[0].xyzw = clamp(result, 0.0, 1.0);\n"\
                "}\n"
            );
        }
        else {
            fpShader->setShaderSource(
                "uniform float intensity;\n"\
                "uniform sampler2D blurredDepthTexture;\n"\
                "uniform sampler2D originalDepthTexture;\n"\
                "uniform sampler2D colorTexture;\n"\
                "void main() {\n"\
                "   float blurred = texture2D(blurredDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   float original = texture2D(originalDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   vec4 color = texture2D(colorTexture, gl_TexCoord[0].xy);\n"\
                "   vec4 result = vec4(1.0 - intensity * clamp((original - blurred), 0.0, 1.0));\n"\
                "   gl_FragData[0].xyzw = clamp(result, 0.0, 1.0);\n"\
                "}\n"
            );
        }

        // create shader attribute and setup one input texture
        ShaderAttribute* shader = new ShaderAttribute;
        shader->addShader( fpShader );
        shader->add( "blurredDepthTexture", osg::Uniform::SAMPLER_2D );
        shader->set( "blurredDepthTexture", 0 );
        shader->add( "originalDepthTexture", osg::Uniform::SAMPLER_2D );
        shader->set( "originalDepthTexture", 1 );
        shader->add( "colorTexture", osg::Uniform::SAMPLER_2D );
        shader->set( "colorTexture", 2 );
        shader->add( "intensity", osg::Uniform::FLOAT );
        shader->set( "intensity", gIntensity );

        // create the unit and attach the shader to it
        aoUnit->getOrCreateStateSet()->setAttributeAndModes( shader );
        blury->addChild( aoUnit );
        depthBypass->addChild( aoUnit );
        colorBypass->addChild( aoUnit );
        aoUnit->setName( "ComputeAO" );
    }

    //
    UnitInOut* tiltShift = new UnitInOut();
    {
        osg::Shader* fpShader = new osg::Shader( osg::Shader::FRAGMENT );

        // create a shader which will process the depth values
        fpShader->setShaderSource(
            "uniform float intensity;\n"\
            "uniform sampler2D blurredDepthTexture;\n"\
            "uniform sampler2D originalDepthTexture;\n"\
            "uniform sampler2D colorTexture;\n"\
            "void main() {\n"\
            "   float blurred = texture2D(blurredDepthTexture, gl_TexCoord[0].xy).x;\n"\
            "   float original = texture2D(originalDepthTexture, gl_TexCoord[0].xy).x;\n"\
            "   vec4 color = texture2D(colorTexture, gl_TexCoord[0].xy);\n"\
            "   vec4 result = color - vec4(intensity * clamp((original - blurred), 0.0, 1.0));\n"\
            "   gl_FragData[0].xyzw = clamp(result, 0.0, 1.0);\n"\
            "}\n"
        );

        // create shader attribute and setup one input texture
        ShaderAttribute* shader = new ShaderAttribute;
        shader->addShader( fpShader );
        shader->add( "blurredDepthTexture", osg::Uniform::SAMPLER_2D );
        shader->set( "blurredDepthTexture", 0 );
        shader->add( "originalDepthTexture", osg::Uniform::SAMPLER_2D );
        shader->set( "originalDepthTexture", 1 );
        shader->add( "colorTexture", osg::Uniform::SAMPLER_2D );
        shader->set( "colorTexture", 2 );
        shader->add( "intensity", osg::Uniform::FLOAT );
        shader->set( "intensity", gIntensity );

        // create the unit and attach the shader to it
        tiltShift->getOrCreateStateSet()->setAttributeAndModes( shader );
        aoUnit->addChild( tiltShift );
        depthBypass->addChild( tiltShift );
        colorBypass->addChild( tiltShift );
        aoUnit->setName( "ComputeTiltShift" );
    }
    lastUnit = aoUnit;

    return processor;
}

} // end namepsace


class ViewerWidget : public QWidget, public osgViewer::CompositeViewer {
public:
    ViewerWidget( ) : QWidget() {
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
        }


        {
            osgViewer::View* view = new osgViewer::View;
            addView( view );

            osg::Camera* camera = view->getCamera();
            camera->setGraphicsContext( gw );

            const osg::GraphicsContext::Traits* traits = gw->getTraits();

            camera->setClearColor( osg::Vec4( 0.2, 0.2, 0.6, 1.0 ) );
            camera->setViewport( new osg::Viewport( 0, 0, traits->width, traits->height ) );
            camera->setProjectionMatrixAsPerspective( 30.0f, static_cast<double>( traits->width )/static_cast<double>( traits->height ), 1.0f, 100000.0f );

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

                osgPPU::UnitOut* ppuout = new osgPPU::UnitOut();
                ppuout->setName( "PipelineResult" );
                ppuout->setInputTextureIndexForViewportReference( -1 ); // need this here to get viewport from camera
                ppuout->setViewport( new osg::Viewport( 0,0,traits->width, traits->height ) );
                lastUnit->addChild( ppuout );

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
        setGeometry( 100, 100, 800, 600 );
    }


    virtual void paintEvent( QPaintEvent* ) {
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

    // takes ownership
    void addNode( osg::Node * node ) volatile {
        ViewerWidget * that = const_cast< ViewerWidget * >(this);
        boost::lock_guard<boost::mutex> lock( that->_queueMutex );
        that->_addNodeQueue.push( node );
    }

    void removeNode( osg::Node * node ) volatile {
        ViewerWidget * that = const_cast< ViewerWidget * >(this);
        boost::lock_guard<boost::mutex> lock( that->_queueMutex );
        that->_removeNodeQueue.push( node );
    }

protected:
    QTimer _timer;
    boost::mutex _queueMutex;
    std::queue< osg::ref_ptr< osg::Node > > _addNodeQueue; 
    std::queue< osg::ref_ptr< osg::Node > > _removeNodeQueue; 
};

struct Loader
{
    virtual osg::Node * createNode() = 0;
};



struct Interpreter
{
    Interpreter( volatile ViewerWidget * viewer ) : _viewer( viewer ) {}
    void operator()()
    {
        std::string line;
        while (std::getline(std::cin, line)) {
            if ( line.empty() || line[0] == '#' ) continue;
            std::stringstream ss( line );
            std::string cmd;
            if ( ss >> cmd ){
                if ( "load" == cmd ){
                    load( ss ) || std::cerr << "error: cannot load\n";
                }
                else if ( "unload" == cmd ){
                    unload( ss ) || std::cerr << "error: cannot unloaload\n";
                }
                else if ( "list" == cmd ){
                    list( ss ) || std::cerr << "error: cannot list\n";
                }
                else {
                    std::cerr << "error: '" << cmd << "' command not found\n";
                }
            }
        }
        if (QApplication::instance()) QApplication::instance()->quit();
    }

    bool list( std::stringstream & ) const
    {
        for ( auto l : _nodeMap ) {
            std::cout << "    " << l.first << "\n";
        }
        return true;
    }

    bool unload( std::stringstream & ss )
    {
        std::string layerName;
        if ( ss >> layerName )
        {
            const auto found = _nodeMap.find( layerName );
            if ( found != _nodeMap.end() ){
                _viewer->removeNode( found->second.get() );
            }
            else {
                std::cerr << "error: layer '" << layerName << "' not found\n";
                return false;
            }
        }
        else
        {
            std::cerr << "error: not enough arguments\n";
            return false;
        }
        return true;
    }
    bool load( std::stringstream & ss )
    {
        std::string layerName;
        std::string fileName;
        if ( ss >> layerName >> fileName )
        {
            const auto found = _nodeMap.find( layerName );
            if ( found != _nodeMap.end() ){
                std::cerr << "error: '" << layerName << "' already exists\n";
                return false;
            }
            osg::ref_ptr< osg::Node > scene = osgDB::readNodeFile( fileName );
            if ( !scene.get() ) {
                std::cerr << "error: cannot load '" << fileName << "'\n";
                return false;
            }
            // create white material
            osg::Material *material = new osg::Material();
            material->setDiffuse(osg::Material::FRONT,  osg::Vec4(0.97, 0.97, 0.97, 1.0));
            material->setSpecular(osg::Material::FRONT, osg::Vec4(0.5, 0.5, 0.5, 1.0));
            material->setAmbient(osg::Material::FRONT,  osg::Vec4(0.3, 0.3, 0.3, 1.0));
            material->setEmission(osg::Material::FRONT, osg::Vec4(0.0, 0.0, 0.0, 1.0));
            material->setShininess(osg::Material::FRONT, 25.0);
             
            // assign the material to the scene
            scene->getOrCreateStateSet()->setAttribute(material);

            _viewer->addNode( scene.get() );
            _nodeMap.insert( std::make_pair( layerName, scene.get() ) );
            return true;
        }
        else
        {
            std::cerr << "error: not enough arguments\n";
            return false;
        }
    }
public:
    volatile ViewerWidget * _viewer;
    std::map< std::string, osg::ref_ptr< osg::Node > > _nodeMap;
};

int main( int argc, char** argv )
{
    osg::DisplaySettings::instance()->setNumMultiSamples( 4 );
    QApplication app( argc, argv );
    ViewerWidget* viewWidget = new ViewerWidget();
    viewWidget->show();
    
    // start interpretter
    Interpreter interpreter( viewWidget );
    boost::thread interpreterThread( interpreter );

    const int ret = app.exec();

    interpreterThread.join(); // interpretter has a pointer on the viewer, it's safer to join it befaore the viwer gets destroyed

    return ret;
}
