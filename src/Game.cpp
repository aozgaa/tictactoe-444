#include "Game.hpp"

#include <array>

namespace {

struct Line {
    std::array<std::array<int, 3>, N> cells {};
};

constexpr int kLineCount = 76;

std::array<Line, kLineCount> build_lines() {
    std::array<Line, kLineCount> lines {};
    int index = 0;

    auto add_line = [&](int x0, int y0, int z0, int dx, int dy, int dz) {
        Line line {};
        for (int step = 0; step < N; ++step) {
            line.cells[step] = {x0 + step * dx, y0 + step * dy, z0 + step * dz};
        }
        lines[index++] = line;
    };

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            add_line(0, i, j, 1, 0, 0);
            add_line(i, 0, j, 0, 1, 0);
            add_line(i, j, 0, 0, 0, 1);
        }
    }

    for (int k = 0; k < N; ++k) {
        add_line(0, 0, k, 1, 1, 0);
        add_line(0, N - 1, k, 1, -1, 0);
        add_line(0, k, 0, 1, 0, 1);
        add_line(0, k, N - 1, 1, 0, -1);
        add_line(k, 0, 0, 0, 1, 1);
        add_line(k, 0, N - 1, 0, 1, -1);
    }

    add_line(0, 0, 0, 1, 1, 1);
    add_line(0, 0, N - 1, 1, 1, -1);
    add_line(0, N - 1, 0, 1, -1, 1);
    add_line(0, N - 1, N - 1, 1, -1, -1);

    return lines;
}

const std::array<Line, kLineCount> kLines = build_lines();

bool board_full(const Game& game) {
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            for (int z = 0; z < N; ++z) {
                if (game.board[x][y][z] == Cell::Empty) {
                    return false;
                }
            }
        }
    }
    return true;
}

Cell other_player(Cell player) {
    return player == Cell::X ? Cell::O : Cell::X;
}

}  // namespace

bool Game::make_move(int x, int y, int z) {
    if (game_over) return false;
    if (board[x][y][z] != Cell::Empty) return false;

    if (history_cursor < history_size) {
        history_size = history_cursor;
    }

    Move move;
    move.x = x;
    move.y = y;
    move.z = z;
    move.player = current_player;
    move_history[history_cursor++] = move;
    history_size = history_cursor;

    rebuild_board_from_history();
    recompute_state();
    return true;
}

bool Game::undo() {
    if (!can_undo()) return false;
    --history_cursor;
    rebuild_board_from_history();
    recompute_state();
    return true;
}

bool Game::redo() {
    if (!can_redo()) return false;
    ++history_cursor;
    rebuild_board_from_history();
    recompute_state();
    return true;
}

void Game::reset() {
    history_size = 0;
    history_cursor = 0;
    rebuild_board_from_history();
    recompute_state();
}

void Game::set_misere_mode(bool enabled) {
    misere_mode = enabled;
    recompute_state();
}

void Game::set_count_all_lines(bool enabled) {
    count_all_lines = enabled;
    recompute_state();
}

bool Game::can_undo() const {
    return history_cursor > 0;
}

bool Game::can_redo() const {
    return history_cursor < history_size;
}

void Game::rebuild_board_from_history() {
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            for (int z = 0; z < N; ++z) {
                board[x][y][z] = Cell::Empty;
            }
        }
    }

    for (int i = 0; i < history_cursor; ++i) {
        const Move& move = move_history[i];
        board[move.x][move.y][move.z] = move.player;
    }

    current_player = (history_cursor % 2 == 0) ? Cell::X : Cell::O;
}

void Game::recompute_state() {
    winner = Cell::Empty;
    game_over = false;
    x_line_count = 0;
    o_line_count = 0;

    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            for (int z = 0; z < N; ++z) {
                highlighted[x][y][z] = false;
            }
        }
    }

    Cell first_line_owner = Cell::Empty;
    for (const Line& line : kLines) {
        const auto& first = line.cells[0];
        Cell owner = board[first[0]][first[1]][first[2]];
        if (owner == Cell::Empty) continue;

        bool complete = true;
        for (int i = 1; i < N; ++i) {
            const auto& cell = line.cells[i];
            if (board[cell[0]][cell[1]][cell[2]] != owner) {
                complete = false;
                break;
            }
        }
        if (!complete) continue;

        if (first_line_owner == Cell::Empty) {
            first_line_owner = owner;
        }
        if (owner == Cell::X) {
            ++x_line_count;
        } else {
            ++o_line_count;
        }
    }

    const bool full = board_full(*this);

    if (count_all_lines) {
        if (full) {
            game_over = true;
            if (x_line_count != o_line_count) {
                const bool x_wins = misere_mode ? (x_line_count < o_line_count)
                                                : (x_line_count > o_line_count);
                winner = x_wins ? Cell::X : Cell::O;
            }

            for (const Line& line : kLines) {
                const auto& first = line.cells[0];
                Cell owner = board[first[0]][first[1]][first[2]];
                if (owner == Cell::Empty) continue;

                bool complete = true;
                for (int i = 1; i < N; ++i) {
                    const auto& cell = line.cells[i];
                    if (board[cell[0]][cell[1]][cell[2]] != owner) {
                        complete = false;
                        break;
                    }
                }
                if (!complete) continue;

                for (const auto& cell : line.cells) {
                    highlighted[cell[0]][cell[1]][cell[2]] = true;
                }
            }
        }
        return;
    }

    if (first_line_owner != Cell::Empty) {
        game_over = true;
        winner = misere_mode ? other_player(first_line_owner) : first_line_owner;

        for (const Line& line : kLines) {
            const auto& first = line.cells[0];
            Cell owner = board[first[0]][first[1]][first[2]];
            if (owner != first_line_owner) continue;

            bool complete = true;
            for (int i = 1; i < N; ++i) {
                const auto& cell = line.cells[i];
                if (board[cell[0]][cell[1]][cell[2]] != owner) {
                    complete = false;
                    break;
                }
            }
            if (!complete) continue;

            for (const auto& cell : line.cells) {
                highlighted[cell[0]][cell[1]][cell[2]] = true;
            }
        }
        return;
    }

    if (full) {
        game_over = true;
    }
}
