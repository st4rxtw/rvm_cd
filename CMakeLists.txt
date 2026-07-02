cmake_minimum_required(VERSION 3.16)

if("${PLATFORM}" STREQUAL "NX" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/cmake/Switch.cmake" CACHE PATH "")
endif()

project(rvm_cd CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(RVM_SOURCES
    src/rvm/GlobalAppDefinitions.cpp
    src/rvm/FileIO.cpp
    src/rvm/GifLoader.cpp
    src/rvm/InputSystem.cpp
    src/rvm/Scene3D.cpp
    src/rvm/AudioPlayback.cpp
    src/rvm/VideoPlayer_FFmpeg.cpp
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
    src/cd/Title/TitleInit.cpp
    src/cd/Title/SegaObj.cpp
    src/cd/Title/CWLogoObj.cpp
    src/cd/Title/SonicObj.cpp
    src/cd/Title/LogoObj.cpp
    src/cd/Title/BackgroundObj.cpp
    src/cd/Title/CloudsObj.cpp
    src/cd/Title/LiteRibbonObj.cpp
    src/cd/Title/StartObj.cpp
    src/cd/Global/GlobalInit.cpp
    src/cd/Global/ExplosionObj.cpp
    src/cd/Global/SmokePuffObj.cpp
    src/cd/Global/RingSparkleObj.cpp
    src/cd/Global/ObjectScoreObj.cpp
    src/cd/Global/FadeMusicObj.cpp
    src/cd/Global/InvincibilityObj.cpp
    src/cd/Players/PlayerNative.cpp
)

if(NOT DEFINED PLATFORM)
    set(PLATFORM PC)
endif()

include(cmake/${PLATFORM}.cmake)
