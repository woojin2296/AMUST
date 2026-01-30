cmake_minimum_required(VERSION 3.16)

project(amust VERSION 0.2 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        boot_screen_widget.cpp
        boot_screen_widget.h
        main_menu_widget.cpp
        main_menu_widget.h
        progress_pill.cpp
        progress_pill.h
        gpio_controller.cpp
        gpio_controller.h
        hw/tof_sensor_controller.cpp
        hw/tof_sensor_controller.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(amust
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(amust
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(amust PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

find_library(GPIOD_LIBRARY gpiod)
find_path(GPIOD_INCLUDE_DIR gpiod.h)
if(GPIOD_LIBRARY AND GPIOD_INCLUDE_DIR)
    target_include_directories(amust PRIVATE ${GPIOD_INCLUDE_DIR})
    target_link_libraries(amust PRIVATE ${GPIOD_LIBRARY})
    target_compile_definitions(amust PRIVATE AMUST_HAVE_GPIOD=1)
else()
    message(WARNING "libgpiod not found; building without GPIO control")
endif()

set_target_properties(amust PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(amust)
endif()
