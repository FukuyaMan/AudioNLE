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
