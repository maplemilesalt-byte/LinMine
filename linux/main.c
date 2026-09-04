#include "gfx.h"

#include <stdlib.h>
#include <time.h>

#define COLS 9
#define ROWS 9
#define MINES 10
#define TILE 24
#define TOP 48
#define WIDTH (COLS * TILE)
#define HEIGHT (TOP + ROWS * TILE)

typedef struct Cell {
    unsigned char mine;
    unsigned char open;
    unsigned char flag;
    unsigned char number;
} Cell;

static Cell board[ROWS][COLS];
static int game_over;
static int remaining;

static void reset_game(void)
{
    int mines = 0;
    int x, y, nx, ny;
    for (y = 0; y < ROWS; ++y)
        for (x = 0; x < COLS; ++x)
            board[y][x].mine = board[y][x].open = board[y][x].flag = board[y][x].number = 0;

    srand((unsigned)time(NULL));
    while (mines < MINES) {
        x = rand() % COLS; y = rand() % ROWS;
        if (!board[y][x].mine) { board[y][x].mine = 1; ++mines; }
    }

    for (y = 0; y < ROWS; ++y) for (x = 0; x < COLS; ++x) {
        if (board[y][x].mine) continue;
        for (ny = y - 1; ny <= y + 1; ++ny)
            for (nx = x - 1; nx <= x + 1; ++nx)
                if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && board[ny][nx].mine)
                    ++board[y][x].number;
    }
    game_over = 0;
    remaining = ROWS * COLS - MINES;
}

static void open_cell(int x, int y)
{
    int nx, ny;
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS || game_over || board[y][x].open || board[y][x].flag)
        return;
    board[y][x].open = 1;
    if (board[y][x].mine) { game_over = 1; return; }
    if (--remaining == 0) { game_over = 1; return; }
    if (board[y][x].number == 0)
        for (ny = y - 1; ny <= y + 1; ++ny)
            for (nx = x - 1; nx <= x + 1; ++nx)
                open_cell(nx, ny);
}

static void toggle_flag(int x, int y)
{
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS || game_over || board[y][x].open) return;
    board[y][x].flag = !board[y][x].flag;
}

static void draw_digit(int x, int y, int digit)
{
    static const unsigned char glyph[10][5] = {
        {31,17,17,17,31},{0,18,31,16,0},{25,21,21,21,19},{17,21,21,21,31},
        {7,4,4,4,31},{23,21,21,21,29},{31,21,21,21,29},{1,1,25,5,3},
        {31,21,21,21,31},{23,21,21,21,31}
    };
    int row, col;
    if (digit < 0 || digit > 9) return;
    for (row = 0; row < 5; ++row)
        for (col = 0; col < 5; ++col)
            if (glyph[digit][row] & (1u << (4 - col)))
                lm_graphics_fill_rect((LMRect){x + col * 2, y + row * 2, 2, 2}, 0xff0000ffu);
}

static void draw(void)
{
    int x, y;
    lm_graphics_clear(0xffc0c0c0u);
    lm_graphics_draw_rect((LMRect){2, 2, WIDTH - 4, TOP - 4}, 0xff808080u);
    lm_graphics_fill_rect((LMRect){8, 8, WIDTH - 16, TOP - 16}, 0xff000000u);

    for (y = 0; y < ROWS; ++y) for (x = 0; x < COLS; ++x) {
        int px = x * TILE, py = TOP + y * TILE;
        Cell *c = &board[y][x];
        if (!c->open) {
            lm_graphics_fill_rect((LMRect){px, py, TILE, TILE}, 0xffc0c0c0u);
            lm_graphics_draw_rect((LMRect){px, py, TILE, TILE}, 0xff808080u);
            if (c->flag) lm_graphics_fill_rect((LMRect){px + 8, py + 6, 8, 12}, 0xffff0000u);
        } else if (c->mine) {
            lm_graphics_fill_rect((LMRect){px + 5, py + 5, 14, 14}, 0xff202020u);
        } else {
            lm_graphics_fill_rect((LMRect){px, py, TILE, TILE}, 0xffb0b0b0u);
            lm_graphics_draw_rect((LMRect){px, py, TILE, TILE}, 0xff808080u);
            if (c->number) draw_digit(px + 7, py + 7, c->number);
        }
    }
    lm_graphics_present();
}

int main(void)
{
    LMInputButton button;
    LMPoint pos;
    int pressed;
    if (!lm_graphics_init(WIDTH, HEIGHT, "LinMine")) return 1;
    reset_game();
    draw();

    while (!lm_graphics_should_close()) {
        if (!lm_graphics_poll_event(&button, &pos, &pressed)) {
            lm_graphics_delay(1);
            continue;
        }
        if (!pressed) continue;
        if (pos.y < TOP) reset_game();
        else {
            int x = pos.x / TILE, y = (pos.y - TOP) / TILE;
            if (button == LM_BUTTON_LEFT) open_cell(x, y);
            else if (button == LM_BUTTON_RIGHT) toggle_flag(x, y);
        }
        draw();
    }
    lm_graphics_shutdown();
    return 0;
}
