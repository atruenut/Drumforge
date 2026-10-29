# Cross-compile the Windows x64 VST3 (.dll) from Linux/macOS using Zig as the C++ compiler:
#   pip install ziglang
#   cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-zig-win64.cmake -G Ninja && cmake --build build-win --target DrumForge
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
get_filename_component(_tools "${CMAKE_CURRENT_LIST_DIR}/../tools" ABSOLUTE)
set(CMAKE_C_COMPILER   ${_tools}/zigcc)
set(CMAKE_CXX_COMPILER ${_tools}/zigcxx)
set(CMAKE_AR           ${_tools}/zigar CACHE FILEPATH "")
set(CMAKE_RANLIB       ${_tools}/zigranlib CACHE FILEPATH "")
set(CMAKE_RC_COMPILER_WORKS 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
