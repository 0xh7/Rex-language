#ifdef _WIN32

#include "rex_ui.h"

#include <windows.h>
#include <windowsx.h>
#include <string.h>

static HWND ui_hwnd = NULL;
static int ui_running = 0;
static int ui_mouse_x = 0;
static int ui_mouse_y = 0;
static int ui_mouse_down = 0;
static char ui_text[REX_UI_TEXT_MAX];
static int ui_text_len = 0;
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
static int ui_key_copy = 0;
static int ui_key_paste = 0;
static int ui_key_cut = 0;
static int ui_key_select_all = 0;
static int ui_redraw = 0;
static int ui_width = 0;
static int ui_height = 0;
static float ui_dpi_scale = 1.0f;
static const uint32_t* ui_pixels = NULL;
static int ui_pix_w = 0;
static int ui_pix_h = 0;
static int ui_class_registered = 0;
static wchar_t ui_surrogate = 0;
static HMODULE ui_dwm = NULL;
typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
static DwmSetWindowAttributeFn ui_dwm_set_window_attribute = NULL;
static int ui_dwm_tried = 0;

static void ui_dwm_init(void) {
  if (ui_dwm_tried) {
    return;
  }
  ui_dwm_tried = 1;
  ui_dwm = LoadLibraryW(L"dwmapi.dll");
  if (!ui_dwm) {
    return;
  }
  ui_dwm_set_window_attribute = (DwmSetWindowAttributeFn)GetProcAddress(ui_dwm, "DwmSetWindowAttribute");
}

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

