cmake_minimum_required(VERSION 3.10)
project(ProxyServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/proxy_server.cpp
    src/socket_utils.cpp
    src/http_handler.cpp
    src/logger.cpp
    src/cache_manager.cpp
)

# Create executable
add_executable(proxy_server ${SOURCES})

# Link libraries
target_link_libraries(proxy_server PRIVATE pthread)

# Set output directory
set_target_properties(proxy_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS proxy_server DESTINATION bin)
