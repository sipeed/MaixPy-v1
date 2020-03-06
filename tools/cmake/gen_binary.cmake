
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")


# Config toolchain
if(CONFIG_TOOLCHAIN_PATH)
    set(CMAKE_SIZE   "${CONFIG_TOOLCHAIN_PATH}/${CONFIG_TOOLCHAIN_PREFIX}size${EXT}")
    set(CMAKE_OBJDUMP "${CONFIG_TOOLCHAIN_PATH}/${CONFIG_TOOLCHAIN_PREFIX}objdump${EXT}")
else()
    set(CMAKE_SIZE   "size${EXT}")
    set(CMAKE_SIZE   "objdump${EXT}")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --output-format=binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        COMMENT "-- Generating .bin firmware at ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin"
        )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
        COMMENT "============= firmware ============="
        )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -S ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.txt
        )

## copy libs
## libnncase.a
if(CONFIG_LIB_NNCASE_SOURCE_CODE_ENABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND cp ${CMAKE_BINARY_DIR}/nncase/libnncase.a ${SDK_PATH}/components/kendryte_sdk/libs/
                COMMENT "======== copy libnncase.a ======="
        )
endif()




