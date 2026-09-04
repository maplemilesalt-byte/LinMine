#include "gfx.h"
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAX_COLS 30
#define MAX_ROWS 25
#define TILE 16
#define LED_W 13
#define LED_H 23
#define BUTTON_W 24
#define BUTTON_H 24
#define GRID_X 12
#define GRID_Y 55
#define TOP_LED_Y 16
#define BORDER 12
#define BLOCK_COVERED 0
#define BLOCK_FLAG 1
#define BLOCK_QUESTION_COVERED 2
#define BLOCK_HIT_MINE 3
#define BLOCK_WRONG_FLAG 4
#define BLOCK_MINE 5
#define BLOCK_OPEN_0 15
#define BUTTON_HAPPY_CLICKED 0
#define BUTTON_SUNGLASSES 1
#define BUTTON_DEAD 2
#define BUTTON_NORMAL 4
#define LED_EMPTY 1
#define LED_9 2
#define BEGINNER_COLS 9
#define BEGINNER_ROWS 9
#define BEGINNER_MINES 10
#define INTERMEDIATE_COLS 16
#define INTERMEDIATE_ROWS 16
#define INTERMEDIATE_MINES 40
#define EXPERT_COLS 30
#define EXPERT_ROWS 16
#define EXPERT_MINES 99

typedef struct Cell { unsigned char mine, open, state, number; } Cell;
typedef enum { PRESET_BEGINNER, PRESET_INTERMEDIATE, PRESET_EXPERT } GamePreset;

static Cell board[MAX_ROWS][MAX_COLS];
static int game_cols=BEGINNER_COLS, game_rows=BEGINNER_ROWS, mine_count=BEGINNER_MINES;
static int game_over, won, remaining, flags, face_pressed, marks_enabled=1;
static int tablet_mode=0;
static time_t start_time;
static GtkWidget *window_widget, *drawing_area_widget;
static GtkWidget *beginner_item, *intermediate_item, *expert_item, *tablet_item;

static int game_width(void){return GRID_X+game_cols*TILE+BORDER;}
static int game_height(void){return GRID_Y+game_rows*TILE+BORDER;}
static int block_sprite_for_number(int n){return n<=0?BLOCK_OPEN_0:15-n;}
static int led_sprite_for_digit(int d){return d<0||d>9?LED_EMPTY:LED_9+(9-d);}
static void queue_draw(void){lm_graphics_queue_draw();}

static void apply_tablet_mode(void)
{
    if(!window_widget)return;
    if(tablet_mode)gtk_window_maximize(GTK_WINDOW(window_widget));
    else {gtk_window_unmaximize(GTK_WINDOW(window_widget));lm_graphics_resize(game_width(),game_height());}
}

static void reset_game(void)
{
    int mines=0,x,y,nx,ny;
    for(y=0;y<MAX_ROWS;y++)for(x=0;x<MAX_COLS;x++)board[y][x].mine=board[y][x].open=board[y][x].state=board[y][x].number=0;
    while(mines<mine_count){x=rand()%game_cols;y=rand()%game_rows;if(!board[y][x].mine){board[y][x].mine=1;mines++;}}
    for(y=0;y<game_rows;y++)for(x=0;x<game_cols;x++)if(!board[y][x].mine)
        for(ny=y-1;ny<=y+1;ny++)for(nx=x-1;nx<=x+1;nx++)if(nx>=0&&nx<game_cols&&ny>=0&&ny<game_rows&&board[ny][nx].mine)board[y][x].number++;
    game_over=won=flags=face_pressed=0;remaining=game_cols*game_rows-mine_count;start_time=time(NULL);
}

static void update_preset_menu(GamePreset p)
{
    if(beginner_item)gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(beginner_item),p==PRESET_BEGINNER);
    if(intermediate_item)gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(intermediate_item),p==PRESET_INTERMEDIATE);
    if(expert_item)gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(expert_item),p==PRESET_EXPERT);
}

