set(TE_ADD_EXAMPLES OFF CACHE BOOL "Disable Tracktion example targets" FORCE)

add_subdirectory(third_party/tracktion_engine EXCLUDE_FROM_ALL)

add_executable(tracktion_feasibility_headless
    src/phase_a_main.cpp
)

add_executable(tracktion_feasibility_phase_b
    src/phase_b_main.cpp
)

add_executable(tracktion_feasibility_phase_c
    src/phase_c_main.cpp
)

add_executable(tracktion_feasibility_phase_d
    src/phase_d_main.cpp
)

add_executable(tracktion_feasibility_phase_e
    src/phase_e_main.cpp
)

add_executable(tf_e_diag
    src/phase_e_renderer_diagnostic_main.cpp
)

target_compile_features(tracktion_feasibility_headless PRIVATE cxx_std_20)
target_link_libraries(tracktion_feasibility_headless PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_feasibility_phase_b PRIVATE cxx_std_20)
target_link_libraries(tracktion_feasibility_phase_b PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_feasibility_phase_c PRIVATE cxx_std_20)
target_link_libraries(tracktion_feasibility_phase_c PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_feasibility_phase_d PRIVATE cxx_std_20)
target_link_libraries(tracktion_feasibility_phase_d PRIVATE tracktion::tracktion_engine)
target_compile_features(tracktion_feasibility_phase_e PRIVATE cxx_std_20)
target_link_libraries(tracktion_feasibility_phase_e PRIVATE tracktion::tracktion_engine)
target_compile_features(tf_e_diag PRIVATE cxx_std_20)
target_link_libraries(tf_e_diag PRIVATE tracktion::tracktion_engine)

if(MSVC)
    target_compile_options(tracktion_feasibility_headless PRIVATE /utf-8)
    target_compile_options(tracktion_feasibility_phase_b PRIVATE /utf-8)
    target_compile_options(tracktion_feasibility_phase_c PRIVATE /utf-8)
    target_compile_options(tracktion_feasibility_phase_d PRIVATE /utf-8)
    target_compile_options(tracktion_feasibility_phase_e PRIVATE /utf-8)
    target_compile_options(tf_e_diag PRIVATE /utf-8)
endif()

add_test(NAME tracktion_feasibility_phase_a_reconstruction
    COMMAND tracktion_feasibility_headless)

add_test(NAME tracktion_feasibility_phase_b_integer_timeline
    COMMAND tracktion_feasibility_phase_b)

add_test(NAME tracktion_feasibility_phase_c_signal_flow
    COMMAND tracktion_feasibility_phase_c)

add_test(NAME tracktion_feasibility_phase_d_processing_stack
    COMMAND tracktion_feasibility_phase_d)

add_test(NAME tracktion_feasibility_phase_e_pdc
    COMMAND tracktion_feasibility_phase_e)

add_test(NAME tracktion_feasibility_phase_e_renderer_diagnostic
    COMMAND tf_e_diag)
