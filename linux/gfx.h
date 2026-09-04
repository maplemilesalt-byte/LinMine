#ifndef LINMINE_LINUX_GFX_H
#define LINMINE_LINUX_GFX_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LMPoint { int x; int y; } LMPoint;
typedef struct LMRect { int x; int y; int width; int height; } LMRect;

typedef enum LMSpriteSheet {
    LM_SHEET_BLOCKS = 0,
    LM_SHEET_LED = 1,
    LM_SHEET_BUTTON = 2
} LMSpriteSheet;

int lm_graphics_init(int width, int height, const char *title);
void lm_graphics_shutdown(void);
GtkWidget *lm_graphics_window(void);
GtkWidget *lm_graphics_drawing_area(void);
int lm_graphics_load_assets(const char *dir, int color);
void lm_graphics_free_assets(void);
void lm_graphics_queue_draw(void);
void lm_graphics_clear(cairo_t *cr, unsigned int pixel, int width, int height);
void lm_graphics_fill_rect(cairo_t *cr, LMRect rect, unsigned int pixel);
void lm_graphics_draw_rect(cairo_t *cr, LMRect rect, unsigned int pixel);
void lm_graphics_draw_sprite(cairo_t *cr, LMSpriteSheet sheet, int index,
                             int x, int y, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
