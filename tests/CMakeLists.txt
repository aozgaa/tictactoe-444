add_executable(tictactoe-444-tests replay_tests.cpp)
target_link_libraries(tictactoe-444-tests PRIVATE tictactoe_core)
add_test(NAME tictactoe-444-tests COMMAND tictactoe-444-tests)
add_test(
    NAME tictactoe-444-shutdown-smoke
    COMMAND tictactoe-444 --test-sdl-quit
)
