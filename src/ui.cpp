#include <Arduino.h>

#include <config.h>

#include <LovyanGFX.hpp>
#include <LGFX_ESP32_2432S028R_V3.hpp>
#include <cstdio>
#include <lvgl.h>
#include <globals.h>
#include <marlin.h>
#include <sd_card.h>
#include <SdFat.h>

//extern Marlin marlin;
//extern GcodeSender sender;
//extern FsFile printFile;

// Display hardware (panel, touch, backlight)
static LGFX lcd;  

/* Display dimensions */
#define SCREEN_W 320
#define SCREEN_H 240
#define TAB_W     76

/* --- LVGL setup --- */

static lv_color_t lvgl_buffer[LVGL_BUFFER_SIZE];  
 
// LGFX friendly callback to write data to the lcd
void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px) {  
  uint32_t w = area->x2 - area->x1 + 1;  
  uint32_t h = area->y2 - area->y1 + 1;  
  lcd.startWrite();  
  lcd.pushImage(area->x1, area->y1, w, h, (lgfx::rgb565_t*)px);  
  lcd.endWrite();  
  lv_display_flush_ready(disp);  
}  
  
// Touchscreen callback
void read_cb(lv_indev_t* indev, lv_indev_data_t* data) {  
  uint16_t x, y;  
  if (lcd.getTouch(&x, &y)) {  
    data->state   = LV_INDEV_STATE_PRESSED;  
    data->point.x = x;  
    data->point.y = y;  
  } else {  
    data->state = LV_INDEV_STATE_RELEASED;  
  }  
}  

// Heartbeat - controls LVGL refresh
static uint32_t ui_tick(void) {
  return millis();
}


/* --- UI --- */

// Styles - Set up the styles for all screens and widgets
//#define COLOR_BG    lv_color_hex(0xf2f1f6)   // Sidebar + window background
//#define COLOR_PANEL lv_color_white()         // Browser area

lv_style_t style_bg;       // Background
lv_style_t style_panel;    // White browser area
lv_style_t style_header;   // Path header above browser table
lv_style_t style_table;    // Browser table
lv_style_t style_pressed;  // ? not used ?

static void style_init(void)
{
    // Ensure styles are only created once
    static bool inited = false;
    if(inited) return;
    inited = true;

    lv_style_init(&style_bg);
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
    lv_style_set_bg_color(&style_bg, COLOR_BG);
    lv_style_set_pad_all(&style_bg, 0);
    lv_style_set_pad_gap(&style_bg, 0);
    lv_style_set_radius(&style_bg, 0);
    lv_style_set_border_width(&style_bg, 0);

    lv_style_init(&style_panel);
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_bg_color(&style_panel, COLOR_PANEL);
    lv_style_set_pad_all(&style_panel, 0);
    lv_style_set_pad_gap(&style_panel, 0);
    lv_style_set_radius(&style_panel, 0);
    lv_style_set_border_width(&style_panel, 0);

    lv_style_init(&style_header);
    lv_style_set_bg_opa(&style_header, LV_OPA_COVER);
    lv_style_set_bg_color(&style_header, COLOR_PANEL);
    lv_style_set_text_color(&style_header, lv_palette_darken(LV_PALETTE_GREY, 4));
    lv_style_set_pad_all(&style_header, 6);
    lv_style_set_radius(&style_header, 0);
    lv_style_set_border_width(&style_header, 1);
    lv_style_set_border_color(&style_header, lv_palette_lighten(LV_PALETTE_GREY, 1));
    lv_style_set_border_side(&style_header, LV_BORDER_SIDE_BOTTOM);

    lv_style_init(&style_table);
    lv_style_set_bg_opa(&style_table, LV_OPA_COVER);
    lv_style_set_bg_color(&style_table, COLOR_PANEL);
    lv_style_set_pad_all(&style_table, 0);
    lv_style_set_radius(&style_table, 0);
    lv_style_set_border_width(&style_table, 0);

    lv_style_init(&style_pressed);
    lv_style_set_bg_opa(&style_pressed, LV_OPA_20);
    lv_style_set_bg_color(&style_pressed, lv_palette_main(LV_PALETTE_GREY));
}

