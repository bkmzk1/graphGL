
cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

project(graphGL)

find_package(veil      CONFIG REQUIRED)
find_package(SymEngine CONFIG REQUIRED)

add_executable(
    graphGL src/main.cpp
            src/scene.cpp
            src/analyzer.cpp
)

target_link_libraries(
    graphGL PRIVATE veil::veil
                    ${SYMENGINE_LIBRARIES}
)

target_include_directories(
    graphGL PRIVATE ${SYMENGINE_INCLUDE_DIRS}
)