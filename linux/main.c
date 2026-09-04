#include "gfx.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define COLS 9
#define ROWS 9
#define MINES 10

/* Native WinMine layout inside the drawing area. */
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

typedef struct Cell {
    unsigned char mine;
    unsigned char open;
    unsigned char state; /* 0 covered, 1 flag, 2 question */
    unsigned char number;
} Cell;

static Cell board[ROWS][COLS];
static int game_over;
static int won;
static int remaining;
static int flags;
static int face_pressed;
static int marks_enabled = 1;
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
            board[y][x].mine = board[y][x].open = board[y][x].state = board[y][x].number = 0;

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
            if (board[y][x].mine)
                continue;
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
        board[y][x].open || board[y][x].state != 0)
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

static void toggle_mark(int x, int y)
{
    Cell *c;

    if (x < 0 || x >= COLS || y < 0 || y >= ROWS || game_over || board[y][x].open)
        return;

    c = &board[y][x];
    if (marks_enabled) {
        if (c->state == 0) {
            c->state = 1;
            ++flags;
        } else if (c->state == 1) {
            c->state = 2;
            --flags;
        } else {
            c->state = 0;
        }
    } else {
        if (c->state == 1) {
            c->state = 0;
            --flags;
        } else if (c->state == 0) {
            c->state = 1;
            ++flags;
        }
    }
}

static void draw_led_number(cairo_t *cr, int x, int value)
{
    int hundreds, tens, ones;

    value %= 1000;
    if (value < 0)
        value = -value;

    hundreds = value / 100;
    tens = (value / 10) % 10;
    ones = value % 10;

    lm_graphics_draw_sprite(cr, LM_SHEET_LED, led_sprite_for_digit(hundreds),
                            x, TOP_LED_Y, LED_W, LED_H);
    lm_graphics_draw_sprite(cr, LM_SHEET_LED, led_sprite_for_digit(tens),
                            x + LED_W, TOP_LED_Y, LED_W, LED_H);
    lm_graphics_draw_sprite(cr, LM_SHEET_LED, led_sprite_for_digit(ones),
                            x + LED_W * 2, TOP_LED_Y, LED_W, LED_H);
}

static void draw_border(cairo_t *cr, LMRect r, unsigned int outer, unsigned int inner)
{
    lm_graphics_draw_rect(cr, r, outer);
    if (r.width > 2 && r.height > 2)
        lm_graphics_draw_rect(cr,
                              (LMRect){r.x + 1, r.y + 1, r.width - 2, r.height - 2},
                              inner);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    int x, y;
    int elapsed = (int)(time(NULL) - start_time);
    int face;

    (void)widget;
    (void)user_data;

    if (game_over)
        face = won ? BUTTON_SUNGLASSES : BUTTON_DEAD;
    else if (face_pressed)
        face = BUTTON_HAPPY_CLICKED;
    else
        face = BUTTON_NORMAL;

    lm_graphics_clear(cr, 0xffc0c0c0u, WIDTH, HEIGHT);

    draw_border(cr, (LMRect){0, 0, WIDTH, HEIGHT},
                0xffffffffu, 0xff808080u);
    draw_border(cr, (LMRect){7, 7, WIDTH - 14, 46},
                0xff808080u, 0xffffffffu);
    lm_graphics_fill_rect(cr, (LMRect){10, 10, WIDTH - 20, 40}, 0xffc0c0c0u);

    draw_border(cr, (LMRect){GRID_X + 5, TOP_LED_Y - 1, LED_W * 3 + 2, LED_H + 2},
                0xff808080u, 0xffffffffu);
    draw_border(cr, (LMRect){WIDTH - 12 - LED_W * 3 - 2, TOP_LED_Y - 1,
                             LED_W * 3 + 2, LED_H + 2},
                0xff808080u, 0xffffffffu);
    lm_graphics_draw_sprite(cr, LM_SHEET_BUTTON, face,
                            (WIDTH - BUTTON_W) / 2, TOP_LED_Y,
                            BUTTON_W, BUTTON_H);

    draw_led_number(cr, GRID_X + 7, MINES - flags);
    draw_led_number(cr, WIDTH - 12 - LED_W * 3, elapsed);

    draw_border(cr, (LMRect){GRID_X - 3, GRID_Y - 3,
                             COLS * TILE + 6, ROWS * TILE + 6},
                0xff808080u, 0xffffffffu);

    for (y = 0; y < ROWS; ++y) {
        for (x = 0; x < COLS; ++x) {
            Cell *c = &board[y][x];
            int index;

            if (c->open) {
                index = c->mine ? BLOCK_HIT_MINE : block_sprite_for_number(c->number);
            } else if (c->state == 1) {
                index = game_over && !c->mine ? BLOCK_WRONG_FLAG : BLOCK_FLAG;
            } else if (c->state == 2) {
                index = BLOCK_QUESTION_COVERED;
            } else {
                index = game_over && c->mine ? BLOCK_MINE : BLOCK_COVERED;
            }

            lm_graphics_draw_sprite(cr, LM_SHEET_BLOCKS, index,
                                    GRID_X + x * TILE,
                                    GRID_Y + y * TILE,
                                    TILE, TILE);
        }
    }

    return FALSE;
}

