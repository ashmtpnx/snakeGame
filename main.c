/*
 * =====================================================
 *   SNAKE GAME WITH AI MODE
 *   Made in C using the ncurses library
 *
 *   HOW TO COMPILE:
 *     gcc snake.c -o snake -lncurses
 *   HOW TO RUN:
 *     ./snake
 *
 *   CONTROLS:
 *     Arrow Keys  ->  Move the snake (Player mode)
 *     A           ->  Switch between AI and Player mode
 *     R           ->  Restart the game
 *     Q           ->  Quit
 * =====================================================
 */

#include <ncurses.h>   /* for drawing in the terminal */
#include <stdlib.h>    /* for rand() and srand()      */
#include <time.h>      /* for time() to seed random   */

/* ---------------------------------------------------------
   GAME SETTINGS -- change these to your liking
   --------------------------------------------------------- */
#define BOARD_WIDTH     38
#define BOARD_HEIGHT    18
#define MAX_SNAKE_LEN   (BOARD_WIDTH * BOARD_HEIGHT)
#define GAME_SPEED      120      /* ms per frame -- lower = faster snake */
#define POINTS_PER_FOOD 10

/* ---------------------------------------------------------
   DIRECTION VALUES
   We use numbers 0-3 to represent directions.
   --------------------------------------------------------- */
#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3

/* ---------------------------------------------------------
   STRUCTS -- our custom data types

   Think of a struct like a blueprint for storing
   related pieces of information together.
   --------------------------------------------------------- */

/* A single (x, y) position on the board */
typedef struct {
    int x;   /* column: left to right */
    int y;   /* row:    top to bottom */
} Point;

/* The snake itself */
typedef struct {
    Point body[MAX_SNAKE_LEN];  /* body[0] is the HEAD */
    int   length;               /* how many segments it currently has */
    int   direction;            /* UP, DOWN, LEFT, or RIGHT */
} Snake;

/* Everything about the current game */
typedef struct {
    Snake snake;
    Point food;      /* where the food diamond is */
    int   score;
    int   game_over; /* 1 = dead, 0 = alive */
    int   ai_mode;   /* 1 = computer plays, 0 = you play */
} Game;


/* =========================================================
   place_food()
   Drops a food item at a random empty cell on the board.
   Keeps retrying if the spot is occupied by the snake.
   ========================================================= */
void place_food(Game *game) {
    int spot_is_free;

    do {
        spot_is_free = 1;  /* assume free until proven otherwise */

        game->food.x = rand() % BOARD_WIDTH;
        game->food.y = rand() % BOARD_HEIGHT;

        /* does this position overlap any snake segment? */
        for (int i = 0; i < game->snake.length; i++) {
            if (game->snake.body[i].x == game->food.x &&
                game->snake.body[i].y == game->food.y) {
                spot_is_free = 0;
                break;
            }
        }
    } while (!spot_is_free);
}


/* =========================================================
   init_game()
   Resets everything to the starting state.
   Called at startup and whenever the player presses R.
   ========================================================= */
void init_game(Game *game, int start_in_ai_mode) {
    game->snake.length    = 3;
    game->snake.direction = RIGHT;

    /* place the snake in the middle of the board */
    game->snake.body[0] = (Point){ BOARD_WIDTH / 2,     BOARD_HEIGHT / 2 };  /* head   */
    game->snake.body[1] = (Point){ BOARD_WIDTH / 2 - 1, BOARD_HEIGHT / 2 };  /* middle */
    game->snake.body[2] = (Point){ BOARD_WIDTH / 2 - 2, BOARD_HEIGHT / 2 };  /* tail   */

    game->score     = 0;
    game->game_over = 0;
    game->ai_mode   = start_in_ai_mode;

    place_food(game);
}


/* =========================================================
   draw()
   Clears the terminal and redraws the entire game screen.
   Called every frame.
   ========================================================= */
