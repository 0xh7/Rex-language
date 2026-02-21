#if defined(__linux__)

#include "rex_ui.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <string.h>

#ifndef Button6
#define Button6 6
#endif

#ifndef Button7
#define Button7 7
#endif

static Display* ui_display = NULL;
static Window ui_window = 0;
static GC ui_gc;
static Atom ui_wm_delete;
static int ui_running = 0;
static int ui_mouse_x = 0;
static int ui_mouse_y = 0;
static int ui_mouse_down = 0;
static char ui_text[REX_UI_TEXT_MAX];
static int ui_text_len = 0;
static XImage* ui_image = NULL;
static int ui_img_w = 0;
static int ui_img_h = 0;
static int ui_img_owns = 0;
static int ui_scroll_x = 0;
static int ui_scroll_y = 0;
static int ui_key_states[REX_UI_KEY_MAX];
static int ui_mouse_buttons[REX_UI_MOUSE_BUTTONS];
static int ui_key_tab = 0;
static int ui_key_enter = 0;
static int ui_key_space = 0;
static int ui_key_up = 0;
static int ui_key_down = 0;
static int ui_key_backspace = 0;
static int ui_key_delete = 0;
static int ui_key_left = 0;
static int ui_key_right = 0;
static int ui_key_home = 0;
static int ui_key_end = 0;
static int ui_key_ctrl = 0;
static int ui_key_shift = 0;
static int ui_key_copy = 0;
static int ui_key_paste = 0;
static int ui_key_cut = 0;
static int ui_key_select_all = 0;
static int ui_redraw = 0;
static int ui_width = 0;
static int ui_height = 0;
static float ui_dpi_scale = 1.0f;

static void ui_text_push_bytes(const char* data, int len) {
  if (len <= 0) {
    return;
  }
  int space = REX_UI_TEXT_MAX - ui_text_len;
  if (space <= 0) {
    return;
  }
  if (len > space) {
    len = space;
  }
  memcpy(ui_text + ui_text_len, data, (size_t)len);
  ui_text_len += len;
}

static void ui_clear_key_state(void) {
  ui_key_tab = 0;
  ui_key_enter = 0;
  ui_key_space = 0;
  ui_key_up = 0;
  ui_key_down = 0;
  ui_key_backspace = 0;
  ui_key_delete = 0;
  ui_key_left = 0;
  ui_key_right = 0;
  ui_key_home = 0;
  ui_key_end = 0;
  ui_key_ctrl = 0;
  ui_key_shift = 0;
  memset(ui_key_states, 0, sizeof(ui_key_states));
  memset(ui_mouse_buttons, 0, sizeof(ui_mouse_buttons));
  ui_mouse_down = 0;
}

