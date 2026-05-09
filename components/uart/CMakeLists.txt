if(ESP_PLATFORM)
    idf_component_register(
        SRCS "src/uart_8250.cpp"
        INCLUDE_DIRS "include"
    )
else()
    add_library(component_uart STATIC
        src/uart_8250.cpp
        src/linux_console.cpp
    )
    target_include_directories(component_uart PUBLIC include)
endif()
