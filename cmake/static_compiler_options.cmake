function(static_target_compile_settings target)
    #
    # We enable exception catching for test/local example programs..
    #
    if (EMSCRIPTEN)
        target_compile_options(${target} PUBLIC -s DISABLE_EXCEPTION_CATCHING=0)
    endif()
endfunction()

function(static_target_link_settings target)
    #
    # We enable exception catching for test/local example programs..
    #
    if (EMSCRIPTEN)
        target_link_options(${target} PUBLIC -s DISABLE_EXCEPTION_CATCHING=0)
    endif()
endfunction()
