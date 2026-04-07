# 2026-04-06 Gameplay Improvements

## Scope

This change set implemented five related gameplay/UI improvements for the 4x4x4 tic-tac-toe app:

1. Add undo/redo controls and a move-history stack so play can be rewound to the beginning and replayed forward.
3. Added an ImGui toggle for misere mode, where creating a line loses.
5. Added an ImGui toggle for count-all-lines mode, where play continues until the board is full and the result is based on relative line totals instead of the first completed line.

## Final Behavior

### Default mode

- Play stops on the first completed line.
- The player who forms the line wins.
- All cells belonging to completed lines for that winning player are highlighted at game end.

### Misere mode (no count-all-lines mode)

- Play still stops on the first completed line.
- The player who forms the line loses.
- The terminal line cells are still highlighted.

### Count-all-lines mode

- Play does not stop when the first line appears.
- Play continues until the board is full.
- At the end, the game counts all completed lines for both players.
- Higher line count wins in normal scoring.
- Lower line count wins in misere scoring.
- If the line counts are equal, the result is a tie.
- All scored line cells are highlighted at game end.

### Undo/redo behavior

- `Undo` steps the move cursor backward by one move.
- `Redo` reapplies the next move in history.
- Undo can rewind all the way to an empty board.
- If a new move is made after undoing, the abandoned redo branch is discarded.
- Rule toggles recompute outcome state from the current board and history instead of mutating history.

## Implementation Notes

### Game state

`src/Game.hpp` and `src/Game.cpp` were extended to carry:

- Move history (`move_history`, `history_size`, `history_cursor`)
- Rule toggles (`misere_mode`, `count_all_lines`)
- Derived scoring state (`x_line_count`, `o_line_count`, `winner`, `game_over`)

The winner detection logic was refactored around explicit enumeration of all 76 valid lines in the 4x4x4 grid so both first-line and count-all-lines scoring can share the same base evaluation.
Basically, we should count the number of lines "owned" by each player after each move. in count-all-lines mode, just display this count and determine winner when board is full. In first-line-ends mode (ie regular play) instead end the game when a line is detected.

In `count_all_lines` mode, the game now resolves only when the board is full. It does not end on the first completed line. Toggling counting mid-game could trigger/untrigger a notice about a winner.

### Rendering

`src/Renderer.hpp` and `src/Renderer.mm` were extended so `Renderer::end_frame(...)` can accept:

- `last_move_cell`
- `terminal_line_cells`

### UI

`src/main.cpp` now exposes:

- `Undo`
- `Redo`
- `Misere Mode`
- `Count All Lines`

The controls panel also updates status text based on the active rule set and shows live line totals when count-all-lines mode is enabled.

## Verification

- Verified with `cmake --build build`

## Follow-up

- If behavior is changed later, re-check count-all-lines end conditions carefully because that mode is intentionally orthogonal to misere scoring.
- add unit tests for game behavior based on scripted/headless play (ie: a script that dictates moves alon with expected logging that certain game progress is made, eg: player 1 moves to (X,Y), player 2 wins, final score is ..., ...)
