function(add_compile_fail_test test_name source_file)
    add_library(${test_name} OBJECT ${source_file})
    target_link_libraries(${test_name} PRIVATE fsm)

    # do not build with "all"
    set_target_properties(${test_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)

    add_test(
        NAME ${test_name}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${test_name} --config $<CONFIG>
    )

    # compilation must fail -> test passes
    set_tests_properties(${test_name} PROPERTIES
        WILL_FAIL TRUE
        LABELS "compile_time"
    )
endfunction()

add_compile_fail_test(ct_dup_src_event              dup_src_event.cpp)
add_compile_fail_test(ct_dup_external_internal      dup_external_internal.cpp)
add_compile_fail_test(ct_default_plus_other         default_plus_other.cpp)
add_compile_fail_test(ct_two_defaults               two_defaults.cpp)
add_compile_fail_test(ct_missing_state_ctor         missing_state_ctor.cpp)
add_compile_fail_test(ct_default_plus_internal      default_plus_internal.cpp)
add_compile_fail_test(ct_state_not_derived          state_not_derived.cpp)
add_compile_fail_test(ct_initial_state_not_in_var   initial_state_not_in_variant.cpp)
add_compile_fail_test(ct_internal_action_bad_sig    internal_action_bad_sig.cpp)
add_compile_fail_test(ct_guard_bad_sig              guard_bad_sig.cpp)
add_compile_fail_test(ct_action_bad_sig             action_bad_sig.cpp)
add_compile_fail_test(ct_default_action_bad_sig     default_action_bad_sig.cpp)
add_compile_fail_test(dst_not_in_variant            dst_not_in_variant.cpp)
