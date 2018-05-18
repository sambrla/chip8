cmake_minimum_required(VERSION 3.11)
project(Chip-8)

# Set compiler options
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else()
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# Set module path
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Find SFML
find_package(SFML 2.4.2 REQUIRED audio graphics window system)

# Add sources
file(GLOB CHIP8_SRC
    "src/*.hpp"
    "src/*.cpp")
include_directories("modules/cxxopts/include/")
include_directories(${SFML_INCLUDE_DIR})

# Configure output
set(EXECUTABLE_OUTPUT_PATH "bin")
add_executable(chip8 ${CHIP8_SRC})
target_link_libraries(chip8 ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})

# Copy SFML DLL files to output directory (Windows)
if (WIN32)
   add_custom_command(TARGET chip8 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
         ${SFML_ROOT}/bin/openal32.dll
         ${SFML_ROOT}/bin/sfml-audio$<$<CONFIG:Debug>:-d>-2.dll
         ${SFML_ROOT}/bin/sfml-graphics$<$<CONFIG:Debug>:-d>-2.dll
         ${SFML_ROOT}/bin/sfml-window$<$<CONFIG:Debug>:-d>-2.dll
         ${SFML_ROOT}/bin/sfml-system$<$<CONFIG:Debug>:-d>-2.dll
         $<TARGET_FILE_DIR:chip8>)
endif()
