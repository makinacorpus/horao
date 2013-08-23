#ifndef STACK3D_VIEWER_POSTGISUTILS
#define STACK3D_VIEWER_POSTGISUTILS

#include "Log.h"

#include <cassert>

#include <osg/Geometry>

#include <libpq-fe.h>
#include <postgres_fe.h>
#include <catalog/pg_type.h>

extern "C" {
#include <liblwgeom.h>
}

inline
void errorreporter(const char* fmt, va_list ap)
{
    va_start(ap, fmt);
    ERROR << va_arg(ap, const char *);
}
namespace Stack3d {
namespace Viewer {

// utility class mainly for RAII of PGresult

//utility class for RAII of LWGEOM
struct Lwgeom
{
    static void initialize()
    {
        lwgeom_set_handlers(NULL, NULL, NULL, errorreporter, NULL);
    }

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

osg::Geometry * createGeometry( const LWGEOM * lwgeom, const osg::Matrixd & layerToWord );

osg::PrimitiveSet * offsetIndices( osg::PrimitiveSet * primitiveSet, size_t offset);


inline
const std::string 
tileQuery( std::string query, float xmin, float xmax, float ymin, float ymax )
{ 
    const char * spacialMetaComments[] = {"/**WHERE TILE &&", "/**AND TILE &&"};

    for ( size_t i = 0; i < sizeof(spacialMetaComments)/sizeof(char *); i++ ){
        const size_t where = query.find(spacialMetaComments[i]);
        if ( where != std::string::npos )
        {
            query.replace (where, 3, "");
            const size_t end = query.find("*/", where);
            if ( end == std::string::npos ){
                ERROR << "unended comment in query";
                return "";
            }
            query.replace (end, 2, "");
            
            std::stringstream bbox;
            bbox << "ST_MakeEnvelope(" << xmin << "," << ymin << "," << xmax << "," << ymax << ")";
            const size_t tile = query.find("TILE", where);
            assert( tile != std::string::npos );
            query.replace( tile, 4, bbox.str().c_str() ); 
        }
    }
    return query;
}


}
}

#undef LC

#endif // OSGEARTH_DRIVER_POSTGIS_FEATURE_UTILS

