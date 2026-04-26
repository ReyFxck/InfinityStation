#ifndef SELECT_MENU_PAGES_INTERNAL_H
#define SELECT_MENU_PAGES_INTERNAL_H

#include "pages.h"
#include "video.h"
#include "theme.h"
#include "font.h"

unsigned select_menu_pages_center_x_for_text(const char *text);
unsigned select_menu_pages_text_width_px(const char *text);
void select_menu_pages_draw_header_line(unsigned y, uint16_t color);
void select_menu_pages_draw_footer_line(unsigned y, uint16_t color);
void select_menu_pages_draw_icon_cross(unsigned x, unsigned y);
void select_menu_pages_draw_icon_circle(unsigned x, unsigned y);
void select_menu_pages_draw_icon_select(unsigned x, unsigned y);
void select_menu_pages_draw_centered_menu_item(unsigned y, const char *label, int selected);
void select_menu_pages_draw_footer_actions(const char *label0, const char *label1, const char *label2);
void select_menu_pages_draw_frame_limit_rail(unsigned y, int current_mode, int selected);

void select_menu_pages_draw_main_page(const select_menu_state_t *state);
void select_menu_pages_draw_video_page(const select_menu_state_t *state);
void select_menu_pages_draw_display_page(const select_menu_view_state_t *view_state);
void select_menu_pages_draw_aspect_page(const select_menu_state_t *state);
void select_menu_pages_draw_game_page(const select_menu_state_t *state);

#endif
