cmake_minimum_required(VERSION 3.16)
project(pypilot_mdns LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PYPILOT_MDNS_REQUIRE_AVAHI "Fail configure when Avahi is not available" OFF)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(AVAHI QUIET avahi-client avahi-common)
endif()

if(AVAHI_FOUND)
    add_library(pypilot_mdns src/pypilot_mdns_avahi.cpp)
    target_compile_definitions(pypilot_mdns PUBLIC PYPILOT_MDNS_HAVE_AVAHI=1)
    target_include_directories(pypilot_mdns PRIVATE ${AVAHI_INCLUDE_DIRS})
    target_link_libraries(pypilot_mdns PUBLIC ${AVAHI_LIBRARIES})
    target_compile_options(pypilot_mdns PRIVATE ${AVAHI_CFLAGS_OTHER})
else()
    if(PYPILOT_MDNS_REQUIRE_AVAHI)
        message(FATAL_ERROR "Avahi development libraries not found")
    endif()
    add_library(pypilot_mdns src/pypilot_mdns_null.cpp)
endif()

target_include_directories(pypilot_mdns PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(CTest)
enable_testing()

function(add_mdns_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE pypilot_mdns)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_mdns_test(test_mdns_types tests/test_mdns_types.cpp)
add_mdns_test(test_mdns_compile tests/test_mdns_compile.cpp)

add_executable(mdns_example examples/MdnsExample/MdnsExample.cpp)
target_link_libraries(mdns_example PRIVATE pypilot_mdns)
