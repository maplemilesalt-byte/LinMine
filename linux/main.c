#include "gfx.h"

#include <stdlib.h>
#include <time.h>

#define COLS 9
#define ROWS 9
#define MINES 10

/* Native WinMine layout, including its menu bar. */
#define TILE 16
#define LED_W 13
#define LED_H 23
#define BUTTON_W 24
#define BUTTON_H 24
#define MENU_H 18
#define GRID_X 12
#define GRID_Y 73
#define TOP_LED_Y 34
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

/* Original menu.inc structure. */
enum MenuId {
    MENU_NONE,
    MENU_GAME,
    MENU_HELP
};

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
static int menu_open;
static time_t start_time;

static int block_sprite_for_number(int number)
{
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
    menu_open = MENU_NONE;
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

static void draw_menu_item(LMRect r, const char *text, int selected)
{
    if (selected) {
        lm_graphics_fill_rect(r, 0xff000080u);
        lm_graphics_draw_text(text, r.x + 4, r.y + 2, 0xffffffffu);
    } else {
        lm_graphics_fill_rect(r, 0xffc0c0c0u);
        lm_graphics_draw_text(text, r.x + 4, r.y + 2, 0xff000000u);
    }
}

static void draw_separator(LMRect r)
{
    lm_graphics_fill_rect((LMRect){r.x + 3, r.y + 4, r.width - 6, 1}, 0xff808080u);
    lm_graphics_fill_rect((LMRect){r.x + 3, r.y + 5, r.width - 6, 1}, 0xffffffffu);
}

static void draw_menu(void)
{
    if (menu_open == MENU_NONE)
        return;

    if (menu_open == MENU_GAME) {
        LMRect box = {2, MENU_H, 168, 220};
        lm_graphics_fill_rect(box, 0xffc0c0c0u);
        lm_graphics_draw_rect((LMRect){box.x, box.y, box.width, box.height}, 0xff000000u);
        lm_graphics_draw_rect((LMRect){box.x + 1, box.y + 1, box.width - 2, box.height - 2}, 0xffffffffu);

        draw_menu_item((LMRect){box.x + 2, box.y + 2, box.width - 4, 18}, "New    F2", 0);
        draw_separator((LMRect){box.x + 2, box.y + 20, box.width - 4, 8});
        draw_menu_item((LMRect){box.x + 2, box.y + 28, box.width - 4, 18}, "Beginner", 0);
        draw_menu_item((LMRect){box.x + 2, box.y + 46, box.width - 4, 18}, "Intermediate", 0);
        draw_menu_item((LMRect){box.x + 2, box.y + 64, box.width - 4, 18}, "Expert", 0);
        draw_menu_item((LMRect){box.x + 2, box.y + 82, box.width - 4, 18}, "Custom...", 0);
        draw_separator((LMRect){box.x + 2, box.y + 100, box.width - 4, 8});
        draw_menu_item((LMRect){box.x + 2, box.y + 108, box.width - 4, 18}, "Marks (?)", 0);
        draw_menu_item((LMRect){box.x + 2, box.y + 126, box.width - 4, 18}, "Color", 0);
        draw_separator((LMRect){box.x + 2, box.y + 144, box.width - 4, 8});
        draw_menu_item((LMRect){box.x + 2, box.y + 152, box.width - 4, 18}, "Best Times...", 0);
        draw_separator((LMRect){box.x + 2, box.y + 170, box.width - 4, 8});
        draw_menu_item((LMRect){box.x + 2, box.y + 178, box.width - 4, 18}, "Exit", 0);
    } else {
        LMRect box = {58, MENU_H, 150, 76};
        lm_graphics_fill_rect(box, 0xffc0c0c0u);
        lm_graphics_draw_rect((LMRect){box.x, box.y, box.width, box.height}, 0xff000000u);
        lm_graphics_draw_rect((LMRect){box.x + 1, box.y + 1, box.width - 2, box.height - 2}, 0xffffffffu);
        draw_menu_item((LMRect){box.x + 2, box.y + 2, box.width - 4, 18}, "Contents F1", 0);
        draw_menu_item((LMRect){box.x + 2, box.y + 20, box.width - 4, 18}, "Using Help", 0);
        draw_separator((LMRect){box.x + 2, box.y + 38, box.width - 4, 8});
        draw_menu_item((LMRect){box.x + 2, box.y + 46, box.width - 4, 18}, "About Minesweeper...", 0);
    }
}

static void draw_menu_bar(void)
{
    lm_graphics_fill_rect((LMRect){0, 0, WIDTH, MENU_H}, 0xffc0c0c0u);
    lm_graphics_draw_text("Game", 7, 2, 0xff000000u);
    lm_graphics_draw_text("Help", 55, 2, 0xff000000u);
    if (menu_open != MENU_NONE)
        draw_menu();
}

static int game_menu_hit(int x, int y)
{
    LMRect box = {2, MENU_H, 168, 220};
    if (x < box.x || x >= box.x + box.width || y < box.y || y >= box.y + box.height)
        return -1;

    y -= box.y + 2;
    if (y >= 0 && y < 18) return 0;       /* New */
    if (y >= 178 && y < 196) return 10;   /* Exit */
    return -2;
}

static int help_menu_hit(int x, int y)
{
    LMRect box = {58, MENU_H, 150, 76};
    if (x < box.x || x >= box.x + box.width || y < box.y || y >= box.y + box.height)
        return -1;
    y -= box.y + 2;
    if (y >= 0 && y < 18) return 0;
    if (y >= 46 && y < 64) return 2;
    return -2;
}

static void draw(void)
{
    int x, y;
    int elapsed = (int)(time(NULL) - start_time);
    int face;

    if (game_over)
        face = won ? BUTTON_SUNGLASSES : BUTTON_DEAD;
    else if (face_pressed)
        face = BUTTON_HAPPY_CLICKED;
    else
        face = BUTTON_NORMAL;

    lm_graphics_clear(0xffc0c0c0u);
    draw_menu_bar();

    draw_border((LMRect){0, MENU_H, WIDTH, 46}, 0xffffffffu, 0xff808080u);
    lm_graphics_fill_rect((LMRect){10, MENU_H + 3, WIDTH - 20, 40}, 0xffc0c0c0u);

    draw_border((LMRect){GRID_X + 5, TOP_LED_Y - 1, LED_W * 3 + 2, LED_H + 2},
                0xff808080u, 0xffffffffu);
    draw_border((LMRect){WIDTH - 12 - LED_W * 3 - 2, TOP_LED_Y - 1,
                         LED_W * 3 + 2, LED_H + 2}, 0xff808080u, 0xffffffffu);
    lm_graphics_draw_sprite(LM_SHEET_BUTTON, face,
                            (WIDTH - BUTTON_W) / 2, TOP_LED_Y, BUTTON_W, BUTTON_H);

    draw_led_number(GRID_X + 7, MINES - flags);
    draw_led_number(WIDTH - 12 - LED_W * 3, elapsed);

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
    int event_type;
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
        event_type = lm_graphics_poll_event(&button, &pos, &pressed);
        if (!event_type) {
            time_t now = time(NULL);
            if (!game_over && now != last_draw_time) {
                draw();
                last_draw_time = now;
            }
            lm_graphics_delay(16);
            continue;
        }

        if (event_type == 2 && pos.x == -2) { /* F2 */
            reset_game();
            draw();
            last_draw_time = time(NULL);
            continue;
        }

        if (event_type == 2 && pos.x == -1) { /* F1 */
            menu_open = MENU_HELP;
            draw();
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

        if (pressed && button == LM_BUTTON_LEFT && pos.y < MENU_H) {
            if (pos.x >= 2 && pos.x < 53) {
                menu_open = menu_open == MENU_GAME ? MENU_NONE : MENU_GAME;
                draw();
            } else if (pos.x >= 53 && pos.x < 105) {
                menu_open = menu_open == MENU_HELP ? MENU_NONE : MENU_HELP;
                draw();
            } else {
                menu_open = MENU_NONE;
                draw();
            }
            continue;
        }

        if (!pressed && button == LM_BUTTON_LEFT && menu_open != MENU_NONE) {
            int hit = menu_open == MENU_GAME ? game_menu_hit(pos.x, pos.y)
                                             : help_menu_hit(pos.x, pos.y);
            if (hit == 0 && menu_open == MENU_GAME) {
                reset_game();
                menu_open = MENU_NONE;
                draw();
                last_draw_time = time(NULL);
                continue;
            }
            if (hit == 10 && menu_open == MENU_GAME) {
                menu_open = MENU_NONE;
                lm_graphics_request_close();
                continue;
            }
            menu_open = MENU_NONE;
            draw();
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
