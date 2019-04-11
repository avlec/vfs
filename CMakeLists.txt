cmake_minimum_required(VERSION 3.13)
project(apps C)

set(CMAKE_C_STANDARD 11)

add_executable(apps apps/apps.c file/file.c file/file.h disk/disk.c disk/disk.h)
