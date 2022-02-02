cmake_minimum_required(VERSION 3.21)

project(wideint CXX)

include(CTest)

add_library(wideint INTERFACE wideint.hpp)
target_include_directories(wideint INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)
set_target_properties(wideint PROPERTIES CXX_STANDARD 20)

if(BUILD_TESTING)
  add_executable(test_wideint test/test_main.cpp test/test_wuint.cpp test/test_wint.cpp)
  target_link_libraries(test_wideint PRIVATE wideint)
  set_target_properties(test_wideint PROPERTIES CXX_STANDARD 20)

  add_test(test_wideint test_wideint)
endif()
