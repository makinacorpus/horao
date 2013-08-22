#ifndef STACK3D_VIEWER_VECTORLAYERPOSTGIS_H
#define STACK3D_VIEWER_VECTORLAYERPOSTGIS_H

#include "PostGisUtils.h"
#include <memory>
#include <osg/NodeCallback>

#include <osg/ShapeDrawable>

namespace Stack3d {
namespace Viewer {

//! @note no '"' are alowed in error messages
struct VectorLayerPostgis: osg::Group
{

    static VectorLayerPostgis * create(const std::string & host,
                                       const std::string & port,
                                       const std::string & dbname,
                                       const std::string & user,
                                       const std::string & password);
    ~VectorLayerPostgis();
private:
    VectorLayerPostgis( PGconn * conn );

    PGconn * _conn;

    struct VectorLayerPostgisNodeCallback : public osg::NodeCallback
    {
    public:
       virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
       {
          static bool once = false;
          if (!once){
              osg::Box* unitCube = new osg::Box( osg::Vec3(0,0,0), 1.0f);
              osg::ShapeDrawable* unitCubeDrawable = new osg::ShapeDrawable(unitCube);
              osg::Geode* basicShapesGeode = new osg::Geode();
              basicShapesGeode->addDrawable(unitCubeDrawable);
              node->asGroup()->addChild(basicShapesGeode);
              once=true;
          }
          // here we can launch worker thread and add children.
          traverse(node, nv);
       }
    };
};

inline
VectorLayerPostgis *
VectorLayerPostgis::create( const std::string & host,
                           const std::string & port,
                           const std::string & dbname,
                           const std::string & user,
                           const std::string & password)
{
    assert(!dbname.empty());
    const std::string conninfo = (host.empty() ? "" : " host='"+host+"'")
                               + (port.empty() ? "" : " port='"+port+"'")
                               + (" dbname='"+dbname+"'")
                               + (user.empty() ? "" : " user='"+user+"'")
                               + (password.empty() ? "" : " password='"+password+"'")
                               ;
    std::auto_ptr<VectorLayerPostgis> vlp( new VectorLayerPostgis( PQconnectdb(conninfo.c_str()) ) );

    if ( CONNECTION_OK != PQstatus( vlp->_conn) ){
        ERROR << "cannot connect to postgres";
        return NULL;
    }
    
    return vlp.release();

}
 
inline
VectorLayerPostgis::VectorLayerPostgis( PGconn * conn )
    :_conn( conn )
{
   setUpdateCallback(new VectorLayerPostgisNodeCallback);
}

inline
VectorLayerPostgis::~VectorLayerPostgis()
{
    if ( _conn ) {
        PQfinish( _conn );
        _conn = 0L;
    }
}

}
}

#endif
