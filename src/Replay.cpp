#include "Replay.hpp"

#include <sstream>

namespace {

bool is_separator_line(const std::string &line) {
  if (line.empty()) return false;
  for (char ch : line) {
    if (ch != '=') return false;
  }
  return true;
}

std::string trim(const std::string &value) {
  std::size_t start = 0;
  while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r')) { ++start; }

  std::size_t end = value.size();
  while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r')) { --end; }

  return value.substr(start, end - start);
}

bool parse_bool_value(const std::string &key, const std::string &value, ReplayScript &script, std::string &error) {
  if (key == "misere") {
    if (value == "normal") {
      script.misere_mode = false;
      return true;
    }
    if (value == "misere") {
      script.misere_mode = true;
      return true;
    }
    error = "invalid misere value: " + value;
    return false;
  }

  if (key == "mode") {
    if (value == "first-line-ends") {
      script.count_all_lines = false;
      return true;
    }
    if (value == "count-all-lines") {
      script.count_all_lines = true;
      return true;
    }
    error = "invalid mode value: " + value;
    return false;
  }

  error = "unknown header key: " + key;
  return false;
}

bool parse_move_line(const std::string &line, Move &move) {
  std::istringstream stream(line);
  char player = '\0';
  char comma1 = '\0';
  char comma2 = '\0';
  if (!(stream >> player >> move.x >> comma1 >> move.y >> comma2 >> move.z)) return false;
  if (comma1 != ',' || comma2 != ',') return false;

  char extra = '\0';
  if (stream >> extra) return false;

  if (player == 'X') {
    move.player = Cell::X;
    return true;
  }
  if (player == 'O') {
    move.player = Cell::O;
    return true;
  }
  return false;
}

bool in_bounds(int value) {
  return value >= 0 && value < N;
}

} // namespace

bool parse_replay_script(const std::string &text, ReplayScript &script, std::string &error) {
  script = ReplayScript{};
  error.clear();

  std::istringstream input(text);
  std::string line;
  bool saw_separator = false;
  bool saw_mode = false;
  bool saw_misere = false;
  int line_number = 0;

  while (std::getline(input, line)) {
    ++line_number;
    const std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#') continue;

    if (!saw_separator) {
      if (is_separator_line(trimmed)) {
        saw_separator = true;
        continue;
      }

      const std::size_t colon = trimmed.find(':');
      if (colon == std::string::npos) {
        error = "line " + std::to_string(line_number) + ": expected header key/value pair or separator";
        return false;
      }

      const std::string key = trim(trimmed.substr(0, colon));
      const std::string value = trim(trimmed.substr(colon + 1));
      if (!parse_bool_value(key, value, script, error)) {
        error = "line " + std::to_string(line_number) + ": " + error;
        return false;
      }
      if (key == "mode") saw_mode = true;
      if (key == "misere") saw_misere = true;
      continue;
    }

    Move move;
    if (!parse_move_line(trimmed, move)) {
      error = "line " + std::to_string(line_number) + ": expected move in format 'X 0,0,0'";
      return false;
    }
    script.moves.push_back(move);
  }

  if (!saw_mode) {
    error = "missing required header: mode";
    return false;
  }
  if (!saw_misere) {
    error = "missing required header: misere";
    return false;
  }
  if (!saw_separator) {
    error = "missing separator line made of '=' characters";
    return false;
  }

  return true;
}

std::string cell_name(Cell cell) {
  switch (cell) {
  case Cell::X: return "X";
  case Cell::O: return "O";
  case Cell::Empty:
  default: return "Undecided";
  }
}

