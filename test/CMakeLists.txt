# File: `test/CMakeLists.txt`
# language: cmake
set(TEST_SRCS "test_distance.c")
idf_component_register(SRCS ${TEST_SRCS}
        INCLUDE_DIRS ".."    # allow #include `acebott_hw.h` from main
        REQUIRES unity)      # pull in `unity.h` from ESP-IDF test component