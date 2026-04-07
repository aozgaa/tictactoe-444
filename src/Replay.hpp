#pragma once

#include "Game.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct ReplayScript {
  bool misere_mode = false;
  bool count_all_lines = false;
  std::vector<Move> moves;
};

enum class ReplayEventKind {
  ConfigApplied,
  MoveApplied,
  MoveRejected,
  StateSnapshot,
};

struct ReplayStateSnapshot {
  int move_index = 0;
  Cell current_player = Cell::X;
  Cell winner = Cell::Empty;
  bool game_over = false;
  int x_line_count = 0;
  int o_line_count = 0;
};

struct ReplayEvent {
  ReplayEventKind kind = ReplayEventKind::ConfigApplied;
  int move_index = 0;
  Move move{};
  ReplayStateSnapshot snapshot{};
  std::string detail;
};

bool parse_replay_script(const std::string &text, ReplayScript &script, std::string &error);
std::string format_replay_event(const ReplayEvent &event);
std::string cell_name(Cell cell);

class ScriptReplay {
public:
  explicit ScriptReplay(ReplayScript script);

  const ReplayScript &script() const;
  const Game &game() const;
  const std::vector<ReplayEvent> &trace() const;
  std::size_t next_move_index() const;
  bool finished() const;
  bool failed() const;

  bool step();
  std::size_t replay_all();

private:
  ReplayStateSnapshot snapshot() const;
  void append_state_snapshot(int move_index, const std::string &detail);

  ReplayScript script_;
  Game game_;
  std::vector<ReplayEvent> trace_;
  std::size_t next_move_index_ = 0;
  bool failed_ = false;
};
