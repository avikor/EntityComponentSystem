cmake_minimum_required (VERSION 3.8)

project ("EntityComponentSystem")

add_subdirectory ("EntityComponentSystem")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  SET(CMAKE_CXX_FLAGS "/EHsc /permissive /W4 /w14640")
else()
	SET(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wpedantic")
endif()