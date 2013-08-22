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

#include "PostGisUtils"

inline
void populate( const POINTARRAY * array, Symbology::Geometry* target )
{
    const int numPoints = array->npoints;
    for( int v = 0; v < numPoints; v++ )
    {
        const POINT3DZ p3D = getPoint3dz(array, v );
        const osg::Vec3d p( p3D.x, p3D.y, p3D.z );
        if ( target->size() == 0 || p != target->back() ) // remove dupes
            target->push_back( p );
    }
}

inline
Symbology::Polygon * createGeometry( const LWPOLY * lwpoly )
{
    assert( lwpoly );
    const int numRings = lwpoly->nrings;
    osg::ref_ptr<Symbology::Polygon> poly;
    if ( numRings > 0)
    {
        poly = new Symbology::Polygon( lwpoly->rings[0]->npoints );
        populate( lwpoly->rings[0], poly.get() );
    }
    for ( int r = 1; r < numRings; r++)
    {
        osg::ref_ptr<Symbology::Ring> hole = new Symbology::Ring( lwpoly->rings[r]->npoints );
        populate( lwpoly->rings[r], hole.get() );
        poly->getHoles().push_back( hole.get() );
    }
    return poly.release();
}

inline
Symbology::Polygon * createGeometry( const LWTRIANGLE * lwtriangle )
{
    assert( lwtriangle );
    osg::ref_ptr<Symbology::Polygon> poly = new Symbology::Polygon( lwtriangle->points->npoints );
    populate( lwtriangle->points, poly.get() );
    return poly.release();
}

inline
Symbology::LineString * createGeometry( const LWLINE * lwline )
{
    assert( lwline );
    osg::ref_ptr<Symbology::LineString> line = new Symbology::LineString( lwline->points->npoints );
    populate( lwline->points, line.get() );
    return line.release();
}

inline
Symbology::PointSet * createGeometry( const LWPOINT * lwpoint )
{
    assert( lwpoint );
    osg::ref_ptr<Symbology::PointSet> point = new Symbology::PointSet( lwpoint->point->npoints );
    populate( lwpoint->point, point.get() );
    return point.release();
}

template < typename MULTITYPE >
Symbology::MultiGeometry * createGeometry( const MULTITYPE * lwmulti )
{
    assert( lwmulti );
    osg::ref_ptr<Symbology::MultiGeometry> multi = new Symbology::MultiGeometry();
    const int numGeom = lwmulti->ngeoms;
    for ( int g = 0; g<numGeom; g++ )
    {
        osg::ref_ptr<Symbology::Geometry> geom = createGeometry( lwmulti->geoms[g] );
        multi->getComponents().push_back( geom.release() );
    }
    return multi.release();
}

// specialization for multipoint to create a pointset
Symbology::PointSet * createGeometry( const LWMPOINT * lwmulti )
{
    assert( lwmulti );
    const int numPoints = lwmulti->ngeoms;
    osg::ref_ptr<Symbology::PointSet> multi = new Symbology::PointSet( numPoints );
    for( int v = 0; v < numPoints; v++ )
    {
        const POINT3DZ p3D = getPoint3dz( lwmulti->geoms[v]->point, 0 );
        const osg::Vec3d p( p3D.x, p3D.y, p3D.z );
        if ( multi->size() == 0 || p != multi->back() ) // remove dupes
            multi->push_back( p );
    }
    return multi.release();
}

Symbology::MultiGeometry * createGeometry( const LWCOLLECTION * lwmulti )
{
    assert( lwmulti );
    osg::ref_ptr<Symbology::MultiGeometry> multi = new Symbology::MultiGeometry();
    const int numGeom = lwmulti->ngeoms;
    for ( int g = 0; g<numGeom; g++ )
    {
        osg::ref_ptr<Symbology::Geometry> geom = PostGisUtils::createGeometry( lwmulti->geoms[g] );
        multi->getComponents().push_back( geom.release() );
    }
    return multi.release();
}

Symbology::Geometry* PostGisUtils::createGeometry( const LWGEOM * lwgeom )
{
    osg::ref_ptr<Symbology::Geometry> geom;
    //! @todo actually create the geometry
    switch ( lwgeom->type )
    {
    case POLYGONTYPE:
        geom = ::createGeometry( lwgeom_as_lwpoly( lwgeom ) );
        break;
    case MULTIPOLYGONTYPE:
        geom = ::createGeometry( lwgeom_as_lwmpoly( lwgeom ) );
        break;
    case TRIANGLETYPE:
        geom = ::createGeometry( lwgeom_as_lwtriangle( lwgeom ) );
        break;
    case TINTYPE:
        geom = ::createGeometry( lwgeom_as_lwtin( lwgeom ) );
        break;
    case POLYHEDRALSURFACETYPE:
        geom = ::createGeometry( lwgeom_as_lwpsurface( lwgeom ) );
        break;
    case POINTTYPE:
        geom = ::createGeometry( lwgeom_as_lwpoint( lwgeom ) );
        break;
    case MULTIPOINTTYPE:
        geom = ::createGeometry( lwgeom_as_lwmpoint( lwgeom ) );
        break;
    case LINETYPE:
        geom = ::createGeometry( lwgeom_as_lwline( lwgeom ) );
        break;
    case MULTILINETYPE:
        geom = ::createGeometry( lwgeom_as_lwmline( lwgeom ) );
        break;
    case COLLECTIONTYPE:
        geom = ::createGeometry( lwgeom_as_lwcollection( lwgeom ) );
        break;
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

