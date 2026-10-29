cmake_minimum_required(VERSION 3.16)
project(DrumForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(SDK ${CMAKE_SOURCE_DIR}/third_party/vst3sdk)
if(NOT EXISTS ${SDK}/pluginterfaces)
  message(FATAL_ERROR "VST3 SDK missing: run  git clone --recurse-submodules https://github.com/steinbergmedia/vst3sdk.git third_party/vst3sdk")
endif()

# one list of sources shared with build_windows.bat
file(STRINGS ${CMAKE_SOURCE_DIR}/build/sources.txt SRC_LINES)
set(SRCS "")
foreach(l ${SRC_LINES})
  list(APPEND SRCS ${CMAKE_SOURCE_DIR}/${l})
endforeach()

if(WIN32)
  list(APPEND SRCS ${SDK}/public.sdk/source/main/dllmain.cpp ${SDK}/public.sdk/source/common/threadchecker_win32.cpp
       ${CMAKE_SOURCE_DIR}/src/plugin/editor_win.cpp)
elseif(APPLE)
  list(APPEND SRCS ${SDK}/public.sdk/source/main/macmain.cpp ${CMAKE_SOURCE_DIR}/src/plugin/editor_none.cpp)
else()
  list(APPEND SRCS ${SDK}/public.sdk/source/main/linuxmain.cpp ${CMAKE_SOURCE_DIR}/src/plugin/editor_none.cpp)
endif()

add_library(DrumForge MODULE ${SRCS})
target_include_directories(DrumForge PRIVATE ${SDK} ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(DrumForge PRIVATE $<$<CONFIG:Debug>:DEVELOPMENT=1> $<$<NOT:$<CONFIG:Debug>>:RELEASE=1>)
set_target_properties(DrumForge PROPERTIES PREFIX "" SUFFIX ".vst3" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/raw RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/raw)

if(MSVC)
  target_compile_options(DrumForge PRIVATE /O2 /fp:precise /wd4244 /wd4267)
else()
  target_compile_options(DrumForge PRIVATE -O2 -fno-math-errno -fvisibility=hidden)
endif()

if(WIN32)
  target_compile_definitions(DrumForge PRIVATE UNICODE _UNICODE NOMINMAX)
  target_link_libraries(DrumForge PRIVATE user32 gdi32 ole32 shell32 comdlg32)
  if(NOT MSVC)
    target_link_options(DrumForge PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  # modern VST3 bundle layout:  DrumForge.vst3/Contents/x86_64-win/DrumForge.vst3
  add_custom_command(TARGET DrumForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bundle/DrumForge.vst3/Contents/x86_64-win
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DrumForge> ${CMAKE_BINARY_DIR}/bundle/DrumForge.vst3/Contents/x86_64-win/DrumForge.vst3)
elseif(NOT APPLE)
  target_link_libraries(DrumForge PRIVATE dl pthread)
  add_custom_command(TARGET DrumForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bundle/DrumForge.vst3/Contents/x86_64-linux
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:DrumForge> ${CMAKE_BINARY_DIR}/bundle/DrumForge.vst3/Contents/x86_64-linux/DrumForge.so)
endif()

# offline renderer / analysis tool (engine only, no SDK)
add_executable(df_render EXCLUDE_FROM_ALL tools/render.cpp src/engine/engine.cpp src/engine/params.cpp src/engine/grooves.cpp src/engine/groove_player.cpp src/engine/presets.cpp)