// General button helpers
static lv_obj_t *make_button(lv_obj_t *parent, const char *text, int x, int y,
                             int w, int h, lv_event_cb_t callback,
                             void * cb_data) {

  // Create button w/ size & position                              
  lv_obj_t *btn = lv_button_create(parent);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_size(btn, w, h);

  // Add callback
  if (callback != NULL) {
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, cb_data);
  }

  // Create label
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

/* --- Print Tab --- */

// Filename label
lv_obj_t *fileLabel;

// Print/Pause/Stop buttons
static lv_obj_t *print_btn;
static lv_obj_t *pause_btn;
static lv_obj_t *stop_btn;

typedef enum {  PRINT,
                PAUSE,
                STOP
} print_cmd_t;

static void print_file_cb(lv_event_t *e) {

  lv_obj_t * widget;
  lv_obj_t * page;

  widget = (lv_obj_t*)lv_event_get_target(e);
  page = lv_obj_get_parent(widget);
  sd_file_browser(page);
}

static void print_cb(lv_event_t *e) {
  int cmd = (int)lv_event_get_user_data(e);

  lv_obj_set_disabled(print_btn, false);
  lv_obj_set_disabled(pause_btn, false);
  lv_obj_set_disabled(stop_btn, false);

  switch (cmd) {

  case PRINT:

    lv_obj_set_disabled(print_btn, true);

    // If GCODE sender is paused, resume it
    if (sender.isPaused())
    {
      sender.resume();
      return;
    }

    // Start GCODE sender
    // fileLabel contains the file to print
    if (!printFile.open(lv_label_get_text(fileLabel)))
    {
      LOGN("Print - Error opening: ");
      LOG(lv_label_get_text(fileLabel));
      lv_obj_set_disabled(print_btn, false);
    }
    // Reset line number 
    marlin.sendRaw("M110 N0");
    sender.loadFile(&printFile);
    sender.start();
    break;

  case PAUSE:

    lv_obj_set_disabled(pause_btn, true);

    // Pause GCODE sender
    sender.pause();
    break;

  case STOP:

    lv_obj_set_disabled(pause_btn, true);
    lv_obj_set_disabled(stop_btn, true);

    // Stop GCODE sender
    sender.stop();
    printFile.close();
  }
}

void enable_print_btn(void) {
  lv_obj_set_disabled(print_btn, false);
}

