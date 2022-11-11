# Debian 10 Buster
cmake_minimum_required(VERSION 3.13.4)

project (
   respawner
   VERSION 0.1.0
   DESCRIPTION "C-example of respawner for any process"
   LANGUAGES C
)

include ( CMakeDetermineSystem )
include ( CMakeDetermineCCompiler )
include ( CheckIncludeFiles )
include ( CheckTypeSize )
include ( CheckFunctionExists )
include ( CheckSymbolExists )

set( CMAKE_REQUIRED_DEFINITIONS -D_POSIX_C_SOURCE=200112;
                                -D_GNU_SOURCE;
                                -D_XOPEN_SOURCE;
                                -DHAVE_CONFIG_H
)
add_definitions ( ${CMAKE_REQUIRED_DEFINITIONS} )

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
   set( DEBUG_BUILD ON CACHE BOOL "build debug release" )
   add_definitions( -DDEBUG )
   set( CMAKE_C_FLAGS "-Wdisabled-optimization" )
else()
   add_definitions( -DNDEBUG )
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -fomit-frame-pointer -Wall -pedantic -Wundef -Wpointer-arith -Wbad-function-cast -Wno-cast-qual -Wcast-align -Wno-write-strings -Wno-conversion -Wsign-compare -Waggregate-return -Wno-strict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winvalid-pch -Wvariadic-macros -Wvolatile-register-var  -Werror=implicit-function-declaration -Werror=missing-prototypes -Werror=int-to-pointer-cast -Werror=pointer-to-int-cast" )
if (CMAKE_COMPILER_IS_GNUCC )
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmaybe-uninitialized")
else()
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wuninitialized")
endif (CMAKE_COMPILER_IS_GNUCC)
set( CMAKE_C_FLAGS_DEBUG   "-g -ggdb -O0 -Werror=format -Wformat -Werror=format-security -D_FORTIFY_SOURCE=2" )

if ( DISABLE_DEPRECATED )
   set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations" )
   set ( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wno-deprecated-declarations" )
endif ( DISABLE_DEPRECATED )

check_include_files (sys/time.h HAVE_SYS_TIME_H)
check_include_files (sys/timerfd.h HAVE_SYS_TIMERFD_H)
check_include_files (sys/signalfd.h HAVE_SYS_SIGNALFD_H)
check_include_files (sys/prctl.h HAVE_SYS_PRCTRL_H)
check_include_files (sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files (sys/resource.h HAVE_SYS_EPOLL_H)
if ( HAVE_TIME_H AND HAVE_SYS_RESOURCE_H )
   #set(CMAKE_EXTRA_INCLUDE_FILES "sys/time.h" "sys/resource.h")
   check_function_exists( setrlimit HAVE_SETRLIMIT )
   #set(CMAKE_EXTRA_INCLUDE_FILES)
endif( HAVE_TIME_H AND HAVE_SYS_RESOURCE_H )
if ( HAVE_SYS_RESOURCE_H )
   check_function_exists( prctl HAVE_PRCTL )
endif( HAVE_SYS_RESOURCE_H )

message("CMAKE_REQUIRED_DEFINITIONS: "  ${CMAKE_REQUIRED_DEFINITIONS})

add_subdirectory(include)
add_subdirectory(src)

include (CTest)
add_subdirectory (Unity)
add_subdirectory(test)
