#ifndef UI_H
#define UI_H

#include <lvgl.h>
extern lv_obj_t *fileLabel;
extern lv_style_t style_bg;       // Background
extern lv_style_t style_panel;    // White browser area
extern lv_style_t style_header;   // Path header above browser table
extern lv_style_t style_table;    // Browser table
extern lv_style_t style_pressed;  // ? not used ?

void ui_init(void);
void ui_update(void);
void enable_print_btn(void);

#endif // UI_H