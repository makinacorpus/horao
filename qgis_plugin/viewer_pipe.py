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

from xml.sax.saxutils import quoteattr
import xml.etree.ElementTree as ET

import subprocess
import os
import sys

class ViewerPipe:
    """Communication pipe with the viewer"""

    def __init__( self ):
        self.process = None

    def running( self ):
        # if poll() returns something, the process has ended
        return self.process is not None and self.process.poll() is None

    def start( self, execName ):
        self.stop()
        self.process = subprocess.Popen(execName, shell = True, stdin = subprocess.PIPE, stdout = subprocess.PIPE )

    def stop( self ):
        if self.running():
            self.process.terminate()

    def __del__( self ):
        self.stop()

    # send command to the viewer pipe
    # cmd: command name
    # args: dict of arguments
    # return value: [ status, dict ]
    def evaluate( self, cmd, args ):
        if not self.running():
            return [ 'broken_pipe', { 'msg': 'Viewer process has ended'} ]
        
        toSend = cmd + ' ' + ' '.join(["%s=%s" % (k, quoteattr(str(v), {'"' : "&quot;"} )) for k,v in args.iteritems()])
        sys.stderr.write( toSend + "\n" )

        self.process.stdin.write( toSend + "\n" )
        ret = self.process.stdout.readline()
        try:
            root = ET.fromstring( ret )
            ret = [ root.tag, root.attrib ]
            return ret
        except ET.ParseError:
            return [ 'error', {'msg': 'XML Parsing error'} ]


