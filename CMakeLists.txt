#########################################################################
### macos
# % brew install qt
# open qt designer to edit *.ui files:
# % /opt/homebrew/opt/qt/libexec/Designer.app/Contents/MacOS/Designer &
### or win32
# % vcpkg install qt 
# if needed add toolchain when running cmake: 
# % cmake -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ..
# open qt designer to edit *.ui files:

#########################################################################
# go modern
cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)

#########################################################################
# 
project(lexitek-rotation-table)

if(MSVC)
  #set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

  #add_compile_options("$<$<CONFIG:Release>:/O1>")
  #add_compile_definitions("$<$<CONFIG:Release>:NDEBUG>")

  #add_link_options(
  #  "$<$<CONFIG:Release>:/DEBUG:NONE;/OPT:REF;/OPT:ICF>"
  #)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(cppServer)
add_subdirectory(qtClient)
#add_subdirectory(lvClient)
add_subdirectory(tests)