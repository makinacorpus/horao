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

std::ostream & operator<<( std::ostream & o, const osg::Vec3 & v )
{
    o << "( " << v.x() << ", " << v.y() << ", " << v.z() << " )";
    return o;
}

// nop callback
void CALLBACK noStripCallback(GLboolean flag)
{
    //assert( flag );
    (void)flag;
}

//
// Custom tessellator that is used to have GL_TRIANGLES instead of GL_TRIANGLE_FAN or GL_TRIANGLE_STRIP
// GLU says if there is a GLU_TESS_EDGE_FLAG, it will only use GL_TRIANGLES
struct CustomTessellator : osgUtil::Tessellator
{
    virtual void beginTessellation()
    {
	reset();
	
	if(!_tobj) _tobj = osg::gluNewTess();
	
	osg::gluTessCallback(_tobj, GLU_TESS_VERTEX_DATA, (osg::GLU_TESS_CALLBACK) osgUtil::Tessellator::vertexCallback);
	osg::gluTessCallback(_tobj, GLU_TESS_BEGIN_DATA,  (osg::GLU_TESS_CALLBACK) osgUtil::Tessellator::beginCallback);
	osg::gluTessCallback(_tobj, GLU_TESS_END_DATA,    (osg::GLU_TESS_CALLBACK) osgUtil::Tessellator::endCallback);
	osg::gluTessCallback(_tobj, GLU_TESS_COMBINE_DATA,(osg::GLU_TESS_CALLBACK) osgUtil::Tessellator::combineCallback);
osg::gluTessCallback(_tobj, GLU_TESS_ERROR_DATA,  (osg::GLU_TESS_CALLBACK) osgUtil::Tessellator::errorCallback);
	//Here is the New TESS Callback :
	//Force to only creates triangles
	osg::gluTessCallback(_tobj, GLU_TESS_EDGE_FLAG,  (osg::GLU_TESS_CALLBACK) noStripCallback);
	if(tessNormal.length()>0.0) osg::gluTessNormal(_tobj, tessNormal.x(), tessNormal.y(), tessNormal.z());
	osg::gluTessBeginPolygon(_tobj,this);  
    }
};

static CustomTessellator tess;

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
    for (int i=0; i<3; i++) _nrml.push_back( normal );
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
    //// Not completely correct, but better than no normals at all. TODO: update this
    //// to generate a proper normal vector in ECEF mode.
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



inline
void transformLocalizeAppend( const POINTARRAY * array, osg::Vec3Array * target, const osg::Matrixd & layerToWord, bool reverse = false )
{
    //! @todo add actual transformation of points here
    const int numPoints = array->npoints;
    
    const int offset = target->size();
    target->resize( offset + numPoints - 1);
    for( int v = 0; v < numPoints - 1; v++ )
    {
        const POINT3DZ p3D = getPoint3dz(array, reverse ? numPoints - 1 - v : v );
        const osg::Vec3d p( p3D.x, p3D.y, p3D.z );
        if ( target->size() == 0 || p != target->back() ) // remove dupes
            (*target)[offset + v] =  p * layerToWord;
    }
}

inline
osg::Geometry * createGeometry( const LWPOLY * lwpoly, const osg::Matrixd & layerToWord )
{
    assert( lwpoly );
    const int numRings = lwpoly->nrings;
    if ( numRings == 0 ) return NULL;
    
    osg::ref_ptr<osg::Vec3Array> allPoints = new osg::Vec3Array();
    transformLocalizeAppend( lwpoly->rings[0], allPoints.get(), layerToWord );

    osg::ref_ptr<osg::Geometry> osgGeom = new osg::Geometry();
    osgGeom->setUseVertexBufferObjects(true);

    for ( int r = 0; r < numRings; r++)
    {
        osg::ref_ptr<osg::DrawElementsUInt> elem =  new osg::DrawElementsUInt( GL_POLYGON, lwpoly->rings[r]->npoints );
        osgGeom->addPrimitiveSet( elem.get() );
        for (int i=0; i<lwpoly->rings[r]->npoints; i++) elem->setElement(i,i);
        transformLocalizeAppend( lwpoly->rings[r], allPoints.get(), layerToWord, r > 0 );
    }
    
    osgGeom->setVertexArray( allPoints.get() );

    tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_POLYGONS );
    tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
    tess.retessellatePolygons( *osgGeom );

    //// Normal computation.
    //// Not completely correct, but better than no normals at all. TODO: update this
    //// to generate a proper normal vector in ECEF mode.
    ////
    //// We cannot accurately rely on triangles from the tessellation, since we could have
    //// very "degraded" triangles (close to a line), and the normal computation would be bad.
    //// In this case, we would have to average the normal vector over each triangle of the polygon.
    //// The Newell's formula is simpler and more direct here.
    osg::Vec3 normal( 0.0, 0.0, 0.0 );
    const int sz = lwpoly->rings[0]->npoints;
    for ( int i = 0; i < sz; ++i )
    {
       osg::Vec3 pi = (*allPoints)[i];
       osg::Vec3 pj = (*allPoints)[ (i+1) % sz ];
       normal[0] += ( pi[1] - pj[1] ) * ( pi[2] + pj[2] );
       normal[1] += ( pi[2] - pj[2] ) * ( pi[0] + pj[0] );
       normal[2] += ( pi[0] - pj[0] ) * ( pi[1] + pj[1] );
    }
    normal.normalize();

    osg::ref_ptr< osg::Vec3Array > nrml = new osg::Vec3Array(allPoints->size());
    osgGeom->setNormalArray( nrml.get() );
    for ( size_t i=0; i<nrml->size(); i++) (*nrml)[i] = normal;

    return osgGeom.release();
}

