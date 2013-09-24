/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2013 Pelican Mapping
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "PostGisUtils.h"
#include "Log.h"

#include <osgUtil/Tessellator>

#include <boost/format.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/undirected_dfs.hpp>
#include <boost/noncopyable.hpp>

#define POLY2TRI
#ifdef POLY2TRI
#include "../poly2tri/poly2tri.h"
#include <memory>
#endif

namespace Stack3d {
namespace Viewer {

inline
void errorreporter(const char* fmt, va_list ap)
{
    struct RaiiCharPtr 
    { 
        ~RaiiCharPtr(){ free(ptr); }
        char * ptr;
    };
    RaiiCharPtr msg;

	if (!lw_vasprintf (&msg.ptr, fmt, ap))
	{
		va_end (ap);
		return;
	}
    
    throw Exception(msg.ptr);
}

struct LwgeomInitialiser 
{
    LwgeomInitialiser(){
        lwgeom_set_handlers(NULL, NULL, NULL, errorreporter, NULL);
        DEBUG_TRACE << "initialized error handler\n";
    }
} lwgeomInitialiser; // dummy class to ensure initilization is done



// we do that orselves since an osg::Box for each feature is really slow
void TriangleMesh::addBar( const osg::Vec3 & center, float width, float depth, float height )
{
    /*
    const osg::Vec3 c = center*_layerToWord;
    const osg::Vec3 v[8] = 
    { 
        c + osg::Vec3( -width, -depth, -height )*.5f,
        c + osg::Vec3(  width, -depth, -height )*.5f, 
        c + osg::Vec3(  width,  depth, -height )*.5f, 
        c + osg::Vec3( -width,  depth, -height )*.5f, 
        c + osg::Vec3( -width, -depth,  height )*.5f, 
        c + osg::Vec3(  width, -depth,  height )*.5f, 
        c + osg::Vec3(  width,  depth,  height )*.5f, 
        c + osg::Vec3( -width,  depth,  height )*.5f 
    };
    const int offset = _vtx.size();
    _vtx.push_back( v[2] );
    _vtx.push_back( v[1] );
    _vtx.push_back( v[0] );
    _vtx.push_back( v[3] );
    _vtx.push_back( v[2] );
    _vtx.push_back( v[0] );
    for (int i=0; i<6; i++) _nrml.push_back( osg::Vec3(  0,  0, -1) );
    _vtx.push_back( v[4] );
    _vtx.push_back( v[5] );
    _vtx.push_back( v[6] );
    _vtx.push_back( v[4] );
    _vtx.push_back( v[6] );
    _vtx.push_back( v[7] );
    for (int i=0; i<6; i++) _nrml.push_back( osg::Vec3(  0,  0,  1) );
    _vtx.push_back( v[0] );
    _vtx.push_back( v[1] );
    _vtx.push_back( v[4] );
    _vtx.push_back( v[1] );
    _vtx.push_back( v[5] );
    _vtx.push_back( v[4] );
    for (int i=0; i<6; i++) _nrml.push_back( osg::Vec3(  0, -1,  1) );
    _vtx.push_back( v[2] );
    _vtx.push_back( v[3] );
    _vtx.push_back( v[7] );
    _vtx.push_back( v[2] );
    _vtx.push_back( v[7] );
    _vtx.push_back( v[6] );
    for (int i=0; i<6; i++) _nrml.push_back( osg::Vec3(  0,  1,  0) );
    _vtx.push_back( v[7] );
    _vtx.push_back( v[3] );
    _vtx.push_back( v[0] );
    _vtx.push_back( v[4] );
    _vtx.push_back( v[7] );
    _vtx.push_back( v[0] );
    for (int i=0; i<6; i++) _nrml.push_back( osg::Vec3( -1,  0,  0) );
    _vtx.push_back( v[2] );
    _vtx.push_back( v[5] );
    _vtx.push_back( v[1] );
    _vtx.push_back( v[2] );
    _vtx.push_back( v[6] );
    _vtx.push_back( v[5] );
    for (int i=0; i<6; i++) _nrml.push_back( osg::Vec3(  1,  0,  0) );
    const int numFaces = 6;
    const int numTriPerFace = 2;
    const int numVtxPerTri = 3;
    for (int i=0; i<numFaces*numTriPerFace*numVtxPerTri; i++) _tri.push_back( i + offset );
    */

    // we build a bevelled box, without a bottom
    // it's base is centerd on origin

    const float x = .5f*width;
    const float y = .5f*depth;
    const float e = width/20;

    const unsigned o = unsigned(_vtx.size());
    // vertex indices for base, top of the side faces and cap
    const unsigned b[8] = { o, o+1, o+2, o+3, o+4, o+5, o+6, o+7 }; 
    const unsigned t[8] = { o+8, o+9, o+10, o+11, o+12, o+13, o+14, o+15 };
    const unsigned c[4] = { o+16, o+17, o+18, o+19 };

    const osg::Vec3 vb[8] = {
        osg::Vec3(-x+e, -y  , 0),
        osg::Vec3( x-e, -y  , 0),
        osg::Vec3( x  , -y+e, 0),
        osg::Vec3( x  ,  y-e, 0),
        osg::Vec3( x-e,  y  , 0),
        osg::Vec3(-x+e,  y  , 0),
        osg::Vec3(-x  ,  y-e, 0),
        osg::Vec3(-x  , -y+e, 0)};

    const osg::Vec3 nb[8] = { 
        osg::Vec3( 0, -1, 0 ),
        osg::Vec3( 0, -1, 0 ),
        osg::Vec3( 1,  0, 0 ),
        osg::Vec3( 1,  0, 0 ),
        osg::Vec3( 0,  1, 0 ),
        osg::Vec3( 0,  1, 0 ),
        osg::Vec3(-1,  0, 0 ),
        osg::Vec3(-1,  0, 0 )};

    osg::Vec3 vt[8];
    for (size_t i=0; i<8; i++) vt[i] = vb[i] + osg::Vec3(0,0,height-e);


    const osg::Vec3 vc[4] = {
        osg::Vec3(-x+e, -y+e, height),
        osg::Vec3( x-e, -y+e, height),
        osg::Vec3( x-e,  y-e, height),
        osg::Vec3(-x+e,  y-e, height)};

    const osg::Vec3 nc[4] = {
        osg::Vec3(0, 0, 1),
        osg::Vec3(0, 0, 1),
        osg::Vec3(0, 0, 1),
        osg::Vec3(0, 0, 1)};

    _vtx.insert(_vtx.end(), vb, vb+8);
    _nrml.insert(_nrml.end(), nb, nb+8);

    _vtx.insert(_vtx.end(), vt, vt+8);
    _nrml.insert(_nrml.end(), nb, nb+8); // same nrml as bottom

    _vtx.insert(_vtx.end(), vc, vc+4);
    _nrml.insert(_nrml.end(), nc, nc+4);

    // sides, loop / indices
    for (size_t i=0; i<8; i++){
       _tri.push_back(b[i]);
       _tri.push_back(b[(i+1)%8]);
       _tri.push_back(t[i]);
       _tri.push_back(t[i]);
       _tri.push_back(b[(i+1)%8]);
       _tri.push_back(t[(i+1)%8]);
    }
    
    // top
    _tri.push_back(c[0]);
    _tri.push_back(c[1]);
    _tri.push_back(c[2]);
    _tri.push_back(c[0]);
    _tri.push_back(c[2]);
    _tri.push_back(c[3]);

    // top bevel
    for (size_t i=0; i<4; i++){
        _tri.push_back(t[(i*2)%8]); 
        _tri.push_back(t[(i*2+1)%8]); 
        _tri.push_back(c[i]); 
        _tri.push_back(c[i]); 
        _tri.push_back(t[(i*2+1)%8]);
        _tri.push_back(c[(i+1)%4]); 
    }

    // top corners
    for (size_t i=0; i<4; i++){
        _tri.push_back(t[(i*2+1)%8]); 
        _tri.push_back(t[(i*2+2)%8]); 
        _tri.push_back(c[(i+1)%4]);
    }
    
    // translate all vtx by base center
    const osg::Vec3 bc = center*_layerToWord;
    const size_t sz = _vtx.size();
    assert( _vtx.size() - o == 20 );
    for ( size_t i=o; i<sz; i++ ) _vtx[i] += bc;
}

void TriangleMesh::push_back( const LWTRIANGLE * lwtriangle )
{
    assert( lwtriangle );
    const int offset = _vtx.size();
    for( int v = 0; v < 3; v++ )
    {
        const POINT3DZ p3D = getPoint3dz( lwtriangle->points, v );
        const osg::Vec3d p( p3D.x, p3D.y, p3D.z );
        _vtx.push_back( p * _layerToWord );
    }

    for (int i=0; i<3; i++) _tri.push_back( i + offset );

    osg::Vec3 normal( 0.0, 0.0, 0.0 );
    for ( int i = 0; i < 3; ++i ) {
       osg::Vec3 pi = _vtx[offset+i];
       osg::Vec3 pj = _vtx[offset +( (i+1) % 3 ) ];
       normal[0] += ( pi[1] - pj[1] ) * ( pi[2] + pj[2] );
       normal[1] += ( pi[2] - pj[2] ) * ( pi[0] + pj[0] );
       normal[2] += ( pi[0] - pj[0] ) * ( pi[1] + pj[1] );
    }
    normal.normalize();

    if (( FLAGS_GET_Z( lwtriangle->flags ) == 0 ) && ( normal[2] < 0 )) {
        // if this is a 2D surface and the normal is pointing down, reverse the triangle
        normal[2] = 1;
        std::swap( _tri[ offset ], _tri[offset + 2] );
    }
    for (int i=0; i<3; i++) _nrml.push_back( normal );
}

// nop callback
void CALLBACK noStripCB(GLboolean flag)
{
    //assert( flag );
    (void)flag;
}

void CALLBACK tessBeginCB(GLenum which)
{
    //assert( which == GL_TRIANGLES );
    (void)which;
}

void CALLBACK tessEndCB(GLenum which)
{
    //assert( which == GL_TRIANGLES );
    (void)which;
}

void CALLBACK tessErrorCB(GLenum errorCode)
{
    const GLubyte * errorStr = gluErrorString(errorCode);
    throw Exception(reinterpret_cast<const char *>(errorStr));
}

void CALLBACK tessVertexCB(const GLdouble *vtx, void *data)
{
    // cast back to double type
    TriangleMesh * that = (TriangleMesh * )data;
    that->_tri.push_back( that->_vtx.size() );
    that->_vtx.push_back( osg::Vec3( vtx[0], vtx[1], vtx[2] ) );
}

void CALLBACK tessCombineCB(GLdouble coords[3], GLdouble * /*vertex_data*/[4], GLfloat /*weight*/[4], void ** outData, void *data)
{
    GLdouble * vertex = (GLdouble *) malloc(3 * sizeof(GLdouble));
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    vertex[2] = coords[2];
    *outData = vertex;
    TriangleMesh * that = (TriangleMesh * )data;
    that->_tri.push_back( that->_vtx.size() );
    that->_vtx.push_back( osg::Vec3( vertex[0], vertex[1], vertex[2] ) );
}

// for RAII off GLUtesselator
struct Tessellator
{
    Tessellator(){
        _tess = gluNewTess();
        gluTessProperty(_tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
        //gluTessProperty(_tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE); 
        gluTessCallback(_tess, GLU_TESS_BEGIN, (void (*)(void))tessBeginCB);
        gluTessCallback(_tess, GLU_TESS_END, (void (*)(void))tessEndCB);
        gluTessCallback(_tess, GLU_TESS_ERROR, (void (*)(void))tessErrorCB);
        gluTessCallback(_tess, GLU_TESS_VERTEX_DATA, (void (*)())tessVertexCB);
	    gluTessCallback(_tess, GLU_TESS_EDGE_FLAG,  (void (*)())noStripCB);
	    gluTessCallback(_tess, GLU_TESS_COMBINE_DATA,  (void (*)())tessCombineCB);
    }
    ~Tessellator(){
        gluDeleteTess(_tess);
    }

    GLUtesselator * _tess;
};

struct Validity {
    /**
     * @note the class has private ctor to force the use of functions valid() and invalid(reason) that are clearer in the code than to remember that "Valid constructed with a reason is invalid"
     */
    static const Validity valid() {
        return Validity();
    }
    static const Validity invalid( const std::string& reason ) {
        return Validity( reason );
    }
    operator bool() const {
        return _valid;
    }
    const std::string& reason() const {
        return _reason;
    }
private:
    bool _valid; // not const to allow default copy
    std::string _reason;
    /**
     * @brief default ctor for valid
     */
    Validity():_valid( true ) {}
    /**
     * @brief if we construct with a reason, the class is invalid
     */
    Validity( const std::string& preason ):
        _valid( false ),
        _reason( preason )
    {}
};

// to detect unconnected interior in polygon
struct LoopDetector : public boost::dfs_visitor<> {
    LoopDetector( bool& hasLoop ):_hasLoop( hasLoop ) {}

