# 4x4x4 Tic-Tac-Toe

A 3D tic-tac-toe game on a 4x4x4 grid (64 cells) built with Apple Metal and Dear ImGui.

You can play it as standard first-line-wins tic-tac-toe, or switch rules mid-game with the built-in UI:

- `Misere Mode`: making a line loses instead of wins
- `Count All Lines`: play continues until the board is full, then the winner is based on total completed lines
- `Undo` / `Redo`: rewind and replay move history

## Controls

- Drag to rotate the cube
- Click a cell to place X or O
- `Cell Size` changes spacing so inner cells are easier to reach
- `Reset Orientation` restores the default camera angle
- `New Game` clears the board
- `Undo` and `Redo` move backward and forward through the recorded move history
- `Misere Mode` flips the scoring so the player who completes a line loses
- `Count All Lines` keeps play going until the board is full and scores all completed lines
- `Empty cell color` changes the debug color for unoccupied cells

## Gameplay Modes

### Default mode

- Play stops on the first completed line
- The player who forms that line wins

### Misere mode

- Play still stops on the first completed line
- The player who forms that line loses

### Count-all-lines mode

- Play does not stop when the first line appears
- Play continues until the board is full
- The game counts all completed lines for X and O
- Higher line count wins in normal scoring
- Lower line count wins when `Misere Mode` is also enabled
- Equal line counts produce a draw

## Dependencies

All dependencies are vendored as git submodules under `ext/`:

| Library | Purpose |
| --- | --- |
| [ocornut/imgui](https://github.com/ocornut/imgui) | UI overlay |
| [bkaradzic/metal-cpp](https://github.com/bkaradzic/metal-cpp) | C++ Metal API |
| [g-truc/glm](https://github.com/g-truc/glm) | Math |
| [libsdl-org/SDL (SDL2)](https://github.com/libsdl-org/SDL/tree/SDL2) | Window + input |

SDL2 is built and installed to `env/`. The rest are header-only and compiled in-place.

## Prerequisites

- macOS with Xcode installed
- CMake >= 3.28
- Ninja
- Metal shader toolchain available through `xcrun`

## Build

### 1. Clone with submodules

```sh
git clone --recurse-submodules <repo-url>
cd tictactoe-444
```

Or if already cloned:

```sh
git submodule update --init --recursive
```

### 2. Build vendored dependencies

This builds SDL2 and installs it into `env/`:

```sh
cmake -P cmake/prepare_vendored.cmake
```

If the Metal shader toolchain is missing, install it and re-run the build:

```sh
xcodebuild -downloadComponent MetalToolchain
```

### 3. Configure

```sh
cmake --preset vendored_env
```

### 4. Build

```sh
cmake --build build
```

### 5. Run

Run the executable from `build/` so it can find `default.metallib`:

```sh
cd build && ./tictactoe-444
```
