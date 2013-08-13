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
# Initialize Qt resources from file resources.py
import resources_rc
import os.path

import subprocess

SIMPLEVIEWER_BIN = "/home/hme/src/3dstack/build/bin/simpleViewer"

class Canvas3D:

    def __init__(self, iface):
        self.process = None
        # Save reference to the QGIS interface
        self.iface = iface
        # initialize plugin directory
        self.plugin_dir = os.path.dirname(__file__)

        # map of layer object => visibility (bool)
        self.layers = {}

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

    # send command to the viewer pipe
    def sendCommand( self, cmd ):
        if self.process:
            self.process.stdin.write( cmd + "\n" )

    def addLayer( self, layer ):
        print "layer %s added: %s" % (layer.id(), layer.source() )
        providerName = layer.dataProvider().name()

        style = {}
        # get layer' style
        if layer.type() == 0: # vector
            renderer = layer.rendererV2()
            print renderer.type(), renderer.symbols()
            # first symbol
            sym = renderer.symbols()[0]
            # first symbol layer
            symL = sym.symbolLayer( 0 )

            if symL.type() == 0: # point
                style['point-fill'] = symL.color().name()
            elif symL.type() == 1: # line
                style['stroke'] = symL.color().name()
            elif symL.type() == 2: # polygon
                style['fill'] = symL.color().name()
                style['stroke'] = symL.borderColor().name()
#                style['stroke-width'] = str(symL.borderWidth() / self.iface.mapCanvas().mapUnitsPerPixel() * 1000) + "px"
#                style['stroke-linecap'] = 'square'

#            style['render-depth-test'] = 'false'
            style['altitude-clamping'] = 'terrain'

            css = ''
            for k,v in style.iteritems():
                css = css + "%s: %s;\n" % (k,v)
            xmlStyle = """<styles><style type="text/css">world { %s } </style></styles>""" % css

            features = ''
            if providerName == 'ogr':
                features = """<features driver="ogr">
                  <url>%s</url>
                </features>""" % layer.source()
            elif providerName == 'postgres':
                # parse connection string
                s = layer.source().split(' ')
                connection = {}
                for si in s:
                    ss = si.split('=')
                    if len(ss) > 1:
                        if ss[0] == 'table':
                            ss[1] = ss[1].split('.')[1]
                        connection[ ss[0] ] = ss[1].strip("'\"")
                connectionStr = "dbname=%s tables=%s" % (connection['dbname'], connection['table'])
                features = """<features driver="ogr">
                    <ogr_driver>PostgreSQL</ogr_driver>
                    <connection>PG:%s</connection>
                  </features>""" % connectionStr

            xml = """<model name="%s" driver="feature_geom">
                  %s
                  %s
                </model>""" % ( layer.id(), features, xmlStyle )
            print xml
            self.sendCommand( xml )

        #
        # raster layers
        #
        elif layer.type() == 1:
            provider = layer.dataProvider()
            if provider.name() == 'gdal':
                xml = """<elevation driver="gdal"><url>%s</url></elevation>""" % layer.source()

                self.sendCommand( xml )

                xml = """<image driver="gdal"><url>%s</url></image>""" % layer.source()
                self.sendCommand( xml )

        # add it to the layer map
        self.layers[ layer ] = True

    def removeLayer( self, layer ):
        print "layer %s removed" % layer.id()
        xml = """<unload layer="%s"/>""" % layer.id()
        # TODO

        del self.layers[ layer ]

    def setExtent( self, epsg, xmin, ymin, xmax, ymax ):
        xml = """
        <options>
            <profile srs = "%s"
               xmin = "%f"
               ymin = "%f"
               xmax = "%f"
               ymax = "%f"/>
        </options>""" % (epsg, xmin, ymin, xmax, ymax)
        print xml
        self.sendCommand( xml )

    def setLayerVisibility( self, layer, visibility ):
        # TODO
        if visibility:
            print "layer %s is now visible" % layer.id()
        else:
            print "layer %s is now hidden" % layer.id()
        self.layers[ layer ] = visibility

    # qgis signal : layer added
    def onLayerAdded( self, layer ):
        self.addLayer( layer )

    # qgis signal : layer removed
    def onLayerRemoved( self, layerId ):
        registry = QgsMapLayerRegistry.instance()
        layer = registry.mapLayer( layerId )
        if layer:
            self.removeLayer( layer )

    # qgis signal : layers changed
    def onLayersChanged( self ):
        print "layers changed"
        renderer = self.iface.mapCanvas().mapRenderer()
        # returns visible layers
        layers = renderer.layerSet()
        for layer, visible in self.layers.iteritems():
            if layer.id() in layers and not visible:
                self.setLayerVisibility( layer, True )
            if layer.id() not in layers and visible:
                self.setLayerVisibility( layer, False )

    # run method that performs all the real work
    def run(self):
        print "Canvas 3D start"

        registry = QgsMapLayerRegistry.instance()
        # when a layer is added
        QObject.connect( registry, SIGNAL( "layerWasAdded( QgsMapLayer * )" ), self.onLayerAdded )
        # when a layer is removed
        QObject.connect( registry, SIGNAL( "layerWillBeRemoved( QString )" ), self.onLayerRemoved )
        # when a layer' state changes (in particular: its visibility)
        QObject.connect( self.iface.mapCanvas(), SIGNAL( "layersChanged()" ), self.onLayersChanged )

        # set extent
        renderer = self.iface.mapCanvas().mapRenderer()
        extent = renderer.fullExtent()
        epsg = renderer.destinationCrs().authid()

        if extent.isEmpty():
            print "no layer loaded, no extent defined, aborting"
            return

        if self.process:
            self.process.terminate()
        
        self.process = subprocess.Popen(SIMPLEVIEWER_BIN, stdin = subprocess.PIPE)

        self.setExtent( epsg, extent.xMinimum(), extent.yMinimum(), extent.xMaximum(), extent.yMaximum() )

        # load every visible layer
        layers = registry.mapLayers()

        # returns visible layers
        visibleLayers = self.iface.mapCanvas().mapRenderer().layerSet()

        for lid, l in layers.iteritems():
            if l.id() in visibleLayers:
                self.addLayer( l )
            else:
                self.layers[ l ] = False


