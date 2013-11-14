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
#ifndef STACK3D_VIEWER_VIEWERWIDGET_H
#define STACK3D_VIEWER_VIEWERWIDGET_H

#include <osgViewer/Viewer>
#include <osgDB/WriteFile>
#include <osgViewer/ViewerEventHandlers>

#include <queue>

namespace Stack3d {
namespace Viewer {

struct ViewerWidget: osgViewer::Viewer {
    ViewerWidget();
    void addNode( const std::string& nodeId, osg::Node* ) volatile;
    void removeNode( const std::string& nodeId ) volatile;
    void setVisible( const std::string& nodeId, bool visible ) volatile;
    void setDone( bool flag ) volatile;
    void setStateSet( const std::string& nodeId, osg::StateSet* ) volatile;
    void setLookAt( const osg::Vec3& eye, const osg::Vec3& center, const osg::Vec3& up ) volatile;
    void lookAtExtent( double xmin, double ymin, double xmax, double ymax ) volatile;
    void writeFile( const std::string& filename ) volatile;

private:

    osgGA::CameraManipulator* getCurrentManipulator();
    OpenThreads::Mutex _mutex;
    osg::ref_ptr<osg::Group> _root;
    typedef std::map< std::string, osg::ref_ptr<osg::Node> > NodeMap;
    NodeMap _nodeMap;
    void frame( double time );
};

}
}
#endif
