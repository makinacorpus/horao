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

#include <osgUtil/Tessellator>


namespace Stack3d {
namespace Viewer {

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
    const osg::Vec3 bc = center*_layerToWord - osg::Vec3(0,0,.5f*height); // center of the base
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
void CALLBACK noStripCallback(GLboolean flag)
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
    ERROR << errorStr;
}

void CALLBACK tessVertexCB(const GLvoid *vtx, void *data)
{
    // cast back to double type
    const GLdouble * p = (const GLdouble*)vtx;
    TriangleMesh * that = (TriangleMesh * )data;
    that->_tri.push_back( that->_vtx.size() );
    that->_vtx.push_back( osg::Vec3( p[0], p[1], p[2] ) );
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
	    gluTessCallback(_tess, GLU_TESS_EDGE_FLAG,  (void (*)())noStripCallback);
    }
    ~Tessellator(){
        gluDeleteTess(_tess);
    }

    GLUtesselator * _tess;
};

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
            const POINT3DZ p3D = getPoint3dz( lwpoly->rings[r], v ? v : ringSize - 1 - v );
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

template< typename MULTITYPE >
void TriangleMesh::push_back( const MULTITYPE * lwmulti )
{
    assert( lwmulti );
    const int numGeom = lwmulti->ngeoms;
    for ( int g = 0; g<numGeom; g++ ) push_back( lwmulti->geoms[g] );

}

void TriangleMesh::push_back( const LWGEOM * lwgeom )
{
    osg::ref_ptr<osg::Geometry> geom;
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
