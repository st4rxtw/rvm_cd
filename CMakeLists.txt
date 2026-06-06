cmake_minimum_required(VERSION 3.16)
project(rvm_cd CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW   REQUIRED glfw3)
pkg_check_modules(OPENAL REQUIRED openal)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswresample)

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

set(RVM_SOURCES
    src/rvm/GlobalAppDefinitions.cpp
    src/rvm/FileIO.cpp
    src/rvm/GifLoader.cpp
    src/rvm/InputSystem.cpp
    src/rvm/Scene3D.cpp
    src/rvm/AudioPlayback.cpp
    src/rvm/AnimationSystem.cpp
    src/rvm/TextSystem.cpp
    src/rvm/GraphicsSystem.cpp
    src/rvm/RenderDevice.cpp
    src/rvm/EngineCallbacks.cpp
    src/rvm/Log.cpp
    src/rvm/ObjectSystem.cpp
    src/rvm/StageSystem.cpp
    src/rvm/PlayerSystem.cpp
)

set(CD_SOURCES
    src/cd/Game.cpp
)

add_executable(rvm_cd
    src/main.cpp
    ${RVM_SOURCES}
    ${CD_SOURCES}
)

target_include_directories(rvm_cd PRIVATE
    src
    third_party/glad/include
    third_party/stb
    ${GLFW_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(rvm_cd PRIVATE
    glad
    OpenGL::GL
    ${GLFW_LIBRARIES}
    ${OPENAL_LIBRARIES}
    ${FFMPEG_LIBRARIES}
)

target_compile_options(rvm_cd PRIVATE
    ${GLFW_CFLAGS_OTHER}
    ${OPENAL_CFLAGS_OTHER}
    ${FFMPEG_CFLAGS_OTHER}
)