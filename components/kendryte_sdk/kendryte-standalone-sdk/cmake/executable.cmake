if (NOT BUILDING_SDK)
    if(EXISTS ${SDK_ROOT}/libkendryte.a)
        add_library(kendryte STATIC IMPORTED)
        set_property(TARGET kendryte PROPERTY IMPORTED_LOCATION ${SDK_ROOT}/libkendryte.a)
        include_directories(${SDK_ROOT}/include/)
    else()
        header_directories(${SDK_ROOT}/lib)
        add_subdirectory(${SDK_ROOT}/lib)
    endif()
endif ()

removeDuplicateSubstring(${CMAKE_C_FLAGS} CMAKE_C_FLAGS)
removeDuplicateSubstring(${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS)

message("SOURCE_FILES=${SOURCE_FILES}")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})


set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(${PROJECT_NAME}
        -Wl,--start-group
        gcc m c
        -Wl,--whole-archive
        kendryte
        -Wl,--no-whole-archive
        -Wl,--end-group
        )
        
if (EXISTS ${SDK_ROOT}/src/${PROJ}/project.cmake)
    include(${SDK_ROOT}/src/${PROJ}/project.cmake)
endif ()

IF(SUFFIX)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SUFFIX ${SUFFIX})
ENDIF()

# Build target
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --output-format=binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}${SUFFIX} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        DEPENDS ${PROJECT_NAME}
        COMMENT "Generating .bin file ...")

# show information
include(${CMAKE_CURRENT_LIST_DIR}/dump-config.cmake)
