cmake_minimum_required(VERSION 3.11)
project(Chip-8)

# Set compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wpedantic")
endif()

# Add sources
file(GLOB SOURCE_FILES "src/*.cpp")
include_directories("modules/cxxopts/include/")

# Configure output
set(EXECUTABLE_OUTPUT_PATH "bin")
add_executable(chip8 ${SOURCE_FILES})

# Find and add SFML
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(SFML 2.4.2 REQUIRED audio graphics window system)
if(SFML_FOUND)
    include_directories(${SFML_INCLUDE_DIR})
    target_link_libraries(chip8 ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
endif()
