if(ESP_PLATFORM)
    idf_component_register(
        SRCS "src/z80_cpu.cpp"
        INCLUDE_DIRS "include"
    )
else()
    add_library(component_cpu STATIC src/z80_cpu.cpp)
    target_include_directories(component_cpu PUBLIC include ${chips_SOURCE_DIR})
endif()
