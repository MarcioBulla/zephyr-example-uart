cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr)
project(zephyr-example-uart)

target_sources(app PRIVATE src/main.c)
