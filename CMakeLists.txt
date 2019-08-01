cmake_minimum_required(VERSION 3.2)
project(libpf9802)

option(PF9802_BUILD_EXAMPLES "Build examples" OFF)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_subdirectory(pf9802)

if (PF9802_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif (PF9802_BUILD_EXAMPLES)