static int ui_keycode_from_keysym(KeySym sym) {
  if (sym >= XK_A && sym <= XK_Z) {
    return (int)sym;
  }
  if (sym >= XK_a && sym <= XK_z) {
    return (int)(sym - XK_a + 'A');
  }
  if (sym >= XK_0 && sym <= XK_9) {
    return (int)sym;
  }
  switch (sym) {
    case XK_Escape: return REX_KEY_ESCAPE;
    case XK_Return: return REX_KEY_ENTER;
    case XK_Tab:
    case XK_ISO_Left_Tab: return REX_KEY_TAB;
    case XK_space: return REX_KEY_SPACE;
    case XK_BackSpace: return REX_KEY_BACKSPACE;
    case XK_Insert: return REX_KEY_INSERT;
    case XK_Delete: return REX_KEY_DELETE;
    case XK_Right: return REX_KEY_RIGHT;
    case XK_Left: return REX_KEY_LEFT;
    case XK_Down: return REX_KEY_DOWN;
    case XK_Up: return REX_KEY_UP;
    case XK_Page_Up: return REX_KEY_PAGE_UP;
    case XK_Page_Down: return REX_KEY_PAGE_DOWN;
    case XK_Home: return REX_KEY_HOME;
    case XK_End: return REX_KEY_END;
    case XK_Caps_Lock: return REX_KEY_CAPS_LOCK;
    case XK_Scroll_Lock: return REX_KEY_SCROLL_LOCK;
    case XK_Num_Lock: return REX_KEY_NUM_LOCK;
    case XK_Print: return REX_KEY_PRINT_SCREEN;
    case XK_Pause: return REX_KEY_PAUSE;
    case XK_F1: return REX_KEY_F1;
    case XK_F2: return REX_KEY_F2;
    case XK_F3: return REX_KEY_F3;
    case XK_F4: return REX_KEY_F4;
    case XK_F5: return REX_KEY_F5;
    case XK_F6: return REX_KEY_F6;
    case XK_F7: return REX_KEY_F7;
    case XK_F8: return REX_KEY_F8;
    case XK_F9: return REX_KEY_F9;
    case XK_F10: return REX_KEY_F10;
    case XK_F11: return REX_KEY_F11;
    case XK_F12: return REX_KEY_F12;
    case XK_F13: return REX_KEY_F13;
    case XK_F14: return REX_KEY_F14;
    case XK_F15: return REX_KEY_F15;
    case XK_F16: return REX_KEY_F16;
    case XK_F17: return REX_KEY_F17;
    case XK_F18: return REX_KEY_F18;
    case XK_F19: return REX_KEY_F19;
    case XK_F20: return REX_KEY_F20;
    case XK_F21: return REX_KEY_F21;
    case XK_F22: return REX_KEY_F22;
    case XK_F23: return REX_KEY_F23;
    case XK_F24: return REX_KEY_F24;
    case XK_F25: return REX_KEY_F25;
    case XK_KP_0: return REX_KEY_KP_0;
    case XK_KP_1: return REX_KEY_KP_1;
    case XK_KP_2: return REX_KEY_KP_2;
    case XK_KP_3: return REX_KEY_KP_3;
    case XK_KP_4: return REX_KEY_KP_4;
    case XK_KP_5: return REX_KEY_KP_5;
    case XK_KP_6: return REX_KEY_KP_6;
    case XK_KP_7: return REX_KEY_KP_7;
    case XK_KP_8: return REX_KEY_KP_8;
    case XK_KP_9: return REX_KEY_KP_9;
    case XK_KP_Decimal: return REX_KEY_KP_DECIMAL;
    case XK_KP_Divide: return REX_KEY_KP_DIVIDE;
    case XK_KP_Multiply: return REX_KEY_KP_MULTIPLY;
    case XK_KP_Subtract: return REX_KEY_KP_SUBTRACT;
    case XK_KP_Add: return REX_KEY_KP_ADD;
    case XK_KP_Enter: return REX_KEY_KP_ENTER;
    case XK_KP_Equal: return REX_KEY_KP_EQUAL;
    case XK_Shift_L: return REX_KEY_LEFT_SHIFT;
    case XK_Shift_R: return REX_KEY_RIGHT_SHIFT;
    case XK_Control_L: return REX_KEY_LEFT_CONTROL;
    case XK_Control_R: return REX_KEY_RIGHT_CONTROL;
    case XK_Alt_L: return REX_KEY_LEFT_ALT;
    case XK_Alt_R: return REX_KEY_RIGHT_ALT;
    case XK_Super_L: return REX_KEY_LEFT_SUPER;
    case XK_Super_R: return REX_KEY_RIGHT_SUPER;
    case XK_Menu: return REX_KEY_MENU;
    case XK_minus: return REX_KEY_MINUS;
    case XK_underscore: return REX_KEY_MINUS;
    case XK_equal: return REX_KEY_EQUAL;
    case XK_plus: return REX_KEY_EQUAL;
    case XK_bracketleft: return REX_KEY_LEFT_BRACKET;
    case XK_braceleft: return REX_KEY_LEFT_BRACKET;
    case XK_bracketright: return REX_KEY_RIGHT_BRACKET;
    case XK_braceright: return REX_KEY_RIGHT_BRACKET;
    case XK_backslash: return REX_KEY_BACKSLASH;
    case XK_bar: return REX_KEY_BACKSLASH;
    case XK_semicolon: return REX_KEY_SEMICOLON;
    case XK_colon: return REX_KEY_SEMICOLON;
    case XK_apostrophe: return REX_KEY_APOSTROPHE;
    case XK_quotedbl: return REX_KEY_APOSTROPHE;
    case XK_grave: return REX_KEY_GRAVE_ACCENT;
    case XK_asciitilde: return REX_KEY_GRAVE_ACCENT;
    case XK_comma: return REX_KEY_COMMA;
    case XK_less: return REX_KEY_COMMA;
    case XK_period: return REX_KEY_PERIOD;
    case XK_greater: return REX_KEY_PERIOD;
    case XK_slash: return REX_KEY_SLASH;
    case XK_question: return REX_KEY_SLASH;
    default: break;
  }
  return REX_KEY_UNKNOWN;
}

