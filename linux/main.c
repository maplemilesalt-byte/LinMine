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

/* Block sprites, in the exact order supplied for blocks.bmp. */
#define BLOCK_COVERED 0
#define BLOCK_FLAG 1
#define BLOCK_QUESTION_COVERED 2
#define BLOCK_HIT_MINE 3
#define BLOCK_WRONG_FLAG 4
#define BLOCK_MINE 5
#define BLOCK_QUESTION_OPEN 6
#define BLOCK_8 7
#define BLOCK_7 8
#define BLOCK_6 9
#define BLOCK_5 10
#define BLOCK_4 11
#define BLOCK_3 12
#define BLOCK_2 13
#define BLOCK_1 14
#define BLOCK_OPEN_0 15

/* Face sprites, in the exact order supplied for button.bmp. */
#define BUTTON_HAPPY_CLICKED 0
#define BUTTON_SUNGLASSES 1
#define BUTTON_DEAD 2
#define BUTTON_SURPRISED 3
#define BUTTON_NORMAL 4

/* LED sprites, in the exact order supplied for led.bmp. */
#define LED_NEGATIVE 0
#define LED_EMPTY 1
#define LED_9 2
#define LED_8 3
#define LED_7 4
#define LED_6 5
#define LED_5 6
#define LED_4 7
#define LED_3 8
#define LED_2 9
#define LED_1 10
#define LED_0 11

/* LED sprites are 13x23, and the sheet is 13x276 (vertical). */

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
static int face_pressed;
static time_t start_time;

static int block_sprite_for_number(int number)
{
    /* The sheet stores 8..1 in ascending sprite-index order. */
    if (number <= 0)
        return BLOCK_OPEN_0;
    return BLOCK_1 - number + 1;
}

static int led_sprite_for_digit(int digit)
{
    if (digit < 0 || digit > 9)
        return LED_EMPTY;
    return LED_9 + (9 - digit);
}

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
    face_pressed = 0;
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

    value %= 1000;
    if (value < 0)
        value = -value;

    hundreds = value / 100;
    tens = (value / 10) % 10;
    ones = value % 10;

    lm_graphics_draw_sprite(LM_SHEET_LED, led_sprite_for_digit(hundreds),
                            x, TOP_LED_Y, LED_W, LED_H);
    lm_graphics_draw_sprite(LM_SHEET_LED, led_sprite_for_digit(tens),
                            x + LED_W, TOP_LED_Y, LED_W, LED_H);
    lm_graphics_draw_sprite(LM_SHEET_LED, led_sprite_for_digit(ones),
                            x + LED_W * 2, TOP_LED_Y, LED_W, LED_H);
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
    int elapsed = game_over ? (int)(time(NULL) - start_time) : (int)(time(NULL) - start_time);
    int face;

    if (game_over)
        face = won ? BUTTON_SUNGLASSES : BUTTON_DEAD;
    else if (face_pressed)
        face = BUTTON_HAPPY_CLICKED;
    else
        face = BUTTON_NORMAL;

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
                    index = block_sprite_for_number(c->number);
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

static int face_hit(int x, int y)
{
    int fx = (WIDTH - BUTTON_W) / 2;
    return y >= TOP_LED_Y && y < TOP_LED_Y + BUTTON_H &&
           x >= fx && x < fx + BUTTON_W;
}

int main(void)
{
    LMInputButton button;
    LMPoint pos;
    int pressed;
    time_t last_draw_time = 0;
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
    last_draw_time = time(NULL);

    while (!lm_graphics_should_close()) {
        if (!lm_graphics_poll_event(&button, &pos, &pressed)) {
            time_t now = time(NULL);
            if (!game_over && now != last_draw_time) {
                draw();
                last_draw_time = now;
            }
            lm_graphics_delay(16);
            continue;
        }

        if (face_hit(pos.x, pos.y)) {
            if (pressed && button == LM_BUTTON_LEFT) {
                face_pressed = 1;
                draw();
            } else if (!pressed && button == LM_BUTTON_LEFT) {
                if (face_pressed)
                    reset_game();
                face_pressed = 0;
                draw();
                last_draw_time = time(NULL);
            }
            continue;
        }

        if (!pressed)
            continue;

        if (pos.y >= GRID_Y && pos.x >= GRID_X) {
            int x = (pos.x - GRID_X) / TILE;
            int y = (pos.y - GRID_Y) / TILE;
            if (button == LM_BUTTON_LEFT)
                open_cell(x, y);
            else if (button == LM_BUTTON_RIGHT)
                toggle_flag(x, y);
        }
        draw();
        last_draw_time = time(NULL);
    }

    lm_graphics_shutdown();
    return 0;
}