static void set_preset(GamePreset p)
{
    if(p==PRESET_BEGINNER){game_cols=BEGINNER_COLS;game_rows=BEGINNER_ROWS;mine_count=BEGINNER_MINES;}
    else if(p==PRESET_INTERMEDIATE){game_cols=INTERMEDIATE_COLS;game_rows=INTERMEDIATE_ROWS;mine_count=INTERMEDIATE_MINES;}
    else{game_cols=EXPERT_COLS;game_rows=EXPERT_ROWS;mine_count=EXPERT_MINES;}
    reset_game();
    if(window_widget&&drawing_area_widget&&!tablet_mode){lm_graphics_resize(game_width(),game_height());queue_draw();}
    update_preset_menu(p);
}

static void on_beginner(GtkCheckMenuItem*i,gpointer d){(void)d;if(gtk_check_menu_item_get_active(i))set_preset(PRESET_BEGINNER);}
static void on_intermediate(GtkCheckMenuItem*i,gpointer d){(void)d;if(gtk_check_menu_item_get_active(i))set_preset(PRESET_INTERMEDIATE);}
static void on_expert(GtkCheckMenuItem*i,gpointer d){(void)d;if(gtk_check_menu_item_get_active(i))set_preset(PRESET_EXPERT);}
static void on_tablet_mode(GtkCheckMenuItem*i,gpointer d){(void)d;tablet_mode=gtk_check_menu_item_get_active(i)!=FALSE;apply_tablet_mode();queue_draw();}

static void open_cell(int x,int y)
{
    int nx,ny;
    if(x<0||x>=game_cols||y<0||y>=game_rows||game_over||board[y][x].open||board[y][x].state)return;
    board[y][x].open=1;if(board[y][x].mine){game_over=1;won=0;return;}
    if(--remaining==0){game_over=1;won=1;return;}
    if(board[y][x].number==0)for(ny=y-1;ny<=y+1;ny++)for(nx=x-1;nx<=x+1;nx++)open_cell(nx,ny);
}

static void toggle_mark(int x,int y)
{
    Cell*c;
    if(x<0||x>=game_cols||y<0||y>=game_rows||game_over||board[y][x].open)return;
    c=&board[y][x];
    if(marks_enabled){if(c->state==0){c->state=1;flags++;}else if(c->state==1){c->state=2;flags--;}else c->state=0;}
    else if(c->state==1){c->state=0;flags--;}else if(c->state==0){c->state=1;flags++;}
}

static void draw_border(cairo_t*cr,LMRect r,unsigned int a,unsigned int b){lm_graphics_draw_rect(cr,r,a);if(r.width>2&&r.height>2)lm_graphics_draw_rect(cr,(LMRect){r.x+1,r.y+1,r.width-2,r.height-2},b);}
static void draw_leds(cairo_t*cr,int x,int value){int h,t,o;value%=1000;if(value<0)value=-value;h=value/100;t=(value/10)%10;o=value%10;lm_graphics_draw_sprite(cr,LM_SHEET_LED,led_sprite_for_digit(h),x,TOP_LED_Y,LED_W,LED_H);lm_graphics_draw_sprite(cr,LM_SHEET_LED,led_sprite_for_digit(t),x+LED_W,TOP_LED_Y,LED_W,LED_H);lm_graphics_draw_sprite(cr,LM_SHEET_LED,led_sprite_for_digit(o),x+LED_W*2,TOP_LED_Y,LED_W,LED_H);}

