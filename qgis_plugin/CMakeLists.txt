add_custom_command( OUTPUT resources_rc.py 
    COMMAND pyrcc4 -o resources_rc.py resources.qrc 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS resources.qrc )

add_custom_target(qgis_plugin ALL DEPENDS resources_rc.py)
