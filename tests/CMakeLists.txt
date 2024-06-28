fips_begin_sharedlib(test_basic cmdline)
    fips_files(test_basic.cpp test_data.h test_basic.x)
fips_end_sharedlib()

gtest_suite_begin(crTest)
    fips_files(test.cpp test_data.h test_basic.x)
    if (FIPS_MACOS)
        fips_libs(dl)
    endif()
    if (FIPS_LINUX)
        fips_libs(dl stdc++fs)
    endif()    
gtest_suite_end(crTest)

if (FIPS_LINUX OR FIPS_MACOS)
    # no-stack-protector as this interfere with our crash handling test
    set_target_properties(crTest PROPERTIES COMPILE_FLAGS "-fno-stack-protector")
endif()
