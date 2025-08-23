cmake_minimum_required(VERSION 3.10)
project(bracketext)

# Set C++ standard (e.g., C++11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/obj)

# Find Lua 5.3 headers and library
find_package(Lua 5.3 REQUIRED)

# Collect all .cpp files from src/
file(GLOB SOURCES "src/*.cpp")

# Create executable in bin/
add_executable(${PROJECT_NAME} ${SOURCES})

# Include Lua headers and your src/ directory
target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    ${LUA_INCLUDE_DIR}  # Path to lua5.3 headers (/usr/include/lua5.3/)
    ${utfcpp_SOURCE_DIR}/source  # UTF8-CPP headers are in "source/"
)

# Add a custom "debug" target (runs gdb automatically)
add_custom_target(
    debug
    COMMAND gdb -ex "run" --args $<TARGET_FILE:bracketext>
    DEPENDS bracketext
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Link Lua library
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LUA_LIBRARIES}  # Links liblua5.3.so
)