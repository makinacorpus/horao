add_library( horao SHARED
    ViewerWidget.cpp
    Interpreter.cpp
    StringUtils.cpp
)
target_link_libraries( horao 
	${OPENSCENEGRAPH_LIBRARIES}  
    ${PostgreSQL_LIBRARY}
    ${LWGEOM_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    X11
)

add_executable( Interpreter_test
    Interpreter_test.cpp
)
set_target_properties( Interpreter_test PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( Interpreter_test
    horao
)
add_test(Interpreter_test ${EXECUTABLE_OUTPUT_PATH}/Interpreter_testd)

add_executable( horaoViewer 
    main.cpp
)
set_target_properties( horaoViewer PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( horaoViewer
    horao
)

add_library( osgdb_postgis MODULE 
    ReaderWriterPOSTGIS.cpp 
    SFosg.cpp
)
set_target_properties( osgdb_postgis PROPERTIES DEBUG_POSTFIX "d" )
set_target_properties( osgdb_postgis PROPERTIES PREFIX "")
target_link_libraries( osgdb_postgis
	${OPENSCENEGRAPH_LIBRARIES}  
    ${PostgreSQL_LIBRARY}
    ${LWGEOM_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${GDAL_LIBRARY}
    poly2tri
)

add_executable( SFosg_test
    SFosg_test.cpp
    SFosg.cpp
)
set_target_properties( SFosg_test PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( SFosg_test
    ${LWGEOM_LIBRARY}
	${OPENSCENEGRAPH_LIBRARIES}  
    ${OPENGL_glu_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    poly2tri
)
add_test(SFosg_test ${EXECUTABLE_OUTPUT_PATH}/SFosg_testd)

add_library( osgdb_mnt MODULE 
    ReaderWriterMNT.cpp 
)
set_target_properties( osgdb_mnt PROPERTIES DEBUG_POSTFIX "d" )
set_target_properties( osgdb_mnt PROPERTIES PREFIX "")
target_link_libraries( osgdb_mnt
	${OPENSCENEGRAPH_LIBRARIES}  
    ${OPENGL_glu_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${GDAL_LIBRARY}
)

install( TARGETS  horaoViewer horao osgdb_postgis osgdb_mnt
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin 
)


