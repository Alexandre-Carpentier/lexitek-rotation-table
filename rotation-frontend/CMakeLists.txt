#########################################################################
# go modern
set(CMAKE_CXX_STANDARD 23)

#########################################################################
# Front end project
project(rotation-frontend)

#########################################################################
# Packages QT and gRPC
find_package(Qt6 COMPONENTS Widgets)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

#########################################################################
# gRPC configuration
set(PROTO_FILES ${CMAKE_SOURCE_DIR}/proto/MSG_Position.proto)
set(PROTO_IMPORT_DIR ${CMAKE_SOURCE_DIR}/proto)

add_library(protobuf_lib_frontend ${PROTO_FILES})
target_include_directories(protobuf_lib_frontend PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(protobuf_lib_frontend gRPC::grpc++)
protobuf_generate(
  TARGET protobuf_lib_frontend
  LANGUAGE cpp
  IMPORT_DIRS ${PROTO_IMPORT_DIR}
)

get_property(grpc_cpp_plugin_location
  TARGET gRPC::grpc_cpp_plugin
  PROPERTY LOCATION			
)

protobuf_generate(
  TARGET protobuf_lib_frontend
  LANGUAGE grpc	
  IMPORT_DIRS ${PROTO_IMPORT_DIR}
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)

#########################################################################
# QT configuration
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#########################################################################
# EXECUTABLE
add_executable(rotation-frontend WIN32 src/main.cpp
	src/mainwindow.h 
	src/mainwindow.cpp
	src/frontend_rpc.h 

	src/imgs/opticalHeader.qrc
	src/shell.h 
	)

target_link_libraries(rotation-frontend PRIVATE Qt6::Widgets Qt6::Core)
target_link_libraries(rotation-frontend PRIVATE protobuf_lib_frontend)
message("[*] Building (rotation-frontend executable...\n")