static void ui_text_push_utf8(uint32_t codepoint) {
  if (codepoint < 32 || codepoint == 127) {
    return;
  }
  char buf[4];
  int len = 0;
  if (codepoint <= 0x7F) {
    buf[len++] = (char)codepoint;
  } else if (codepoint <= 0x7FF) {
    buf[len++] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
    buf[len++] = (char)(0x80 | (codepoint & 0x3F));
  } else if (codepoint <= 0xFFFF) {
    buf[len++] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
    buf[len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
    buf[len++] = (char)(0x80 | (codepoint & 0x3F));
  } else {
    buf[len++] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
    buf[len++] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
    buf[len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
    buf[len++] = (char)(0x80 | (codepoint & 0x3F));
  }
  ui_text_push_bytes(buf, len);
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
  memset(ui_key_states, 0, sizeof(ui_key_states));
  memset(ui_mouse_buttons, 0, sizeof(ui_mouse_buttons));
  ui_mouse_down = 0;
}

static int ui_keycode_from_vk(WPARAM vk, LPARAM lparam) {
  if (vk >= 'A' && vk <= 'Z') {
    return (int)vk;
  }
  if (vk >= '0' && vk <= '9') {
    return (int)vk;
  }
  if (vk == VK_SHIFT) {
    UINT scancode = (lparam >> 16) & 0xFFu;
    vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
  } else if (vk == VK_CONTROL) {
    vk = (lparam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
  } else if (vk == VK_MENU) {
    vk = (lparam & 0x01000000) ? VK_RMENU : VK_LMENU;
  }
  switch (vk) {
    case VK_ESCAPE: return REX_KEY_ESCAPE;
    case VK_RETURN:
      if (lparam & 0x01000000) {
        return REX_KEY_KP_ENTER;
      }
      return REX_KEY_ENTER;
    case VK_TAB: return REX_KEY_TAB;
    case VK_BACK: return REX_KEY_BACKSPACE;
    case VK_INSERT: return REX_KEY_INSERT;
    case VK_DELETE: return REX_KEY_DELETE;
    case VK_RIGHT: return REX_KEY_RIGHT;
    case VK_LEFT: return REX_KEY_LEFT;
    case VK_DOWN: return REX_KEY_DOWN;
    case VK_UP: return REX_KEY_UP;
    case VK_PRIOR: return REX_KEY_PAGE_UP;
    case VK_NEXT: return REX_KEY_PAGE_DOWN;
    case VK_HOME: return REX_KEY_HOME;
    case VK_END: return REX_KEY_END;
    case VK_CAPITAL: return REX_KEY_CAPS_LOCK;
    case VK_SCROLL: return REX_KEY_SCROLL_LOCK;
    case VK_NUMLOCK: return REX_KEY_NUM_LOCK;
    case VK_SNAPSHOT: return REX_KEY_PRINT_SCREEN;
    case VK_PAUSE: return REX_KEY_PAUSE;
    case VK_F1: return REX_KEY_F1;
    case VK_F2: return REX_KEY_F2;
    case VK_F3: return REX_KEY_F3;
    case VK_F4: return REX_KEY_F4;
    case VK_F5: return REX_KEY_F5;
    case VK_F6: return REX_KEY_F6;
    case VK_F7: return REX_KEY_F7;
    case VK_F8: return REX_KEY_F8;
    case VK_F9: return REX_KEY_F9;
    case VK_F10: return REX_KEY_F10;
    case VK_F11: return REX_KEY_F11;
    case VK_F12: return REX_KEY_F12;
    case VK_F13: return REX_KEY_F13;
    case VK_F14: return REX_KEY_F14;
    case VK_F15: return REX_KEY_F15;
    case VK_F16: return REX_KEY_F16;
    case VK_F17: return REX_KEY_F17;
    case VK_F18: return REX_KEY_F18;
    case VK_F19: return REX_KEY_F19;
    case VK_F20: return REX_KEY_F20;
    case VK_F21: return REX_KEY_F21;
    case VK_F22: return REX_KEY_F22;
    case VK_F23: return REX_KEY_F23;
    case VK_F24: return REX_KEY_F24;
    case VK_NUMPAD0: return REX_KEY_KP_0;
    case VK_NUMPAD1: return REX_KEY_KP_1;
    case VK_NUMPAD2: return REX_KEY_KP_2;
    case VK_NUMPAD3: return REX_KEY_KP_3;
    case VK_NUMPAD4: return REX_KEY_KP_4;
    case VK_NUMPAD5: return REX_KEY_KP_5;
    case VK_NUMPAD6: return REX_KEY_KP_6;
    case VK_NUMPAD7: return REX_KEY_KP_7;
    case VK_NUMPAD8: return REX_KEY_KP_8;
    case VK_NUMPAD9: return REX_KEY_KP_9;
    case VK_DECIMAL: return REX_KEY_KP_DECIMAL;
    case VK_DIVIDE: return REX_KEY_KP_DIVIDE;
    case VK_MULTIPLY: return REX_KEY_KP_MULTIPLY;
    case VK_SUBTRACT: return REX_KEY_KP_SUBTRACT;
    case VK_ADD: return REX_KEY_KP_ADD;
    case VK_SEPARATOR: return REX_KEY_KP_ENTER;
    case VK_LSHIFT: return REX_KEY_LEFT_SHIFT;
    case VK_RSHIFT: return REX_KEY_RIGHT_SHIFT;
    case VK_LCONTROL: return REX_KEY_LEFT_CONTROL;
    case VK_RCONTROL: return REX_KEY_RIGHT_CONTROL;
    case VK_LMENU: return REX_KEY_LEFT_ALT;
    case VK_RMENU: return REX_KEY_RIGHT_ALT;
    case VK_LWIN: return REX_KEY_LEFT_SUPER;
    case VK_RWIN: return REX_KEY_RIGHT_SUPER;
    case VK_APPS: return REX_KEY_MENU;
    case VK_OEM_1: return REX_KEY_SEMICOLON;
    case VK_OEM_PLUS: return REX_KEY_EQUAL;
    case VK_OEM_COMMA: return REX_KEY_COMMA;
    case VK_OEM_MINUS: return REX_KEY_MINUS;
    case VK_OEM_PERIOD: return REX_KEY_PERIOD;
    case VK_OEM_2: return REX_KEY_SLASH;
    case VK_OEM_3: return REX_KEY_GRAVE_ACCENT;
    case VK_OEM_4: return REX_KEY_LEFT_BRACKET;
    case VK_OEM_5: return REX_KEY_BACKSLASH;
    case VK_OEM_102: return REX_KEY_BACKSLASH;
    case VK_OEM_6: return REX_KEY_RIGHT_BRACKET;
    case VK_OEM_7: return REX_KEY_APOSTROPHE;
    default: break;
  }
  return REX_KEY_UNKNOWN;
}

static LRESULT CALLBACK rex_ui_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_CLOSE:
      ui_running = 0;
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      ui_running = 0;
      PostQuitMessage(0);
      return 0;
    case WM_SIZE:
      ui_width = LOWORD(lparam);
      ui_height = HIWORD(lparam);
      ui_redraw = 1;
      return 0;
    case WM_MOUSEMOVE:
      ui_mouse_x = GET_X_LPARAM(lparam);
      ui_mouse_y = GET_Y_LPARAM(lparam);
      return 0;
    case WM_LBUTTONDOWN:
      ui_mouse_down = 1;
      ui_mouse_buttons[REX_MOUSE_LEFT] = 1;
      SetCapture(hwnd);
      return 0;
    case WM_LBUTTONUP:
      ui_mouse_down = 0;
      ui_mouse_buttons[REX_MOUSE_LEFT] = 0;
      ReleaseCapture();
      return 0;
    case WM_RBUTTONDOWN:
      ui_mouse_buttons[REX_MOUSE_RIGHT] = 1;
      return 0;
    case WM_RBUTTONUP:
      ui_mouse_buttons[REX_MOUSE_RIGHT] = 0;
      return 0;
    case WM_MBUTTONDOWN:
      ui_mouse_buttons[REX_MOUSE_MIDDLE] = 1;
      return 0;
    case WM_MBUTTONUP:
      ui_mouse_buttons[REX_MOUSE_MIDDLE] = 0;
      return 0;
    case WM_XBUTTONDOWN:
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) {
        ui_mouse_buttons[REX_MOUSE_BUTTON_4] = 1;
      } else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2) {
        ui_mouse_buttons[REX_MOUSE_BUTTON_5] = 1;
      }
      return 0;
    case WM_XBUTTONUP:
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1) {
        ui_mouse_buttons[REX_MOUSE_BUTTON_4] = 0;
      } else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2) {
        ui_mouse_buttons[REX_MOUSE_BUTTON_5] = 0;
      }
      return 0;
    case WM_MOUSEWHEEL: {
      int delta = GET_WHEEL_DELTA_WPARAM(wparam);
      ui_scroll_y += delta / WHEEL_DELTA;
      return 0;
    }
    case WM_MOUSEHWHEEL: {
      int delta = GET_WHEEL_DELTA_WPARAM(wparam);
      ui_scroll_x += delta / WHEEL_DELTA;
      return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
      int code = ui_keycode_from_vk(wparam, lparam);
      if (code >= 0 && code < REX_UI_KEY_MAX) {
        ui_key_states[code] = 1;
      }
      int ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
      switch (wparam) {
        case VK_TAB: ui_key_tab = 1; break;
        case VK_RETURN: ui_key_enter = 1; break;
        case VK_SPACE: ui_key_space = 1; break;
        case VK_UP: ui_key_up = 1; break;
        case VK_DOWN: ui_key_down = 1; break;
        case VK_BACK: ui_key_backspace = 1; break;
        case VK_DELETE: ui_key_delete = 1; break;
        case VK_LEFT: ui_key_left = 1; break;
        case VK_RIGHT: ui_key_right = 1; break;
        case VK_HOME: ui_key_home = 1; break;
        case VK_END: ui_key_end = 1; break;
        case 'C': if (ctrl) ui_key_copy = 1; break;
        case 'V': if (ctrl) ui_key_paste = 1; break;
        case 'X': if (ctrl) ui_key_cut = 1; break;
        case 'A': if (ctrl) ui_key_select_all = 1; break;
        default: break;
      }
      return 0;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
      {
        int code = ui_keycode_from_vk(wparam, lparam);
        if (code >= 0 && code < REX_UI_KEY_MAX) {
          ui_key_states[code] = 0;
        }
      }
      switch (wparam) {
        case VK_TAB: ui_key_tab = 0; break;
        case VK_RETURN: ui_key_enter = 0; break;
        case VK_SPACE: ui_key_space = 0; break;
        case VK_UP: ui_key_up = 0; break;
        case VK_DOWN: ui_key_down = 0; break;
        case VK_BACK: ui_key_backspace = 0; break;
        case VK_DELETE: ui_key_delete = 0; break;
        case VK_LEFT: ui_key_left = 0; break;
        case VK_RIGHT: ui_key_right = 0; break;
        case VK_HOME: ui_key_home = 0; break;
        case VK_END: ui_key_end = 0; break;
        default: break;
      }
      return 0;
    case WM_KILLFOCUS:
      ui_clear_key_state();
      return 0;
    case WM_CHAR: {
      wchar_t wc = (wchar_t)wparam;
      if (wc >= 0xD800 && wc <= 0xDBFF) {
        ui_surrogate = wc;
        return 0;
      }
      uint32_t codepoint = (uint32_t)wc;
      if (wc >= 0xDC00 && wc <= 0xDFFF) {
        if (ui_surrogate) {
          codepoint = 0x10000u + (((uint32_t)ui_surrogate - 0xD800u) << 10)
            + ((uint32_t)wc - 0xDC00u);
          ui_surrogate = 0;
        } else {
          codepoint = 0xFFFD;
        }
      } else if (ui_surrogate) {
        ui_surrogate = 0;
      }
      ui_text_push_utf8(codepoint);
      return 0;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      if (ui_pixels && ui_pix_w > 0 && ui_pix_h > 0) {
        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biWidth = ui_pix_w;
        bmi.bmiHeader.biHeight = -ui_pix_h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        StretchDIBits(
          hdc,
          0,
          0,
          ui_pix_w,
          ui_pix_h,
          0,
          0,
          ui_pix_w,
          ui_pix_h,
          ui_pixels,
          &bmi,
          DIB_RGB_COLORS,
          SRCCOPY
        );
      }
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

int rex_ui_platform_init(const char* title, int width, int height) {
  SetProcessDPIAware();
  WNDCLASSA wc;
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = rex_ui_wndproc;
  wc.hInstance = GetModuleHandleA(NULL);
  wc.lpszClassName = "RexUIWindow";
  if (!ui_class_registered) {
    if (!RegisterClassA(&wc)) {
      DWORD err = GetLastError();
      if (err != ERROR_CLASS_ALREADY_EXISTS) {
        return 0;
      }
    }
    ui_class_registered = 1;
  }

  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  RECT rect = { 0, 0, width, height };
  AdjustWindowRect(&rect, style, 0);
  ui_hwnd = CreateWindowA(
    wc.lpszClassName,
    title ? title : "Rex",
    style,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    rect.right - rect.left,
    rect.bottom - rect.top,
    NULL,
    NULL,
    wc.hInstance,
    NULL
  );
  if (!ui_hwnd) {
    return 0;
  }
  ShowWindow(ui_hwnd, SW_SHOW);
  ui_running = 1;
  ui_width = width;
  ui_height = height;
  return 1;
}

void rex_ui_platform_shutdown(void) {
  if (ui_hwnd) {
    DestroyWindow(ui_hwnd);
    ui_hwnd = NULL;
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
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      ui_running = 0;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  if (!ui_running) {
    input->closed = 1;
    return 0;
  }
  if (ui_hwnd) {
    RECT rect;
    if (GetClientRect(ui_hwnd, &rect)) {
      ui_width = rect.right - rect.left;
      ui_height = rect.bottom - rect.top;
    }
    HDC hdc = GetDC(ui_hwnd);
    if (hdc) {
      int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
      if (dpi > 0) {
        ui_dpi_scale = (float)dpi / 96.0f;
      }
      ReleaseDC(ui_hwnd, hdc);
    }
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
  input->key_ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  input->key_shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
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
  if (!ui_hwnd || !pixels) {
    return;
  }
  ui_pixels = pixels;
  ui_pix_w = width;
  ui_pix_h = height;
  InvalidateRect(ui_hwnd, NULL, FALSE);
  UpdateWindow(ui_hwnd);
}

int rex_ui_platform_get_clipboard(char* buffer, int capacity) {
  if (!buffer || capacity <= 0) {
    return 0;
  }
  if (!OpenClipboard(ui_hwnd)) {
    return 0;
  }
  HANDLE data = GetClipboardData(CF_TEXT);
  if (!data) {
    CloseClipboard();
    return 0;
  }
  char* text = (char*)GlobalLock(data);
  if (!text) {
    CloseClipboard();
    return 0;
  }
  int len = (int)strlen(text);
  if (len >= capacity) {
    len = capacity - 1;
  }
  memcpy(buffer, text, (size_t)len);
  buffer[len] = '\0';
  GlobalUnlock(data);
  CloseClipboard();
  return len;
}

void rex_ui_platform_set_clipboard(const char* text) {
  if (!text) {
    text = "";
  }
  if (!OpenClipboard(ui_hwnd)) {
    return;
  }
  EmptyClipboard();
  size_t len = strlen(text) + 1;
  HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, len);
  if (mem) {
    char* dst = (char*)GlobalLock(mem);
    if (dst) {
      memcpy(dst, text, len);
      GlobalUnlock(mem);
      SetClipboardData(CF_TEXT, mem);
    }
  }
  CloseClipboard();
}

void rex_ui_platform_set_titlebar_dark(int dark) {
  if (!ui_hwnd) {
    return;
  }
  ui_dwm_init();
  if (!ui_dwm_set_window_attribute) {
    return;
  }
  BOOL value = dark ? TRUE : FALSE;
  const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
  HRESULT hr = ui_dwm_set_window_attribute(ui_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
  if (FAILED(hr)) {
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
    ui_dwm_set_window_attribute(ui_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &value, sizeof(value));
  }
}

#endif
