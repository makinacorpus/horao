#include "viewer/PostGisUtils.h"
#include "TestGeometry.h"

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>

#include <iostream>

int main(int argc, char ** argv)
{
    std::vector< TestGeometry > testGeometry( createTestGeometries() );
    
    for (size_t t=0; t<testGeometry.size(); t++){

        osgGIS::Mesh mesh( osg::Matrix::identity() );

        std::cout << "debug: " << t << (testGeometry[t].isValid ? " valid " : " invalid ")  
            << testGeometry[t].comment << " " << testGeometry[t].wkt << "\n";


        try {
            mesh.push_back( osgGIS::WKT( testGeometry[t].wkt.c_str() ) );
        }
        catch (std::exception & e ) {
            if ( testGeometry[t].isValid 
                    && std::string(e.what()).find("not handled") == std::string::npos ){
                std::cerr << "failed to process valid geometry: "  
                    << testGeometry[t].wkt << " " << testGeometry[t].comment 
                    << " " << e.what() << "\n";
                return EXIT_FAILURE;
            }
            //std::cout << "debug:" << "invalid geometry (" << testGeometry[t].comment  << ") caused:"<< e.what() << "\n";
        }

        osg::ref_ptr<osg::Geometry> osgGeom = mesh.createGeometry();

        if( argc==2 && "-v" == std::string(argv[1]) ){
            osgViewer::Viewer v;
            osg::ref_ptr< osg::Geode > geode = new osg::Geode;
            geode->addDrawable( osgGeom.get() );
            v.setSceneData( geode.get() );
            v.setUpViewInWindow(800, 0, 800, 800 );
            v.addEventHandler(new osgViewer::StatsHandler);
            v.addEventHandler(new osgGA::StateSetManipulator( v.getCamera()->getOrCreateStateSet()) );
            v.addEventHandler(new osgViewer::ScreenCaptureHandler);
            v.setFrameStamp( new osg::FrameStamp );
            v.addEventHandler(new osgViewer::WindowSizeHandler);
            v.realize();
            v.run();
        }
    }

    return EXIT_SUCCESS;
}
