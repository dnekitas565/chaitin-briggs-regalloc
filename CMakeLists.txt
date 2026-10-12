cmake_minimum_required(VERSION 3.16)
project(RegAlloc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra)
endif()

# The local MSYS2 toolchain cannot link libstdc++ dynamically; static linking is harmless elsewhere.
if(MINGW)
  add_link_options(-static)
endif()

enable_testing()

# --- Chaitin-Briggs register allocator library (standard library only) ---
add_library(regalloc_core STATIC
  src/parser.cpp src/cfg.cpp src/liveness.cpp src/interference.cpp src/costmodel.cpp
  src/spillcode.cpp src/allocator.cpp src/interp.cpp src/emit.cpp)
target_include_directories(regalloc_core PUBLIC include)

add_executable(regalloc src/main.cpp)
target_link_libraries(regalloc regalloc_core)

# --- Tests: one binary per file, each a separate ctest entry ---
set(TEST_NAMES test_parser test_interp test_cfg test_ig test_cost test_alloc test_coalesce test_emit test_novelty test_integration)
foreach(t IN LISTS TEST_NAMES)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} regalloc_core)
  target_compile_definitions(${t} PRIVATE SAMPLES_DIR="${CMAKE_SOURCE_DIR}/samples")
  add_test(NAME ${t} COMMAND ${t})
endforeach()
# The integration test drives the CLI binary.
target_compile_definitions(test_integration PRIVATE REGALLOC_EXE="$<TARGET_FILE:regalloc>")
add_dependencies(test_integration regalloc)
