add_executable(option_b_low_level_graph src/main.cpp)
target_compile_features(option_b_low_level_graph PRIVATE cxx_std_20)
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
add_executable(option_b_libsamplerate_timing_contract src/libsamplerate_timing_contract.cpp)
target_compile_features(option_b_libsamplerate_timing_contract PRIVATE cxx_std_20)
target_link_libraries(option_b_libsamplerate_timing_contract PRIVATE samplerate)
add_test(NAME option_b_libsamplerate_timing_contract COMMAND option_b_libsamplerate_timing_contract)
