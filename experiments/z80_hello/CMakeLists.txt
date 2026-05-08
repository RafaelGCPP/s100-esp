add_executable(z80_hello main.cpp)
target_include_directories(z80_hello PRIVATE ${chips_SOURCE_DIR})
target_link_libraries(z80_hello PRIVATE chips_impl)