static gboolean on_draw(GtkWidget*w,cairo_t*cr,gpointer data)
{
    int x,y,elapsed=(int)(time(NULL)-start_time),face;(void)w;(void)data;
    face=game_over?(won?BUTTON_SUNGLASSES:BUTTON_DEAD):(face_pressed?BUTTON_HAPPY_CLICKED:BUTTON_NORMAL);
    lm_graphics_clear(cr,0xffc0c0c0u,game_width(),game_height());
    draw_border(cr,(LMRect){0,0,game_width(),game_height()},0xffffffffu,0xff808080u);
    draw_border(cr,(LMRect){7,7,game_width()-14,46},0xff808080u,0xffffffffu);
    lm_graphics_fill_rect(cr,(LMRect){10,10,game_width()-20,40},0xffc0c0c0u);
    draw_border(cr,(LMRect){GRID_X+5,TOP_LED_Y-1,LED_W*3+2,LED_H+2},0xff808080u,0xffffffffu);
    draw_border(cr,(LMRect){game_width()-12-LED_W*3-2,TOP_LED_Y-1,LED_W*3+2,LED_H+2},0xff808080u,0xffffffffu);
    lm_graphics_draw_sprite(cr,LM_SHEET_BUTTON,face,(game_width()-BUTTON_W)/2,TOP_LED_Y,BUTTON_W,BUTTON_H);
    draw_leds(cr,GRID_X+7,mine_count-flags);draw_leds(cr,game_width()-12-LED_W*3,elapsed);
    draw_border(cr,(LMRect){GRID_X-3,GRID_Y-3,game_cols*TILE+6,game_rows*TILE+6},0xff808080u,0xffffffffu);
    for(y=0;y<game_rows;y++)for(x=0;x<game_cols;x++){Cell*c=&board[y][x];int i;if(c->open)i=c->mine?BLOCK_HIT_MINE:block_sprite_for_number(c->number);else if(c->state==1)i=game_over&&!c->mine?BLOCK_WRONG_FLAG:BLOCK_FLAG;else if(c->state==2)i=BLOCK_QUESTION_COVERED;else i=game_over&&c->mine?BLOCK_MINE:BLOCK_COVERED;lm_graphics_draw_sprite(cr,LM_SHEET_BLOCKS,i,GRID_X+x*TILE,GRID_Y+y*TILE,TILE,TILE);}
    return FALSE;
}

static void on_new_game(GtkWidget*w,gpointer d){(void)w;(void)d;reset_game();queue_draw();}
static void on_exit_game(GtkWidget*w,gpointer d){(void)w;(void)d;gtk_main_quit();}
static void on_window_destroy(GtkWidget*w,gpointer d){(void)w;(void)d;gtk_main_quit();}
static void on_marks(GtkCheckMenuItem*i,gpointer d){(void)d;marks_enabled=gtk_check_menu_item_get_active(i)!=FALSE;}
static void show_info(GtkWidget*p,const char*t,const char*b){GtkWidget*d=gtk_message_dialog_new(GTK_WINDOW(p),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,"%s",t);gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d),"%s",b);gtk_dialog_run(GTK_DIALOG(d));gtk_widget_destroy(d);}
static void on_help(GtkWidget*w,gpointer d){(void)w;show_info(GTK_WIDGET(d),"Minesweeper Help","Left click opens a square. Right click marks it.\nF2 starts a new game.\nChoose a difficulty from Game.\nGame -> Tablet Mode maximizes the game window for touchscreens.");}
static void on_about(GtkWidget*w,gpointer d){(void)w;show_info(GTK_WIDGET(d),"About Minesweeper","Linux port of the classic Windows Minesweeper.\nLinMine");}

static void on_custom(GtkWidget*w,gpointer data)
{
    GtkWidget*d,*grid,*ws,*hs,*ms,*l;int r;(void)w;
    d=gtk_dialog_new_with_buttons("Custom Field",GTK_WINDOW(data),GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,"Cancel",GTK_RESPONSE_CANCEL,"OK",GTK_RESPONSE_OK,NULL);
    grid=gtk_grid_new();gtk_grid_set_row_spacing(GTK_GRID(grid),6);gtk_grid_set_column_spacing(GTK_GRID(grid),8);gtk_container_set_border_width(GTK_CONTAINER(grid),12);
    l=gtk_label_new("Width:");ws=gtk_spin_button_new_with_range(8,30,1);gtk_spin_button_set_value(GTK_SPIN_BUTTON(ws),game_cols);gtk_grid_attach(GTK_GRID(grid),l,0,0,1,1);gtk_grid_attach(GTK_GRID(grid),ws,1,0,1,1);
    l=gtk_label_new("Height:");hs=gtk_spin_button_new_with_range(8,25,1);gtk_spin_button_set_value(GTK_SPIN_BUTTON(hs),game_rows);gtk_grid_attach(GTK_GRID(grid),l,0,1,1,1);gtk_grid_attach(GTK_GRID(grid),hs,1,1,1,1);
    l=gtk_label_new("Mines:");ms=gtk_spin_button_new_with_range(1,999,1);gtk_spin_button_set_value(GTK_SPIN_BUTTON(ms),mine_count);gtk_grid_attach(GTK_GRID(grid),l,0,2,1,1);gtk_grid_attach(GTK_GRID(grid),ms,1,2,1,1);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(d))),grid,TRUE,TRUE,0);gtk_widget_show_all(d);r=gtk_dialog_run(GTK_DIALOG(d));
    if(r==GTK_RESPONSE_OK){int c=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ws)),rr=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(hs)),m=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ms));if(m>=c*rr)m=c*rr-1;game_cols=c;game_rows=rr;mine_count=m;reset_game();if(!tablet_mode)lm_graphics_resize(game_width(),game_height());queue_draw();}
    gtk_widget_destroy(d);
}

