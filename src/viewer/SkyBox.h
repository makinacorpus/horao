/**
 *   Horao
 *
 *   Copyright (C) 2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.

 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STACK3D_VIEWER_SKYBOX_H
#define STACK3D_VIEWER_SKYBOX_H

#include <osg/Depth>
#include <osg/TexGen>
#include <osg/TextureCubeMap>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <cassert>


class SkyBox : public osg::Transform {
public:
    SkyBox();
    SkyBox( const SkyBox& copy, osg::CopyOp copyop= osg::CopyOp::SHALLOW_COPY )
        : osg::Transform( copy, copyop )
    {}

    META_Node( osg, SkyBox );

    void setEnvironmentMap( unsigned int unit, osg::Image* posX,
                            osg::Image* negX, osg::Image* posY, osg::Image* negY,
                            osg::Image* posZ, osg::Image* negZ );
    virtual bool computeLocalToWorldMatrix( osg::Matrix& matrix,
                                            osg::NodeVisitor* nv ) const;
    virtual bool computeWorldToLocalMatrix( osg::Matrix& matrix,
                                            osg::NodeVisitor* nv ) const;
protected:
    virtual ~SkyBox() {}
};

inline
SkyBox::SkyBox()
{
    setReferenceFrame( osg::Transform::ABSOLUTE_RF );
    setCullingActive( false );
    osg::StateSet* ss = getOrCreateStateSet();
    ss->setAttributeAndModes( new osg::Depth(
                                  osg::Depth::LEQUAL, 1.0f, 1.0f ) );
    ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    ss->setMode( GL_CULL_FACE, osg::StateAttribute::OFF );
    ss->setRenderBinDetails( 5, "RenderBin" );

}

inline
void SkyBox::setEnvironmentMap( unsigned int unit,
                                osg::Image* posX, osg::Image* negX,
                                osg::Image* posY, osg::Image* negY,
                                osg::Image* posZ, osg::Image* negZ )
{
    assert( posX && negX && posY && negY && posZ && negZ );

    osg::ref_ptr<osg::TextureCubeMap> cubemap =
        new osg::TextureCubeMap;
    cubemap->setImage( osg::TextureCubeMap::POSITIVE_X, posX );
    cubemap->setImage( osg::TextureCubeMap::NEGATIVE_X, negX );

    cubemap->setImage( osg::TextureCubeMap::POSITIVE_Y,
                       posY
                     );
    cubemap->setImage( osg::TextureCubeMap::NEGATIVE_Y,
                       negY
                     );
    cubemap->setImage( osg::TextureCubeMap::POSITIVE_Z,
                       posZ
                     );
    cubemap->setImage( osg::TextureCubeMap::NEGATIVE_Z,
                       negZ
                     );
    // Please find details in the source code
    cubemap->setResizeNonPowerOfTwoHint( false );
    getOrCreateStateSet()->setTextureAttributeAndModes(
        unit, cubemap.get() );
}

inline
bool SkyBox::computeLocalToWorldMatrix( osg::Matrix& matrix,
                                        osg::NodeVisitor* nv ) const
{
    if ( nv && nv->getVisitorType()==
            osg::NodeVisitor::CULL_VISITOR ) {
        osgUtil::CullVisitor* cv =
            static_cast<osgUtil::CullVisitor*>( nv );
        matrix.preMult( osg::Matrix::translate( cv->getEyeLocal() ) );
        return true;
    }
    else {
        return osg::Transform::computeLocalToWorldMatrix( matrix, nv );
    }
}

inline
bool SkyBox::computeWorldToLocalMatrix( osg::Matrix& matrix,
                                        osg::NodeVisitor* nv ) const
{
    if ( nv && nv->getVisitorType()==
            osg::NodeVisitor::CULL_VISITOR ) {
        osgUtil::CullVisitor* cv =
            static_cast<osgUtil::CullVisitor*>( nv );
        matrix.postMult( osg::Matrix::translate(
                             -cv->getEyeLocal() ) );
        return true;
    }
    else {
        return osg::Transform::computeWorldToLocalMatrix( matrix, nv );
    }
}




#endif