void draw(Game *game) {
    clear();

    /* draw the four border walls */
    for (int x = 0; x <= BOARD_WIDTH + 1; x++) {
        mvaddch(0,                x, ACS_HLINE);  /* top wall    */
        mvaddch(BOARD_HEIGHT + 1, x, ACS_HLINE);  /* bottom wall */
    }
    for (int y = 1; y <= BOARD_HEIGHT; y++) {
        mvaddch(y, 0,               ACS_VLINE);  /* left wall  */
        mvaddch(y, BOARD_WIDTH + 1, ACS_VLINE);  /* right wall */
    }
    /* corner pieces */
    mvaddch(0,                0,               ACS_ULCORNER);
    mvaddch(0,                BOARD_WIDTH + 1, ACS_URCORNER);
    mvaddch(BOARD_HEIGHT + 1, 0,               ACS_LLCORNER);
    mvaddch(BOARD_HEIGHT + 1, BOARD_WIDTH + 1, ACS_LRCORNER);

    /* draw the food as a diamond
       (+1 offset because row/col 0 is the border) */
    mvaddch(game->food.y + 1, game->food.x + 1, ACS_DIAMOND);

    /* draw the snake: @ for head, o for each body segment */
    mvaddch(game->snake.body[0].y + 1,
            game->snake.body[0].x + 1, '@');

    for (int i = 1; i < game->snake.length; i++)
        mvaddch(game->snake.body[i].y + 1,
                game->snake.body[i].x + 1, 'o');

    /* status bar below the board */
    mvprintw(BOARD_HEIGHT + 2, 1,
            "Score: %d  |  Mode: %-6s  |  [A] Toggle AI   [R] Restart   [Q] Quit",
            game->score, game->ai_mode ? "AI" : "Player");

    if (game->ai_mode)
        mvprintw(BOARD_HEIGHT + 3, 1,
                "AI is playing -- picks the direction closest to food each step.");
    else
        mvprintw(BOARD_HEIGHT + 3, 1,
                "Your turn! Use ARROW KEYS to guide the snake.");

    refresh();  /* push all drawings to the actual screen */
}


/* =========================================================
   is_safe_to_enter()
   Returns 1 if the AI can safely move to position (x, y).
   Returns 0 if that cell is a wall or the snake's own body.

   We skip checking the TAIL because it will have moved
   away by the time the snake arrives there.
   ========================================================= */
int is_safe_to_enter(Game *game, int x, int y) {
    /* out of bounds means hitting a wall */
    if (x < 0 || x >= BOARD_WIDTH)  return 0;
    if (y < 0 || y >= BOARD_HEIGHT) return 0;

    /* check all body segments except the tail */
    for (int i = 0; i < game->snake.length - 1; i++)
        if (game->snake.body[i].x == x &&
            game->snake.body[i].y == y)
            return 0;

    return 1;  /* safe to move here */
}


/* =========================================================
   ai_choose_direction()  <-- the brain of the AI

   ALGORITHM: Greedy (Manhattan Distance)
   ----------------------------------------
   At each step, the AI looks at all 4 possible turns.
   For each direction it asks:
     1. Would I hit a wall or myself?  -> skip it
     2. How far would I be from food?  -> measure it

   It picks the direction that gets CLOSEST to food.

   Manhattan Distance = |x1 - x2| + |y1 - y2|
   (like counting city blocks -- no diagonals)
   ========================================================= */
int ai_choose_direction(Game *game) {
    /* how x and y change for each direction */
    int move_x[] = { 0,  0, -1,  1 };  /* UP, DOWN, LEFT, RIGHT */
    int move_y[] = {-1,  1,  0,  0 };

    /* the direction directly opposite to each one */
    int opposite[] = { DOWN, UP, RIGHT, LEFT };

    int head_x = game->snake.body[0].x;
    int head_y = game->snake.body[0].y;

    int best_direction = -1;
    int best_distance  = 99999;

    for (int d = 0; d < 4; d++) {

        /* never reverse 180 degrees -- you'd crash into your own neck */
        if (d == opposite[game->snake.direction]) continue;

        /* where would the head land if we went this way? */
        int next_x = head_x + move_x[d];
        int next_y = head_y + move_y[d];

        /* skip directions that lead to a wall or body collision */
        if (!is_safe_to_enter(game, next_x, next_y)) continue;

        /* Manhattan distance from this next cell to the food */
        int dist = abs(next_x - game->food.x)
                 + abs(next_y - game->food.y);

        /* keep the direction that brings us closest */
        if (dist < best_distance) {
            best_distance  = dist;
            best_direction = d;
        }
    }

    /* if all 4 directions are blocked, just keep going (death is inevitable) */
    if (best_direction == -1)
        return game->snake.direction;

    return best_direction;
}


/* =========================================================
   move_snake()
   Moves the snake one step forward.
   Handles: eating food, growing longer, and dying.
   ========================================================= */