static GtkWidget*make_menu_bar(GtkWidget*p)
{
    GtkWidget*bar=gtk_menu_bar_new(),*gi=gtk_menu_item_new_with_mnemonic("Game"),*hi=gtk_menu_item_new_with_mnemonic("Help"),*gm=gtk_menu_new(),*hm=gtk_menu_new(),*i;GtkAccelGroup*a=gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(p),a);gtk_menu_item_set_submenu(GTK_MENU_ITEM(gi),gm);gtk_menu_item_set_submenu(GTK_MENU_ITEM(hi),hm);gtk_menu_shell_append(GTK_MENU_SHELL(bar),gi);gtk_menu_shell_append(GTK_MENU_SHELL(bar),hi);
    i=gtk_menu_item_new_with_label("New");gtk_widget_add_accelerator(i,"activate",a,GDK_KEY_F2,0,GTK_ACCEL_VISIBLE);g_signal_connect(i,"activate",G_CALLBACK(on_new_game),NULL);gtk_menu_shell_append(GTK_MENU_SHELL(gm),i);gtk_menu_shell_append(GTK_MENU_SHELL(gm),gtk_separator_menu_item_new());
    beginner_item=gtk_radio_menu_item_new(NULL);gtk_menu_item_set_label(GTK_MENU_ITEM(beginner_item),"Beginner");intermediate_item=gtk_radio_menu_item_new_from_widget(GTK_RADIO_MENU_ITEM(beginner_item));gtk_menu_item_set_label(GTK_MENU_ITEM(intermediate_item),"Intermediate");expert_item=gtk_radio_menu_item_new_from_widget(GTK_RADIO_MENU_ITEM(beginner_item));gtk_menu_item_set_label(GTK_MENU_ITEM(expert_item),"Expert");g_signal_connect(beginner_item,"toggled",G_CALLBACK(on_beginner),NULL);g_signal_connect(intermediate_item,"toggled",G_CALLBACK(on_intermediate),NULL);g_signal_connect(expert_item,"toggled",G_CALLBACK(on_expert),NULL);gtk_menu_shell_append(GTK_MENU_SHELL(gm),beginner_item);gtk_menu_shell_append(GTK_MENU_SHELL(gm),intermediate_item);gtk_menu_shell_append(GTK_MENU_SHELL(gm),expert_item);
    i=gtk_menu_item_new_with_label("Custom...");g_signal_connect(i,"activate",G_CALLBACK(on_custom),p);gtk_menu_shell_append(GTK_MENU_SHELL(gm),i);gtk_menu_shell_append(GTK_MENU_SHELL(gm),gtk_separator_menu_item_new());
    i=gtk_check_menu_item_new_with_label("Marks (?)");gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(i),TRUE);g_signal_connect(i,"toggled",G_CALLBACK(on_marks),NULL);gtk_menu_shell_append(GTK_MENU_SHELL(gm),i);
    tablet_item=gtk_check_menu_item_new_with_label("Tablet Mode");gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tablet_item),tablet_mode);g_signal_connect(tablet_item,"toggled",G_CALLBACK(on_tablet_mode),NULL);gtk_menu_shell_append(GTK_MENU_SHELL(gm),tablet_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(gm),gtk_separator_menu_item_new());
    i=gtk_menu_item_new_with_label("Exit");g_signal_connect(i,"activate",G_CALLBACK(on_exit_game),NULL);gtk_menu_shell_append(GTK_MENU_SHELL(gm),i);
    i=gtk_menu_item_new_with_label("Contents");gtk_widget_add_accelerator(i,"activate",a,GDK_KEY_F1,0,GTK_ACCEL_VISIBLE);g_signal_connect(i,"activate",G_CALLBACK(on_help),p);gtk_menu_shell_append(GTK_MENU_SHELL(hm),i);i=gtk_menu_item_new_with_label("Using Help");g_signal_connect(i,"activate",G_CALLBACK(on_help),p);gtk_menu_shell_append(GTK_MENU_SHELL(hm),i);gtk_menu_shell_append(GTK_MENU_SHELL(hm),gtk_separator_menu_item_new());i=gtk_menu_item_new_with_label("About Minesweeper...");g_signal_connect(i,"activate",G_CALLBACK(on_about),p);gtk_menu_shell_append(GTK_MENU_SHELL(hm),i);
    return bar;
}

