cmake_minimum_required(VERSION 3.16)
project(rvm_cd CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

add_executable(rvm_cd
    src/main.cpp
)

target_include_directories(rvm_cd PRIVATE
    third_party/glad/include
    third_party/stb
    ${GLFW_INCLUDE_DIRS}
)

target_link_libraries(rvm_cd PRIVATE
    glad
    OpenGL::GL
    ${GLFW_LIBRARIES}
)

target_compile_options(rvm_cd PRIVATE ${GLFW_CFLAGS_OTHER})