if (WIN32)
    set(EXT ".exe")
else ()
    set(EXT "")
endif ()

message(STATUS "Check for RISCV toolchain ...")
if(NOT TOOLCHAIN)
    find_path(_TOOLCHAIN riscv64-unknown-elf-gcc${EXT})
    global_set(TOOLCHAIN "${_TOOLCHAIN}")
elseif(NOT "${TOOLCHAIN}" MATCHES "/$")
    global_set(TOOLCHAIN "${TOOLCHAIN}")
endif()

if (NOT TOOLCHAIN)
    message(FATAL_ERROR "TOOLCHAIN must be set, to absolute path of kendryte-toolchain dist/bin folder.")
endif ()

message(STATUS "Using ${TOOLCHAIN} RISCV toolchain")

global_set(CMAKE_C_COMPILER "${TOOLCHAIN}/riscv64-unknown-elf-gcc${EXT}")
global_set(CMAKE_CXX_COMPILER "${TOOLCHAIN}/riscv64-unknown-elf-g++${EXT}")
global_set(CMAKE_LINKER "${TOOLCHAIN}/riscv64-unknown-elf-ld${EXT}")
global_set(CMAKE_AR "${TOOLCHAIN}/riscv64-unknown-elf-ar${EXT}")
global_set(CMAKE_OBJCOPY "${TOOLCHAIN}/riscv64-unknown-elf-objcopy${EXT}")
global_set(CMAKE_SIZE "${TOOLCHAIN}/riscv64-unknown-elf-size${EXT}")
global_set(CMAKE_OBJDUMP "${TOOLCHAIN}/riscv64-unknown-elf-objdump${EXT}")
if (WIN32)
    if(EXISTS "${TOOLCHAIN}/make${EXT}")
        global_set(CMAKE_MAKE_PROGRAM "${TOOLCHAIN}/make${EXT}")
    else()
        global_set(CMAKE_MAKE_PROGRAM "${TOOLCHAIN}/mingw32-make${EXT}")
    endif()
endif ()

execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=crt0.o OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE CRT0_OBJ)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=crtbegin.o OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE CRTBEGIN_OBJ)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=crtend.o OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE CRTEND_OBJ)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=crti.o OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE CRTI_OBJ)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=crtn.o OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE CRTN_OBJ)

global_set(CMAKE_C_LINK_EXECUTABLE
        "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> \"${CRTI_OBJ}\" \"${CRTBEGIN_OBJ}\" <OBJECTS> \"${CRTEND_OBJ}\" \"${CRTN_OBJ}\" -o <TARGET> <LINK_LIBRARIES>")

global_set(CMAKE_CXX_LINK_EXECUTABLE
        "<CMAKE_CXX_COMPILER>  <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> \"${CRTI_OBJ}\" \"${CRTBEGIN_OBJ}\" <OBJECTS> \"${CRTEND_OBJ}\" \"${CRTN_OBJ}\" -o <TARGET> <LINK_LIBRARIES>")

get_filename_component(_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
if (NOT "${TOOLCHAIN}" STREQUAL "${_BIN_DIR}")
    message(FATAL_ERROR "CMAKE_C_COMPILER is not in kendryte-toolchain dist/bin folder.")
endif ()
