#include "viewer/PostGisUtils.h"
#include "TestGeometry.h"

#include <osgViewer/Viewer>
#include <iostream>


using namespace Stack3d;
using namespace Viewer;

int main()
{

    std::vector< TestGeometry > testGeometry( createTestGeometries() );

    TriangleMesh mesh( osg::Matrix::identity() );
    
    for (size_t t=0; t<testGeometry.size(); t++){

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

        std::cerr << testGeometry[t].wkt << "\n";

        try {
            mesh.push_back( g.get() );
        }
        catch (std::exception & e ) {
            if ( testGeometry[t].isValid ){
                ERROR << "failed to process valid geometry: "  << testGeometry[t].wkt << " " << testGeometry[t].comment << " " << e.what() << "\n";
                return EXIT_FAILURE;
            }
            else { 
                WARNING << "failed to process invalid geometry: "  << testGeometry[t].wkt << " " << testGeometry[t].comment << e.what() << "\n";
            }
        }

    }
    osg::ref_ptr<osg::Geometry> osgGeom = mesh.createGeometry();

    std::cerr << "here\n";
    osgViewer::Viewer v;
    osg::ref_ptr< osg::Geode > geode;
    geode->addDrawable( osgGeom.get() );
    v.setSceneData( geode.get() );
    v.realize();
    v.run();


    std::cerr << "not implemented\n";
    return EXIT_SUCCESS;
}
