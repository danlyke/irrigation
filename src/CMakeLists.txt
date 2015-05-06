cmake_minimum_required(VERSION 2.8)
set(CMAKE_INSTALL_PREFIX $ENV{HOME})
project(FbyNetCPP)
add_definitions(-std=c++11 -Wall -Wextra -Weffc++)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

LINK_DIRECTORIES( ${LINK_DIRECTORIES} $ENV{HOME}/lib)
include_directories($ENV{HOME}/include)

add_executable(example_irrigationcontrol example_irrigationcontrol.cpp irrigation.cpp)
target_link_libraries (example_irrigationcontrol LINK_PUBLIC fby sqlite3 pq boost_filesystem boost_system gdbm pcre boost_iostreams boost_program_options cgicc ftdi /home/danlyke/lib/libgtest.a /home/danlyke/lib/libgtest_main.a pthread)
