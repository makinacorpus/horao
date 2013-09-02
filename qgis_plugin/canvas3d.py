# -*- coding: utf-8 -*-
"""
/***************************************************************************
 Canvas3D
                                 A QGIS plugin
 3D canvas
                              -------------------
        begin                : 2013-08-12
        copyright            : (C) 2013 by Oslandia
        email                : infos@oslandia.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""
# Import the PyQt and QGIS libraries
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *

from viewer_pipe import ViewerPipe

# Initialize Qt resources from file resources.py
import resources_rc
import os.path
import sys

SIMPLEVIEWER_BIN = "/home/hme/src/3dstack/build/bin/simpleViewer"
#SIMPLEVIEWER_BIN = "/home/hme/src/3dstack/qgis_plugin/fake_viewer.py"

class LayerInfo:
    def __init__( self ):
        self.id = None
        self.visible = False

    def __init__( self, id, visibility ):
        self.id = id
        self.visible = visibility

class Canvas3D:

    def __init__(self, iface):
        # Save reference to the QGIS interface
        self.iface = iface
        # initialize plugin directory
        self.plugin_dir = os.path.dirname(__file__)

        # map of layer object => LayerInfo
        self.layers = {}

        self.vpipe = ViewerPipe()

        self.signalsConnected = False

    def initGui(self):
        # Create action that will start plugin configuration
        self.action = QAction(
            QIcon(":/plugins/canvas3d/icon.png"),
            u"3D canvas", self.iface.mainWindow())
        # connect the action to the run method
        self.action.triggered.connect(self.run)

        # Add toolbar button and menu item
        self.iface.addToolBarIcon(self.action)
        self.iface.addPluginToMenu(u"&Canvas3D", self.action)

    def unload(self):
        # Remove the plugin menu item and icon
        self.iface.removePluginMenu(u"&Canvas3D", self.action)
        self.iface.removeToolBarIcon(self.action)

    def sendToViewer( self, cmd, args ):
        if self.vpipe:
            r = self.vpipe.evaluate( cmd, args )
            if r[0] == 'broken_pipe':
                # the viewer is not here anymore, ignoring
                return
            if r[0] != 'ok':
                QMessageBox.warning( None, "Communication error", r[1]['msg'] )

    def addLayer( self, layer ):
        print "layer %s added: %s" % (layer.id(), layer.source() )
        providerName = layer.dataProvider().name()

        style = {}
        ret = None
        # get layer' style
        if layer.type() == 0: # vector
            renderer = layer.rendererV2()
            print renderer.type(), renderer.symbols()
            # first symbol
            sym = renderer.symbols()[0]
            # first symbol layer
            symL = sym.symbolLayer( 0 )

            if symL.type() == 0: # point
                style['point_fill'] = symL.color().name()
            elif symL.type() == 1: # line
                style['stroke_color'] = symL.color().name()
            elif symL.type() == 2: # polygon
                style['fill_color'] = symL.color().name()
                style['stroke_color'] = symL.borderColor().name()
#                style['stroke-width'] = str(symL.borderWidth() / self.iface.mapCanvas().mapUnitsPerPixel() * 1000) + "px"
#                style['stroke-linecap'] = 'square'

            if providerName == 'postgres':
                # parse connection string
#                sys.stderr.write(layer.source())
                connection = {}
                geocolumn = 'geom'
                args = {}
                query = ''
                table = ''

                # connection info followed by table and queries
                # connection info : k='v' (optional ' for integers)
                # queries : table="...." (geocolumn_name) sql=... until end of line
                (connection_str, queries_str) = layer.source().split("table=")

                s = connection_str.split(' ')
                for si in s:
                    ss = si.split('=')
                    if len(ss) > 1:
                        connection[ ss[0] ] = ss[1].strip("'\"")

                (table, q) = queries_str.split('" ')
                # if the table is a query (from DB manager):
                if table[0:2] == '"(':
                    table.strip('"')
                else:
                    table=table+'"'
                (geocolumn,query) = q.split('sql=')
                geocolumn=geocolumn.strip('() ')

#                sys.stderr.write( "table:%s\ngeocolumn:%s\nquery:%s\n" % (table, geocolumn, query) )

                args['id'] = layer.id()
                args['conn_info'] = ' '.join( ["%s='%s'" % (k,v) for k,v in connection.iteritems() if k in ['dbname','user','port']] )
                renderer = self.iface.mapCanvas().mapRenderer()
                extent = renderer.fullExtent()
                center = extent.center()
                args['extend'] = "%f %f,%f %f" % ( extent.xMinimum(), extent.yMinimum(), extent.xMaximum(), extent.yMaximum() )
                args['center'] = "%f %f" % (center.x(), center.y())
                
                if table[0:2] == '"(':
                    # table == query (DB manager)
                    query = table.strip('"()')
                else:
                    query = "SELECT * FROM %s /**WHERE TILE && geom*/" % table


                if layer.hasScaleBasedVisibility():
                    # TODO : conversion from 1:N scale to distance to ground
                    args['lod'] = "%f %f" % (layer.minimumScale(), layer.maximumScale() )
                    args['query_0'] = query
                    args['tile_size'] = 2000 # TODO: how to set it ?
                else:
                    args['query'] = query
                    
                self.sendToViewer( 'loadVectorPostgis', args )
                self.layers[ layer ] = LayerInfo( layer.id(), False )
                    
                # send symbology
                    
                style['id'] = layer.id()
                #self.sendToViewer( 'setSymbology', style )

        #
        # raster layers
        #
        elif layer.type() == 1:
            provider = layer.dataProvider()
            if provider.name() == 'gdal':
                # warning: band # start at 1
                if provider.bandCount() == 1 and provider.dataType( 1 ) != QGis.Byte:
                    self.sendToViewer( 'loadElevationGDAL', { 'id': layer.id(), 'url':layer.source()} )
                else:
                    self.sendToViewer( 'loadImageGDAL', { 'id': layer.id(), 'url':layer.source()} )

                self.layers[ layer ] = LayerInfo( layer.id(), False )


    def removeLayer( self, layer ):
        if self.layers.has_key( layer ):
            layerId = self.layers[ layer ].id
            self.sendToViewer( 'unloadLayer', { 'id': layerId } )

            del self.layers[ layer ]

    def setExtent( self, epsg, xmin, ymin, xmax, ymax ):
        self.sendToViewer( 'setFullExtent', {'x_min': xmin, 'y_min': ymin, 'x_max' : xmax, 'y_max': ymax } )

    def setLayerVisibility( self, layer, visibility ):
        if not self.layers.has_key( layer ):
            return
        layerId = self.layers[ layer ].id
        if visibility:
            self.sendToViewer( 'showLayer', {'id' : layerId } )
        else:
            self.sendToViewer( 'hideLayer', {'id' : layerId } )
        self.layers[ layer ].visible = visibility

    # qgis signal : layer added
    def onLayerAdded( self, layer ):
        self.addLayer( layer )
        self.setLayerVisibility( layer, True )

    # qgis signal : layer removed
    def onLayerRemoved( self, layerId ):
        registry = QgsMapLayerRegistry.instance()
        layer = registry.mapLayer( layerId )
        if layer:
            self.removeLayer( layer )

    # qgis signal : layers changed
    def onLayersChanged( self ):
        renderer = self.iface.mapCanvas().mapRenderer()
        # returns visible layers
        layers = renderer.layerSet()
        for layer, p in self.layers.iteritems():
            if layer.id() in layers and not p.visible:
                self.setLayerVisibility( layer, True )
            if layer.id() not in layers and p.visible:
                self.setLayerVisibility( layer, False )

    # run method that performs all the real work
    def run(self):

        registry = QgsMapLayerRegistry.instance()
        if not self.signalsConnected:
            # when a layer is added
            QObject.connect( registry, SIGNAL( "layerWasAdded( QgsMapLayer * )" ), self.onLayerAdded )
            # when a layer is removed
            QObject.connect( registry, SIGNAL( "layerWillBeRemoved( QString )" ), self.onLayerRemoved )
            # when a layer' state changes (in particular: its visibility)
            QObject.connect( self.iface.mapCanvas(), SIGNAL( "layersChanged()" ), self.onLayersChanged )
            self.signalsConnected = True

        # set extent
        renderer = self.iface.mapCanvas().mapRenderer()
        extent = renderer.fullExtent()
        epsg = renderer.destinationCrs().authid()

        if extent.isEmpty():
            QMessageBox.information( None, "Canvas3D", "No layer loaded, no extent defined, aborting" )
            return

        self.vpipe.start( SIMPLEVIEWER_BIN )

#        self.setExtent( epsg, extent.xMinimum(), extent.yMinimum(), extent.xMaximum(), extent.yMaximum() )

        # load every visible layer
        layers = registry.mapLayers()

        # returns visible layers
        visibleLayers = self.iface.mapCanvas().mapRenderer().layerSet()

        for lid, l in layers.iteritems():
            self.addLayer( l )
            if l.id() in visibleLayers:
                self.setLayerVisibility( l, True )
            else:
                self.setLayerVisibility( l, False )



