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

juce_add_plugin(a2_vst3_latency_fixture
    COMPANY_NAME AudioNLEPrototype
    PLUGIN_MANUFACTURER_CODE Anle
    PLUGIN_CODE Ltcy
    VERSION 0.1.0
    VST3_CAN_REPLACE_VST2 FALSE
    FORMATS VST3
    PRODUCT_NAME "A2 Latency Fixture"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(a2_vst3_latency_fixture PRIVATE src/vst3_latency_fixture_plugin.cpp)
target_compile_features(a2_vst3_latency_fixture PRIVATE cxx_std_20)
target_link_libraries(a2_vst3_latency_fixture PRIVATE juce::juce_audio_utils)
target_compile_definitions(a2_vst3_latency_fixture PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

add_executable(tracktion_selective_reuse_vst3_pdc
    src/vst3_pdc_main.cpp
)
add_dependencies(tracktion_selective_reuse_vst3_pdc a2_vst3_latency_fixture_VST3)


target_compile_features(tracktion_selective_reuse_a2_1 PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_a2_1 PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_a2_2 PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_a2_2 PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_a2_3 PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_a2_3 PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_pdc PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_pdc PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_selective_reuse_vst3_pdc PRIVATE cxx_std_20)
target_link_libraries(tracktion_selective_reuse_vst3_pdc PRIVATE tracktion::tracktion_engine)
target_compile_definitions(tracktion_selective_reuse_vst3_pdc PRIVATE
    JUCE_PLUGINHOST_VST3=1
    A2_VST3_FIXTURE_PATH="$<TARGET_FILE:a2_vst3_latency_fixture_VST3>"
)

if(MSVC)
    target_compile_options(tracktion_selective_reuse_a2_1 PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_a2_2 PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_a2_3 PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_pdc PRIVATE /utf-8)
    target_compile_options(tracktion_selective_reuse_vst3_pdc PRIVATE /utf-8)
endif()

add_test(NAME tracktion_selective_reuse_a2_1_custom_source
    COMMAND tracktion_selective_reuse_a2_1)

add_test(NAME tracktion_selective_reuse_a2_2_processing
    COMMAND tracktion_selective_reuse_a2_2)

add_test(NAME tracktion_selective_reuse_a2_3_mixed_rate
    COMMAND tracktion_selective_reuse_a2_3)

add_test(NAME tracktion_selective_reuse_pdc
    COMMAND tracktion_selective_reuse_pdc)

add_test(NAME tracktion_selective_reuse_vst3_latency_diagnostic
    COMMAND tracktion_selective_reuse_vst3_pdc)
