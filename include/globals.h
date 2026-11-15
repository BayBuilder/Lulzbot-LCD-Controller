#ifndef GLOBALS_H
#define GLOBALS_H

#include <marlin.h>
#include <SdFat.h>
#include <lvgl.h>

extern Marlin marlin;
extern GcodeSender sender;

extern FsFile printFile;

extern lv_obj_t *NozzleT;
extern lv_obj_t *BedT;

extern lv_subject_t * temps;
extern char temps_buf[128];

extern char NozzTemp[7];
extern char NozzTarget[7];
extern char BedTemp[7];
extern char BedTarget[7];

#endif // GLOBALS_H