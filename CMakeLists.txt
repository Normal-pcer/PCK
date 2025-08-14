cmake_minimum_required(VERSION 3.24)

project(PCK)          # 项目名称

set(CMAKE_CXX_STANDARD 26)

add_executable(main main.cpp player.cpp game.cpp card.cpp)
