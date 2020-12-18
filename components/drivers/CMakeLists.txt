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

    if(CONFIG_WS2812_ENABLE)
        list(APPEND ADD_INCLUDE "ws2812/include")
        aux_source_directory("ws2812/src"  WS2812_SRCS)
        list(APPEND ADD_SRCS ${WS2812_SRCS})
    endif()
    if(CONFIG_HTPA_ENABLE)
        list(APPEND ADD_INCLUDE "htpa/include")
        aux_source_directory("htpa/src"  HTPA_SRCS)
        list(APPEND ADD_SRCS ${HTPA_SRCS})
    endif()
    if(CONFIG_AMG88XX_ENABLE)
        list(APPEND ADD_INCLUDE "amg88xx/include")
	aux_source_directory("amg88xx/src"  AMG88XX_SRCS)
	list(APPEND ADD_SRCS ${AMG88XX_SRCS})
    endif()

    list(APPEND ADD_SRCS ${FLASH_SRCS}
                        ${JOYSTICK_SRCS}
                        ${LCD_SRCS}
                        ${SD_CARD_SRCS}
                        )

    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()
endif()

