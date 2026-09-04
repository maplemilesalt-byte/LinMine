#include "gfx.h"
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdio.h>

#define LM_SHEET_COUNT 3
#define LM_BLOCK_W 16
#define LM_BLOCK_H 16
#define LM_BLOCK_COUNT 16
#define LM_LED_W 13
#define LM_LED_H 23
#define LM_LED_COUNT 12
#define LM_BUTTON_W 24
#define LM_BUTTON_H 24
#define LM_BUTTON_COUNT 5

static GtkWidget *lm_window;
static GtkWidget *lm_drawing_area;
static GdkPixbuf *lm_sheets[LM_SHEET_COUNT];

static const char *lm_path(char *dst, size_t size, const char *dir, const char *name)
{
    int n;
    if (!dir || !name || size == 0)
        return NULL;
    n = snprintf(dst, size, "%s/%s", dir, name);
    if (n < 0 || (size_t)n >= size)
        return NULL;
    return dst;
}

static GdkPixbuf *lm_load_bmp(const char *dir, const char *name)
{
    char path[1024];
    GError *error = NULL;
    GdkPixbuf *pixbuf;

    if (!lm_path(path, sizeof(path), dir, name))
        return NULL;

    pixbuf = gdk_pixbuf_new_from_file(path, &error);
    if (!pixbuf) {
        fprintf(stderr, "LinMine: cannot load %s: %s\n",
                path, error ? error->message : "unknown error");
        if (error)
            g_error_free(error);
        return NULL;
    }
    return pixbuf;
}

int lm_graphics_init(int width, int height, const char *title)
{
    lm_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!lm_window)
        return 0;

    gtk_window_set_title(GTK_WINDOW(lm_window), title ? title : "Minesweeper");
    gtk_window_set_resizable(GTK_WINDOW(lm_window), TRUE);

    lm_drawing_area = gtk_drawing_area_new();
    if (!lm_drawing_area)
        return 0;

    /* These are borrowed pointers, and should become NULL automatically
       when GTK destroys the widgets. */
    g_object_add_weak_pointer(G_OBJECT(lm_window), (gpointer *)&lm_window);
    g_object_add_weak_pointer(G_OBJECT(lm_drawing_area), (gpointer *)&lm_drawing_area);

    gtk_widget_set_size_request(lm_drawing_area, width, height);
    gtk_widget_set_can_focus(lm_drawing_area, TRUE);
    return 1;
}

void lm_graphics_shutdown(void)
{
    lm_graphics_free_assets();
    lm_drawing_area = NULL;
    lm_window = NULL;
}

GtkWidget *lm_graphics_window(void)
{
    return lm_window;
}

GtkWidget *lm_graphics_drawing_area(void)
{
    return lm_drawing_area;
}

void lm_graphics_resize(int width, int height)
{
    if (!lm_drawing_area || !GTK_IS_WIDGET(lm_drawing_area))
        return;

    gtk_widget_set_size_request(lm_drawing_area, width, height);
    gtk_widget_queue_resize(lm_drawing_area);
}

int lm_graphics_load_assets(const char *dir, int color)
{
    lm_graphics_free_assets();
    lm_sheets[LM_SHEET_BLOCKS] = lm_load_bmp(dir, color ? "blocks.bmp" : "blocksbw.bmp");
    lm_sheets[LM_SHEET_LED] = lm_load_bmp(dir, color ? "led.bmp" : "ledbw.bmp");
    lm_sheets[LM_SHEET_BUTTON] = lm_load_bmp(dir, color ? "button.bmp" : "buttonbw.bmp");

    if (!lm_sheets[LM_SHEET_BLOCKS] || !lm_sheets[LM_SHEET_LED] ||
        !lm_sheets[LM_SHEET_BUTTON]) {
        lm_graphics_free_assets();
        return 0;
    }
    return 1;
}

void lm_graphics_free_assets(void)
{
    int i;
    for (i = 0; i < LM_SHEET_COUNT; ++i) {
        if (lm_sheets[i])
            g_object_unref(lm_sheets[i]);
        lm_sheets[i] = NULL;
    }
}

void lm_graphics_queue_draw(void)
{
    if (lm_drawing_area && GTK_IS_WIDGET(lm_drawing_area))
        gtk_widget_queue_draw(lm_drawing_area);
}

static void lm_set_source(cairo_t *cr, unsigned int pixel)
{
    double alpha = ((pixel >> 24) & 0xff) / 255.0;
    if (alpha == 0.0)
        alpha = 1.0;
    cairo_set_source_rgba(cr,
                          ((pixel >> 16) & 0xff) / 255.0,
                          ((pixel >> 8) & 0xff) / 255.0,
                          (pixel & 0xff) / 255.0,
                          alpha);
}

void lm_graphics_clear(cairo_t *cr, unsigned int pixel, int width, int height)
{
    (void)width;
    (void)height;
    lm_set_source(cr, pixel);
    cairo_paint(cr);
}

void lm_graphics_fill_rect(cairo_t *cr, LMRect rect, unsigned int pixel)
{
    lm_set_source(cr, pixel);
    cairo_rectangle(cr, rect.x, rect.y, rect.width, rect.height);
    cairo_fill(cr);
}

void lm_graphics_draw_rect(cairo_t *cr, LMRect rect, unsigned int pixel)
{
    lm_set_source(cr, pixel);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, rect.x + 0.5, rect.y + 0.5,
                    rect.width - 1.0, rect.height - 1.0);
    cairo_stroke(cr);
}

void lm_graphics_draw_sprite(cairo_t *cr, LMSpriteSheet sheet, int index,
                             int x, int y, int width, int height)
{
    static const int widths[LM_SHEET_COUNT] = {
        LM_BLOCK_W, LM_LED_W, LM_BUTTON_W
    };
    static const int heights[LM_SHEET_COUNT] = {
        LM_BLOCK_H, LM_LED_H, LM_BUTTON_H
    };
    static const int counts[LM_SHEET_COUNT] = {
        LM_BLOCK_COUNT, LM_LED_COUNT, LM_BUTTON_COUNT
    };
    GdkPixbuf *pixbuf;
    int src_y;
    double sx, sy;

    if (!cr || sheet < 0 || sheet >= LM_SHEET_COUNT || index < 0 ||
        index >= counts[sheet] || width <= 0 || height <= 0)
        return;

    pixbuf = lm_sheets[sheet];
    if (!pixbuf)
        return;

    src_y = index * heights[sheet];
    sx = (double)width / widths[sheet];
    sy = (double)height / heights[sheet];

    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_scale(cr, sx, sy);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, -src_y);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
    cairo_rectangle(cr, 0, 0, widths[sheet], heights[sheet]);
    cairo_fill(cr);
    cairo_restore(cr);
}
