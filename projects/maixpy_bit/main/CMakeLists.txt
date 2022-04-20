############### Add include ###################
# list(APPEND ADD_INCLUDE "include"
#     )
# list(APPEND ADD_PRIVATE_INCLUDE "")
###############################################

############ Add source files #################
list(APPEND ADD_SRCS  "src/main.cpp"
    )
# aux_source_directory(src ADD_SRCS)
# list(REMOVE_ITEM COMPONENT_SRCS "src/test2.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS micropython kendryte_sdk drivers boards utils spiffs)
if(CONFIG_LIB_NNCASE_SOURCE_CODE_ENABLE AND CONFIG_MICROPYTHON_KMODEL_V4_SUPPORT)
    list(APPEND ADD_REQUIREMENTS nncase)
endif()
###############################################

############ Add static libs ##################
# list(APPEND ADD_STATIC_LIB "lib/libtest.a")
###############################################


register_component()

