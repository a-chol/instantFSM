cmake_minimum_required(VERSION 2.8)

project(instantFSM-gtests)

set(GTEST_ROOT GTEST_ROOT-NOTFOUND CACHE PATH "gtest root directory")

if (WIN32)
  if (NOT GTEST_ROOT)
    message (FATAL_ERROR "GTEST_ROOT path not specified. Cannot build tests")
  endif (NOT GTEST_ROOT)
endif (WIN32)

find_package(GTest REQUIRED)

if (NOT GTEST_FOUND) 
  message (FATAL_ERROR "GTest could not be found")
endif (NOT GTEST_FOUND)

include_directories(
  ..
  ${GTEST_INCLUDE_DIRS}
)

enable_testing()

add_executable(gtest gtests.cpp)
add_executable(gtest-MultiTU multiTUA.cpp multiTUB.cpp)

if (UNIX)
  set(ADDITIONAL_LIBS "stdc++" "pthread")
endif (UNIX)

target_link_libraries(gtest ${GTEST_LIBRARIES} ${ADDITIONAL_LIBS})
target_link_libraries(gtest-MultiTU ${GTEST_LIBRARIES} ${ADDITIONAL_LIBS})

if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")
endif (UNIX)

GTEST_ADD_TESTS(gtest "" gtests.cpp)
GTEST_ADD_TESTS(gtest-MultiTU "" multiTUA.cpp multiTUB.cpp)
# add_test(gtestTest gtest)
