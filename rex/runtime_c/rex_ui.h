#ifndef REX_UI_H
#define REX_UI_H

#include "rex_rt.h"

#include <stdint.h>

#define REX_UI_TEXT_MAX 1024
#define REX_UI_KEY_MAX 512
#define REX_UI_MOUSE_BUTTONS 5

#define REX_MOUSE_LEFT 0
#define REX_MOUSE_RIGHT 1
#define REX_MOUSE_MIDDLE 2
#define REX_MOUSE_BUTTON_4 3
#define REX_MOUSE_BUTTON_5 4

#define REX_KEY_UNKNOWN -1
#define REX_KEY_SPACE 32
#define REX_KEY_APOSTROPHE 39
#define REX_KEY_COMMA 44
#define REX_KEY_MINUS 45
#define REX_KEY_PERIOD 46
#define REX_KEY_SLASH 47
#define REX_KEY_0 48
#define REX_KEY_1 49
#define REX_KEY_2 50
#define REX_KEY_3 51
#define REX_KEY_4 52
#define REX_KEY_5 53
#define REX_KEY_6 54
#define REX_KEY_7 55
#define REX_KEY_8 56
#define REX_KEY_9 57
#define REX_KEY_SEMICOLON 59
#define REX_KEY_EQUAL 61
#define REX_KEY_A 65
#define REX_KEY_B 66
#define REX_KEY_C 67
#define REX_KEY_D 68
#define REX_KEY_E 69
#define REX_KEY_F 70
#define REX_KEY_G 71
#define REX_KEY_H 72
#define REX_KEY_I 73
#define REX_KEY_J 74
#define REX_KEY_K 75
#define REX_KEY_L 76
#define REX_KEY_M 77
#define REX_KEY_N 78
#define REX_KEY_O 79
#define REX_KEY_P 80
#define REX_KEY_Q 81
#define REX_KEY_R 82
#define REX_KEY_S 83
#define REX_KEY_T 84
#define REX_KEY_U 85
#define REX_KEY_V 86
#define REX_KEY_W 87
#define REX_KEY_X 88
#define REX_KEY_Y 89
#define REX_KEY_Z 90
#define REX_KEY_LEFT_BRACKET 91
#define REX_KEY_BACKSLASH 92
#define REX_KEY_RIGHT_BRACKET 93
#define REX_KEY_GRAVE_ACCENT 96
#define REX_KEY_ESCAPE 256
#define REX_KEY_ENTER 257
#define REX_KEY_TAB 258
#define REX_KEY_BACKSPACE 259
#define REX_KEY_INSERT 260
#define REX_KEY_DELETE 261
#define REX_KEY_RIGHT 262
#define REX_KEY_LEFT 263
#define REX_KEY_DOWN 264
#define REX_KEY_UP 265
#define REX_KEY_PAGE_UP 266
#define REX_KEY_PAGE_DOWN 267
#define REX_KEY_HOME 268
#define REX_KEY_END 269
#define REX_KEY_CAPS_LOCK 280
#define REX_KEY_SCROLL_LOCK 281
#define REX_KEY_NUM_LOCK 282
#define REX_KEY_PRINT_SCREEN 283
#define REX_KEY_PAUSE 284
#define REX_KEY_F1 290
#define REX_KEY_F2 291
#define REX_KEY_F3 292
#define REX_KEY_F4 293
#define REX_KEY_F5 294
#define REX_KEY_F6 295
#define REX_KEY_F7 296
#define REX_KEY_F8 297
#define REX_KEY_F9 298
#define REX_KEY_F10 299
#define REX_KEY_F11 300
#define REX_KEY_F12 301
#define REX_KEY_F13 302
#define REX_KEY_F14 303
#define REX_KEY_F15 304
#define REX_KEY_F16 305
#define REX_KEY_F17 306
#define REX_KEY_F18 307
#define REX_KEY_F19 308
#define REX_KEY_F20 309
#define REX_KEY_F21 310
#define REX_KEY_F22 311
#define REX_KEY_F23 312
#define REX_KEY_F24 313
#define REX_KEY_F25 314
#define REX_KEY_KP_0 320
#define REX_KEY_KP_1 321
#define REX_KEY_KP_2 322
#define REX_KEY_KP_3 323
#define REX_KEY_KP_4 324
#define REX_KEY_KP_5 325
#define REX_KEY_KP_6 326
#define REX_KEY_KP_7 327
#define REX_KEY_KP_8 328
#define REX_KEY_KP_9 329
#define REX_KEY_KP_DECIMAL 330
#define REX_KEY_KP_DIVIDE 331
#define REX_KEY_KP_MULTIPLY 332
#define REX_KEY_KP_SUBTRACT 333
#define REX_KEY_KP_ADD 334
#define REX_KEY_KP_ENTER 335
#define REX_KEY_KP_EQUAL 336
#define REX_KEY_LEFT_SHIFT 340
#define REX_KEY_LEFT_CONTROL 341
#define REX_KEY_LEFT_ALT 342
#define REX_KEY_LEFT_SUPER 343
#define REX_KEY_RIGHT_SHIFT 344
#define REX_KEY_RIGHT_CONTROL 345
#define REX_KEY_RIGHT_ALT 346
#define REX_KEY_RIGHT_SUPER 347
#define REX_KEY_MENU 348

typedef struct RexUIPlatformInput {
  int mouse_x;
  int mouse_y;
  int mouse_down;
  int mouse_buttons[REX_UI_MOUSE_BUTTONS];
  int scroll_x;
  int scroll_y;
  int key_states[REX_UI_KEY_MAX];
  int key_tab;
  int key_enter;
  int key_space;
  int key_up;
  int key_down;
  int key_backspace;
  int key_delete;
  int key_left;
  int key_right;
  int key_home;
  int key_end;
  int key_ctrl;
  int key_shift;
  int key_copy;
  int key_paste;
  int key_cut;
  int key_select_all;
  int width;
  int height;
  float dpi_scale;
  char text[REX_UI_TEXT_MAX];
  int text_len;
  int closed;
  int redraw;
} RexUIPlatformInput;

int rex_ui_platform_init(const char* title, int width, int height);
void rex_ui_platform_shutdown(void);
int rex_ui_platform_poll(RexUIPlatformInput* input);
void rex_ui_platform_present(const uint32_t* pixels, int width, int height);
int rex_ui_platform_get_clipboard(char* buffer, int capacity);
void rex_ui_platform_set_clipboard(const char* text);
void rex_ui_platform_set_titlebar_dark(int dark);

#endif
