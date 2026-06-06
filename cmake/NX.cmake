find_package(PkgConfig REQUIRED)

pkg_check_modules(OPENAL REQUIRED openal)
find_package(SDL2 REQUIRED)
pkg_check_modules(FFMPEG libavcodec libavformat libavutil libswresample libswscale)
if(NOT FFMPEG_FOUND)
    message(WARNING
        "FFmpeg not found — music and video will be silent/skipped.\n"
        "Install with: dkp-pacman -S switch-ffmpeg")
endif()

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_compile_definitions(glad PRIVATE __SWITCH__)

add_executable(rvm_cd.elf
    src/main.cpp
    ${RVM_SOURCES}
    ${CD_SOURCES}
)

target_compile_definitions(rvm_cd.elf PRIVATE
    __SWITCH__
    $<$<BOOL:${FFMPEG_FOUND}>:HAVE_FFMPEG>
)

target_include_directories(rvm_cd.elf PRIVATE
    src
    third_party/glad/include
    third_party/stb
    ${OPENAL_INCLUDE_DIRS}
    $<$<BOOL:${FFMPEG_FOUND}>:${FFMPEG_INCLUDE_DIRS}>
)

target_link_libraries(rvm_cd.elf PRIVATE
    glad
    ${OPENAL_LIBRARIES}
    SDL2::SDL2
    EGL glapi drm_nouveau
    $<$<BOOL:${FFMPEG_FOUND}>:${FFMPEG_LIBRARIES}>
    nx
    m
)

target_compile_options(rvm_cd.elf PRIVATE
    $<$<BOOL:${FFMPEG_FOUND}>:${FFMPEG_CFLAGS_OTHER}>
)

nx_generate_nacp(rvm_cd.nacp
    NAME    "Sonic CD (rvm_cd)"
    AUTHOR  "yuyu"
    VERSION "1.0.0"
)

set(NRO_ICON_ARG "")
if(EXISTS ${CMAKE_SOURCE_DIR}/icon.png)
    find_program(IMAGEMAGICK_CMD NAMES magick convert)
    if(IMAGEMAGICK_CMD)
        set(_icon_jpg ${CMAKE_BINARY_DIR}/icon.jpg)
        add_custom_command(
            OUTPUT  ${_icon_jpg}
            COMMAND ${IMAGEMAGICK_CMD} ${CMAKE_SOURCE_DIR}/icon.png
                    -resize 256x256^ -gravity center -extent 256x256
                    ${_icon_jpg}
            DEPENDS ${CMAKE_SOURCE_DIR}/icon.png
            COMMENT "Converting icon.png → icon.jpg"
        )
        add_custom_target(nro_icon DEPENDS ${_icon_jpg})
        add_dependencies(rvm_cd.elf nro_icon)
        set(NRO_ICON_ARG ICON ${_icon_jpg})
    else()
        message(WARNING "ImageMagick not found — NRO will have no icon. Install: pacman -S imagemagick")
    endif()
endif()

nx_create_nro(rvm_cd.elf
    NACP rvm_cd.nacp
    ${NRO_ICON_ARG}
)
