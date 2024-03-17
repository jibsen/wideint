cmake_minimum_required(VERSION 3.19)

project(wideint CXX)

include(CTest)

if(MSVC)
  add_compile_options(/W3)
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -pedantic)
endif()

add_library(wideint INTERFACE wideint.hpp)
target_include_directories(wideint INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)
target_compile_features(wideint INTERFACE cxx_std_20)

if(BUILD_TESTING)
  add_executable(test_wideint test/test_main.cpp test/test_wuint.cpp test/test_wint.cpp)
  target_link_libraries(test_wideint PRIVATE wideint)

  add_test(test_wideint test_wideint)
endif()
