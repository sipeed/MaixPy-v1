

################# Add include #################
list(APPEND ADD_INCLUDE "include"
                        "kendryte-standalone-sdk/lib/bsp/include"
                        "kendryte-standalone-sdk/lib/drivers/include"
                        "kendryte-standalone-sdk/lib/freertos/include"
                        "kendryte-standalone-sdk/lib/freertos/conf"
                        "kendryte-standalone-sdk/lib/freertos/portable"
                        "kendryte-standalone-sdk/lib/utils/include"
    )
list(APPEND ADD_PRIVATE_INCLUDE "include_private")
###############################################

############## Add source files ###############
aux_source_directory("kendryte-standalone-sdk/lib/bsp"    ADD_SRCS_BSP)
aux_source_directory("kendryte-standalone-sdk/lib/drivers" ADD_SRCS_DRIVER)
aux_source_directory("kendryte-standalone-sdk/lib/freertos" ADD_SRCS_FREERTOS)
aux_source_directory("kendryte-standalone-sdk/lib/freertos/portable" ADD_SRCS_FREERTOS2)
list(APPEND ADD_SRCS  ${ADD_SRCS_BSP}
                      ${ADD_SRCS_DRIVER}
                      ${ADD_SRCS_FREERTOS}
                      ${ADD_SRCS_FREERTOS2}
                        )
# list(REMOVE_ITEM COMPONENT_SRCS "src/test.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS gcc m c)
###############################################

############ Add static libs ##################
# list(APPEND ADD_STATIC_LIB "lib/libtest.a")
###############################################

add_definitions(-DCONFIG_LOG_LEVEL=${CONFIG_SDK_LOG_LEVEL}
                -DCONFIG_LOG_ENABLE=1
                -DCONFIG_LOG_COLORS=1
                -DLOG_KERNEL=1
                -D__riscv64
                )

register_component()


