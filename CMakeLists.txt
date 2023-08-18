cmake_minimum_required(VERSION 3.26)
project(mote)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(wmote)
add_subdirectory(dsulib)

add_executable(mote main.cpp)

target_compile_definitions(wmote PRIVATE WIIMOTELIBPP_DEBUG=1)
target_link_libraries(${PROJECT_NAME} PRIVATE wmote dsulib)
target_link_libraries(mote PRIVATE -fsanitize=address)