inline
osg::Geometry * createGeometry( const LWTRIANGLE * lwtriangle, const osg::Matrixd & layerToWord )
{
    assert( lwtriangle );
    osg::ref_ptr<osg::Geometry> osgGeom = new osg::Geometry();
    osgGeom->setUseVertexBufferObjects(true);

    osg::ref_ptr<osg::Vec3Array> allPoints = new osg::Vec3Array();
    transformLocalizeAppend( lwtriangle->points, allPoints.get(), layerToWord );
    osgGeom->setVertexArray( allPoints.get() );

    osg::ref_ptr<osg::DrawElementsUInt> elem = new osg::DrawElementsUInt( GL_TRIANGLES, 3 );
    osgGeom->addPrimitiveSet( elem.get() );
    for (int i=0; i<3; i++) elem->setElement(i,i);

    osg::Vec3 normal( 0.0, 0.0, 0.0 );
    const int sz = 3;
    for ( int i = 0; i < sz; ++i )
    {
       osg::Vec3 pi = (*allPoints)[i];
       osg::Vec3 pj = (*allPoints)[ (i+1) % sz ];
       normal[0] += ( pi[1] - pj[1] ) * ( pi[2] + pj[2] );
       normal[1] += ( pi[2] - pj[2] ) * ( pi[0] + pj[0] );
       normal[2] += ( pi[0] - pj[0] ) * ( pi[1] + pj[1] );
    }
    normal.normalize();

    osg::ref_ptr< osg::Vec3Array > nrml = new osg::Vec3Array(allPoints->size());
    osgGeom->setNormalArray( nrml.get() );
    for ( size_t i=0; i<nrml->size(); i++) (*nrml)[i] = normal;

    return osgGeom.release();
}

//inline
//osg::Geometry * createGeometry( const LWLINE * lwline )
//{
//    assert( lwline );
//    osg::ref_ptr<osg::Vec3Array> allPoints = new osg::Vec3Array();
//    transformAndLocalize( lwtriangle->points, allPoints );
//
//    osg::ref_ptr<Symbology::LineString> line = new Symbology::LineString( lwline->points->npoints );
//    populate( lwline->points, line.get() );
//    return line.release();
//}
//
//inline
//Symbology::PointSet * createGeometry( const LWPOINT * lwpoint )
//{
//    assert( lwpoint );
//    osg::ref_ptr<Symbology::PointSet> point = new Symbology::PointSet( lwpoint->point->npoints );
//    populate( lwpoint->point, point.get() );
//    return point.release();
//}

osg::PrimitiveSet * offsetIndices( osg::PrimitiveSet * primitiveSet, size_t offset)
{
    // we convert the DrawElements to UInt to avoid indices overflows
    osg::ref_ptr<osg::PrimitiveSet> elem;
    if ( const osg::DrawElementsUByte * elemUByte 
            = dynamic_cast<osg::DrawElementsUByte*>( primitiveSet ) ){
        elem = new osg::DrawElementsUInt( elemUByte->getMode(), elemUByte->begin(), elemUByte->end());
    }
    else if ( const osg::DrawElementsUShort * elemUShort 
            = dynamic_cast<osg::DrawElementsUShort*>( primitiveSet ) ){
        elem = new osg::DrawElementsUInt( elemUShort->getMode(), elemUShort->begin(), elemUShort->end());
    }
    else{
        elem = primitiveSet;
    }

    elem->offsetIndices( offset );
    return elem.release();
}

