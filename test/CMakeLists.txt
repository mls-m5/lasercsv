

enable_testing()


function(test testname)
    add_executable(test-${testname} test-${testname}.cpp)

    target_include_directories(test-${testname} PRIVATE tests)

    target_link_libraries(test-${testname}
        lasercsv
        )

    add_test(NAME test-${testname}
        COMMAND test-${testname}
        )

endfunction()

test(1)
