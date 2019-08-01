cmake_minimum_required(VERSION 3.2)
project(pf9802)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_library(${PROJECT_NAME} src/pf9802.c src/utils.c)
target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_link_libraries(${PROJECT_NAME} PUBLIC ev)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)

