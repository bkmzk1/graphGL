
cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

project(graphGL VERSION 1.0.0)

find_package(glfw3     REQUIRED)
find_package(OpenGL    REQUIRED)
find_package(veil      CONFIG REQUIRED)
find_package(SymEngine CONFIG REQUIRED)

set(IMGUI_DIR 
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/dearimgui"
)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

add_executable(
    graphGL src/main.cpp
            src/scene.cpp
            src/math.cpp
            ${IMGUI_SOURCES}
)

target_link_libraries(
    graphGL PRIVATE glfw
                    OpenGL::GL
                    veil::veil
                    ${SYMENGINE_LIBRARIES}
)

target_include_directories(
    graphGL PRIVATE ${SYMENGINE_INCLUDE_DIRS}
                    ${IMGUI_DIR}
)