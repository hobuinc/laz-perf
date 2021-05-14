function(emscripten_target_compile_settings target)
    #
    # We enable exception catching for test/local example programs..
    #
    set(STATIC_LIB_OPTIONS "-s DISABLE_EXCEPTION_CATCHING=0")
endfunction()