static void queue_draw(void)
{
    lm_graphics_queue_draw();
}

static void on_new_game(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    reset_game();
    queue_draw();
}

static void on_exit_game(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    gtk_main_quit();
}

static void on_marks_toggled(GtkCheckMenuItem *item, gpointer data)
{
    (void)data;
    marks_enabled = gtk_check_menu_item_get_active(item) ? 1 : 0;
}

static void on_about(GtkWidget *widget, gpointer data)
{
    GtkWidget *window = GTK_WIDGET(data);
    (void)widget;

    gtk_show_about_dialog(GTK_WINDOW(window),
                          "program-name", "Minesweeper",
                          "comments", "Linux port of the classic Windows Minesweeper.",
                          "version", "LinMine",
                          NULL);
}

static void on_help_contents(GtkWidget *widget, gpointer data)
{
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog;
    (void)widget;

    dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_CLOSE,
                                    "Minesweeper Help");
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog),
        "Left click opens a square. Right click marks it.\n"
        "F2 starts a new game.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_placeholder(GtkWidget *widget, gpointer data)
{
    GtkWidget *window = GTK_WIDGET(data);
    const char *label = g_object_get_data(G_OBJECT(widget), "linmine-label");
    GtkWidget *dialog;
    (void)widget;

    dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_CLOSE,
                                    "%s",
                                    label ? label : "Not implemented");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static GtkWidget *add_placeholder(GtkWidget *menu, GtkWidget *window,
                                   const char *label)
{
    GtkWidget *item = gtk_menu_item_new_with_mnemonic(label);
    g_object_set_data_full(G_OBJECT(item), "linmine-label",
                           g_strdup(label), g_free);
    g_signal_connect(item, "activate", G_CALLBACK(on_placeholder), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    return item;
}

static GtkWidget *make_menu_bar(GtkWidget *window)
{
    GtkWidget *bar = gtk_menu_bar_new();
    GtkWidget *game_item = gtk_menu_item_new_with_mnemonic("_Game");
    GtkWidget *help_item = gtk_menu_item_new_with_mnemonic("_Help");
    GtkWidget *game_menu = gtk_menu_new();
    GtkWidget *help_menu = gtk_menu_new();
    GtkWidget *item;
    GtkAccelGroup *accels = gtk_accel_group_new();

    gtk_window_add_accel_group(GTK_WINDOW(window), accels);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(game_item), game_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), game_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar), help_item);

    item = gtk_menu_item_new_with_mnemonic("_New");
    gtk_widget_add_accelerator(item, "activate", accels, GDK_KEY_F2, 0, GTK_ACCEL_VISIBLE);
    g_signal_connect(item, "activate", G_CALLBACK(on_new_game), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), item);

    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), gtk_separator_menu_item_new());
    add_placeholder(game_menu, window, "_Beginner");
    add_placeholder(game_menu, window, "_Intermediate");
    add_placeholder(game_menu, window, "_Expert");
    add_placeholder(game_menu, window, "_Custom...");
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), gtk_separator_menu_item_new());

    item = gtk_check_menu_item_new_with_mnemonic("_Marks (?)");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
    g_signal_connect(item, "toggled", G_CALLBACK(on_marks_toggled), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), item);

    add_placeholder(game_menu, window, "Co_lor");
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), gtk_separator_menu_item_new());
    add_placeholder(game_menu, window, "Best _Times...");
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), gtk_separator_menu_item_new());

    item = gtk_menu_item_new_with_mnemonic("E_xit");
    g_signal_connect(item, "activate", G_CALLBACK(on_exit_game), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(game_menu), item);

    item = gtk_menu_item_new_with_mnemonic("_Contents");
    gtk_widget_add_accelerator(item, "activate", accels, GDK_KEY_F1, 0, GTK_ACCEL_VISIBLE);
    g_signal_connect(item, "activate", G_CALLBACK(on_help_contents), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), item);
    add_placeholder(help_menu, window, "Using _Help");
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), gtk_separator_menu_item_new());

    item = gtk_menu_item_new_with_mnemonic("_About Minesweeper...");
    g_signal_connect(item, "activate", G_CALLBACK(on_about), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), item);

    return bar;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    int x, y;
    (void)widget;
    (void)data;

    if (event->button != GDK_BUTTON_PRIMARY && event->button != GDK_BUTTON_SECONDARY)
        return FALSE;

    x = (int)event->x;
    y = (int)event->y;

    if (event->button == GDK_BUTTON_PRIMARY) {
        int fx = (WIDTH - BUTTON_W) / 2;
        if (x >= fx && x < fx + BUTTON_W &&
            y >= TOP_LED_Y && y < TOP_LED_Y + BUTTON_H) {
            face_pressed = 1;
            queue_draw();
            return TRUE;
        }
    }

    if (y >= GRID_Y && x >= GRID_X &&
        x < GRID_X + COLS * TILE && y < GRID_Y + ROWS * TILE) {
        int cell_x = (x - GRID_X) / TILE;
        int cell_y = (y - GRID_Y) / TILE;
        if (event->button == GDK_BUTTON_PRIMARY)
            open_cell(cell_x, cell_y);
        else
            toggle_mark(cell_x, cell_y);
        queue_draw();
        return TRUE;
    }

    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    int x = (int)event->x;
    int y = (int)event->y;
    int fx = (WIDTH - BUTTON_W) / 2;
    (void)widget;
    (void)data;

    if (event->button != GDK_BUTTON_PRIMARY)
        return FALSE;

    if (face_pressed) {
        if (x >= fx && x < fx + BUTTON_W &&
            y >= TOP_LED_Y && y < TOP_LED_Y + BUTTON_H)
            reset_game();
        face_pressed = 0;
        queue_draw();
        return TRUE;
    }

    return FALSE;
}

