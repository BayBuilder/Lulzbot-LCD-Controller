#ifndef SD_CARD_H
#define SD_CARD_H

#include <lvgl.h>

// File to print
extern char selected_file[SD_MAX_PATH_LEN];

bool sd_init(void);
void sd_file_browser(lv_obj_t * page);

#endif // SD_CARD_H
