cmake_minimum_required(VERSION 3.13)
project(disk C)

set(CMAKE_C_STANDARD 11)

add_executable(disk disk/disk.c disk/disk.h)

project(file)

set(CMAKE_C_STANDARD 11)

add_executable(file file/file.c file/file.h disk/disk.c disk/disk.h)

project(apps)

set(CMAKE_C_STANDARD 11)

add_executable(apps apps/apps.c file/file.h disk/disk.h)
