#ifndef STACK3D_VIEWER_POSTGISUTILS
#define STACK3D_VIEWER_POSTGISUTILS

#include "Log.h"

#include <cassert>
#include <GL/glu.h>

#include <osg/Geometry>

#include <libpq-fe.h>
#include <postgres_fe.h>
#include <catalog/pg_type.h>

extern "C" {
#include <liblwgeom.h>
}

#ifndef CALLBACK
#define CALLBACK
#endif


namespace Stack3d {
namespace Viewer {

//utility class for RAII of LWGEOM
struct Lwgeom
{
    struct WKT {};
    struct WKB {};
    Lwgeom( const char * wkt, WKT )
        : _geom( lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE) )
    {}
    Lwgeom( const char * wkb, WKB )
        : _geom( lwgeom_from_hexwkb(wkb, LW_PARSER_CHECK_NONE) )
    {}
    operator bool() const { return _geom; }
    const LWGEOM * get() const { return _geom; }
    const LWGEOM * operator->() const { return _geom; }
    ~Lwgeom()
    {
        if (_geom) lwgeom_free(_geom);
    }
private:
    LWGEOM * _geom;

};

inline
std::ostream & operator<<( std::ostream & o, const osg::Vec3 & v )
{
    o << "( " << v.x() << ", " << v.y() << ", " << v.z() << " )";
    return o;
}



// temporary structure to avois creation of many mall osg::geometries (slow)
struct TriangleMesh
{
    TriangleMesh( const osg::Matrixd & layerToWord )
        : _layerToWord(layerToWord)
    {}

    void push_back( const LWGEOM * );

    void addBar( const osg::Vec3 & center, float width, float depth, float height );
    
    osg::Geometry * createGeometry() const;

    std::vector<osg::Vec3>::iterator begin(){return _vtx.begin();}
    std::vector<osg::Vec3>::iterator end(){return _vtx.end();}

private:
    std::vector<osg::Vec3> _vtx;
    std::vector<osg::Vec3> _nrml;
    std::vector<unsigned> _tri;
    const osg::Matrixd _layerToWord;

    template< typename MULTITYPE >
    void push_back( const MULTITYPE * );

    friend void CALLBACK tessVertexCB(const GLvoid *vtx, void *data);

    void push_back( const LWTRIANGLE * );
    void push_back( const LWPOLY * );
};

}
}

#undef LC

#endif // OSGEARTH_DRIVER_POSTGIS_FEATURE_UTILS

