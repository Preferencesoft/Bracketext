cmake_minimum_required(VERSION 3.10)
project(bracketext)

# Set C++ standard (e.g., C++11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/obj)

# Enable coverage for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
endif()

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

# ===== PACKAGING CONFIGURATION =====
include(GNUInstallDirs)
include(CPack)

# Package metadata
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "Bracket a tex macro language")
set(CPACK_PACKAGE_VENDOR "Preferencesoft")
set(CPACK_PACKAGE_CONTACT "john@preferencesoft.com")

# Set package generators (choose one or more)
set(CPACK_GENERATOR "TGZ")  # Start with just TGZ for testing
# set(CPACK_GENERATOR "DEB;TGZ;ZIP")  # Multiple formats

# For DEB packages (if using)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <your-email@example.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "liblua5.3-dev")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# Set the package file name
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")

# Installation targets - THESE ARE REQUIRED FOR PACKAGING
install(TARGETS bracketext
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install documentation files
install(FILES 
    README.md 
    LICENSE
    DESTINATION share/doc/${PROJECT_NAME}
)