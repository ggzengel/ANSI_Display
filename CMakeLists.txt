# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD waveshare_rp2350_lcd_1.28 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)


project(ANSI_Display C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()



# === Config library ===
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/lib/Config DIR_Config_SRCS)
add_library(Config ${DIR_Config_SRCS})
target_link_libraries(Config PUBLIC pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc)

# === Fonts library ===
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/lib/Fonts DIR_Fonts_SRCS)
add_library(Fonts ${DIR_Fonts_SRCS})

# === GUI library ===
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/lib/GUI DIR_GUI_SRCS)
add_library(GUI ${DIR_GUI_SRCS})
target_link_libraries(GUI PUBLIC Config)
target_include_directories(GUI PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lib/Config)

# === LCD library ===
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/lib/LCD DIR_LCD_SRCS)
add_library(LCD ${DIR_LCD_SRCS})
target_link_libraries(LCD PUBLIC Config)
target_include_directories(LCD PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lib/Config)

# Add executable from src/main.c
add_executable(ANSI_Display src/main.c src/usb_descriptors.c)

pico_set_program_name(ANSI_Display "ANSI_Display")
pico_set_program_version(ANSI_Display "0.1")

# Enable TinyUSB for USB-CDC
target_compile_definitions(ANSI_Display PRIVATE
    PICO_STDIO_USB_ENABLE=1
    CFG_TUSB_MCU=OPT_MCU_RP2040
    CFG_TUSB_RHPORT0_MODE=OPT_MODE_DEVICE
    CFG_TUD_CDC=1
    CFG_TUD_CDC_RX_BUFSIZE=256
    CFG_TUD_CDC_TX_BUFSIZE=256
)

# Enable stdio over USB, disable UART
pico_enable_stdio_uart(ANSI_Display 0)
pico_enable_stdio_usb(ANSI_Display 1)

# Link with Pico libraries, TinyUSB, and our lib sub-libraries
target_link_libraries(ANSI_Display
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_spi
    hardware_gpio
    hardware_i2c
    hardware_pwm
    Config
    Fonts
    GUI
    LCD
)

# Add include directories
target_include_directories(ANSI_Display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/lib/Config
    ${CMAKE_CURRENT_LIST_DIR}/lib/Fonts
    ${CMAKE_CURRENT_LIST_DIR}/lib/GUI
    ${CMAKE_CURRENT_LIST_DIR}/lib/LCD
)

pico_add_extra_outputs(ANSI_Display)