static int ui_mask_shift(unsigned long mask) {
  int shift = 0;
  while ((mask & 1) == 0) {
    mask >>= 1;
    shift++;
  }
  return shift;
}

static int ui_mask_bits(unsigned long mask) {
  int bits = 0;
  while (mask & 1) {
    bits++;
    mask >>= 1;
  }
  return bits;
}

static unsigned long ui_pack_color(XImage* img, uint32_t argb) {
  int rbits = ui_mask_bits(img->red_mask);
  int gbits = ui_mask_bits(img->green_mask);
  int bbits = ui_mask_bits(img->blue_mask);
  int rshift = ui_mask_shift(img->red_mask);
  int gshift = ui_mask_shift(img->green_mask);
  int bshift = ui_mask_shift(img->blue_mask);
  uint32_t r = (argb >> 16) & 0xFF;
  uint32_t g = (argb >> 8) & 0xFF;
  uint32_t b = argb & 0xFF;
  if (rbits > 0) {
    r = (r * ((1u << rbits) - 1u) + 127u) / 255u;
  }
  if (gbits > 0) {
    g = (g * ((1u << gbits) - 1u) + 127u) / 255u;
  }
  if (bbits > 0) {
    b = (b * ((1u << bbits) - 1u) + 127u) / 255u;
  }
  return ((unsigned long)r << rshift) | ((unsigned long)g << gshift) | ((unsigned long)b << bshift);
}

