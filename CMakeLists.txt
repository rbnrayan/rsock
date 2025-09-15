cmake_minimum_required(VERSION 3.12)
project(rsock)

set (CMAKE_CXX_STANDARD 23)

add_library(rsock rsock.cpp rsock.h)
