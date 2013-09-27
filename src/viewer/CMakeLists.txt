add_library( horao SHARED
    ViewerWidget.cpp
    Interpreter.cpp
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

install( TARGETS  horaoViewer horao 
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin 
)


