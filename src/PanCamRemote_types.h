#include <Arduino.h>

// message structure for sensor values
typedef struct output_image {
   int16_t focus;
   int16_t iris;
   int16_t zoom;
   bool autoIris;
   bool record;
} output_image;

// message structure for sensor values
typedef struct message {
   int16_t focus;
   int16_t focus_ll;    // lower limit for fine adjusting
   int16_t focus_ul;    // upper limit for fine adjusting
   int16_t iris;
   int16_t zoom;
   bool autoIris;
   bool record;
   bool prog_mode;      // programming mode for presets enabled
   bool preset1;
   bool preset2;
   bool preset3;
} message;