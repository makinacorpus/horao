add_executable( Interpreter_test
    Interpreter_test.cpp
)
set_target_properties( Interpreter_test PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( Interpreter_test

)
add_test(Interpreter_test ${EXECUTABLE_OUTPUT_PATH}/Interpreter_testd)



add_executable( PostGisUtils_test
    PostGisUtils_test.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/PostGisUtils.cpp
)
set_target_properties( PostGisUtils_test PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( PostGisUtils_test
    ${LWGEOM_LIBRARY}
	${OPENSCENEGRAPH_LIBRARIES}  
    ${OPENGL_glu_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    poly2tri
)
add_test(PostGisUtils_test ${EXECUTABLE_OUTPUT_PATH}/PostGisUtils_testd)

