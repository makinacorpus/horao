add_executable( simpleViewer 
    main.cpp
    ViewerWidget.cpp
    Interpreter.cpp
)
set_target_properties( simpleViewer PROPERTIES DEBUG_POSTFIX "d" )
target_link_libraries( simpleViewer
	${OPENSCENEGRAPH_LIBRARIES}  
	${QT_LIBRARIES}
	${Boost_LIBRARIES}
)
install( TARGETS simpleViewer DESTINATION bin )
