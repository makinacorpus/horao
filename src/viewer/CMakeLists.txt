add_executable( simpleViewer 
    main.cpp
    ViewerWidget.cpp
    Interpreter.cpp
    StringUtils.cpp
)
set_target_properties( simpleViewer PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( simpleViewer
	${OPENSCENEGRAPH_LIBRARIES}  
    ${PostgreSQL_LIBRARY}
    ${LWGEOM_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${OPENGL_gl_LIBRARY}
)

add_library( osgdb_postgis MODULE 
    ReaderWriterPOSTGIS.cpp 
    PostGisUtils.cpp
)
set_target_properties( osgdb_postgis PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( osgdb_postgis
	${OPENSCENEGRAPH_LIBRARIES}  
    ${PostgreSQL_LIBRARY}
    ${LWGEOM_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${OPENGL_gl_LIBRARY}
)
set_target_properties(osgdb_postgis PROPERTIES PREFIX "")

#add_library( osgdb_mnt MODULE 
#    ReaderWriterMNT.cpp 
#)
#set_target_properties( osgdb_mnt PROPERTIES DEBUG_POSTFIX "d" )
#target_link_libraries( osgdb_mnt
#	${OPENSCENEGRAPH_LIBRARIES}  
#    ${OPENGL_glu_LIBRARY}
#    ${OPENGL_gl_LIBRARY}
#    ${GDAL_LIBRARY}
#)
#set_target_properties(osgdb_mnt PROPERTIES PREFIX "")

install( TARGETS  simpleViewer osgdb_postgis #osgdb_mnt
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin 
)


