#include "Replay.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

std::string read_file(const char *path) {
  std::ifstream input(path);
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "usage: tictactoe-444-replay <script-file>\n";
    return 1;
  }

  const std::string text = read_file(argv[1]);
  if (text.empty()) {
    std::cerr << "failed to read script: " << argv[1] << "\n";
    return 1;
  }

  ReplayScript script;
  std::string error;
  if (!parse_replay_script(text, script, error)) {
    std::cerr << "parse error: " << error << "\n";
    return 1;
  }

  ScriptReplay replay(script);
  replay.replay_all();

  for (const ReplayEvent &event : replay.trace()) { std::cout << format_replay_event(event) << "\n"; }

  return replay.failed() ? 1 : 0;
}
