# AddressSanitizer / UndefinedBehaviorSanitizer toggles.
# ThreadSanitizer is deliberately not wired here -- it's intended for targeted
# non-realtime/control-path test runs only (see docs/TESTING.md), not a blanket flag.

function(pw8_apply_sanitizers target)
    if(PW8_ENABLE_ASAN)
        target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address)
    endif()

    if(PW8_ENABLE_UBSAN)
        target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=undefined)
    endif()
endfunction()
