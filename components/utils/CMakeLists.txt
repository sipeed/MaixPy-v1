if(CONFIG_COMPONENT_UTILS_ENABLE)

    list(APPEND ADD_INCLUDE "include")
    aux_source_directory("src"    ADD_SRCS)
    register_component()

endif()


