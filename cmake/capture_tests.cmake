qt_add_executable(mark-shot-capture-own-windows-guard-test
    tests/capture_own_windows_guard_test.cpp
    src/capture_own_windows_guard.cpp
    src/capture_own_windows_guard.h
    src/debug_log.cpp
    src/debug_log.h
)
target_include_directories(mark-shot-capture-own-windows-guard-test PRIVATE src)
target_link_libraries(mark-shot-capture-own-windows-guard-test
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        Qt6::Widgets
)
add_test(NAME capture-own-windows-guard COMMAND mark-shot-capture-own-windows-guard-test)

qt_add_executable(mark-shot-capture-delay-test
    tests/capture_delay_test.cpp
    src/capture_delay/capture_delay_option.cpp
    src/capture_delay/capture_delay_option.h
    src/capture_delay/capture_delay_scheduler.cpp
    src/capture_delay/capture_delay_scheduler.h
    src/debug_log.cpp
    src/debug_log.h
)
target_include_directories(mark-shot-capture-delay-test PRIVATE src)
target_link_libraries(mark-shot-capture-delay-test
    PRIVATE
        Qt6::Core
        Qt6::Test
)
add_test(NAME capture-delay COMMAND mark-shot-capture-delay-test)
