#############################################################################
# Author:      Alexandre CARPENTIER
# Modified by:
# Created:     27/04/2026
# Copyright:   (c) Alexandre CARPENTIER
# Licence:     LGPL-2.1-or-later
#############################################################################
#go modern
set(CMAKE_CXX_STANDARD 23)

# main application executable project name
project(doTest)
message("[*] Building ${PROJECT_NAME}")

message("[*] Add the google tests framework")
enable_testing()

# GoogleTest is already available from cppDaq - no need to fetch again
if(NOT TARGET gtest_main)
    message(FATAL_ERROR "GoogleTest targets not found. Ensure rotation-backend is processed before rotation-test.")
endif()

#set include directories for production source
include_directories(${PROJECT_SOURCE_DIR}../qtClient/src/)
include_directories(${PROJECT_SOURCE_DIR}../cppServer/src/)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_executable(doTest
	src/doTest.cpp
)

# Link against cppServer-static which provides cppDaqStatic transitively
target_link_libraries(doTest PRIVATE 
	gtest_main 
	cppServer-static
)

add_executable(doSimulation
	src/doSimulation.cpp
)

# Link against cppServer-static which provides cppDaqStatic transitively
target_link_libraries(doSimulation PRIVATE 
	cppServer-static
)

# Add google test
include(GoogleTest)
#gtest_discover_tests(doTest DISCOVERY_TIMEOUT 10 ) # Timeout for unit tests (défaut: 5))

message("[*] ${PROJECT_NAME} Built.")