cmake_minimum_required(VERSION 3.4.1)

add_library(
        native-lib
        SHARED

        simple_png_encoder.h
        native-lib.cpp)

find_package( zlib REQUIRED )
find_library(log-lib log)

target_link_libraries(native-lib
        PUBLIC ZLIB::ZLIB
        ${log-lib})
target_compile_options(native-lib PRIVATE
        "$<$<CONFIG:RELEASE>:-Os>"
        "$<$<CONFIG:DEBUG>:-Os>"
        )
