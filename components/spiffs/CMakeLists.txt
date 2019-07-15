if(CONFIG_COMPONENT_SPIFFS_ENABLE)

    list(APPEND ADD_INCLUDE "spiffs_port/include"
                            "core/src")
    aux_source_directory("spiffs_port/src"    PORT_SRCS)
    aux_source_directory("core/src"         SPIFFS_SRCS)
    list(APPEND ADD_SRCS ${SPIFFS_SRCS}
                         ${PORT_SRCS}
                         )
    list(APPEND ADD_REQUIREMENTS drivers)
    register_component()

endif()


