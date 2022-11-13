# Debian 10 Buster
cmake_minimum_required(VERSION 3.13.4)

include_directories(
   ${CMAKE_BINARY_DIR}/include
   ${CMAKE_SOURCE_DIR}/include
   ${CMAKE_CURRENT_BINARY_DIR}
)
file(GLOB HEADERS *.h ../include/*.h)
file(GLOB SOURCES *.c)

add_subdirectory(utils)

add_executable(${PROJECT_NAME}
   ${HEADERS} ${SOURCES}
   $<TARGET_OBJECTS:utils>
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT binary
)
