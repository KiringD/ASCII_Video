cmake_minimum_required(VERSION 3.24)
project(ascii_video LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(ascii_video main.cpp video.cpp Video.h progressbar.cpp Progressbar.h)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
target_link_libraries( ascii_video ${OpenCV_LIBS} )
