cmake_minimum_required(VERSION 3.10)

# Project name and language
project(Sqliter C)

# Specify where to find header files
include_directories(include)

# Gather all source files in the src directory
file(GLOB SOURCES "src/*.c")

# Define the executable and its source files
add_executable(Sqliter ${SOURCES})