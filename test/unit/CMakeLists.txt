# Debian 10 Buster
cmake_minimum_required(VERSION 3.13.4)


file (GLOB TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.c" )
foreach (TEST_C ${TESTS})
   get_filename_component (TEST_BASE ${TEST_C} NAME_WE)
   set (TEST_BIN "${TEST_BASE}_app")
   add_executable (${TEST_BIN} ${TEST_C})
   if (IS_ARM32)
      # unity bug
      target_compile_definitions(${TEST_BIN} PUBLIC -DUNITY_SUPPORT_64)
   endif()
   target_link_libraries (${TEST_BIN} utils unity)
   target_include_directories (${TEST_BIN} PUBLIC
      ${PROJECT_SOURCE_DIR}/src
      ${PROJECT_SOURCE_DIR}/include
      ${PROJECT_SOURCE_DIR}/Unity/src
      ${PROJECT_BINARY_DIR})
   add_test (${TEST_BASE} ${TEST_BIN})
endforeach (TEST_C)
