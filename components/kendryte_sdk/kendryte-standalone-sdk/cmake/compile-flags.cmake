add_compile_flags(LD
        -nostartfiles
        -static
        -Wl,--gc-sections
        -Wl,-static
        -Wl,--start-group
        -Wl,--whole-archive
        -Wl,--no-whole-archive
        -Wl,--end-group
        -Wl,-EL
        -Wl,--no-relax
        -T ${SDK_ROOT}/lds/kendryte.ld
        )

# C Flags Settings
add_compile_flags(BOTH
        -mcmodel=medany
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
        -Os
        -ggdb
        )

add_compile_flags(C -std=gnu11 -Wno-pointer-to-int-cast)
add_compile_flags(CXX -std=gnu++17)

if (BUILDING_SDK)
    add_compile_flags(BOTH
            -Wall
            -Werror=all
            -Wno-error=unused-function
            -Wno-error=unused-but-set-variable
            -Wno-error=unused-variable
            -Wno-error=deprecated-declarations
            -Wextra
            -Werror=frame-larger-than=32768
            -Wno-unused-parameter
            -Wno-sign-compare
            -Wno-error=missing-braces
            -Wno-error=return-type
            -Wno-error=pointer-sign
            -Wno-missing-braces
            -Wno-strict-aliasing
            -Wno-implicit-fallthrough
            -Wno-missing-field-initializers
            -Wno-int-to-pointer-cast
            -Wno-error=comment
            -Wno-error=logical-not-parentheses
            -Wno-error=duplicate-decl-specifier
            -Wno-error=parentheses
            )

    add_compile_flags(C -Wno-old-style-declaration)
else ()
    add_compile_flags(BOTH -L${SDK_ROOT}/include/)
endif ()

