cmake_minimum_required(VERSION 2.8)
set(CMAKE_INSTALL_PREFIX $ENV{HOME})
project(FbyNetCPP)
add_definitions(-std=c++11 -Wall -Wextra -Weffc++ -g)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

LINK_DIRECTORIES( ${LINK_DIRECTORIES} $ENV{HOME}/lib)
include_directories($ENV{HOME}/include)

add_executable(irrigationcontrol_daemon irrigationcontrol_daemon.cpp irrigation.cpp)
target_link_libraries (irrigationcontrol_daemon LINK_PUBLIC fby sqlite3 pq boost_filesystem boost_system gdbm pcre boost_iostreams boost_program_options cgicc ftdi /home/danlyke/lib/libgtest.a /home/danlyke/lib/libgtest_main.a pthread)

add_executable(relayboardtest relayboardtest.cpp irrigation.cpp)
target_link_libraries (relayboardtest LINK_PUBLIC fby sqlite3 pq boost_filesystem boost_system gdbm pcre boost_iostreams boost_program_options cgicc ftdi /home/danlyke/lib/libgtest.a /home/danlyke/lib/libgtest_main.a pthread)

