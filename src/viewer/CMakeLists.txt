add_executable( simpleViewer 
    main.cpp
    ViewerWidget.cpp
    Interpreter.cpp
)
set_target_properties( simpleViewer PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( simpleViewer
    ${OSGEARTH_LIBRARIES}  
	${OPENSCENEGRAPH_LIBRARIES}  
	${QT_LIBRARIES}
    ${LIBXML2_LIBRARIES}
	${Boost_LIBRARIES}
)
install( TARGETS simpleViewer DESTINATION bin )
