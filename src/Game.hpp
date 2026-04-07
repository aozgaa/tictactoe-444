#pragma once

#include <array>

static constexpr int N = 4;   // board dimension

enum class Cell { Empty, X, O };

struct Move {
    int x = 0;
    int y = 0;
    int z = 0;
    Cell player = Cell::Empty;
};

struct Game {
    static constexpr int MaxMoves = N * N * N;

    Cell board[N][N][N] = {};
    Cell current_player = Cell::X;
    Cell winner = Cell::Empty;
    bool game_over = false;
    bool misere_mode = false;
    bool count_all_lines = false;
    int x_line_count = 0;
    int o_line_count = 0;
    bool highlighted[N][N][N] = {};
    std::array<Move, MaxMoves> move_history = {};
    int history_size = 0;
    int history_cursor = 0;

    bool make_move(int x, int y, int z);
    bool undo();
    bool redo();
    void reset();
    void set_misere_mode(bool enabled);
    void set_count_all_lines(bool enabled);
    bool can_undo() const;
    bool can_redo() const;

private:
    void rebuild_board_from_history();
    void recompute_state();
};
