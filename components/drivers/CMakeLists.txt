if(CONFIG_COMPONENT_DRIVERS_ENABLE)

    list(APPEND ADD_INCLUDE "flash/include"
                            "joystick/include"
                            "lcd/include"
                            "sd_card/include"
                            )
    aux_source_directory("flash/src"    FLASH_SRCS)
    aux_source_directory("joystick/src" JOYSTICK_SRCS)
    aux_source_directory("lcd/src"      LCD_SRCS)
    aux_source_directory("sd_card/src"  SD_CARD_SRCS)
    list(APPEND ADD_SRCS ${FLASH_SRCS}
                        ${JOYSTICK_SRCS}
                        ${LCD_SRCS}
                        ${SD_CARD_SRCS}
                        )

    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()
endif()

