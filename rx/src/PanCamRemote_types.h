#ifndef __PANCAMTYPES_H__
#define __PANCAMTYPES_H__

#include <Arduino.h>

// message structure for sensor values
typedef struct message {
   uint16_t focus;
   uint16_t iris;
   uint16_t zoom;
   uint16_t focus_ll;    // lower limit for fine adjusting
   uint16_t focus_ul;    // upper limit for fine adjusting
   bool autoIris;
   bool record;
   bool prog_mode;      // programming mode for presets enabled
   bool preset1;
   bool preset2;
   bool preset3;
} message;

enum lens_control_analog {FOCUS, IRIS, ZOOM, FOCUS_LL, FOCUS_UL};
enum lens_control_digital {AUTO_IRIS, RECORD, PROG_MODE, PRESET1, PRESET2, PRESET3};
enum protocolType {INIT, WS, ESP_NOW, HTTP};

#endif