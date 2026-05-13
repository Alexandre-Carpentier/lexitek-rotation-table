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
project(qt-rotation-table)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_subdirectory(rotation-frontend)
add_subdirectory(rotation-backend)
add_subdirectory(rotation-test)
