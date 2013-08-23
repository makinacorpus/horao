add_executable( terrainLoader
    main.cpp
)
set_target_properties( terrainLoader PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( terrainLoader
	${OPENSCENEGRAPH_LIBRARIES}  
        ${GDAL_LIBRARY}
)
install( TARGETS terrainLoader DESTINATION bin )
