
if(CONFIG_COMPONENT_KENDRYTE_SDK_ENABLE)
    ################# Add include #################
    list(APPEND ADD_INCLUDE "include"
                            "kendryte-standalone-sdk/lib/bsp/include"
                            "kendryte-standalone-sdk/lib/drivers/include"
                            "kendryte-standalone-sdk/lib/utils/include"
        )
    if(CONFIG_FREERTOS_ENABLE)
        list(APPEND ADD_INCLUDE "kendryte-standalone-sdk/lib/freertos/include"
                                "kendryte-standalone-sdk/lib/freertos/portable"
        )
    endif()
    # list(APPEND ADD_PRIVATE_INCLUDE "include_private")
    ###############################################

    ############## Add source files ###############
    append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/bsp")
    append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/drivers")
    append_srcs_dir(ADD_SRCS "src")
    if(CONFIG_FREERTOS_ENABLE)
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/freertos")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/freertos/portable")
        list(REMOVE_ITEM ADD_SRCS "kendryte-standalone-sdk/lib/freertos/portable/heap_4.c")
        list(APPEND ADD_SRCS "src2/heap_4.c")
        # asm with freerots support
        set(ADD_ASM_SRCS "kendryte-standalone-sdk/lib/bsp/crt.S"
                         "kendryte-standalone-sdk/lib/freertos/portable/portasm.S"
           )
    else()
        # asm with no freerots support
        set(ADD_ASM_SRCS "src2/crt.S")
        list(APPEND ADD_SRCS "src2/locks.c")
        list(APPEND ADD_SRCS "src2/bsp2.c")
    endif()
    list(APPEND ADD_SRCS ${ADD_ASM_SRCS})
    # list(REMOVE_ITEM COMPONENT_SRCS "src/test.c")
    SET_PROPERTY(SOURCE ${ADD_ASM_SRCS} PROPERTY LANGUAGE C)
    SET_SOURCE_FILES_PROPERTIES(${ADD_ASM_SRCS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp -D __riscv64")
    ###############################################

    ###### Add required/dependent components ######
    list(APPEND ADD_REQUIREMENTS gcc m c atomic)
    ###############################################

    ############ Add static libs ##################
    if(CONFIG_MIC_ARRAY_ENABLE)
        list(APPEND ADD_STATIC_LIB "libs/lib_mic.a")
    endif()
    list(APPEND ADD_STATIC_LIB "libs/lib_sipeed_kpu.a")
    ###############################################

    list(APPEND ADD_DEFINITIONS -DCONFIG_LOG_LEVEL=${CONFIG_SDK_LOG_LEVEL}
                                -DCONFIG_LOG_ENABLE=1
                                -DCONFIG_LOG_COLORS=1
                                -DLOG_KERNEL=1
                                -D__riscv64
                                )

    register_component()
endif()

