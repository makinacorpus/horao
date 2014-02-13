Horao
=====

Oslandia's 3D visualisation stack

* [Documentation](http://oslandia.github.io/horao/)


Current architecture
--------------------

A simple viewer built around OpenSceneGraph can be found in the src/viewer subfolder.
It is designed to listen to commands on its standard input.
See doc/protocol.txt for the basic protocol specifications (still subject to changes)

The other piece is a Python plugin that is used to connect QGIS signals to the viewer (in another process) to allow loading
of QGIS layers with 3D geometries.

If you want to be able to properly load PostGIS vector layers with 3D data as well as synchronize color changes between the 2D view and the 3D view, you should use a recent version of QGIS (~ end of august 2013).
