# The Snake Game (C + ncurses)

A classic Snake game written in C using the `ncurses` library for terminal rendering, with a built-in AI mode that plays the game for you using a greedy pathfinding algorithm.

## Features

- Classic Snake gameplay in the terminal
- Toggleable AI mode — watch the computer play using a greedy Manhattan-distance algorithm
- Score tracking
- Restart without quitting
- Simple, well-commented single-file implementation

## Demo

```
┌──────────────────────────────────────┐
│                                       │
│            @ooo                  ♦   │
│                                       │
│                                       │
└──────────────────────────────────────┘
Score: 30  |  Mode: Player  |  [A] Toggle AI   [R] Restart   [Q] Quit
Your turn! Use ARROW KEYS to guide the snake.
```

## Requirements

- GCC (or any C compiler)
- `ncurses` library

On Debian/Ubuntu:
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
```

On macOS (via Homebrew):
```bash
brew install ncurses
```

## Build

```bash
gcc main.c -o snake -lncurses
```

## Run

```bash
./snake
```

## Controls

| Key          | Action                          |
|--------------|----------------------------------|
| Arrow Keys   | Move the snake (Player mode)    |
| `A`          | Toggle between AI and Player mode |
| `R`          | Restart the game                |
| `Q`          | Quit                            |

## How the AI works

The AI mode uses a greedy algorithm: at every step, it checks the four possible directions (excluding moving backward into itself) and picks whichever one minimizes Manhattan distance to the food, as long as that direction doesn't lead into a wall or the snake's own body. It's intentionally simple — it doesn't plan ahead or guarantee survival, so it can occasionally trap itself.

## Configuration

A few constants at the top of `main.c` control the game:

```c
#define BOARD_WIDTH     38
#define BOARD_HEIGHT    18
#define GAME_SPEED      120   // ms per frame, lower = faster
#define POINTS_PER_FOOD 10
```

Adjust these and recompile to change the board size, speed, or scoring.

## Known issues

See the [Issues](../../issues) tab — including an edge case where the AI's tail-collision check can be inaccurate on the same step food is eaten.

## License

MIT (or update to whatever you prefer)
