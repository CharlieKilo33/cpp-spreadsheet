if(EXISTS "/home/user/projects/yandex/build/antlr4_runtime/runtime/antlr4_tests[1]_tests.cmake")
  include("/home/user/projects/yandex/build/antlr4_runtime/runtime/antlr4_tests[1]_tests.cmake")
else()
  add_test(antlr4_tests_NOT_BUILT antlr4_tests_NOT_BUILT)
endif()
