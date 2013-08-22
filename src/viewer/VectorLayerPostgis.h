#ifndef STACK3D_VIEWER_VECTORLAYERPOSTGIS_H
#define STACK3D_VIEWER_VECTORLAYERPOSTGIS_H

#include "PostGisUtils.h"

#include <osg/NodeCallback>
#include <osg/ShapeDrawable>

#include <memory>
#include <sstream>

namespace Stack3d {
namespace Viewer {


//! @note no '"' are alowed in error messages
struct VectorLayerPostgis: osg::Group
{

    static VectorLayerPostgis * create(const std::string & host,
                                       const std::string & port,
                                       const std::string & dbname,
                                       const std::string & user,
                                       const std::string & password,
                                       const std::string & query);
    ~VectorLayerPostgis();

    const std::string & query() const { return _query; }
private:
    VectorLayerPostgis( PGconn * conn, const std::string & query );

    PGconn * _conn;
    const std::string _query;

    struct VectorLayerPostgisNodeCallback : public osg::NodeCallback
    {
    public:
       virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
       {
          VectorLayerPostgis * vlp = dynamic_cast<VectorLayerPostgis *>( node );
          assert( vlp );
          static bool once = false;
          if (!once){
              osg::Box* unitCube = new osg::Box( osg::Vec3(0,0,0), 1.0f);
              osg::ShapeDrawable* unitCubeDrawable = new osg::ShapeDrawable(unitCube);
              osg::Geode* basicShapesGeode = new osg::Geode();
              basicShapesGeode->addDrawable(unitCubeDrawable);
              vlp->addChild(basicShapesGeode);
              once=true;
              std::cerr << tileQuery( vlp->query(), -1, -2, 1, 2 ) << "\n";
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
                           const std::string & password,
                           const std::string & query)
{
    assert(!dbname.empty());
    const std::string conninfo = (host.empty() ? "" : " host='"+host+"'")
                               + (port.empty() ? "" : " port='"+port+"'")
                               + (" dbname='"+dbname+"'")
                               + (user.empty() ? "" : " user='"+user+"'")
                               + (password.empty() ? "" : " password='"+password+"'")
                               ;
    std::auto_ptr<VectorLayerPostgis> vlp( new VectorLayerPostgis( PQconnectdb(conninfo.c_str()), query ) );

    if ( CONNECTION_OK != PQstatus( vlp->_conn) ){
        ERROR << "cannot connect to postgres";
        return NULL;
    }

    
    
    return vlp.release();

}
 
inline
VectorLayerPostgis::VectorLayerPostgis( PGconn * conn, const std::string & query )
    :_conn( conn )
    ,_query( query )
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
