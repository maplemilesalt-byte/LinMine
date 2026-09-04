#include "gfx.h"

#include <stdlib.h>
#include <time.h>

#define COLS 9
#define ROWS 9
#define MINES 10

/* Native WinMine layout. */
#define TILE 16
#define LED_W 13
#define LED_H 23
#define BUTTON_W 24
#define BUTTON_H 24
#define GRID_X 12
#define GRID_Y 55
#define TOP_LED_Y 16
#define WIDTH (GRID_X + COLS * TILE + 12)
#define HEIGHT (GRID_Y + ROWS * TILE + 12)

#define BLOCK_OPEN_0 0
#define BLOCK_MINE 10
#define BLOCK_WRONG_FLAG 11
#define BLOCK_HIT_MINE 12
#define BLOCK_QUESTION 13
#define BLOCK_FLAG 14
#define BLOCK_COVERED 15

#define BUTTON_NORMAL 0
#define BUTTON_PRESSED 1
#define BUTTON_DEAD 2
#define BUTTON_WIN 3

#define LED_NEGATIVE 10

typedef struct Cell {
    unsigned char mine;
    unsigned char open;
    unsigned char flag;
    unsigned char number;
} Cell;

static Cell board[ROWS][COLS];
static int game_over;
static int won;
static int remaining;
static int flags;
static time_t start_time;

static void reset_game(void)
{
    int mines = 0;
    int x, y, nx, ny;

    for (y = 0; y < ROWS; ++y)
        for (x = 0; x < COLS; ++x)
            board[y][x].mine = board[y][x].open = board[y][x].flag = board[y][x].number = 0;

    while (mines < MINES) {
        x = rand() % COLS;
        y = rand() % ROWS;
        if (!board[y][x].mine) {
            board[y][x].mine = 1;
            ++mines;
        }
    }

    for (y = 0; y < ROWS; ++y) {
        for (x = 0; x < COLS; ++x) {
            if (board[y][x].mine) continue;
            for (ny = y - 1; ny <= y + 1; ++ny)
                for (nx = x - 1; nx <= x + 1; ++nx)
                    if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && board[ny][nx].mine)
                        ++board[y][x].number;
        }
    }

    game_over = 0;
    won = 0;
    flags = 0;
    remaining = ROWS * COLS - MINES;
    start_time = time(NULL);
}

static void open_cell(int x, int y)
{
    int nx, ny;

    if (x < 0 || x >= COLS || y < 0 || y >= ROWS || game_over ||
        board[y][x].open || board[y][x].flag)
        return;

    board[y][x].open = 1;
    if (board[y][x].mine) {
        game_over = 1;
        won = 0;
        return;
    }

    if (--remaining == 0) {
        game_over = 1;
        won = 1;
        return;
    }

    if (board[y][x].number == 0)
        for (ny = y - 1; ny <= y + 1; ++ny)
            for (nx = x - 1; nx <= x + 1; ++nx)
                open_cell(nx, ny);
}

static void toggle_flag(int x, int y)
{
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS || game_over || board[y][x].open)
        return;

    if (board[y][x].flag) {
        board[y][x].flag = 0;
        --flags;
    } else {
        board[y][x].flag = 1;
        ++flags;
    }
}

static void draw_led_number(int x, int value)
{
    int hundreds, tens, ones;

    if (value < 0) {
        lm_graphics_draw_sprite(LM_SHEET_LED, LED_NEGATIVE, x, TOP_LED_Y, LED_W, LED_H);
        value = -value;
    }

    value %= 1000;
    hundreds = value / 100;
    tens = (value / 10) % 10;
    ones = value % 10;

    lm_graphics_draw_sprite(LM_SHEET_LED, hundreds, x, TOP_LED_Y, LED_W, LED_H);
    lm_graphics_draw_sprite(LM_SHEET_LED, tens, x + LED_W, TOP_LED_Y, LED_W, LED_H);
    lm_graphics_draw_sprite(LM_SHEET_LED, ones, x + LED_W * 2, TOP_LED_Y, LED_W, LED_H);
}

