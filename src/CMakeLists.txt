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

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        C_STANDARD          11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE _POSIX_C_SOURCE=200112
    PRIVATE _GNU_SOURCE
    PRIVATE _XOPEN_SOURCE
    PRIVATE HAVE_CONFIG_H
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        # Clang
        $<$<C_COMPILER_ID:Clang>:
            -Wcast-align
            -Wcast-qual
            -Wconversion
            -Wexit-time-destructors
            -Wglobal-constructors
            -Wmissing-noreturn
            -Wmissing-prototypes
            -Wno-missing-braces
            -Wold-style-cast
            -Wshadow
            -Wweak-vtables
            -Werror
            -Wall
            -Wextra
            $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,8.0.0>:-Wextra-semi-stmt>
        >

        # GCC
        $<$<C_COMPILER_ID:GNU>:
            -Waddress
            -Waggregate-return
            -Wformat-nonliteral
            -Wformat-security
            -Wformat
            -Winit-self
            -Wmissing-declarations
            -Wmissing-include-dirs
            -Wno-multichar
            -Wparentheses
            -Wtype-limits
            -Wunused-parameter
            -Wunreachable-code
            -Wwrite-strings
            -Wpointer-arith
            -Wundef
            -Wbad-function-cast
            -Wcast-qual
            -Wcast-align
            -Wsign-compare
            -Wstrict-prototypes
            -Wredundant-decls
            -Wnested-externs
            -Winvalid-pch
            -Wvariadic-macros
            -Wvolatile-register-var
            -Wmissing-prototypes
            -Wsign-conversion
            -Wswitch-default
            -Werror
            -Wall
            -Wextra
        >

        # MSVC
        $<$<C_COMPILER_ID:MSVC>:
            /Wall
        >
)

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
   set(DEBUG_BUILD ON CACHE BOOL "build debug release")
   target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
   target_compile_options(${PROJECT_NAME}
       PRIVATE -g -ggdb -O0 -Wdisabled-optimization
   )
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O2)
endif()

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT binary
)