static void create_printTab(lv_obj_t *page) {
  const int size = 14; // % of display width
  lv_obj_t *obj;

  obj = make_button(page, "File:", lv_pct(0 * size), lv_pct(0 * size),
                     lv_pct(1.5 * size), lv_pct(1 * size), print_file_cb, NULL);

  fileLabel = lv_label_create(page);
  lv_label_set_text(fileLabel, "<none>");
  lv_obj_align_to(fileLabel, obj, LV_ALIGN_OUT_LEFT_MID, 0, 0);
  lv_obj_set_x(fileLabel, lv_pct(2 * size));

  print_btn = make_button(page, LV_SYMBOL_PLAY "PRINT", lv_pct(0 * size), lv_pct(2 * size),
              lv_pct(33), lv_pct(1 * size), print_cb, (void *)PRINT);
  lv_obj_set_disabled(print_btn, true);

  pause_btn = make_button(page, LV_SYMBOL_PAUSE "PAUSE", 0, 0, lv_pct(33), lv_pct(1 * size), print_cb, (void *)PAUSE);
  lv_obj_set_disabled(pause_btn, true);
  lv_obj_align_to(pause_btn, print_btn, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

  stop_btn = make_button(page, LV_SYMBOL_STOP "STOP", 0, 0, lv_pct(33), lv_pct(1 * size), print_cb, (void *)STOP);
  lv_obj_set_disabled(stop_btn, true);
  lv_obj_align_to(stop_btn, pause_btn, LV_ALIGN_OUT_RIGHT_MID, 2, 0);


}

/* --- Manual Tab --- */

// Movement amount buttons
static lv_obj_t *mm1_btn;
static lv_obj_t *mm10_btn;
static lv_obj_t *mm50_btn;

// Default movement is 10mm
static int manual_mm = 10;

typedef enum {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  Z_UP,
  Z_DOWN,
  EXTRUDE,
  RETRACT,
  XY_HOME,
  Z_HOME,
  MM1,
  MM10,
  MM50
} manual_cmd_t;

const char *manual_cmd[] = {
    "G0 X0.0 Y-%d.0 Z0.0 F3000", // UP
    "G0 X0.0 Y%d.0 Z0.0 F3000",  // DOWN
    "G0 X-%d.0 Y0.0 Z0.0 F3000", // LEFT
    "G0 X%d.0 Y0.0 Z0.0 F3000",  // RIGHT
    "G0 X0.0 Y0.0 Z%d.0 F3000",  // Z_UP
    "G0 X0.0 Y0.0 Z-%d.0 F3000", // Z_DOWN
    "G1 E%d F75",                // EXTRUDE
    "G1 E-%d F75",               // RETRACT
    "G28 X Y",                   // XY_HOME
    "G28 Z",                     // Z_HOME
    "",                          // MM1
    "",                          // MM10
    ""                           // MM50
};

static void manual_cb(lv_event_t *e) {
  int cmd = (int)lv_event_get_user_data(e);
  char buff[30];

  if (cmd <= Z_HOME) {
    switch (cmd) {
    case XY_HOME:
    case Z_HOME:
      marlin.sendRaw((char *)manual_cmd[cmd]);
      break;

    default:
      snprintf(buff, sizeof(buff), manual_cmd[cmd], manual_mm);
      marlin.sendRaw("G91");
      marlin.sendRaw(buff);
      marlin.sendRaw("G90");
    }
  }
}

static void manual_mm_cb(lv_event_t *e) {
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

  lv_obj_set_state(mm1_btn, LV_STATE_CHECKED, false);
  lv_obj_set_state(mm10_btn, LV_STATE_CHECKED, false);
  lv_obj_set_state(mm50_btn, LV_STATE_CHECKED, false);

  lv_obj_set_state(btn, LV_STATE_CHECKED, true);
  manual_mm = (int)lv_event_get_user_data(e);
}

static void create_manualTab(lv_obj_t *page) {

  const int size = 14; // % of display width
  lv_obj_t *obj;

  // X/Y Move: 3x3 matrix
  make_button(page, LV_SYMBOL_UP, lv_pct(2 * size), lv_pct(1 * size),
              lv_pct(size), lv_pct(size), manual_cb, (void *)UP);
  make_button(page, "X/Y", lv_pct(2 * size), lv_pct(2 * size), lv_pct(size),
              lv_pct(size), manual_cb, (void *)XY_HOME);
  make_button(page, LV_SYMBOL_DOWN, lv_pct(2 * size), lv_pct(3 * size),
              lv_pct(size), lv_pct(size), manual_cb, (void *)DOWN);
  make_button(page, LV_SYMBOL_LEFT, lv_pct(1 * size), lv_pct(2 * size),
              lv_pct(size), lv_pct(size), manual_cb, (void *)LEFT);
  make_button(page, LV_SYMBOL_RIGHT, lv_pct(3 * size), lv_pct(2 * size),
              lv_pct(size), lv_pct(size), manual_cb, (void *)RIGHT);

  // Z Move: 1x3 matrix
  make_button(page, LV_SYMBOL_UP, lv_pct(5 * size), lv_pct(1 * size),
              lv_pct(size), lv_pct(size), manual_cb, (void *)Z_UP);
  make_button(page, "Z", lv_pct(5 * size), lv_pct(2 * size), lv_pct(size),
              lv_pct(size), manual_cb, (void *)Z_HOME);
  make_button(page, LV_SYMBOL_DOWN, lv_pct(5 * size), lv_pct(3 * size),
              lv_pct(size), lv_pct(size), manual_cb, (void *)Z_DOWN);

  // Extrude & Retract
  make_button(page, "Extrude", lv_pct(4.5 * size), lv_pct(4.45 * size),
              lv_pct(2 * size), lv_pct(size), manual_cb, (void *)EXTRUDE);
  make_button(page, "Retract", lv_pct(4.5 * size), lv_pct(5.55 * size),
              lv_pct(2 * size), lv_pct(size), manual_cb, (void *)RETRACT);

  // Step size: 3x1 button matrix, 1 always checked
  obj = make_button(page, "1", lv_pct(1 * size), lv_pct(5 * size), lv_pct(size),
                    lv_pct(size), manual_mm_cb, (void *)1);
  lv_obj_set_checkable(obj, true);
  mm1_btn = obj;

  obj = make_button(page, "10", lv_pct(2 * size), lv_pct(5 * size),
                    lv_pct(size), lv_pct(size), manual_mm_cb, (void *)10);
  lv_obj_set_checkable(obj, true);
  lv_obj_set_state(obj, LV_STATE_CHECKED, true);
  mm10_btn = obj;

  obj = make_button(page, "50", lv_pct(3 * size), lv_pct(5 * size),
                    lv_pct(size), lv_pct(size), manual_mm_cb, (void *)50);
  lv_obj_set_checkable(obj, true);
  mm50_btn = obj;
}

/* --- Prepare Tab --- */

// Temperature dropdown options
typedef enum {
  NOZZLE_OFF,
  ABS,
  PETG,
  PLA,
  PROBE,
  BED_OFF,
  C40,
  C60,
  C80,
  C100
} prep_cmd_t;

const char *prep_cmd[] = {
    "M104 S0 T0",   // Nozzle heater OFF
    "M104 S240 T0", // ABS Temp
    "M104 S230 T0", // PETG Temp
    "M104 S200 T0", // PLA Temp
    "M104 S160 T0", // Probe Temp
    "M140 S0",      // Bed heater OFF
    "M140 S40",     // Bed @40C
    "M140 S60",     // Bed @60C
    "M140 S80",     // Bed @80C
    "M140 S100",    // Bed @100C
};

//Status label
lv_obj_t *NozzleT;
lv_subject_t *temps;
char temps_buf[128];

static void prep_cmd_cb(lv_event_t *e) {
  int offset = (int)lv_event_get_user_data(e);
  lv_obj_t *dropdown = lv_event_get_target_obj(e);
  prep_cmd_t cmd = (prep_cmd_t)lv_dropdown_get_selected(dropdown);
  marlin.sendRaw((char *)prep_cmd[cmd + offset]);
  marlin.sendRaw("M105"); // Force temp reading
}

static void create_prepareTab(lv_obj_t *page) {
 const int size = 14; // % of display width
  lv_obj_t *obj1, *obj2;

  // Nozzle temp
  obj1 = lv_dropdown_create(page);
  lv_obj_set_pos(obj1, lv_pct(0.5 * size), lv_pct(5 * size));
  lv_obj_set_width(obj1, lv_pct(2.5 * size));
  lv_dropdown_set_dir(obj1, LV_DIR_TOP);
  lv_dropdown_set_options(obj1, "Off\nABS\nPETG\nPLA\nProbe");
  lv_obj_add_event_cb(obj1, prep_cmd_cb, LV_EVENT_VALUE_CHANGED, (void *)NOZZLE_OFF);
  obj2 = lv_dropdown_get_list(obj1);
  lv_obj_set_scrollable(obj2, false);

  obj2 = lv_label_create(page);
  lv_label_set_text(obj2, "Nozz Temp");
  lv_obj_align_to(obj2, obj1, LV_ALIGN_OUT_TOP_MID, 0, 0);

  // Bed temp
  obj1 = lv_dropdown_create(page);
  lv_obj_set_pos(obj1, lv_pct(4 * size), lv_pct(5 * size));
  lv_obj_set_width(obj1, lv_pct(2.5 * size));
  lv_dropdown_set_dir(obj1, LV_DIR_TOP);
  lv_dropdown_set_options(obj1, "Off\n40C\n60C\n80C\n100C");
  lv_obj_add_event_cb(obj1, prep_cmd_cb, LV_EVENT_VALUE_CHANGED, (void *)BED_OFF);
  obj2 = lv_dropdown_get_list(obj1);
  lv_obj_set_scrollable(obj2, false);

  obj2 = lv_label_create(page);
  lv_label_set_text(obj2, "Bed Temp");
  lv_obj_align_to(obj2, obj1, LV_ALIGN_OUT_TOP_MID, 0, 0);

  // Status line w/ nozzle and bed temperatures
  temps = lv_subject_create(LV_SUBJECT_TYPE_STRING);
  lv_subject_set_string_buffer_static(temps, temps_buf, NULL, sizeof(temps_buf));
  lv_subject_set_string(temps, "Hello");

  NozzleT = lv_label_create(page);
  lv_label_bind_text(NozzleT, temps, NULL);
  lv_obj_set_align(NozzleT, LV_ALIGN_BOTTOM_LEFT);

#if 0
  NozzleT = lv_label_create(page);
  lv_obj_align(NozzleT, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_label_set_text(NozzleT, "Nozzle: ---/--- ");
  
  BedT = lv_label_create(page);
  lv_obj_align_to(BedT, NozzleT, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  lv_label_set_text(BedT, "Bed: ---/--- ");
#endif // 0
}

/* --- Settings Tab --- */

void settings_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  marlin.resetHardware();
  marlin.resetState();
}

static void create_settingsTab(lv_obj_t *page) {
 const int size = 14; // % of display width
  lv_obj_t *obj;

  obj = make_button(page, "RESET Printer", 0,0,
                    lv_pct(3 * size), lv_pct(size), settings_cb, NULL);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
}

void ui_init(void) {

  /* --- LGFX (hardware) init --- */
  LOG("Initializing LGFX");
  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(255);

  /* --- LGVL (graphics library) init --- */
  LOG("Initializing LVGL");
  lv_init();

  // LCD setup
  lv_display_t *disp = lv_display_create(320, 240);
  lv_display_set_flush_cb(disp, flush_cb);
  lv_display_set_buffers(disp, lvgl_buffer, NULL, sizeof(lvgl_buffer),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Basic default colors so objects are visible
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x101820), 0);
  lv_obj_set_style_text_color(lv_screen_active(), lv_color_white(), 0);

  // Touchscreen setup
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, read_cb);

  // LVGL "heartbeat"
  lv_tick_set_cb(ui_tick); // Without this, nothing in LVGL updates

  LOG("Display hardware initialized");

  /* --- UI Creation --- */

  //Create styles for all tabs, buttons, etc
  style_init();

  // Main UI will be a Tabview object
  lv_obj_t *screen = lv_screen_active();

  lv_obj_set_size(screen, SCREEN_W, SCREEN_H);

  lv_obj_t *tabs = lv_tabview_create(screen);
  lv_obj_set_size(tabs, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(tabs, 0, 0);

  lv_tabview_set_tab_bar_position(tabs, LV_DIR_LEFT);
  lv_tabview_set_tab_bar_size(tabs, TAB_W);

  // Compact left navigation
  lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabs);
  lv_obj_set_style_pad_all(tab_bar, 2, 0);

  // Define tabs
  lv_obj_t *printTab = lv_tabview_add_tab(tabs, "Print");
  lv_obj_t *manualTab = lv_tabview_add_tab(tabs, "Manual\nControl");
  lv_obj_t *prepareTab = lv_tabview_add_tab(tabs, "Prepare");
  lv_obj_t *settingsTab = lv_tabview_add_tab(tabs, "settings");

  // No scrolling the tab bar or the tabs
  lv_obj_set_scrollable(lv_tabview_get_content(tabs), false);
  lv_obj_set_scrollable(printTab, false);
  lv_obj_set_scrollable(manualTab, false);
  lv_obj_set_scrollable(prepareTab, false);
  lv_obj_set_scrollable(settingsTab, false);

  // Eliminate the padding so the entire tab area can be used
  //lv_obj_set_style_pad_all(printTab, 0, 0);
  lv_obj_set_style_pad_all(manualTab, 0, 0);
  lv_obj_set_style_pad_all(prepareTab, 0, 0);
  lv_obj_set_style_pad_all(settingsTab, 0, 0);

  // Create tabs
  create_printTab(printTab);
  create_manualTab(manualTab);
  create_prepareTab(prepareTab);
  create_settingsTab(settingsTab);

  // Done!
  lv_tabview_set_active(tabs, 0, LV_ANIM_OFF);

  LOG("UI initialized");
}

// Call this every ~5ms to keep things running smoothly
void ui_update(void) { lv_timer_handler(); }
