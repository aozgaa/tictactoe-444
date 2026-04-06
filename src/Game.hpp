#pragma once

static constexpr int N = 4;   // board dimension

enum class Cell { Empty, X, O };

struct Game {
    Cell board[N][N][N] = {};
    Cell current_player = Cell::X;
    Cell winner = Cell::Empty;
    bool game_over = false;

    bool make_move(int x, int y, int z);
    void reset();

private:
    Cell check_winner() const;
};
