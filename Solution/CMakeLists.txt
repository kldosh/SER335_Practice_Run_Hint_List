cmake_minimum_required(VERSION 3.23)
project(SER_Assignment6 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(SER_Assignment6 DoshFilters.c BmpProcessor.o)
target_link_libraries(SER_Assignment6 m)
