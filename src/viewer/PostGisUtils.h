#ifndef STACK3D_VIEWER_POSTGISUTILS
#define STACK3D_VIEWER_POSTGISUTILS

#include <cassert>

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

struct PostGisUtils
{
    // utility class mainly for RAII of PGresult
    struct QueryResult
    {
        QueryResult( PGconn * conn, const std::string & query )
            : _res( PQexec( conn, query.c_str() ) )
            , _error( PQresultErrorMessage(_res) )
        {
            //nop
        }

        ~QueryResult() 
        { 
            PQclear(_res);
        }

        operator bool() const { return _error.empty(); }

        PGresult * get(){ return _res; }

        const std::string & error() const { return _error; }

    private:
        PGresult * _res;
        const std::string _error;
        // non copyable
        QueryResult( const QueryResult & );
        QueryResult operator=( const QueryResult &); 
    };

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

    static osg::Node * createGeometry( const LWGEOM * lwgeom );
};

#undef LC

#endif // OSGEARTH_DRIVER_POSTGIS_FEATURE_UTILS

