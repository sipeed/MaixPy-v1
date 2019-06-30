if(CONFIG_BOARD_M5STICK)

    list(APPEND ADD_INCLUDE "m5stick/include")
    aux_source_directory("m5stick/src" ADD_SRCS)
    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

endif()
    
list(APPEND ADD_INCLUDE "include")
aux_source_directory("src" BOARDS_SRCS)
list(APPEND ADD_SRCS ${BOARDS_SRCS})

register_component()
