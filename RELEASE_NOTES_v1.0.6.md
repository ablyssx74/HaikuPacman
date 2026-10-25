# HaikuPacman v1.0.6

The ghosts work again, and they get smarter with the levels, not faster.

## Highlights
- **Ghosts stay in the maze.** They could go through walls and run off the screen. They moved in fractions of a pixel but only chose a direction when they stood exactly on a tile line; at the higher levels' speeds they skipped that moment, never turned, and kept going straight through whatever was there. A ghost now moves one pixel at a time and chooses only on a tile, among the tiles that are not walls: never into a wall, never back where it came from (unless it is in a dead end), never out of the maze except through the tunnels at the sides, and never into the ghost house from outside. Inky and Clyde used to start inside a wall; all four start in the ghost house, and a ghost in the house heads out through the door.
- **The ghosts are not much faster at the higher levels.** They used to gain a quarter of a pixel a frame each level, so they were faster than Pacman (2 pixels a frame) by level 3, and Blinky much more at the end of a level. They now gain 0.05 a frame a level, up to 1.3, and Blinky's burst at the end of a level is 10% and 20%.
- **They get smarter instead.** Every ghost takes the shortest way round the walls to where it is going (the old straight-line steering got stuck behind walls, and in some corners of the maze never caught Pacman at all). What changes with the level is how often they take a wrong turn and how well they work together:
  - level 1: a wrong turn at one corner in five
  - level 2: one in twelve
  - level 3: no wrong turns; Inky aims past Pacman from Blinky's side, and Clyde keeps away only when Pacman is very close
  - level 4 and on: Clyde never backs off either
- **The window title shows the level.**
- **Tests.** `make test` runs the game's own ghost code for 150,000 frames at each of nine levels, with Pacman jumping about and the ghosts frightened now and then, and checks that no ghost enters a wall, leaves the maze or moves more than two pixels a frame, that all of them leave the ghost house, and that the higher levels catch Pacman sooner.

## Install
Install `HaikuPacman-1.0.6-1-x86_64.hpkg` on 64-bit Haiku, or `HaikuPacman-1.0.6-1-x86_gcc2.hpkg` on 32-bit Haiku, for the whole system.

## Tested on
- Haiku R1/beta6, 64-bit VM: the tests pass, and the game was played from the package.
- The 32-bit package was built on 32-bit Haiku and not run.
