add_executable(tracktion_selective_reuse_a2_1
    src/a2_1_main.cpp
)

add_executable(tracktion_selective_reuse_a2_2
    src/a2_2_main.cpp
)

add_executable(tracktion_selective_reuse_a2_3
    src/a2_3_main.cpp
)

add_executable(tracktion_selective_reuse_pdc
    src/pdc_main.cpp
)

add_executable(tracktion_selective_reuse_tail
    src/tail_main.cpp
)

function(add_a2_vst3_latency_fixture target code latency)
    juce_add_plugin(${target}
        COMPANY_NAME AudioNLEPrototype
        PLUGIN_MANUFACTURER_CODE Anle
        PLUGIN_CODE ${code}
        VERSION 0.1.0
        VST3_CAN_REPLACE_VST2 FALSE
        FORMATS VST3
        PRODUCT_NAME "A2 Latency Fixture ${latency}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
    )
    target_sources(${target} PRIVATE src/vst3_latency_fixture_plugin.cpp)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_link_libraries(${target} PRIVATE juce::juce_audio_utils)
    target_compile_definitions(${target} PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0 A2_LATENCY_SAMPLES=${latency})
endfunction()

add_a2_vst3_latency_fixture(a2_vst3_latency_fixture_256 L256 256)
add_a2_vst3_latency_fixture(a2_vst3_latency_fixture_768 L768 768)
add_a2_vst3_latency_fixture(a2_vst3_latency_fixture_1024 L102 1024)
add_a2_vst3_latency_fixture(a2_vst3_latency_fixture_2048 L204 2048)

juce_add_plugin(a2_vst3_tail_fixture
    COMPANY_NAME AudioNLEPrototype
    PLUGIN_MANUFACTURER_CODE Anle
    PLUGIN_CODE Tail
    VERSION 0.1.0
    VST3_CAN_REPLACE_VST2 FALSE
    FORMATS VST3
    PRODUCT_NAME "A2 Tail Fixture"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)
target_sources(a2_vst3_tail_fixture PRIVATE src/vst3_tail_fixture_plugin.cpp)
target_compile_features(a2_vst3_tail_fixture PRIVATE cxx_std_20)
target_link_libraries(a2_vst3_tail_fixture PRIVATE juce::juce_audio_utils)
target_compile_definitions(a2_vst3_tail_fixture PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

add_executable(tracktion_selective_reuse_vst3_pdc
    src/vst3_actual_pdc_main.cpp
)
add_dependencies(tracktion_selective_reuse_vst3_pdc
    a2_vst3_latency_fixture_256_VST3
    a2_vst3_latency_fixture_768_VST3
    a2_vst3_latency_fixture_1024_VST3
    a2_vst3_latency_fixture_2048_VST3
)

add_executable(tracktion_selective_reuse_vst3_tail
    src/vst3_actual_tail_main.cpp
)
add_dependencies(tracktion_selective_reuse_vst3_tail a2_vst3_tail_fixture_VST3)

add_executable(tracktion_selective_reuse_real_media
    src/real_media_main.cpp
)

add_executable(tracktion_selective_reuse_realtime_source
    src/realtime_source_main.cpp
)

add_executable(tracktion_selective_reuse_multisource
    src/multisource_main.cpp
)



target_compile_features(tracktion_selective_reuse_a2_1 PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_a2_1 PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_a2_2 PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_a2_2 PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_a2_3 PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_a2_3 PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_pdc PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_pdc PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_tail PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_tail PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_vst3_pdc PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_vst3_pdc PRIVATE tracktion::tracktion_engine)
target_compile_definitions(tracktion_selective_reuse_vst3_pdc PRIVATE
    JUCE_PLUGINHOST_VST3=1
    A2_VST3_FIXTURE_256_PATH="$<TARGET_FILE:a2_vst3_latency_fixture_256_VST3>"
    A2_VST3_FIXTURE_768_PATH="$<TARGET_FILE:a2_vst3_latency_fixture_768_VST3>"
    A2_VST3_FIXTURE_1024_PATH="$<TARGET_FILE:a2_vst3_latency_fixture_1024_VST3>"
    A2_VST3_FIXTURE_2048_PATH="$<TARGET_FILE:a2_vst3_latency_fixture_2048_VST3>"
)
target_compile_features(tracktion_selective_reuse_vst3_tail PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_vst3_tail PRIVATE tracktion::tracktion_engine)
target_compile_definitions(tracktion_selective_reuse_vst3_tail PRIVATE
    JUCE_PLUGINHOST_VST3=1
    A2_VST3_TAIL_FIXTURE_PATH="$<TARGET_FILE:a2_vst3_tail_fixture_VST3>"
)
target_compile_features(tracktion_selective_reuse_real_media PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_real_media PRIVATE tracktion::tracktion_engine Psapi)
target_compile_features(tracktion_selective_reuse_realtime_source PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_realtime_source PRIVATE tracktion::tracktion_engine Psapi)
target_compile_features(tracktion_selective_reuse_multisource PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_multisource PRIVATE tracktion::tracktion_engine Psapi)

if(MSVC)
    target_compile_options(tracktion_selective_reuse_a2_1 PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_a2_2 PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_a2_3 PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_pdc PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_tail PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_vst3_pdc PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_vst3_tail PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_real_media PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_realtime_source PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_multisource PRIVATE /utf-8)
endif()

add_test(NAME tracktion_selective_reuse_a2_1_custom_source
    COMMAND tracktion_selective_reuse_a2_1)

add_test(NAME tracktion_selective_reuse_a2_2_processing
    COMMAND tracktion_selective_reuse_a2_2)

add_test(NAME tracktion_selective_reuse_a2_3_mixed_rate
    COMMAND tracktion_selective_reuse_a2_3)

add_test(NAME tracktion_selective_reuse_pdc
    COMMAND tracktion_selective_reuse_pdc)

add_test(NAME tracktion_selective_reuse_tail
    COMMAND tracktion_selective_reuse_tail)

add_test(NAME tracktion_selective_reuse_vst3_pdc
    COMMAND tracktion_selective_reuse_vst3_pdc)

add_test(NAME tracktion_selective_reuse_vst3_tail
    COMMAND tracktion_selective_reuse_vst3_tail)

add_test(NAME tracktion_selective_reuse_real_media
    COMMAND tracktion_selective_reuse_real_media)

add_test(NAME tracktion_selective_reuse_realtime_source
    COMMAND tracktion_selective_reuse_realtime_source)

add_test(NAME tracktion_selective_reuse_multisource
    COMMAND tracktion_selective_reuse_multisource)
