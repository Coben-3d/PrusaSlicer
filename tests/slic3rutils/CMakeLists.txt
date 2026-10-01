get_filename_component(_TEST_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
add_executable(${_TEST_NAME}_tests
    ${_TEST_NAME}_tests_main.cpp
    slic3r_jobs_tests.cpp
    slic3r_version_tests.cpp
    slic3r_arrangejob_tests.cpp
    secretstore_tests.cpp
    )

# mold linker for successful linking needs also to link TBB library and link it before libslic3r.
target_link_libraries(${_TEST_NAME}_tests test_common TBB::tbb TBB::tbbmalloc libslic3r_gui libslic3r libseqarrange)

if (MSVC)
    target_link_libraries(${_TEST_NAME}_tests Setupapi.lib)
endif ()
set_property(TARGET ${_TEST_NAME}_tests PROPERTY FOLDER "tests")

if (WIN32)
    prusaslicer_copy_dlls(${_TEST_NAME}_tests)
endif()

# catch_discover_tests(${_TEST_NAME}_tests TEST_PREFIX "${_TEST_NAME}: ")
set(_catch_args "exclude:[NotWorking];-s")
list(APPEND _catch_args "${CATCH_EXTRA_ARGS}")
add_test(${_TEST_NAME}_tests ${_TEST_NAME}_tests ${_catch_args})

# Small protocol target also builds without the full GUI library.
add_executable(loaded_filament_color_tests
    loaded_filament_color_tests.cpp
    ../../src/slic3r/Utils/LoadedFilamentColor.cpp)
target_include_directories(loaded_filament_color_tests PRIVATE ../../src)
target_link_libraries(loaded_filament_color_tests PRIVATE test_common nlohmann_json::nlohmann_json)
add_test(NAME loaded_filament_color_tests COMMAND loaded_filament_color_tests)

add_executable(loaded_filament_color_config_tests loaded_filament_color_config_tests.cpp)
target_link_libraries(loaded_filament_color_config_tests PRIVATE test_common libslic3r_gui)
add_test(NAME loaded_filament_color_config_tests COMMAND loaded_filament_color_config_tests)

# Opt-in loopback harness: never connects by default or discovers printers.
add_executable(loaded_filament_color_client loaded_filament_color_client.cpp)
target_link_libraries(loaded_filament_color_client PRIVATE libslic3r_gui)
