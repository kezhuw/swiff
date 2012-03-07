cmake_minimum_required(VERSION 2.8)

project(swiff)

enable_testing()

set(C_FLAGS
-std=c99
-Werror
-Wall
-Wextra
-Wno-unused-function
-Wno-unused-label
)

string(REPLACE ";" " " CMAKE_C_FLAGS "${C_FLAGS}")

set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(base)