static gboolean on_timer(gpointer data)
{
    (void)data;
    if (!game_over)
        queue_draw();
    return G_SOURCE_CONTINUE;
}

static void on_window_destroy(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
    gtk_main_quit();
}

int main(int argc, char **argv)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *bar;
    GtkWidget *drawing_area;
    GtkCssProvider *css;
    GdkRGBA clear_color;

    gtk_init(&argc, &argv);
    srand((unsigned)time(NULL));

    if (!lm_graphics_init(WIDTH, HEIGHT, "Minesweeper"))
        return 1;

    window = lm_graphics_window();
    drawing_area = lm_graphics_drawing_area();
    gtk_container_remove(GTK_CONTAINER(window), drawing_area);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    bar = make_menu_bar(window);
    gtk_box_pack_start(GTK_BOX(box), bar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), drawing_area, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), box);

    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(on_button_release), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "menubar { background: #c0c0c0; padding: 0; }"
        "menubar > menuitem { padding: 2px 7px; }"
        "menu { background: #c0c0c0; }"
        "menu menuitem { padding: 3px 18px 3px 6px; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    clear_color.red = 0.75;
    clear_color.green = 0.75;
    clear_color.blue = 0.75;
    clear_color.alpha = 1.0;
    gtk_widget_override_background_color(drawing_area, GTK_STATE_FLAG_NORMAL, &clear_color);

    if (!lm_graphics_load_assets("../winmine/bmp", 1)) {
        fprintf(stderr, "LinMine: failed to load WinMine BMP assets\n");
        lm_graphics_shutdown();
        return 1;
    }

    reset_game();
    g_timeout_add_seconds(1, on_timer, NULL);

    gtk_widget_show_all(window);
    gtk_widget_grab_focus(drawing_area);
    gtk_main();

    lm_graphics_shutdown();
    return 0;
}
