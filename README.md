# MPCS 51044 (Winter 2020) Course Project - Jonah Usadi

## Overview - 2-player Snake Game

This is a terminal-based implementation of the 2-player variant of the game [snake](https://en.wikipedia.org/wiki/Snake_(video_game_genre)). Rendering is handled by the library ncurses.

### My testing environment

- clang 6.0.0
- gcc 9.2.1
- ncurses 6.1
- xterm-256color

### Known issues

- ncurses `clear()` function can cause the screen to flicker. Calling `erase()` instead of `clear()` solves the flickering problem, but this leads to unpredictable display behavior. Because of this, `clear()` is used in `snake.h`

- Terminals without color are not supported
