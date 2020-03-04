
########## set C flags #########
set(CMAKE_C_FLAGS 	-mcmodel=medany
                    -mabi=lp64f
                    -march=rv64imafc
                    -fno-common
                    -ffunction-sections
                    -fdata-sections
                    -fstrict-volatile-bitfields
                    -fno-zero-initialized-in-bss
                    -ffast-math
                    -fno-math-errno
                    -fsingle-precision-constant
                    -ffloat-store
                    -std=gnu11
                    -Os
                    -Wall
                    -Werror=all
                    -Wno-error=unused-function
                    -Wno-error=unused-but-set-variable
                    -Wno-error=unused-variable
                    -Wno-error=deprecated-declarations
                    -Wno-error=maybe-uninitialized
                    -Wextra
                    -Werror=frame-larger-than=32768
                    -Wno-unused-parameter
                    -Wno-unused-function
                    -Wno-implicit-fallthrough
                    -Wno-sign-compare
                    -Wno-error=missing-braces
                    -Wno-old-style-declaration
                    -Wno-error=pointer-sign
                    -Wno-pointer-to-int-cast
                    -Wno-strict-aliasing
                    -Wno-override-init
                    -Wno-error=format=
                    -Wno-error=format-truncation=
                    -Wno-error=restrict
                    -Wno-error=sequence-point
                    -Wno-int-to-pointer-cast
                    )
################################


###### set CXX(cpp) flags ######
set(CMAKE_CXX_FLAGS -mcmodel=medany
                    -mabi=lp64f
                    -march=rv64imafc
                    -fno-common
                    -ffunction-sections
                    -fdata-sections
                    -fstrict-volatile-bitfields
                    -fno-zero-initialized-in-bss
                    -Os
                    -std=gnu++17
                    -Wall
                    -Wno-error=unused-function
                    -Wno-error=unused-but-set-variable
                    -Wno-error=unused-variable
                    -Wno-error=deprecated-declarations
                    -Wno-error=maybe-uninitialized
                    -Wextra
                    -Werror=frame-larger-than=32768
                    -Wno-unused-parameter
                    -Wno-unused-function
                    -Wno-implicit-fallthrough
                    -Wno-sign-compare
                    -Wno-error=missing-braces
                    -Wno-error=pointer-sign
                    -Wno-strict-aliasing
                    -Wno-error=format=
                    -Wno-error=format-truncation=
                    -Wno-error=restrict
                    -Wno-error=sequence-point
                    -Wno-int-to-pointer-cast
                    )
################################

set(LINK_FLAGS ${LINK_FLAGS}
            -static
            -Wl,-static
            -nostartfiles
            -Wl,--gc-sections
            -Wl,-EL
            -T ${PROJECT_SOURCE_DIR}/compile/kendryte.ld
            -Wl,--start-group
            -Wl,--whole-archive
            kendryte_sdk/libkendryte_sdk.a  main/libmain.a
            -Wl,--no-whole-archive
            -Wl,--end-group
            )
set(CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS}
                        ${LINK_FLAGS}
                        )
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_C_LINK_FLAGS}
                        #${LINK_FLAGS}
                        )
# set(CMAKE_EXE_LINKER_FLAGS  ${CMAKE_EXE_LINKER_FLAGS}
#                             ${LINK_FLAGS}
#                             )
# set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS}
#                               ${LINK_FLAGS}
#                               )
# set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS}
#                               ${LINK_FLAGS}
#                               )


# Convert list to string
string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE ";" " " LINK_FLAGS "${LINK_FLAGS}")
string(REPLACE ";" " " CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS}")
string(REPLACE ";" " " CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
