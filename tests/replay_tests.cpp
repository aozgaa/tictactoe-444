#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "Replay.hpp"

#include <doctest/doctest.h>

#include <sstream>
#include <string>
#include <vector>

namespace {

TEST_CASE("normal mode ends on first completed line") {
  const std::string text = "mode: first-line-ends\n"
                           "misere: normal\n"
                           "====================\n"
                           "X 0,0,0\n"
                           "O 0,1,0\n"
                           "X 1,1,1\n"
                           "O 0,2,0\n"
                           "X 2,2,2\n"
                           "O 0,3,0\n"
                           "X 3,3,3\n";

  ReplayScript script;
  std::string error;
  REQUIRE(parse_replay_script(text, script, error));
  ScriptReplay replay{script};

  CHECK(replay.step());
  CHECK(replay.next_move_index() == 1);
  CHECK(replay.game().board[0][0][0] == Cell::X);

  replay.replay_all();
  CHECK_FALSE(replay.failed());
  CHECK(replay.game().game_over);
  CHECK(replay.game().winner == Cell::X);
  CHECK(replay.game().x_line_count == 1);
}

TEST_CASE("misere mode awards win to the other player on a completed line") {
  const std::string text = "mode: first-line-ends\n"
                           "misere: misere\n"
                           "====================\n"
                           "X 0,0,0\n"
                           "O 0,1,0\n"
                           "X 1,1,1\n"
                           "O 0,2,0\n"
                           "X 2,2,2\n"
                           "O 1,0,0\n"
                           "X 3,3,3\n";

  ReplayScript script;
  std::string error;
  REQUIRE(parse_replay_script(text, script, error));
  ScriptReplay replay{script};

  replay.replay_all();

  CHECK_FALSE(replay.failed());
  CHECK(replay.game().game_over);
  CHECK(replay.game().winner == Cell::O);
}

TEST_CASE("replay rejects moves from the wrong player") {
  const std::string text = "mode: first-line-ends\n"
                           "misere: normal\n"
                           "====================\n"
                           "X 0,0,0\n"
                           "X 1,0,0\n";

  ReplayScript script;
  std::string error;
  REQUIRE(parse_replay_script(text, script, error));
  ScriptReplay replay{script};

  replay.replay_all();

  REQUIRE(replay.failed());
  CHECK(replay.next_move_index() == 1);
  REQUIRE_FALSE(replay.trace().empty());
  CHECK(replay.trace().back().kind == ReplayEventKind::MoveRejected);
}

TEST_CASE("count-all-lines mode scores the full board correctly") {
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

  std::ostringstream oss;
  oss << "mode: count-all-lines\n"
         "misere: normal\n"
         "====================\n";
  for (std::size_t i = 0; i < even_cells.size(); ++i) {
    oss << "X " << even_cells[i] << "\n";
    oss << "O " << odd_cells[i] << "\n";
  }

  ReplayScript script;
  std::string error;
  REQUIRE(parse_replay_script(oss.str(), script, error));
  ScriptReplay replay{script};

  replay.replay_all();

  CHECK_FALSE(replay.failed());
  CHECK(replay.game().game_over);
  CHECK(replay.game().winner == Cell::Empty);
  CHECK(replay.game().x_line_count == 12);
  CHECK(replay.game().o_line_count == 12);
}

} // namespace
