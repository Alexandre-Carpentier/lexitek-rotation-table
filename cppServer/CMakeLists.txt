#########################################################################
# CMake configuration for cppServer project
#

#########################################################################
# go modern
set(CMAKE_CXX_STANDARD 23)

#########################################################################
# Back end project
project (cppServer)

#########################################################################
# Address Sanitizer
option(ENABLE_ASAN "Enable Address Sanitizer" ON)
if(ENABLE_ASAN)
  add_compile_options(/fsanitize=address)
  add_link_options(/fsanitize=address)
endif()

#########################################################################
# Fetch external dependencies
include(FetchContent)

#########################################################################
# Fetch Lua source code
FetchContent_Declare(
  lua
  GIT_REPOSITORY https://github.com/lua/lua
  GIT_TAG v5.4.7
)
FetchContent_MakeAvailable(lua)

# Manually build Lua as a library since it doesn't have CMakeLists.txt
file(GLOB LUA_SOURCES 
  ${lua_SOURCE_DIR}/*.c
)
# Remove standalone executables and amalgamation file
list(REMOVE_ITEM LUA_SOURCES 
  ${lua_SOURCE_DIR}/lua.c 
  ${lua_SOURCE_DIR}/luac.c
  ${lua_SOURCE_DIR}/onelua.c
)

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})

# Define LUA_USE_WINDOWS for Windows build
if(WIN32)
  target_compile_definitions(lua PRIVATE LUA_USE_WINDOWS)
endif()

#########################################################################
# Fetch cppDaq source code
FetchContent_Declare(
  cppDaq
  GIT_REPOSITORY https://github.com/Alexandre-Carpentier/cppDaq
)

FetchContent_GetProperties(cppDaq)
if(NOT cppdaq_POPULATED)
  FetchContent_populate(cppDaq)
  
  # Patch the CMakeLists.txt to exclude test executables but keep GoogleTest
  file(READ ${cppdaq_SOURCE_DIR}/CMakeLists.txt CPPDAQ_CMAKE_CONTENT)
  string(REPLACE "add_subdirectory(test)" "# add_subdirectory(test)" CPPDAQ_CMAKE_CONTENT "${CPPDAQ_CMAKE_CONTENT}")
  file(WRITE ${cppdaq_SOURCE_DIR}/CMakeLists.txt "${CPPDAQ_CMAKE_CONTENT}")
  
  add_subdirectory(${cppdaq_SOURCE_DIR} ${cppdaq_BINARY_DIR})
  
  # Manually add GoogleTest from cppDaq's vendored copy
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_subdirectory(${cppdaq_SOURCE_DIR}/test/googletest ${CMAKE_BINARY_DIR}/googletest-build EXCLUDE_FROM_ALL)
endif()

#########################################################################
# Package gRPC

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

#########################################################################
# gRPC configuration
set(PROTO_FILES ${CMAKE_SOURCE_DIR}/proto/MSG_Position.proto)
set(PROTO_IMPORT_DIR ${CMAKE_SOURCE_DIR}/proto)

add_library(protobuf_lib_backend ${PROTO_FILES})
target_include_directories(protobuf_lib_backend PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(protobuf_lib_backend gRPC::grpc++)
protobuf_generate(
  TARGET protobuf_lib_backend
  LANGUAGE cpp
  IMPORT_DIRS ${PROTO_IMPORT_DIR}
)

get_property(grpc_cpp_plugin_location
  TARGET gRPC::grpc_cpp_plugin
  PROPERTY LOCATION
)

protobuf_generate(
  TARGET protobuf_lib_backend
  LANGUAGE grpc 
  IMPORT_DIRS ${PROTO_IMPORT_DIR}
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)

#########################################################################
# LIB
add_library(cppServer-static
	src/main.cpp
	src/rotation.h
	src/rotation.cpp
	src/backend_rpc.h

	src/hardwareControl.h
	src/timerControl.h
	src/fileControl.h
	src/automationControl.h

	src/colors.h	
)

target_include_directories(cppServer-static PUBLIC 
	${cppdaq_SOURCE_DIR}/app/src
)

target_include_directories(cppServer-static PUBLIC 
	${lua_SOURCE_DIR}
)

target_link_libraries(cppServer-static PUBLIC cppDaq)
target_link_libraries(cppServer-static PUBLIC lua)
target_link_libraries(cppServer-static PRIVATE protobuf_lib_backend)

#########################################################################
# EXECUTABLE
add_executable(cppServer
	src/main.cpp
	src/rotation.h
	src/rotation.cpp
	src/backend_rpc.h

	src/hardwareControl.h
	src/timerControl.h
	src/fileControl.h
	src/automationControl.h

	src/colors.h	
)

target_include_directories(cppServer PUBLIC 
	${cppdaq_SOURCE_DIR}/app/src
)

target_include_directories(cppServer PUBLIC 
	${lua_SOURCE_DIR}
)

target_link_libraries(cppServer PRIVATE cppDaqStatic lua)
target_link_libraries(cppServer PRIVATE protobuf_lib_backend)

message("[*] Building cppServer executable...\n")