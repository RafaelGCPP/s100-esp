if(ESP_PLATFORM)
    idf_component_register(
        SRCS "src/memory_map.cpp"
        INCLUDE_DIRS "include"
    )
else()
    add_library(component_memory STATIC src/memory_map.cpp)
    target_include_directories(component_memory PUBLIC include)
endif()