    template <class Edge, class Graph>
    void back_edge( Edge, const Graph& ) {
        _hasLoop = true;
    }
private:
    bool& _hasLoop;
};


struct Ring : std::vector< osg::Vec3 > {};
struct Ring2d : std::vector< osg::Vec2 > {};

//! stores polygon with points converted to osg format
//! to avoid doing conversion several time
//! and prunes dupplicate points
struct Poly : boost::noncopyable
{
    Poly( const LWPOLY * lwpoly, const osg::Matrix & layerToWord  )
    : rings( lwpoly->nrings )
    {
        const size_t nrings = rings.size();
        for (size_t r=0; r<nrings; r++){
            const size_t npoints = lwpoly->rings[r]->npoints;
            rings[r].reserve(npoints);
            for (size_t p=0; p<npoints; p++){
                const POINT3DZ p3D = getPoint3dz( lwpoly->rings[r], p );
                const osg::Vec3 point =  osg::Vec3( p3D.x, p3D.y, p3D.z ) * layerToWord;
                if ( !p || rings[r].back() != point ) rings[r].push_back( point );
            }
        }
    }

    std::vector< Ring > rings;
};

struct Poly2d : boost::noncopyable
{
    // project Polygon on base
    Poly2d( const Poly & poly, const osg::Vec3 base[2] )
    : rings( poly.rings.size() )
    {
        const size_t nrings = rings.size();
        for (size_t r=0; r<nrings; r++){
            const size_t npoints = poly.rings[r].size();
            rings[r].reserve(npoints);
            const osg::Vec3 * srcRing = &(poly.rings[r][0]);
            for (size_t p=0; p<npoints; p++){
                rings[r].push_back( osg::Vec2( srcRing[p]*base[0], srcRing[p]*base[1] ) );
            }
        }
    }

