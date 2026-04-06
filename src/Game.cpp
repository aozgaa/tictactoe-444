#include "Game.hpp"

bool Game::make_move(int x, int y, int z) {
    if (game_over) return false;
    if (board[x][y][z] != Cell::Empty) return false;
    board[x][y][z] = current_player;
    winner = check_winner();
    if (winner != Cell::Empty) {
        game_over = true;
    } else {
        bool full = true;
        for (int i = 0; i < N && full; ++i)
            for (int j = 0; j < N && full; ++j)
                for (int k = 0; k < N && full; ++k)
                    if (board[i][j][k] == Cell::Empty) full = false;
        if (full) game_over = true;
    }
    current_player = (current_player == Cell::X) ? Cell::O : Cell::X;
    return true;
}

void Game::reset() {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            for (int k = 0; k < N; ++k)
                board[i][j][k] = Cell::Empty;
    current_player = Cell::X;
    winner = Cell::Empty;
    game_over = false;
}

// Check all 76 lines in a 4x4x4 grid.
Cell Game::check_winner() const {
    auto line = [&](int x0, int y0, int z0, int dx, int dy, int dz) -> Cell {
        Cell a = board[x0][y0][z0];
        if (a == Cell::Empty) return Cell::Empty;
        for (int s = 1; s < N; ++s)
            if (board[x0+s*dx][y0+s*dy][z0+s*dz] != a) return Cell::Empty;
        return a;
    };

    Cell r;

    // Axis-aligned (48): along x, y, z for each pair of the other two coords
    for (int i = 0; i < N; ++i) for (int j = 0; j < N; ++j) {
        r = line(0,i,j, 1,0,0); if (r != Cell::Empty) return r;
        r = line(i,0,j, 0,1,0); if (r != Cell::Empty) return r;
        r = line(i,j,0, 0,0,1); if (r != Cell::Empty) return r;
    }

    // Face diagonals (24): 2 diagonals x 3 axis-pairs x 4 planes
    for (int k = 0; k < N; ++k) {
        r = line(0,  0,k,  1, 1,0); if (r != Cell::Empty) return r;  // XY +
        r = line(0,N-1,k,  1,-1,0); if (r != Cell::Empty) return r;  // XY -
        r = line(0,k,  0,  1,0, 1); if (r != Cell::Empty) return r;  // XZ +
        r = line(0,k,N-1,  1,0,-1); if (r != Cell::Empty) return r;  // XZ -
        r = line(k,0,  0,  0,1, 1); if (r != Cell::Empty) return r;  // YZ +
        r = line(k,0,N-1,  0,1,-1); if (r != Cell::Empty) return r;  // YZ -
    }

    // Space diagonals (4)
    r = line(0,  0,  0,  1, 1, 1); if (r != Cell::Empty) return r;
    r = line(0,  0,N-1,  1, 1,-1); if (r != Cell::Empty) return r;
    r = line(0,N-1,  0,  1,-1, 1); if (r != Cell::Empty) return r;
    r = line(0,N-1,N-1,  1,-1,-1); if (r != Cell::Empty) return r;

    return Cell::Empty;
}
