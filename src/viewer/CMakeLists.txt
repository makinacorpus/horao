add_executable( simpleViewer 
    main.cpp
    ViewerWidget.cpp
    Interpreter.cpp
    PostGisUtils.cpp
)
set_target_properties( simpleViewer PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( simpleViewer
	${OPENSCENEGRAPH_LIBRARIES}  
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    ${LWGEOM_LIBRARY}
)

add_library( osgdb_postgis MODULE 
    ReaderWriterPOSTGIS.cpp 
)
set_target_properties( osgdb_postgis PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( osgdb_postgis
	${OPENSCENEGRAPH_LIBRARIES}  
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    ${LWGEOM_LIBRARY}
)
set_target_properties(osgdb_postgis PROPERTIES PREFIX "")
install( TARGETS  simpleViewer osgdb_postgis
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin 
)