static gboolean on_button_press(GtkWidget*w,GdkEventButton*e,gpointer d){int x=(int)e->x,y=(int)e->y;(void)w;(void)d;if(e->button==GDK_BUTTON_PRIMARY){int fx=(game_width()-BUTTON_W)/2;if(x>=fx&&x<fx+BUTTON_W&&y>=TOP_LED_Y&&y<TOP_LED_Y+BUTTON_H){face_pressed=1;queue_draw();return TRUE;}}if(y>=GRID_Y&&x>=GRID_X&&x<GRID_X+game_cols*TILE&&y<GRID_Y+game_rows*TILE){int cx=(x-GRID_X)/TILE,cy=(y-GRID_Y)/TILE;if(e->button==GDK_BUTTON_PRIMARY)open_cell(cx,cy);else if(e->button==GDK_BUTTON_SECONDARY)toggle_mark(cx,cy);queue_draw();return TRUE;}return FALSE;}
static gboolean on_button_release(GtkWidget*w,GdkEventButton*e,gpointer d){int x=(int)e->x,y=(int)e->y,fx=(game_width()-BUTTON_W)/2;(void)w;(void)d;if(e->button==GDK_BUTTON_PRIMARY&&face_pressed){if(x>=fx&&x<fx+BUTTON_W&&y>=TOP_LED_Y&&y<TOP_LED_Y+BUTTON_H)reset_game();face_pressed=0;queue_draw();return TRUE;}return FALSE;}
static gboolean on_timer(gpointer d){(void)d;if(!game_over)queue_draw();return G_SOURCE_CONTINUE;}

int main(int argc,char**argv)
{
    GtkWidget*box,*bar,*da;GtkCssProvider*css;GError *icon_error=NULL;
    gtk_init(&argc,&argv);srand((unsigned)time(NULL));
    if(!lm_graphics_init(game_width(),game_height(),"Minesweeper"))return 1;
    window_widget=lm_graphics_window();drawing_area_widget=lm_graphics_drawing_area();da=drawing_area_widget;
    gtk_window_set_icon_from_file(GTK_WINDOW(window_widget),"winmine-1.png",&icon_error);
    if(icon_error){fprintf(stderr,"LinMine: cannot load window icon: %s\n",icon_error->message);g_error_free(icon_error);icon_error=NULL;}
    box=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);bar=make_menu_bar(window_widget);gtk_box_pack_start(GTK_BOX(box),bar,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(box),da,FALSE,FALSE,0);gtk_container_add(GTK_CONTAINER(window_widget),box);
    g_signal_connect(window_widget,"destroy",G_CALLBACK(on_window_destroy),NULL);
    gtk_widget_add_events(da,GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);g_signal_connect(da,"draw",G_CALLBACK(on_draw),NULL);g_signal_connect(da,"button-press-event",G_CALLBACK(on_button_press),NULL);g_signal_connect(da,"button-release-event",G_CALLBACK(on_button_release),NULL);
    css=gtk_css_provider_new();gtk_css_provider_load_from_data(css,"menubar { background: #c0c0c0; padding: 0; color: #000000; } menubar menuitem { color: #000000; } menu { background: #c0c0c0; color: #000000; } menu menuitem { color: #000000; padding: 3px 18px 3px 6px; }",-1,NULL);gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),GTK_STYLE_PROVIDER(css),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);g_object_unref(css);
    if(!lm_graphics_load_assets("../winmine/bmp",1)){fprintf(stderr,"LinMine: failed to load WinMine BMP assets\n");lm_graphics_shutdown();return 1;}
    reset_game();gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(beginner_item),TRUE);g_timeout_add_seconds(1,on_timer,NULL);gtk_widget_show_all(window_widget);gtk_widget_grab_focus(da);gtk_main();lm_graphics_shutdown();return 0;
}