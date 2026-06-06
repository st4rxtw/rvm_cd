find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW   REQUIRED glfw3)
pkg_check_modules(OPENAL REQUIRED openal)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswresample libswscale)

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

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
