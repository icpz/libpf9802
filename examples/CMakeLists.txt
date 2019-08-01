cmake_minimum_required(VERSION 3.2)
project(examples)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(ev_example src/ev_example.c)
target_link_libraries(ev_example pf9802)
