cmake_minimum_required(VERSION 3.20)

project(AudioNLEPrototypeBootstrap LANGUAGES C CXX)

enable_testing()

add_subdirectory(prototype/tracktion-feasibility)
add_subdirectory(prototype/tracktion-selective-reuse)