int rex_ui_platform_init(const char* title, int width, int height) {
  ui_display = XOpenDisplay(NULL);
  if (!ui_display) {
    return 0;
  }
  int screen = DefaultScreen(ui_display);
  ui_window = XCreateSimpleWindow(
    ui_display,
    RootWindow(ui_display, screen),
    10,
    10,
    (unsigned int)width,
    (unsigned int)height,
    1,
    BlackPixel(ui_display, screen),
    WhitePixel(ui_display, screen)
  );
  XStoreName(ui_display, ui_window, title ? title : "Rex");
  XSelectInput(ui_display, ui_window, ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask | FocusChangeMask);
  ui_wm_delete = XInternAtom(ui_display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(ui_display, ui_window, &ui_wm_delete, 1);
  XMapWindow(ui_display, ui_window);
  ui_gc = XCreateGC(ui_display, ui_window, 0, NULL);
  ui_running = 1;
  ui_width = width;
  ui_height = height;
  int screen_width = DisplayWidth(ui_display, screen);
  int screen_mm = DisplayWidthMM(ui_display, screen);
  if (screen_mm > 0) {
    float dpi = (float)screen_width * 25.4f / (float)screen_mm;
    ui_dpi_scale = dpi / 96.0f;
  }
  return 1;
}

void rex_ui_platform_shutdown(void) {
  if (ui_image) {
    if (!ui_img_owns) {
      ui_image->data = NULL;
    }
    XDestroyImage(ui_image);
    ui_image = NULL;
    ui_img_owns = 0;
  }
  if (ui_display && ui_window) {
    XDestroyWindow(ui_display, ui_window);
    ui_window = 0;
  }
  if (ui_display) {
    XCloseDisplay(ui_display);
    ui_display = NULL;
  }
  ui_running = 0;
}

int rex_ui_platform_poll(RexUIPlatformInput* input) {
  ui_text_len = 0;
  ui_scroll_x = 0;
  ui_scroll_y = 0;
  ui_key_copy = 0;
  ui_key_paste = 0;
  ui_key_cut = 0;
  ui_key_select_all = 0;
  ui_redraw = 0;
  while (ui_display && XPending(ui_display)) {
    XEvent ev;
    XNextEvent(ui_display, &ev);
    switch (ev.type) {
      case ClientMessage:
        if ((Atom)ev.xclient.data.l[0] == ui_wm_delete) {
          ui_running = 0;
        }
        break;
      case Expose:
        ui_redraw = 1;
        break;
      case ConfigureNotify:
        ui_width = ev.xconfigure.width;
        ui_height = ev.xconfigure.height;
        ui_redraw = 1;
        break;
      case FocusOut:
        ui_clear_key_state();
        break;
      case DestroyNotify:
        ui_running = 0;
        break;
      case MotionNotify:
        ui_mouse_x = ev.xmotion.x;
        ui_mouse_y = ev.xmotion.y;
        break;
      case ButtonPress:
        if (ev.xbutton.button == Button1) {
          ui_mouse_down = 1;
          ui_mouse_buttons[REX_MOUSE_LEFT] = 1;
        } else if (ev.xbutton.button == Button2) {
          ui_mouse_buttons[REX_MOUSE_MIDDLE] = 1;
        } else if (ev.xbutton.button == Button3) {
          ui_mouse_buttons[REX_MOUSE_RIGHT] = 1;
        } else if (ev.xbutton.button == Button4) {
          ui_scroll_y += 1;
        } else if (ev.xbutton.button == Button5) {
          ui_scroll_y -= 1;
        } else if (ev.xbutton.button == Button6) {
          ui_scroll_x -= 1;
        } else if (ev.xbutton.button == Button7) {
          ui_scroll_x += 1;
        }
        break;
      case ButtonRelease:
        if (ev.xbutton.button == Button1) {
          ui_mouse_down = 0;
          ui_mouse_buttons[REX_MOUSE_LEFT] = 0;
        } else if (ev.xbutton.button == Button2) {
          ui_mouse_buttons[REX_MOUSE_MIDDLE] = 0;
        } else if (ev.xbutton.button == Button3) {
          ui_mouse_buttons[REX_MOUSE_RIGHT] = 0;
        }
        break;
      case KeyPress: {
        char buf[8];
        KeySym sym = 0;
        int len = XLookupString(&ev.xkey, buf, sizeof(buf), &sym, NULL);
        int code = ui_keycode_from_keysym(sym);
        if (code >= 0 && code < REX_UI_KEY_MAX) {
          ui_key_states[code] = 1;
        }
        switch (sym) {
          case XK_Tab: ui_key_tab = 1; break;
          case XK_Return: ui_key_enter = 1; break;
          case XK_space: ui_key_space = 1; break;
          case XK_Up: ui_key_up = 1; break;
          case XK_Down: ui_key_down = 1; break;
          case XK_BackSpace: ui_key_backspace = 1; break;
          case XK_Delete: ui_key_delete = 1; break;
          case XK_Left: ui_key_left = 1; break;
          case XK_Right: ui_key_right = 1; break;
          case XK_Home: ui_key_home = 1; break;
          case XK_End: ui_key_end = 1; break;
          case XK_Control_L:
          case XK_Control_R:
            ui_key_ctrl = 1;
            break;
          case XK_Shift_L:
          case XK_Shift_R:
            ui_key_shift = 1;
            break;
          default: break;
        }
        if ((ev.xkey.state & ControlMask) != 0 || ui_key_ctrl) {
          if (sym == XK_c || sym == XK_C) ui_key_copy = 1;
          if (sym == XK_v || sym == XK_V) ui_key_paste = 1;
          if (sym == XK_x || sym == XK_X) ui_key_cut = 1;
          if (sym == XK_a || sym == XK_A) ui_key_select_all = 1;
        }
        for (int i = 0; i < len; i++) {
          unsigned char ch = (unsigned char)buf[i];
          if (ch >= 32 && ch != 127) {
            ui_text_push_bytes((const char*)&buf[i], 1);
          }
        }
        break;
      }
      case KeyRelease: {
        KeySym sym = XLookupKeysym(&ev.xkey, 0);
        int code = ui_keycode_from_keysym(sym);
        if (code >= 0 && code < REX_UI_KEY_MAX) {
          ui_key_states[code] = 0;
        }
        switch (sym) {
          case XK_Tab: ui_key_tab = 0; break;
          case XK_Return: ui_key_enter = 0; break;
          case XK_space: ui_key_space = 0; break;
          case XK_Up: ui_key_up = 0; break;
          case XK_Down: ui_key_down = 0; break;
          case XK_BackSpace: ui_key_backspace = 0; break;
          case XK_Delete: ui_key_delete = 0; break;
          case XK_Left: ui_key_left = 0; break;
          case XK_Right: ui_key_right = 0; break;
          case XK_Home: ui_key_home = 0; break;
          case XK_End: ui_key_end = 0; break;
          case XK_Control_L:
          case XK_Control_R:
            ui_key_ctrl = 0;
            break;
          case XK_Shift_L:
          case XK_Shift_R:
            ui_key_shift = 0;
            break;
          default: break;
        }
        break;
      }
      default:
        break;
    }
  }
  if (!ui_running) {
    input->closed = 1;
    return 0;
  }
  input->mouse_x = ui_mouse_x;
  input->mouse_y = ui_mouse_y;
  input->mouse_down = ui_mouse_down;
  for (int i = 0; i < REX_UI_MOUSE_BUTTONS; i++) {
    input->mouse_buttons[i] = ui_mouse_buttons[i];
  }
  input->scroll_x = ui_scroll_x;
  input->scroll_y = ui_scroll_y;
  memcpy(input->key_states, ui_key_states, sizeof(ui_key_states));
  input->key_tab = ui_key_tab;
  input->key_enter = ui_key_enter;
  input->key_space = ui_key_space;
  input->key_up = ui_key_up;
  input->key_down = ui_key_down;
  input->key_backspace = ui_key_backspace;
  input->key_delete = ui_key_delete;
  input->key_left = ui_key_left;
  input->key_right = ui_key_right;
  input->key_home = ui_key_home;
  input->key_end = ui_key_end;
  input->key_ctrl = ui_key_ctrl;
  input->key_shift = ui_key_shift;
  input->key_copy = ui_key_copy;
  input->key_paste = ui_key_paste;
  input->key_cut = ui_key_cut;
  input->key_select_all = ui_key_select_all;
  input->text_len = ui_text_len;
  if (ui_text_len > 0) {
    memcpy(input->text, ui_text, (size_t)ui_text_len);
  }
  input->width = ui_width;
  input->height = ui_height;
  input->dpi_scale = ui_dpi_scale;
  input->redraw = ui_redraw;
  input->closed = 0;
  return 1;
}

void rex_ui_platform_present(const uint32_t* pixels, int width, int height) {
  if (!ui_display || !ui_window || !pixels) {
    return;
  }
  if (!ui_image || width != ui_img_w || height != ui_img_h) {
    if (ui_image) {
      if (!ui_img_owns) {
        ui_image->data = NULL;
      }
      XDestroyImage(ui_image);
      ui_img_owns = 0;
    }
    ui_img_w = width;
    ui_img_h = height;
    ui_image = XCreateImage(
      ui_display,
      DefaultVisual(ui_display, DefaultScreen(ui_display)),
      DefaultDepth(ui_display, DefaultScreen(ui_display)),
      ZPixmap,
      0,
      (char*)pixels,
      (unsigned int)width,
      (unsigned int)height,
      32,
      0
    );
    if (!ui_image) {
      return;
    }
    if (ui_image->bits_per_pixel != 32) {
      size_t bytes = (size_t)ui_image->bytes_per_line * (size_t)height;
      char* data = (char*)malloc(bytes);
      if (!data) {
        return;
      }
      ui_image->data = data;
      ui_img_owns = 1;
    } else {
      ui_img_owns = 0;
    }
  } else {
    if (!ui_img_owns) {
      ui_image->data = (char*)pixels;
    }
  }
  if (ui_image->bits_per_pixel != 32) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        uint32_t argb = pixels[y * width + x];
        unsigned long packed = ui_pack_color(ui_image, argb);
        XPutPixel(ui_image, x, y, packed);
      }
    }
  }
  XPutImage(ui_display, ui_window, ui_gc, ui_image, 0, 0, 0, 0, (unsigned int)width, (unsigned int)height);
  XFlush(ui_display);
}

int rex_ui_platform_get_clipboard(char* buffer, int capacity) {
  if (!ui_display || !buffer || capacity <= 0) {
    return 0;
  }
  int bytes = 0;
  char* data = XFetchBytes(ui_display, &bytes);
  if (!data || bytes <= 0) {
    if (data) {
      XFree(data);
    }
    return 0;
  }
  if (bytes >= capacity) {
    bytes = capacity - 1;
  }
  memcpy(buffer, data, (size_t)bytes);
  buffer[bytes] = '\0';
  XFree(data);
  return bytes;
}

void rex_ui_platform_set_clipboard(const char* text) {
  if (!ui_display || !text) {
    return;
  }
  XStoreBytes(ui_display, text, (int)strlen(text));
}

void rex_ui_platform_set_titlebar_dark(int dark) {
  (void)dark;
}

#endif
