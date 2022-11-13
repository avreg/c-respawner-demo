# Debian 10 Buster
cmake_minimum_required(VERSION 3.13.4)

project (
   c-respawner-demo
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

add_subdirectory(include)
add_subdirectory(src)

include (CTest)
add_subdirectory (Unity)
add_subdirectory(test)
