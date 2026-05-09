if(ESP_PLATFORM)
    idf_component_register(
        SRCS "src/machine.cpp"
        INCLUDE_DIRS "include"
        REQUIRES cpu memory uart
    )
else()
    add_library(component_machine STATIC src/machine.cpp)
    target_include_directories(component_machine PUBLIC
        include
        ${CMAKE_CURRENT_LIST_DIR}/../cpu/include
        ${CMAKE_CURRENT_LIST_DIR}/../memory/include
        ${CMAKE_CURRENT_LIST_DIR}/../uart/include
    )
    target_link_libraries(component_machine PUBLIC component_cpu component_memory component_uart)
endif()