template < typename MULTITYPE >
osg::Geometry * createGeometry( const MULTITYPE * lwmulti, const osg::Matrixd & layerToWord )
{
    assert( lwmulti );
    const int numGeom = lwmulti->ngeoms;

    osg::ref_ptr<osg::Geometry> multi = new osg::Geometry();
    multi->setUseVertexBufferObjects(true);

    osg::ref_ptr<osg::Vec3Array> vertices( new osg::Vec3Array );
	multi->setVertexArray( vertices.get() );
    osg::ref_ptr<osg::Vec3Array> normals( new osg::Vec3Array );
	multi->setNormalArray( normals.get() );
	multi->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
    
    for ( int g = 0; g<numGeom; g++ )
    {
        const int offset = vertices->size();
        osg::ref_ptr<osg::Geometry> geom = createGeometry( lwmulti->geoms[g], layerToWord );
        // merge 
        const osg::Vec3Array * vtx = dynamic_cast<const osg::Vec3Array *>(geom->getVertexArray());
        const osg::Vec3Array * nrml = dynamic_cast<const osg::Vec3Array *>(geom->getNormalArray());
        assert(vtx && nrml);
        for ( size_t v=0; v < vtx->size(); v++ ) {
           vertices->push_back( (*vtx)[v] );
           normals->push_back( (*nrml)[v] );
        }

        // modify vtx indice of primitives
        for( size_t s=0; s<geom->getNumPrimitiveSets(); s++ ){
            multi->addPrimitiveSet( offsetIndices( geom->getPrimitiveSet(s), offset) );
        }
    }
    return multi.release();
}


// specialization for multipoint to create a pointset
//Symbology::PointSet * createGeometry( const LWMPOINT * lwmulti )
//{
//    assert( lwmulti );
//    const int numPoints = lwmulti->ngeoms;
//    osg::ref_ptr<Symbology::PointSet> multi = new Symbology::PointSet( numPoints );
//    for( int v = 0; v < numPoints; v++ )
//    {
//        const POINT3DZ p3D = getPoint3dz( lwmulti->geoms[v]->point, 0 );
//        const osg::Vec3d p( p3D.x, p3D.y, p3D.z );
//        if ( multi->size() == 0 || p != multi->back() ) // remove dupes
//            multi->push_back( p );
//    }
//    return multi.release();
//}


osg::Geometry* createGeometry( const LWGEOM * lwgeom, const osg::Matrixd & layerToWord )
{
    osg::ref_ptr<osg::Geometry> geom;
    //! @todo actually create the geometry
    switch ( lwgeom->type )
    {
    case POLYGONTYPE:
        geom = createGeometry( lwgeom_as_lwpoly( lwgeom ), layerToWord );
        break;
    case MULTIPOLYGONTYPE:
        geom = createGeometry( lwgeom_as_lwmpoly( lwgeom ), layerToWord );
        break;
    case TRIANGLETYPE:
        geom = createGeometry( lwgeom_as_lwtriangle( lwgeom ), layerToWord );
        break;
    case TINTYPE:
        geom = createGeometry( lwgeom_as_lwtin( lwgeom ), layerToWord );
        break;
    case POLYHEDRALSURFACETYPE:
        geom = createGeometry( lwgeom_as_lwpsurface( lwgeom ), layerToWord );
        break;
    case COLLECTIONTYPE:
        geom = createGeometry( lwgeom_as_lwcollection( lwgeom ), layerToWord );
        break;
    case POINTTYPE:
        assert(false && "POINTTYPE not implemented");
    case MULTIPOINTTYPE:
        assert(false && "MULTIPOINTTYPE not implemented");
    case LINETYPE:
        assert(false && "LINETYPE not implemented");
    case MULTILINETYPE:
        assert(false && "MULTIPOINTTYPE not implemented");
    case MULTISURFACETYPE:
        assert(false && "MULTISURFACETYPE not implemented");
    case MULTICURVETYPE:
        assert(false && "MULTICURVETYPE not implemented");
    case CIRCSTRINGTYPE:
        assert(false && "CIRCSTRINGTYPE not implemented");
    case COMPOUNDTYPE:
        assert(false && "COMPOUNDTYPE not implemented");
    case CURVEPOLYTYPE:
        assert(false && "CURVEPOLYTYPE not implemented");
    }
    return geom.release();
}


}

}
