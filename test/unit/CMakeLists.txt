add_executable( Interpreter_test
    Interpreter_test.cpp
)
set_target_properties( Interpreter_test PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( Interpreter_test
	${OPENSCENEGRAPH_LIBRARIES}  
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
install( TARGETS Interpreter_test DESTINATION bin )