std::string format_replay_event(const ReplayEvent &event) {
  std::ostringstream out;

  switch (event.kind) {
  case ReplayEventKind::ConfigApplied: out << "[config] " << event.detail; break;
  case ReplayEventKind::MoveApplied:
    out << "[move " << event.move_index << "] applied " << cell_name(event.move.player) << " at (" << event.move.x
        << "," << event.move.y << "," << event.move.z << ")";
    break;
  case ReplayEventKind::MoveRejected:
    out << "[move " << event.move_index << "] rejected " << cell_name(event.move.player) << " at (" << event.move.x
        << "," << event.move.y << "," << event.move.z << "): " << event.detail;
    break;
  case ReplayEventKind::StateSnapshot:
    out << "[state " << event.snapshot.move_index << "] current=" << cell_name(event.snapshot.current_player)
        << " winner=" << cell_name(event.snapshot.winner) << " over=" << (event.snapshot.game_over ? "true" : "false")
        << " lines(X=" << event.snapshot.x_line_count << ",O=" << event.snapshot.o_line_count << ")";
    if (!event.detail.empty()) out << " " << event.detail;
    break;
  }

  return out.str();
}

ScriptReplay::ScriptReplay(ReplayScript script) : script_(std::move(script)) {
  game_.set_misere_mode(script_.misere_mode);
  game_.set_count_all_lines(script_.count_all_lines);

  ReplayEvent config_event;
  config_event.kind = ReplayEventKind::ConfigApplied;
  config_event.detail = "mode=" + std::string(script_.count_all_lines ? "count-all-lines" : "first-line-ends") +
                        " misere=" + std::string(script_.misere_mode ? "misere" : "normal");
  config_event.snapshot = snapshot();
  trace_.push_back(config_event);
  append_state_snapshot(0, "after config");
}

const ReplayScript &ScriptReplay::script() const {
  return script_;
}

const Game &ScriptReplay::game() const {
  return game_;
}

const std::vector<ReplayEvent> &ScriptReplay::trace() const {
  return trace_;
}

std::size_t ScriptReplay::next_move_index() const {
  return next_move_index_;
}

bool ScriptReplay::finished() const {
  return failed_ || next_move_index_ >= script_.moves.size();
}

bool ScriptReplay::failed() const {
  return failed_;
}

bool ScriptReplay::step() {
  if (finished()) return false;

  const std::size_t move_index = next_move_index_ + 1;
  const Move move = script_.moves[next_move_index_];

  auto reject = [&](const std::string &detail) {
    ReplayEvent event;
    event.kind = ReplayEventKind::MoveRejected;
    event.move_index = static_cast<int>(move_index);
    event.move = move;
    event.detail = detail;
    event.snapshot = snapshot();
    trace_.push_back(event);
    failed_ = true;
  };

  if (!in_bounds(move.x) || !in_bounds(move.y) || !in_bounds(move.z)) {
    reject("coordinate out of bounds");
    return false;
  }
  if (game_.game_over) {
    reject("game is already over");
    return false;
  }
  if (move.player != game_.current_player) {
    reject("wrong player to move");
    return false;
  }
  if (game_.board[move.x][move.y][move.z] != Cell::Empty) {
    reject("target cell is not empty");
    return false;
  }
  if (!game_.make_move(move.x, move.y, move.z)) {
    reject("game rejected move");
    return false;
  }

  ++next_move_index_;

  ReplayEvent applied;
  applied.kind = ReplayEventKind::MoveApplied;
  applied.move_index = static_cast<int>(move_index);
  applied.move = move;
  applied.snapshot = snapshot();
  trace_.push_back(applied);
  append_state_snapshot(static_cast<int>(move_index), "after move");
  return true;
}

std::size_t ScriptReplay::replay_all() {
  const std::size_t start = next_move_index_;
  while (step()) {}
  return next_move_index_ - start;
}

ReplayStateSnapshot ScriptReplay::snapshot() const {
  ReplayStateSnapshot result;
  result.move_index = static_cast<int>(next_move_index_);
  result.current_player = game_.current_player;
  result.winner = game_.winner;
  result.game_over = game_.game_over;
  result.x_line_count = game_.x_line_count;
  result.o_line_count = game_.o_line_count;
  return result;
}

void ScriptReplay::append_state_snapshot(int move_index, const std::string &detail) {
  ReplayEvent event;
  event.kind = ReplayEventKind::StateSnapshot;
  event.move_index = move_index;
  event.snapshot = snapshot();
  event.snapshot.move_index = move_index;
  event.detail = detail;
  trace_.push_back(event);
}
