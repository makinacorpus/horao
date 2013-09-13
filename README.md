Horao
=====

Oslandia's 3D visualisation stack

Current architecture
--------------------

A simple viewer built around OpenSceneGraph can be found in the src/viewer subfolder.
It is designed to listen to commands on its standard input.
See doc/protocol.txt for the basic protocol specifications (still subject to changes)

The other piece is a Python plugin that is used to connect QGIS signals to the viewer (in another process) to allow loading
of QGIS layers with 3D geometries.

It depends on a local branch of QGIS where 3D geometries and 3D types are properly imported by QGIS together
with some addition regarding handling of symbology signals.
The branch can be found on our repository: https://github.com/Oslandia/Quantum-GIS/tree/postgis3d_and_symbology_emit
