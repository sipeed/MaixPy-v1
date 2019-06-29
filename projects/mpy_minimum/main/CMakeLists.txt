############### Add include ###################
# list(APPEND ADD_INCLUDE "include"
#     )
# list(APPEND ADD_PRIVATE_INCLUDE "${CMAKE_BINARY_DIR}/micropython/genhdr")
###############################################

############ Add source files #################
list(APPEND ADD_SRCS  "src/main.c"
                        "${SDK_PATH}/components/micropython/core/lib/utils/printf.c"
                        "${SDK_PATH}/components/micropython/core/lib/utils/stdout_helpers.c"
                        "${SDK_PATH}/components/micropython/core/lib/utils/pyexec.c"
                        "${SDK_PATH}/components/micropython/core/lib/libc/string0.c"
                        "${SDK_PATH}/components/micropython/core/lib/mp-readline/readline.c"
    )
# aux_source_directory(src ADD_SRCS)
# list(REMOVE_ITEM COMPONENT_SRCS "src/test2.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS micropython)
###############################################

############ Add static libs ##################
# list(APPEND ADD_STATIC_LIB "lib/libtest.a")
###############################################


register_component()

