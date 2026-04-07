#include "Replay.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void expect_true(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    ++g_failures;
  }
}

void expect_equal(int actual, int expected, const std::string &message) {
  if (actual != expected) {
    std::cerr << "FAIL: " << message << " expected=" << expected << " actual=" << actual << "\n";
    ++g_failures;
  }
}

void expect_cell(Cell actual, Cell expected, const std::string &message) {
  if (actual != expected) {
    std::cerr << "FAIL: " << message << " expected=" << cell_name(expected) << " actual=" << cell_name(actual) << "\n";
    ++g_failures;
  }
}

ReplayScript parse_script_or_die(const std::string &text) {
  ReplayScript script;
  std::string error;
  const bool ok = parse_replay_script(text, script, error);
  expect_true(ok, "script parses");
  if (!ok) {
    std::cerr << "parse error: " << error << "\n";
  }
  return script;
}

std::string default_header(const std::string &mode, const std::string &misere) {
  return "mode: " + mode + "\nmisere: " + misere + "\n====================\n";
}

void test_first_line_normal() {
  const std::string script_text = default_header("first-line-ends", "normal") +
                                  "X 0,0,0\n"
                                  "O 0,1,0\n"
                                  "X 1,1,1\n"
                                  "O 0,2,0\n"
                                  "X 2,2,2\n"
                                  "O 0,3,0\n"
                                  "X 3,3,3\n";

  ScriptReplay replay(parse_script_or_die(script_text));
  expect_true(replay.step(), "first move applied");
  expect_equal(static_cast<int>(replay.next_move_index()), 1, "cursor advanced after one step");
  expect_cell(replay.game().board[0][0][0], Cell::X, "board updated after one step");

  replay.replay_all();
  expect_true(!replay.failed(), "normal replay completes");
  expect_true(replay.game().game_over, "normal mode ends on first line");
  expect_cell(replay.game().winner, Cell::X, "X wins normal first-line game");
  expect_equal(replay.game().x_line_count, 1, "one X line counted");
}

void test_first_line_misere() {
  const std::string script_text = default_header("first-line-ends", "misere") +
                                  "X 0,0,0\n"
                                  "O 0,1,0\n"
                                  "X 1,1,1\n"
                                  "O 0,2,0\n"
                                  "X 2,2,2\n"
                                  "O 1,0,0\n"
                                  "X 3,3,3\n";

  ScriptReplay replay(parse_script_or_die(script_text));
  replay.replay_all();

  expect_true(!replay.failed(), "misere replay completes");
  expect_true(replay.game().game_over, "misere mode ends on first line");
  expect_cell(replay.game().winner, Cell::O, "player making line loses in misere mode");
}

void test_reject_wrong_player() {
  const std::string script_text = default_header("first-line-ends", "normal") +
                                  "X 0,0,0\n"
                                  "X 1,0,0\n";

  ScriptReplay replay(parse_script_or_die(script_text));
  replay.replay_all();

  expect_true(replay.failed(), "replay fails on wrong player");
  expect_equal(static_cast<int>(replay.next_move_index()), 1, "cursor stops before rejected move");
  expect_true(!replay.trace().empty(), "trace is populated");
  expect_true(replay.trace().back().kind == ReplayEventKind::MoveRejected, "trace ends with rejection");
}

void test_count_all_lines_full_board_tie() {
  std::vector<std::string> even_cells;
  std::vector<std::string> odd_cells;
  for (int x = 0; x < N; ++x) {
    for (int y = 0; y < N; ++y) {
      for (int z = 0; z < N; ++z) {
        const std::string cell = std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
        if (((x + y + z) % 2) == 0) {
          even_cells.push_back(cell);
        } else {
          odd_cells.push_back(cell);
        }
      }
    }
  }

  std::ostringstream script;
  script << default_header("count-all-lines", "normal");
  for (std::size_t i = 0; i < even_cells.size(); ++i) {
    script << "X " << even_cells[i] << "\n";
    script << "O " << odd_cells[i] << "\n";
  }

  ScriptReplay replay(parse_script_or_die(script.str()));
  replay.replay_all();

  expect_true(!replay.failed(), "count-all-lines replay completes");
  expect_true(replay.game().game_over, "count-all-lines ends when board is full");
  expect_cell(replay.game().winner, Cell::Empty, "parity board produces tie");
  expect_equal(replay.game().x_line_count, 12, "X face diagonals counted");
  expect_equal(replay.game().o_line_count, 12, "O face diagonals counted");
}

} // namespace

int main() {
  test_first_line_normal();
  test_first_line_misere();
  test_reject_wrong_player();
  test_count_all_lines_full_board_tie();

  if (g_failures != 0) {
    std::cerr << g_failures << " test(s) failed\n";
    return 1;
  }

  std::cout << "all replay tests passed\n";
  return 0;
}
