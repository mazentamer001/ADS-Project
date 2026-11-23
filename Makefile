cmake_minimum_required(VERSION 3.12)
project(LogicSim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimized build (same as -O2 in the old Makefile)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -pedantic)

# ---- Libraries --------------------------------------------------------------
# Custom data structures (Milestone 1)
add_library(ds_lib STATIC
  ds/Graph.cpp
  ds/Queue.cpp
  ds/Stack.cpp
)
target_include_directories(ds_lib PUBLIC ds)

# Application code (parser, algorithms, simulators, analysis, output)
add_library(app_lib STATIC
  src/Parser.cpp
  src/SimParser.cpp
  src/GraphAlgos.cpp
  src/TimingAnalyzer.cpp
  src/VCDWriter.cpp
  src/EventSim.cpp
  src/LevelizedSim.cpp
)
target_include_directories(app_lib PUBLIC src)
target_compile_definitions(app_lib PUBLIC HAVE_SIMS HAVE_GRAPHALGOS)
target_link_libraries(app_lib PUBLIC ds_lib)

# ---- Executables ------------------------------------------------------------
add_executable(sim src/main.cpp)
target_link_libraries(sim PRIVATE app_lib)

add_executable(test_ds tests/test_ds.cpp)
target_link_libraries(test_ds PRIVATE app_lib)

add_executable(test_analysis tests/test_analysis.cpp)
target_link_libraries(test_analysis PRIVATE app_lib)

add_executable(test_circuits tests/test_circuits.cpp)
target_link_libraries(test_circuits PRIVATE app_lib)

# ---- Tests (run with: ctest --test-dir build --output-on-failure) -----------
enable_testing()
add_test(NAME ds        COMMAND test_ds        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME analysis  COMMAND test_analysis tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_test(NAME circuits  COMMAND test_circuits  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# ---- Demo: run every circuit, write one .vcd each into build/waves ----------
file(GLOB STIM_FILES ${CMAKE_SOURCE_DIR}/tests/*.stim)
set(DEMO_DIR ${CMAKE_BINARY_DIR}/waves)

add_custom_target(demo
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DEMO_DIR}
  COMMENT "Running all demo circuits (VCD files in ${DEMO_DIR})"
  VERBATIM
)
foreach(stim ${STIM_FILES})
  get_filename_component(name ${stim} NAME_WE)
  set(net ${CMAKE_SOURCE_DIR}/tests/${name}.net)
  if(EXISTS ${net})
    # "|| true" so a circuit that errors out (e.g. counter before DFF support)
    # does not stop the rest of the demo
    add_custom_command(TARGET demo POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "=== ${name}"
      COMMAND sh -c "$<TARGET_FILE:sim> ${net} ${stim} -o ${DEMO_DIR}/${name}.vcd || true"
      VERBATIM
    )
  endif()
endforeach()
add_custom_command(TARGET demo POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "=== loop_error"
  COMMAND sh -c "$<TARGET_FILE:sim> ${CMAKE_SOURCE_DIR}/tests/loop_error.net || true"
  VERBATIM
)
add_dependencies(demo sim)

# ---- Regenerate ripple8.net / cla8.net --------------------------------------
add_custom_target(generate-adders
  COMMAND python3 tools/gen_adders.py tests
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Generating adder netlists"
  VERBATIM
)