#include "viewer/PostGisUtils.h"
#include "TestGeometry.h"

#include <osgViewer/Viewer>
#include <iostream>


using namespace Stack3d;
using namespace Viewer;

int main()
{

    std::vector< TestGeometry > testGeometry( createTestGeometries() );

    
    for (size_t t=0; t<testGeometry.size(); t++){

        TriangleMesh mesh( osg::Matrix::identity() );

        std::cerr << testGeometry[t].wkt << " " << testGeometry[t].comment << "\n";


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
        osg::ref_ptr<osg::Geometry> osgGeom = mesh.createGeometry();
        if(0){
            osgViewer::Viewer v;
            osg::ref_ptr< osg::Geode > geode = new osg::Geode;
            geode->addDrawable( osgGeom.get() );
            v.setSceneData( geode.get() );
            v.realize();
            v.run();
        }

    }


    return EXIT_SUCCESS;
}
