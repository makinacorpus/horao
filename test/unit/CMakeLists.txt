add_executable( PostGisUtils_test
    PostGisUtils_test.cpp
)
set_target_properties( PostGisUtils_test PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( PostGisUtils_test
	${OPENSCENEGRAPH_LIBRARIES}  
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
install( TARGETS PostGisUtils_test DESTINATION bin )