void move_snake(Game *game) {
    int move_x[] = { 0,  0, -1,  1 };
    int move_y[] = {-1,  1,  0,  0 };

    int dir    = game->snake.direction;
    int next_x = game->snake.body[0].x + move_x[dir];
    int next_y = game->snake.body[0].y + move_y[dir];

    /* did we hit a wall? */
    if (next_x < 0 || next_x >= BOARD_WIDTH ||
        next_y < 0 || next_y >= BOARD_HEIGHT) {
        game->game_over = 1;
        return;
    }

    /* did we run into our own body? */
    for (int i = 0; i < game->snake.length; i++) {
        if (game->snake.body[i].x == next_x &&
            game->snake.body[i].y == next_y) {
            game->game_over = 1;
            return;
        }
    }

    /* did we eat the food? */
    int ate_food = (next_x == game->food.x && next_y == game->food.y);

    /*
     * HOW THE BODY SHIFT WORKS:
     *
     * Imagine length = 3, body = [HEAD, B1, B2]
     *
     * Normal move (no food):
     *   new body = [newHEAD, HEAD, B1]   <-- B2 is dropped, length stays 3
     *
     * Eating food:
     *   new body = [newHEAD, HEAD, B1, B2]  <-- B2 stays, length becomes 4
     *
     * We use new_length to control this:
     *   - ate food  -> new_length = old + 1, so the back slot is preserved
     *   - no food   -> new_length = old,     so the back slot is overwritten
     */
    int new_length = game->snake.length + (ate_food ? 1 : 0);

    /* shift every segment one position backward */
    for (int i = new_length - 1; i > 0; i--)
        game->snake.body[i] = game->snake.body[i - 1];

    game->snake.body[0] = (Point){ next_x, next_y };  /* place new head */
    game->snake.length  = new_length;

    if (ate_food) {
        game->score += POINTS_PER_FOOD;
        place_food(game);  /* spawn next food */
    }
}


/* =========================================================
   main()
   Sets up the terminal, runs the game loop.
   ========================================================= */
int main(void) {
    srand((unsigned)time(NULL));  /* different food positions each run */

    /* set up ncurses (terminal drawing library) */
    initscr();              /* start ncurses                    */
    cbreak();               /* get key presses without Enter    */
    noecho();               /* don't print keys on screen       */
    keypad(stdscr, TRUE);   /* enable arrow key detection       */
    curs_set(0);            /* hide blinking cursor             */
    timeout(GAME_SPEED);    /* getch() waits this many ms       */

    Game game;
    init_game(&game, 0);    /* start in Player mode             */
    draw(&game);

    /* MAIN GAME LOOP -- runs until Q is pressed */
    while (1) {

        /* if AI is on, pick the best direction BEFORE reading input */
        if (game.ai_mode && !game.game_over)
            game.snake.direction = ai_choose_direction(&game);

        /* wait for a keypress (or timeout after GAME_SPEED ms) */
        int key = getch();

        /* keys that work in any mode */
        if (key == 'q' || key == 'Q') break;
        if (key == 'r' || key == 'R') init_game(&game, game.ai_mode);
        if (key == 'a' || key == 'A') game.ai_mode = !game.ai_mode;

        /* arrow keys only work when the player is in control */
        if (!game.ai_mode) {
            if (key == KEY_UP    && game.snake.direction != DOWN)  game.snake.direction = UP;
            if (key == KEY_DOWN  && game.snake.direction != UP)    game.snake.direction = DOWN;
            if (key == KEY_LEFT  && game.snake.direction != RIGHT) game.snake.direction = LEFT;
            if (key == KEY_RIGHT && game.snake.direction != LEFT)  game.snake.direction = RIGHT;
        }

        /* advance the snake one step */
        if (!game.game_over)
            move_snake(&game);

        draw(&game);

        /* show game over screen */
        if (game.game_over) {
            attron(A_BOLD);
            mvprintw(BOARD_HEIGHT / 2,     BOARD_WIDTH / 2 - 5, "  GAME OVER!  ");
            mvprintw(BOARD_HEIGHT / 2 + 1, BOARD_WIDTH / 2 - 8, " Final Score: %d ", game.score);
            mvprintw(BOARD_HEIGHT / 2 + 2, BOARD_WIDTH / 2 - 9, " Press R to play again ");
            attroff(A_BOLD);
            refresh();
        }
    }

    endwin();   /* restore normal terminal before exiting */
    return 0;
}