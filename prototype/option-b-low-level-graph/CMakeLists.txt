add_library(audionle_engine_source_runtime STATIC
  src/source_runtime_engine.cpp
  src/ffmpeg_native_source_provider.cpp
  src/prepared_src_disk_residency.cpp)
find_package(FFMPEG REQUIRED)
target_compile_features(audionle_engine_source_runtime PUBLIC cxx_std_20)
target_include_directories(audionle_engine_source_runtime PRIVATE ${FFMPEG_INCLUDE_DIRS})
if(MSVC)
  target_compile_options(audionle_engine_source_runtime PRIVATE /utf-8)
endif()
target_compile_definitions(audionle_engine_source_runtime PRIVATE AUDIONLE_PREPARED_RESIDENCY_ENGINE AUDIONLE_PREPARED_RESIDENCY_LIBRARY)
target_link_libraries(audionle_engine_source_runtime PRIVATE samplerate ${FFMPEG_LIBRARIES})

add_executable(option_b_ffmpeg_link_probe src/ffmpeg_link_probe.cpp)
target_compile_features(option_b_ffmpeg_link_probe PRIVATE cxx_std_20)
target_include_directories(option_b_ffmpeg_link_probe PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(option_b_ffmpeg_link_probe PRIVATE ${FFMPEG_LIBRARIES})
if(MSVC)
  target_compile_options(option_b_ffmpeg_link_probe PRIVATE /utf-8)
endif()
add_test(NAME option_b_ffmpeg_link_probe COMMAND option_b_ffmpeg_link_probe)

add_executable(option_b_ffmpeg_native_source src/ffmpeg_native_source_provider_test.cpp)
target_compile_features(option_b_ffmpeg_native_source PRIVATE cxx_std_20)
# Test-only fixture mux/encode uses the same pinned vcpkg FFmpeg libraries as
# the provider.  No machine-global ffmpeg CLI is part of this target.
target_include_directories(option_b_ffmpeg_native_source PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(option_b_ffmpeg_native_source PRIVATE audionle_engine_source_runtime ${FFMPEG_LIBRARIES})
if(MSVC)
  target_compile_options(option_b_ffmpeg_native_source PRIVATE /utf-8)
endif()
add_test(NAME option_b_ffmpeg_native_source COMMAND option_b_ffmpeg_native_source)

add_executable(option_b_low_level_graph src/main.cpp)
target_compile_features(option_b_low_level_graph PRIVATE cxx_std_20)
target_link_libraries(option_b_low_level_graph PRIVATE audionle_engine_source_runtime)
add_test(NAME option_b_low_level_graph COMMAND option_b_low_level_graph)
if(MSVC)
  target_compile_options(option_b_low_level_graph PRIVATE /utf-8)
endif()

function(add_option_b_latency_fixture target code latency)
  juce_add_plugin(${target}
    COMPANY_NAME AudioNLEPrototype
    PLUGIN_MANUFACTURER_CODE Anle
    PLUGIN_CODE ${code}
    VERSION 0.1.0
    FORMATS VST3
    PRODUCT_NAME "Option B Latency ${latency}"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(${target} PRIVATE src/vst3_latency_fixture_plugin.cpp)
  target_link_libraries(${target} PRIVATE juce::juce_audio_utils)
  target_compile_definitions(${target} PRIVATE OB_LATENCY_SAMPLES=${latency} JUCE_VST3_CAN_REPLACE_VST2=0)
endfunction()
add_option_b_latency_fixture(option_b_latency_256 OB25 256)
add_option_b_latency_fixture(option_b_latency_1024 OB10 1024)
add_option_b_latency_fixture(option_b_latency_2048 OB20 2048)

add_executable(option_b_vst3_pdc src/vst3_main.cpp)
add_dependencies(option_b_vst3_pdc option_b_latency_256_VST3 option_b_latency_1024_VST3 option_b_latency_2048_VST3)
target_compile_features(option_b_vst3_pdc PRIVATE cxx_std_20)
target_link_libraries(option_b_vst3_pdc PRIVATE juce::juce_audio_processors_headless)
target_compile_definitions(option_b_vst3_pdc PRIVATE
  JUCE_PLUGINHOST_VST3=1
  OB_VST3_256="$<TARGET_FILE:option_b_latency_256_VST3>"
  OB_VST3_1024="$<TARGET_FILE:option_b_latency_1024_VST3>"
  OB_VST3_2048="$<TARGET_FILE:option_b_latency_2048_VST3>")
add_test(NAME option_b_vst3_pdc COMMAND option_b_vst3_pdc)

juce_add_plugin(option_b_tail_fixture
  COMPANY_NAME AudioNLEPrototype PLUGIN_MANUFACTURER_CODE Anle PLUGIN_CODE ObTl VERSION 0.1.0
  FORMATS VST3 PRODUCT_NAME "Option B Finite Tail" IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(option_b_tail_fixture PRIVATE src/vst3_tail_fixture_plugin.cpp)
target_link_libraries(option_b_tail_fixture PRIVATE juce::juce_audio_utils)
target_compile_definitions(option_b_tail_fixture PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)
add_executable(option_b_vst3_tail_probe src/vst3_tail_probe.cpp)
add_dependencies(option_b_vst3_tail_probe option_b_tail_fixture_VST3)
target_compile_features(option_b_vst3_tail_probe PRIVATE cxx_std_20)
target_link_libraries(option_b_vst3_tail_probe PRIVATE juce::juce_audio_processors_headless)
target_compile_definitions(option_b_vst3_tail_probe PRIVATE JUCE_PLUGINHOST_VST3=1 OB_TAIL_VST3="$<TARGET_FILE:option_b_tail_fixture_VST3>")
add_test(NAME option_b_vst3_tail COMMAND option_b_vst3_tail_probe)

add_executable(option_b_realtime_source_handoff src/realtime_source_handoff.cpp)
target_compile_features(option_b_realtime_source_handoff PRIVATE cxx_std_20)
target_link_libraries(option_b_realtime_source_handoff PRIVATE juce::juce_audio_formats)
if(WIN32)
  target_link_libraries(option_b_realtime_source_handoff PRIVATE Psapi)
endif()
add_test(NAME option_b_realtime_source_handoff COMMAND option_b_realtime_source_handoff)

add_executable(option_b_multisource_arbitration src/multisource_arbitration.cpp)
target_compile_features(option_b_multisource_arbitration PRIVATE cxx_std_20)
target_link_libraries(option_b_multisource_arbitration PRIVATE juce::juce_audio_formats)
if(WIN32)
  target_link_libraries(option_b_multisource_arbitration PRIVATE Psapi)
endif()
add_test(NAME option_b_multisource_arbitration COMMAND option_b_multisource_arbitration)

add_executable(option_b_mixed_rate_src src/mixed_rate_src_s4_s12.cpp)
target_compile_features(option_b_mixed_rate_src PRIVATE cxx_std_20)
target_link_libraries(option_b_mixed_rate_src PRIVATE juce::juce_audio_formats)
add_test(NAME option_b_mixed_rate_src COMMAND option_b_mixed_rate_src)

add_executable(option_b_mixed_rate_src_phase_classes src/mixed_rate_src_phase_classes.cpp)
target_compile_features(option_b_mixed_rate_src_phase_classes PRIVATE cxx_std_20)
target_link_libraries(option_b_mixed_rate_src_phase_classes PRIVATE juce::juce_audio_basics)
add_test(NAME option_b_mixed_rate_src_phase_classes COMMAND option_b_mixed_rate_src_phase_classes)

add_executable(option_b_mixed_rate_src_identity_trace src/mixed_rate_src_identity_trace.cpp)
target_compile_features(option_b_mixed_rate_src_identity_trace PRIVATE cxx_std_20)
target_link_libraries(option_b_mixed_rate_src_identity_trace PRIVATE juce::juce_audio_basics)
add_test(NAME option_b_mixed_rate_src_identity_trace COMMAND option_b_mixed_rate_src_identity_trace)

target_sources(option_b_mixed_rate_src_identity_trace PRIVATE src/mixed_rate_src_worker_cache_identity.cpp)

add_executable(option_b_mixed_rate_src_integer_zoh src/mixed_rate_src_integer_zoh.cpp)
target_compile_features(option_b_mixed_rate_src_integer_zoh PRIVATE cxx_std_20)
add_test(NAME option_b_mixed_rate_src_integer_zoh COMMAND option_b_mixed_rate_src_integer_zoh)

# Prototype-only libsamplerate comparator; no production target consumes it.
include(FetchContent)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(libsamplerate
  GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
  GIT_TAG b9c20b93660c3683fda12e3c2a01f0021bf96c56)
FetchContent_MakeAvailable(libsamplerate)

# Comparator-only SpeexDSP 1.2.1: a single C source is built with its documented
# OUTSIDE_SPEEX prefix, avoiding an autotools dependency or a production link.
FetchContent_Declare(speexdsp
  GIT_REPOSITORY https://github.com/xiph/speexdsp.git
  GIT_TAG e762196693c487249d7061081915c8eaf7511f4f)
cmake_policy(PUSH)
cmake_policy(SET CMP0169 OLD)
FetchContent_GetProperties(speexdsp)
if(NOT speexdsp_POPULATED)
  FetchContent_Populate(speexdsp)
endif()
cmake_policy(POP)
add_library(option_b_speexdsp STATIC ${speexdsp_SOURCE_DIR}/libspeexdsp/resample.c)
target_include_directories(option_b_speexdsp PUBLIC ${speexdsp_SOURCE_DIR}/include/speex ${speexdsp_SOURCE_DIR}/libspeexdsp)
target_compile_definitions(option_b_speexdsp PRIVATE OUTSIDE_SPEEX RANDOM_PREFIX=audionle_speex FLOATING_POINT)
add_executable(option_b_speexdsp_comparator src/speexdsp_comparator.cpp)
target_compile_features(option_b_speexdsp_comparator PRIVATE cxx_std_20)
target_compile_definitions(option_b_speexdsp_comparator PRIVATE OUTSIDE_SPEEX RANDOM_PREFIX=audionle_speex)
target_link_libraries(option_b_speexdsp_comparator PRIVATE option_b_speexdsp)
add_test(NAME option_b_speexdsp_comparator COMMAND option_b_speexdsp_comparator)

# Comparator-only libsoxr 0.1.3.  This deliberately consumes the pinned source
# directly instead of importing libsoxr's top-level CMake project: no package
# lookup, install rule, shared library, or upstream test is in this build graph.
# 6e80... is the annotated 0.1.3 tag; its peeled source commit is 945b592....
FetchContent_Declare(libsoxr_source
  GIT_REPOSITORY https://github.com/chirlu/soxr.git
  GIT_TAG 6e80f8b597c92afd3edf7e714a3b8d7b4534213d)
cmake_policy(PUSH)
cmake_policy(SET CMP0169 OLD)
FetchContent_GetProperties(libsoxr_source)
if(NOT libsoxr_source_POPULATED)
  FetchContent_Populate(libsoxr_source)
endif()
cmake_policy(POP)
set(AVCODEC_FOUND 0)
set(AVUTIL_FOUND 0)
set(WITH_PFFFT 1)
set(HAVE_FENV_H 1)
set(HAVE_STDBOOL_H 1)
set(HAVE_STDINT_H 1)
set(HAVE_LRINT 1)
set(HAVE_BIGENDIAN 0)
set(WITH_CR32 1)
set(WITH_CR32S 1)
set(WITH_CR64 1)
set(WITH_CR64S 1)
set(WITH_VR32 1)
set(WITH_HI_PREC_CLOCK 1)
set(WITH_FLOAT_STD_PREC_CLOCK 0)
set(WITH_DEV_TRACE 0)
set(option_b_soxr_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/libsoxr")
file(MAKE_DIRECTORY "${option_b_soxr_generated_dir}")
configure_file("${libsoxr_source_SOURCE_DIR}/soxr-config.h.in"
  "${option_b_soxr_generated_dir}/soxr-config.h" @ONLY)
add_library(option_b_soxr_static STATIC
  ${libsoxr_source_SOURCE_DIR}/src/soxr.c
  ${libsoxr_source_SOURCE_DIR}/src/data-io.c
  ${libsoxr_source_SOURCE_DIR}/src/dbesi0.c
  ${libsoxr_source_SOURCE_DIR}/src/filter.c
  ${libsoxr_source_SOURCE_DIR}/src/fft4g64.c
  ${libsoxr_source_SOURCE_DIR}/src/cr.c
  ${libsoxr_source_SOURCE_DIR}/src/cr32.c
  ${libsoxr_source_SOURCE_DIR}/src/fft4g32.c
  ${libsoxr_source_SOURCE_DIR}/src/cr64.c
  ${libsoxr_source_SOURCE_DIR}/src/vr32.c
  ${libsoxr_source_SOURCE_DIR}/src/cr32s.c
  ${libsoxr_source_SOURCE_DIR}/src/pffft32s.c
  ${libsoxr_source_SOURCE_DIR}/src/util32s.c
  ${libsoxr_source_SOURCE_DIR}/src/cr64s.c
  ${libsoxr_source_SOURCE_DIR}/src/pffft64s.c
  ${libsoxr_source_SOURCE_DIR}/src/util64s.c)
target_include_directories(option_b_soxr_static PUBLIC ${libsoxr_source_SOURCE_DIR}/src PRIVATE ${option_b_soxr_generated_dir})
target_compile_definitions(option_b_soxr_static PRIVATE SOXR_LIB _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
if(MSVC)
  target_compile_options(option_b_soxr_static PRIVATE /utf-8)
endif()
add_executable(option_b_libsoxr_comparator src/libsoxr_comparator.cpp)
target_compile_features(option_b_libsoxr_comparator PRIVATE cxx_std_20)
target_link_libraries(option_b_libsoxr_comparator PRIVATE option_b_soxr_static)
add_test(NAME option_b_libsoxr_comparator COMMAND option_b_libsoxr_comparator)
add_executable(option_b_libsamplerate_timing_contract src/libsamplerate_timing_contract.cpp)
target_compile_features(option_b_libsamplerate_timing_contract PRIVATE cxx_std_20)
target_link_libraries(option_b_libsamplerate_timing_contract PRIVATE samplerate)
add_test(NAME option_b_libsamplerate_timing_contract COMMAND option_b_libsamplerate_timing_contract)
add_executable(option_b_libsamplerate_mode_history src/libsamplerate_mode_history.cpp)
target_compile_features(option_b_libsamplerate_mode_history PRIVATE cxx_std_20)
target_link_libraries(option_b_libsamplerate_mode_history PRIVATE samplerate)
add_test(NAME option_b_libsamplerate_mode_history COMMAND option_b_libsamplerate_mode_history)
add_executable(option_b_prepared_src_policy src/prepared_src_policy.cpp)
target_compile_features(option_b_prepared_src_policy PRIVATE cxx_std_20)
add_test(NAME option_b_prepared_src_policy COMMAND option_b_prepared_src_policy)
add_executable(option_b_prepared_src_cache src/prepared_src_cache.cpp)
target_compile_features(option_b_prepared_src_cache PRIVATE cxx_std_20)
target_link_libraries(option_b_prepared_src_cache PRIVATE samplerate)
add_test(NAME option_b_prepared_src_cache COMMAND option_b_prepared_src_cache)
add_executable(option_b_prepared_src_real_media src/prepared_src_real_media.cpp)
target_compile_features(option_b_prepared_src_real_media PRIVATE cxx_std_20)
target_link_libraries(option_b_prepared_src_real_media PRIVATE samplerate juce::juce_audio_formats)
if(WIN32)
  target_link_libraries(option_b_prepared_src_real_media PRIVATE Psapi)
endif()
add_test(NAME option_b_prepared_src_real_media COMMAND option_b_prepared_src_real_media)
add_executable(option_b_prepared_src_disk_residency src/prepared_src_disk_residency.cpp)
target_compile_features(option_b_prepared_src_disk_residency PRIVATE cxx_std_20)
add_test(NAME option_b_prepared_src_disk_residency COMMAND option_b_prepared_src_disk_residency)
add_test(NAME option_b_prepared_src_eviction COMMAND option_b_prepared_src_disk_residency)
add_test(NAME option_b_prepared_src_reopen COMMAND option_b_prepared_src_disk_residency)
add_test(NAME option_b_prepared_src_realtime COMMAND option_b_prepared_src_disk_residency)
add_executable(option_b_source_runtime_routes src/source_runtime_routes.cpp)
target_compile_features(option_b_source_runtime_routes PRIVATE cxx_std_20)
target_link_libraries(option_b_source_runtime_routes PRIVATE audionle_engine_source_runtime)
add_test(NAME option_b_source_runtime_routes COMMAND option_b_source_runtime_routes)
add_test(NAME option_b_source_runtime_lifecycle COMMAND option_b_source_runtime_routes)
add_test(NAME option_b_source_runtime_mixed_graph COMMAND option_b_source_runtime_routes)
add_test(NAME option_b_source_runtime_shared_ownership COMMAND option_b_source_runtime_routes)
add_executable(option_b_source_runtime_stereo src/source_runtime_stereo.cpp)
target_compile_features(option_b_source_runtime_stereo PRIVATE cxx_std_20)
target_link_libraries(option_b_source_runtime_stereo PRIVATE audionle_engine_source_runtime)
add_test(NAME option_b_source_runtime_stereo COMMAND option_b_source_runtime_stereo)
add_executable(option_b_production_source_runtime_validation src/production_source_runtime_validation.cpp)
target_compile_features(option_b_production_source_runtime_validation PRIVATE cxx_std_20)
target_link_libraries(option_b_production_source_runtime_validation PRIVATE audionle_engine_source_runtime)
add_test(NAME option_b_production_source_runtime_validation COMMAND option_b_production_source_runtime_validation)
add_executable(option_b_physical_src_range_planner src/source_runtime_planner_service.cpp)
target_compile_features(option_b_physical_src_range_planner PRIVATE cxx_std_20)
add_test(NAME option_b_physical_src_range_planner COMMAND option_b_physical_src_range_planner)
add_test(NAME option_b_native_source_service COMMAND option_b_physical_src_range_planner)

# r8brain is header-only.  The comparator consumes the pinned source directly
# and keeps all float/double staging adapter-owned and fixed-size.
FetchContent_Declare(r8brain_source
  GIT_REPOSITORY https://github.com/avaneev/r8brain-free-src.git
  GIT_TAG 9e73d2dd59fd5b95108fdb4f590083e35758b45f)
cmake_policy(PUSH)
cmake_policy(SET CMP0169 OLD)
FetchContent_GetProperties(r8brain_source)
if(NOT r8brain_source_POPULATED)
  FetchContent_Populate(r8brain_source)
endif()
cmake_policy(POP)
add_executable(option_b_r8brain_adapter src/r8brain_adapter.cpp)
target_compile_features(option_b_r8brain_adapter PRIVATE cxx_std_20)
target_include_directories(option_b_r8brain_adapter PRIVATE ${r8brain_source_SOURCE_DIR})
if(MSVC)
  target_compile_options(option_b_r8brain_adapter PRIVATE /utf-8)
endif()
add_test(NAME option_b_r8brain_adapter COMMAND option_b_r8brain_adapter)

# Engine-private production-boundary skeleton; it is not a production runtime target.
add_executable(option_b_production_src_component_skeleton src/production_src_component_skeleton.cpp)
target_compile_features(option_b_production_src_component_skeleton PRIVATE cxx_std_20)
target_link_libraries(option_b_production_src_component_skeleton PRIVATE samplerate)
add_test(NAME option_b_production_src_component_skeleton COMMAND option_b_production_src_component_skeleton)

add_executable(option_b_production_src_realtime_density src/production_src_realtime_density.cpp)
target_compile_features(option_b_production_src_realtime_density PRIVATE cxx_std_20)
target_link_libraries(option_b_production_src_realtime_density PRIVATE samplerate)
add_test(NAME option_b_production_src_realtime_density COMMAND option_b_production_src_realtime_density)

add_executable(option_b_production_src_cache_longform src/production_src_cache_longform.cpp)
target_compile_features(option_b_production_src_cache_longform PRIVATE cxx_std_20)
add_test(NAME option_b_production_src_cache_longform COMMAND option_b_production_src_cache_longform)

add_executable(option_b_windows_iat_crt_harness src/windows_iat_crt_harness.cpp)
target_compile_features(option_b_windows_iat_crt_harness PRIVATE cxx_std_20)
target_link_libraries(option_b_windows_iat_crt_harness PRIVATE audionle_engine_source_runtime samplerate)
add_test(NAME option_b_windows_iat_crt_harness COMMAND option_b_windows_iat_crt_harness)