    std::vector< Ring2d > rings;
};


inline
const osg::Vec3 normal( const Poly & poly )
{
    assert(poly.rings.size());
    // Uses Newell's formula
    osg::Vec3 normal( 0.0, 0.0, 0.0 );

    const osg::Vec3 * extRing = &(poly.rings[0][0]);
    const size_t npoints = poly.rings[0].size(); 
    for ( size_t i = 0; i < npoints; ++i )
    {
       const osg::Vec3 pi = extRing[i];
       const osg::Vec3 pj = extRing[ (i+1) % npoints ];
       normal[0] += ( pi[1] - pj[1] ) * ( pi[2] + pj[2] );
       normal[1] += ( pi[2] - pj[2] ) * ( pi[0] + pj[0] );
       normal[2] += ( pi[0] - pj[0] ) * ( pi[1] + pj[1] );
    }
    normal.normalize();
    return normal;
}

inline
bool isPlane( const Poly & poly, const osg::Vec3 * nrml = NULL )
{
    assert(poly.rings.size());
    assert(poly.rings[0].size());
    const osg::Vec3 first = poly.rings[0][0];

    const osg::Vec3 n = nrml ? *nrml : normal(poly);
    
    const size_t nrings = poly.rings.size();
    for ( size_t r=0; r<nrings; r++ ){
        const osg::Vec3 * ring = &(poly.rings[r][0]);
        const size_t npoints = poly.rings[r].size(); 
        for ( size_t i = 0; i < npoints; ++i ) {
            if ( (ring[i] - first) * n > FLT_EPSILON ) return false;
        }
    }
    return true;
}

inline
bool isCovered( const osg::Vec2 & point, const Ring2d & ring )
{
    // use ray casting algorithm
    // ray travels from point toward the positive y direction (constant x), 
    // so crossing a boundary segment [start, end] means:
    // - start.x <= point.x < end.x || end.x < point.x <= start.x
    // - intersection of segment with the ray is above the point
    std::size_t intersectionCount = 0; 
    const size_t npoints = ring.size(); 
    for ( size_t i = 0; i < npoints; ++i ) {
       const osg::Vec2 start = ring[i];
       const osg::Vec2 end = ring[ (i+1) % npoints ];
       // special case with vertical segments aligned above the point
       // we don't consider crossing since it will cross the next segment its start
       // ence the <= or >= at start
       if ( start.x() != end.x() 
          && (((start.x() <= point.x()) && (point.x() < end.x())) || ((end.x() < point.x()) && (point.x() <= start.x())))
          && start.y() + ( ( point.x() - start.x() ) / ( end.x() - start.x() ) ) * ( end.y() - start.y()) >= point.y()  ){
           ++intersectionCount;
       }
    }
    return intersectionCount%2; // even -> outside
} 
/*
inline 
const Validity is_valid( const LWPOLY * lwpoly )
{
    const osg::Vec3 normal = normal(lwpoly);

    // Closed simple rings
    const int numRings =  lwpoly->nrings;

    for ( int r=0; r != numRings; ++r ) {
        if ( lwpoly->ring[r].npoints < 4 ) {
            return Validity::invalid( ( boost::format( "not enought points in ring %d" ) % r ).str() );
        }

        const POINT3DZ start = getPoint3dz( lwpoly->rings[0], 0 );
        const POINT3DZ end   = getPoint3dz( lwpoly->rings[0], lwpoly->ring[r].npoints - 1 );
        const double distanceToClose = osg::Vec3( start.x-end.x, start.y-end.y, start.z-end.z ).length2();
        if ( distanceToClose > 0 ) {
            return Validity::invalid( ( boost::format( "ring %d is not closed" ) % r ).str() );
        }

        if ( p.is3D() ? selfIntersects3D( p.ringN( r ) ) : selfIntersects( p.ringN( r ) ) ) {
            return Validity::invalid( ( boost::format( "ring %d self intersects" ) % r ).str() );
        }
    }

    // Orientation 
    // Polygone must be planar (all points in the same plane)
    if ( !isPlane3D< Kernel >( p, toleranceAbs ) ) {
        return Validity::invalid( "points don't lie in the same plane" );
    }

    // interior rings must be oriented opposit to exterior;
    if ( p.hasInteriorRings() ) {
        const CGAL::Vector_3< Kernel > nExt = normal3D< Kernel >( p.exteriorRing() );

        for ( std::size_t r=0; r<p.numInteriorRings(); ++r ) {
            const CGAL::Vector_3< Kernel > nInt = normal3D< Kernel>( p.interiorRingN( r ) );

            if ( nExt * nInt > 0 ) {
                return Validity::invalid( ( boost::format( "interior ring %d is oriented in the same direction as exterior ring" ) % r ).str() );
            }
        }
    }

    // Rings must not share more than one point (no intersection)
    {
        typedef std::pair<int,int> Edge;
        std::vector<Edge> touchingRings;

        for ( size_t ri=0; ri < numRings; ++ri ) { // no need for numRings-1, the next loop won't be entered for the last ring
            for ( size_t rj=ri+1; rj < numRings; ++rj ) {
                std::auto_ptr<Geometry> inter = p.is3D()
                                                ? intersection3D( p.ringN( ri ), p.ringN( rj ) )
                                                : intersection( p.ringN( ri ), p.ringN( rj ) );

                if ( ! inter->isEmpty() && ! inter->is< Point >() ) {
                    return Validity::invalid( ( boost::format( "intersection between ring %d and %d" ) % ri % rj ).str() );
                }
                else if ( ! inter->isEmpty() && inter->is< Point >() ) {
                    touchingRings.push_back( Edge( ri,rj ) );
                }
            }
        }

        {
            using namespace boost;
            typedef adjacency_list< vecS, vecS, undirectedS,
                    no_property,
                    property<edge_color_t, default_color_type> > Graph;
            typedef graph_traits<Graph>::vertex_descriptor vertex_t;

            Graph g( touchingRings.begin(), touchingRings.end(), numRings );

            bool hasLoop = false;
            LoopDetector vis( hasLoop );
            undirected_dfs( g, root_vertex( vertex_t( 0 ) ).visitor( vis ).edge_color_map( get( edge_color, g ) ) );

            if ( hasLoop ) {
                return Validity::invalid( "interior is not connected" );
            }
        }
    }

    if ( p.hasInteriorRings() ) {
        // Interior rings must be interior to exterior ring
        for ( size_t r=0; r < p.numInteriorRings(); ++r ) { // no need for numRings-1, the next loop won't be entered for the last ring
            if ( p.is3D()
                    ? !coversPoints3D( Polygon( p.exteriorRing() ), Polygon( p.interiorRingN( r ) ) )
                    : !coversPoints( Polygon( p.exteriorRing() ), Polygon( p.interiorRingN( r ) ) )
               ) {
                return Validity::invalid( ( boost::format( "exterior ring doesn't cover interior ring %d" ) % r ).str() );
            }
        }

        // Interior ring must not cover one another
        for ( size_t ri=0; ri < p.numInteriorRings(); ++ri ) { // no need for numRings-1, the next loop won't be entered for the last ring
            for ( size_t rj=ri+1; rj < p.numInteriorRings(); ++rj ) {
                if ( p.is3D()
                        ? coversPoints3D( Polygon( p.interiorRingN( ri ) ), Polygon( p.interiorRingN( rj ) ) )
                        : coversPoints( Polygon( p.interiorRingN( ri ) ), Polygon( p.interiorRingN( rj ) ) )
                   ) {
                    return Validity::invalid( ( boost::format( "interior ring %d covers interior ring %d" ) % ri % rj ).str() );
                }
            }
        }
    }
}
*/

#ifdef POLY2TRI
void TriangleMesh::push_back( const LWPOLY * lwpoly )
{
    assert( lwpoly );

    const int numRings = lwpoly->nrings;
    if ( numRings == 0 ) return;

    /*
    if (!is_valid(lwpoly)){
        //! todo draw lines for rings but no contour
        return;
    }
    */

    // Normal computation.
    //
    // We cannot accurately rely on triangles from the tessellation, since we could have
    // very "degraded" triangles (close to a line), and the normal computation would be bad.
    // In this case, we would have to average the normal vector over each triangle of the polygon.
    // The Newell's formula is simpler and more direct here.
    osg::Vec3 longestEdge;
    float norm2 = 0;
    osg::Vec3 normal( 0.0, 0.0, 0.0 );
    const int sz = lwpoly->rings[0]->npoints;
    for ( int i = 0; i < sz; ++i )
    {
       const POINT3DZ p3Di = getPoint3dz( lwpoly->rings[0], i );
       const POINT3DZ p3Dj = getPoint3dz( lwpoly->rings[0], (i+1) % sz );
       const osg::Vec3 pi= osg::Vec3( p3Di.x, p3Di.y, p3Di.z ) * _layerToWord;
       const osg::Vec3 pj= osg::Vec3( p3Dj.x, p3Dj.y, p3Dj.z ) * _layerToWord;
       if ( (pi - pj).length2() > norm2 ){
           longestEdge = pi - pj;
           norm2 = (pi - pj).length2();
       }
       normal[0] += ( pi[1] - pj[1] ) * ( pi[2] + pj[2] );
       normal[1] += ( pi[2] - pj[2] ) * ( pi[0] + pj[0] );
       normal[2] += ( pi[0] - pj[0] ) * ( pi[1] + pj[1] );
    }
    if ( normal.length2() < FLT_MIN || longestEdge.length2() < FLT_MIN ) return; // degenerated to a point
    normal.normalize();
    longestEdge.normalize();

    std::auto_ptr<p2t::CDT> cdt;

    const osg::Vec3 baseX = longestEdge;
    const osg::Vec3 baseY = normal ^ longestEdge;
    // retesselate and add rings
    for ( int r = 0; r < numRings; r++) {
        const int ringSize = lwpoly->rings[r]->npoints;
        std::vector<p2t::Point*> polyline;
        osg::Vec3 prev;
        for( int v = 0; v < ringSize - 1; v++ ) {
            const POINT3DZ p3D = getPoint3dz( lwpoly->rings[r], v );
            const osg::Vec3 p = osg::Vec3( p3D.x, p3D.y, p3D.z ) * _layerToWord;
            if ( v && ( p - prev ).length2() < 2*FLT_MIN) continue;
            polyline.push_back(new p2t::Point(p*baseX, p*baseY) );           
            prev = p;

        }
        if (!r){
            cdt.reset( new p2t::CDT(polyline));
        }
        else {
            cdt->AddHole(polyline);
        }
    }

    cdt->Triangulate();
    std::vector<p2t::Triangle*> triangles(cdt->GetTriangles());
    for (size_t i = 0; i < triangles.size(); i++) {
        p2t::Triangle& t = *triangles[i];
        for (int j=0; j<3; j++){
            p2t::Point& a = *t.GetPoint(j);
            _tri.push_back( _vtx.size() );
            _vtx.push_back( baseX * a.x + baseY * a.y);
        }
    }

    _nrml.resize( _vtx.size(), normal );
}

#else
void TriangleMesh::push_back( const LWPOLY * lwpoly )
{
    assert( lwpoly );

    const int numRings = lwpoly->nrings;
    if ( numRings == 0 ) return;

    size_t totalNumVtx = 0;
    for ( int r = 0; r < numRings; r++) totalNumVtx += lwpoly->rings[r]->npoints;
    std::vector< GLdouble > coord( totalNumVtx*3 );

    const size_t nTriangles = _tri.size();

    // retesselate and add rings
    Tessellator tesselator ;
    gluTessBeginPolygon( tesselator._tess, this); // with NULL data
    size_t currIdx = 0;
    for ( int r = 0; r < numRings; r++) {
        gluTessBeginContour(tesselator._tess);                      // outer quad
        const int ringSize = lwpoly->rings[r]->npoints;
        for( int v = 0; v < ringSize - 1; v++ ) {
            const POINT3DZ p3D = getPoint3dz( lwpoly->rings[r], v );
            const osg::Vec3 p = osg::Vec3( p3D.x, p3D.y, p3D.z ) * _layerToWord;
            coord[currIdx + 0] = p.x();
            coord[currIdx + 1] = p.y();
            coord[currIdx + 2] = p.z();
            gluTessVertex(tesselator._tess, &(coord[currIdx]), &(coord[currIdx]));
            currIdx+=3;
        }
        gluTessEndContour(tesselator._tess);                      // outer quad
    }
    gluTessEndPolygon(tesselator._tess);


    //// Normal computation.
    ////
    //// We cannot accurately rely on triangles from the tessellation, since we could have
    //// very "degraded" triangles (close to a line), and the normal computation would be bad.
    //// In this case, we would have to average the normal vector over each triangle of the polygon.
    //// The Newell's formula is simpler and more direct here.
    osg::Vec3 normal( 0.0, 0.0, 0.0 );
    const int sz = lwpoly->rings[0]->npoints;
    for ( int i = 0; i < sz; ++i )
    {
       const POINT3DZ p3Di = getPoint3dz( lwpoly->rings[0], i );
       const POINT3DZ p3Dj = getPoint3dz( lwpoly->rings[0], (i+1) % sz );
       const osg::Vec3 pi= osg::Vec3( p3Di.x, p3Di.y, p3Di.z ) * _layerToWord;
       const osg::Vec3 pj= osg::Vec3( p3Dj.x, p3Dj.y, p3Dj.z ) * _layerToWord;
       normal[0] += ( pi[1] - pj[1] ) * ( pi[2] + pj[2] );
       normal[1] += ( pi[2] - pj[2] ) * ( pi[0] + pj[0] );
       normal[2] += ( pi[0] - pj[0] ) * ( pi[1] + pj[1] );
    }
    normal.normalize();

    if (( FLAGS_GET_Z( lwpoly->flags ) == 0 ) && ( normal[2] < 0 )) {
        // if this is a 2D surface and the normal is pointing down, reverse each new triangle
        normal[2] = 1;
        for ( size_t i = nTriangles/3; i < _tri.size() / 3; ++i ) {
            std::swap( _tri[3*i], _tri[3*i+2] );
        }
    }
    _nrml.resize( _vtx.size(), normal );

}
#endif
template< typename MULTITYPE >
void TriangleMesh::push_back( const MULTITYPE * lwmulti )
{
    assert( lwmulti );
    const int numGeom = lwmulti->ngeoms;
    for ( int g = 0; g<numGeom; g++ ) push_back( lwmulti->geoms[g] );

}

void TriangleMesh::push_back( const LWGEOM * lwgeom )
{
    assert( lwgeom );
    if ( lwgeom_is_empty( lwgeom ) ) return;
    //! @todo actually create the geometry
    switch ( lwgeom->type )
    {
    case TRIANGLETYPE:          push_back( lwgeom_as_lwtriangle( lwgeom ) ); break;
    case TINTYPE:               push_back( lwgeom_as_lwtin( lwgeom ) ); break;
    case COLLECTIONTYPE:        push_back( lwgeom_as_lwcollection( lwgeom ) ); break;
    case MULTIPOLYGONTYPE:      push_back( lwgeom_as_lwmpoly( lwgeom ) ); break;
    case POLYHEDRALSURFACETYPE: push_back( lwgeom_as_lwpsurface( lwgeom ) ); break;
    case POLYGONTYPE:           push_back( lwgeom_as_lwpoly( lwgeom ) ); break;
    case POINTTYPE:             assert(false && "POINTTYPE not implemented");
    case MULTIPOINTTYPE:        assert(false && "MULTIPOINTTYPE not implemented");
    case LINETYPE:              assert(false && "LINETYPE not implemented");
    case MULTILINETYPE:         assert(false && "MULTIPOINTTYPE not implemented");
    case MULTISURFACETYPE:      assert(false && "MULTISURFACETYPE not implemented");
    case MULTICURVETYPE:        assert(false && "MULTICURVETYPE not implemented");
    case CIRCSTRINGTYPE:        assert(false && "CIRCSTRINGTYPE not implemented");
    case COMPOUNDTYPE:          assert(false && "COMPOUNDTYPE not implemented");
    case CURVEPOLYTYPE:         assert(false && "CURVEPOLYTYPE not implemented");
    }
}

osg::Geometry * TriangleMesh::createGeometry() const
{
    osg::ref_ptr<osg::Geometry> multi = new osg::Geometry();
    multi->setUseVertexBufferObjects(true);

    osg::ref_ptr<osg::Vec3Array> vertices( new osg::Vec3Array( _vtx.begin(), _vtx.end()) );
    multi->setVertexArray( vertices.get() );
    osg::ref_ptr<osg::Vec3Array> normals( new osg::Vec3Array( _nrml.begin(), _nrml.end()) );
    multi->setNormalArray( normals.get() );
    multi->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
    
    osg::ref_ptr<osg::DrawElementsUInt> elem = new osg::DrawElementsUInt( GL_TRIANGLES, _tri.begin(), _tri.end() );
    multi->addPrimitiveSet( elem.get() );
    return multi.release();
}

}

}
