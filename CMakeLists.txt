cmake_minimum_required(VERSION 2.6)

project(twemproxy_sentinel)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${twemproxy_sentinel_SOURCE_DIR}/cmake")

file(GLOB_RECURSE SOURCES src/*.c src/*.h)

add_executable (twemproxy_sentinel ${SOURCES})

find_package(Syslog REQUIRED)
target_link_libraries(twemproxy_sentinel ${SYSLOG_LIBRARIES})
include_directories(${SYSLOG_INCLUDE_DIR})

target_link_libraries(twemproxy_sentinel "${twemproxy_sentinel_SOURCE_DIR}/contrib/yaml-0.1.4/src/.libs/libyaml.a")
include_directories("${twemproxy_sentinel_SOURCE_DIR}/contrib/yaml-0.1.4/include/")

target_link_libraries(twemproxy_sentinel "${twemproxy_sentinel_SOURCE_DIR}/contrib/hiredis/libhiredis.a")
include_directories("${twemproxy_sentinel_SOURCE_DIR}/contrib/hiredis/")

target_link_libraries(twemproxy_sentinel "${twemproxy_sentinel_SOURCE_DIR}/contrib/libevent-2.0.22-stable/.libs/libevent_core.a")
include_directories("${twemproxy_sentinel_SOURCE_DIR}/contrib/libevent-2.0.22-stable/include/")

install(TARGETS twemproxy_sentinel DESTINATION build)


#testing

#file(GLOB_RECURSE TEST_SOURCES test/*.c test/*.h)

#add_executable (testing ${TEST_SOURCES})

#install(TARGETS testing DESTINATION build)
