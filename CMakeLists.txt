cmake_minimum_required(VERSION 3.20)

project(AudioNLEPrototypeBootstrap LANGUAGES C CXX)

enable_testing()

if(MSVC)
  add_compile_options(/utf-8)
endif()

include(FetchContent)
FetchContent_Declare(audionle_juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 37c894f83d379179b2070d437ccd0f1cd9af9576)
FetchContent_MakeAvailable(audionle_juce)

add_subdirectory(prototype/option-b-low-level-graph)
