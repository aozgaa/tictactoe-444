We need to add a script/replay/trace functionality.
This should run against the game logic, which should be factored out from the renderer.

The way it should work is that we have an input format which looks roughly as follows:
```
line 1: game configuration info (count-total-lines vs first-line-ends, regular vs misere mode)
line 2: '================================='
line 3,..: player <X or O>, (X,Y,Z): (<X coord>, <Y coord>, <Z coord>)
```
devise the format so that it is both human readable and easily parseable by a machine (coudl be json lines or something
lighter weight to avoid a json dependency)

then we should be able to replay the game, and the replay should include some kind of logging/tracing amenable to unit
testing. Specifically, we want to check if a move is valid (eg: game is not over, and position played to is empty,
correct player to move) and log error/stop replay if so.
Then we want to log if the move was applied successfully.
Then we want to log/trace current game statistics (lines by X, lines by Y, winner is <X,Y,Undecided>).

We will have a bunch of tests which check different games/moves sequences to ensure that the correct game logic proceeds
in all combinations of modes.
The replay API should allow partial replay (eg: play the next move) so that we can also stop the game partway through
and inspect the game state (eg: ensure the board makes sense).
Unit tests should be able to query any part of the game state.
the trace is moreso as an integration testing tool than a unit testing tool.
avoid private variables.

(can be json lines, or some bare ascii serialization of the move stack