static void draw_border(LMRect r, unsigned int outer, unsigned int inner)
{
    lm_graphics_draw_rect(r, outer);
    if (r.width > 2 && r.height > 2)
        lm_graphics_draw_rect((LMRect){r.x + 1, r.y + 1, r.width - 2, r.height - 2}, inner);
}

static void draw(void)
{
    int x, y;
    int elapsed = (int)(time(NULL) - start_time);
    int face = game_over ? (won ? BUTTON_WIN : BUTTON_DEAD) : BUTTON_NORMAL;

    /* Classic WinMine gray background. */
    lm_graphics_clear(0xffc0c0c0u);

    /* Outer window and upper status panel. */
    draw_border((LMRect){0, 0, WIDTH, HEIGHT}, 0xffffffffu, 0xff808080u);
    draw_border((LMRect){7, 7, WIDTH - 14, 46}, 0xff808080u, 0xffffffffu);
    lm_graphics_fill_rect((LMRect){10, 10, WIDTH - 20, 40}, 0xffc0c0c0u);

    /* Counter wells and the native 24x24 face sprite. */
    draw_border((LMRect){GRID_X + 5, TOP_LED_Y - 1, LED_W * 3 + 2, LED_H + 2},
                0xff808080u, 0xffffffffu);
    draw_border((LMRect){WIDTH - 12 - LED_W * 3 - 2, TOP_LED_Y - 1,
                         LED_W * 3 + 2, LED_H + 2}, 0xff808080u, 0xffffffffu);
    lm_graphics_draw_sprite(LM_SHEET_BUTTON, face,
                            (WIDTH - BUTTON_W) / 2, TOP_LED_Y, BUTTON_W, BUTTON_H);

    draw_led_number(GRID_X + 7, MINES - flags);
    draw_led_number(WIDTH - 12 - LED_W * 3, elapsed);

    /* The board itself is native 16x16 tiles. */
    draw_border((LMRect){GRID_X - 3, GRID_Y - 3,
                         COLS * TILE + 6, ROWS * TILE + 6},
                0xff808080u, 0xffffffffu);

    for (y = 0; y < ROWS; ++y) {
        for (x = 0; x < COLS; ++x) {
            Cell *c = &board[y][x];
            int index;

            if (c->open) {
                if (c->mine)
                    index = BLOCK_HIT_MINE;
                else
                    index = c->number;
            } else if (c->flag) {
                index = game_over && !c->mine ? BLOCK_WRONG_FLAG : BLOCK_FLAG;
            } else {
                index = game_over && c->mine ? BLOCK_MINE : BLOCK_COVERED;
            }

            lm_graphics_draw_sprite(LM_SHEET_BLOCKS, index,
                                    GRID_X + x * TILE,
                                    GRID_Y + y * TILE,
                                    TILE, TILE);
        }
    }

    lm_graphics_present();
}

int main(void)
{
    LMInputButton button;
    LMPoint pos;
    int pressed;
    const char *asset_dir = "../winmine/bmp";

    srand((unsigned)time(NULL));

    if (!lm_graphics_init(WIDTH, HEIGHT, "Minesweeper"))
        return 1;

    if (!lm_graphics_load_assets(asset_dir, 1)) {
        lm_graphics_shutdown();
        return 1;
    }

    reset_game();
    draw();

    while (!lm_graphics_should_close()) {
        if (!lm_graphics_poll_event(&button, &pos, &pressed)) {
            lm_graphics_delay(16);
            continue;
        }
        if (!pressed)
            continue;

        if (pos.y >= TOP_LED_Y && pos.y < TOP_LED_Y + BUTTON_H &&
            pos.x >= (WIDTH - BUTTON_W) / 2 &&
            pos.x < (WIDTH - BUTTON_W) / 2 + BUTTON_W) {
            reset_game();
        } else if (pos.y >= GRID_Y && pos.x >= GRID_X) {
            int x = (pos.x - GRID_X) / TILE;
            int y = (pos.y - GRID_Y) / TILE;
            if (button == LM_BUTTON_LEFT)
                open_cell(x, y);
            else if (button == LM_BUTTON_RIGHT)
                toggle_flag(x, y);
        }
        draw();
    }

    lm_graphics_shutdown();
    return 0;
}
