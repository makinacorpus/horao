#include "viewer/PostGisUtils.h"
#include "TestGeometry.h"

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>

#include <iostream>


using namespace Stack3d;
using namespace Viewer;

int main(int argc, char ** argv)
{

    std::vector< TestGeometry > testGeometry( createTestGeometries() );

    
    for (size_t t=0; t<testGeometry.size(); t++){

        TriangleMesh mesh( osg::Matrix::identity() );

        //std::cerr << t << (testGeometry[t].isValid ? " valid " : " invalid ")  << testGeometry[t].comment << " " << testGeometry[t].wkt << "\n";


        try {
            Lwgeom g( testGeometry[t].wkt.c_str(), Lwgeom::WKT() );
            
            if ( g.get()->type == POINTTYPE 
              || g.get()->type == MULTIPOINTTYPE 
              || g.get()->type == LINETYPE 
              || g.get()->type == MULTILINETYPE 
              || g.get()->type == MULTISURFACETYPE 
              || g.get()->type == MULTICURVETYPE 
              || g.get()->type == CIRCSTRINGTYPE 
              || g.get()->type == COMPOUNDTYPE 
              || g.get()->type == CURVEPOLYTYPE ){
                continue;
            }

            mesh.push_back( g.get() );

            if ( !testGeometry[t].isValid ){
                WARNING << "failed to detect invalid geometry: "  << testGeometry[t].wkt << " " << testGeometry[t].comment << "\n";
                //return EXIT_FAILURE;
            }
        }
        catch (std::exception & e ) {
            if ( testGeometry[t].isValid ){
                ERROR << "failed to process valid geometry: "  << testGeometry[t].wkt << " " << testGeometry[t].comment << " " << e.what() << "\n";
                return EXIT_FAILURE;
            }
            else { 
                //DEBUG_TRACE << "invalid geometry (" << testGeometry[t].comment  << ") caused:"<< e.what() << "\n";
            }
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
