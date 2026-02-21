#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#include "rex_ui.h"
#include <string.h>

static NSWindow* ui_window = nil;
static NSView* ui_view = nil;
static id ui_delegate = nil;
static int ui_running = 0;
static int ui_mouse_x = 0;
static int ui_mouse_y = 0;
static int ui_mouse_down = 0;
static char ui_text[REX_UI_TEXT_MAX];
static int ui_text_len = 0;
static int ui_width = 0;
static int ui_height = 0;
static const uint32_t* ui_pixels = NULL;
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
static float ui_dpi_scale = 1.0f;
static unichar ui_surrogate = 0;

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

static void ui_update_modifiers(NSEventModifierFlags mods) {
  ui_key_ctrl = (mods & NSEventModifierFlagCommand) != 0 || (mods & NSEventModifierFlagControl) != 0;
  ui_key_shift = (mods & NSEventModifierFlagShift) != 0;
}

static int ui_keycode_from_char(unichar c) {
  if (c >= 'a' && c <= 'z') {
    return (int)(c - 'a' + 'A');
  }
  if (c >= 'A' && c <= 'Z') {
    return (int)c;
  }
  if (c >= '0' && c <= '9') {
    return (int)c;
  }
  switch (c) {
    case ' ': return REX_KEY_SPACE;
    case '\t': return REX_KEY_TAB;
    case '\r': return REX_KEY_ENTER;
    case 27: return REX_KEY_ESCAPE;
    case NSBackspaceCharacter: return REX_KEY_BACKSPACE;
    case NSDeleteCharacter: return REX_KEY_DELETE;
    case NSLeftArrowFunctionKey: return REX_KEY_LEFT;
    case NSRightArrowFunctionKey: return REX_KEY_RIGHT;
    case NSUpArrowFunctionKey: return REX_KEY_UP;
    case NSDownArrowFunctionKey: return REX_KEY_DOWN;
    case NSHomeFunctionKey: return REX_KEY_HOME;
    case NSEndFunctionKey: return REX_KEY_END;
    case NSPageUpFunctionKey: return REX_KEY_PAGE_UP;
    case NSPageDownFunctionKey: return REX_KEY_PAGE_DOWN;
    case '\'': return REX_KEY_APOSTROPHE;
    case '"': return REX_KEY_APOSTROPHE;
    case ',': return REX_KEY_COMMA;
    case '<': return REX_KEY_COMMA;
    case '-': return REX_KEY_MINUS;
    case '_': return REX_KEY_MINUS;
    case '.': return REX_KEY_PERIOD;
    case '>': return REX_KEY_PERIOD;
    case '/': return REX_KEY_SLASH;
    case '?': return REX_KEY_SLASH;
    case ';': return REX_KEY_SEMICOLON;
    case ':': return REX_KEY_SEMICOLON;
    case '=': return REX_KEY_EQUAL;
    case '+': return REX_KEY_EQUAL;
    case '[': return REX_KEY_LEFT_BRACKET;
    case '{': return REX_KEY_LEFT_BRACKET;
    case '\\': return REX_KEY_BACKSLASH;
    case '|': return REX_KEY_BACKSLASH;
    case ']': return REX_KEY_RIGHT_BRACKET;
    case '}': return REX_KEY_RIGHT_BRACKET;
    case '`': return REX_KEY_GRAVE_ACCENT;
    case '~': return REX_KEY_GRAVE_ACCENT;
    default: break;
  }
  return REX_KEY_UNKNOWN;
}

static int ui_keycode_from_keycode(unsigned short keyCode) {
  switch (keyCode) {
    case kVK_Escape: return REX_KEY_ESCAPE;
    case kVK_Tab: return REX_KEY_TAB;
    case kVK_Return: return REX_KEY_ENTER;
    case kVK_Delete: return REX_KEY_BACKSPACE;
    case kVK_ForwardDelete: return REX_KEY_DELETE;
    case kVK_Home: return REX_KEY_HOME;
    case kVK_End: return REX_KEY_END;
    case kVK_PageUp: return REX_KEY_PAGE_UP;
    case kVK_PageDown: return REX_KEY_PAGE_DOWN;
    case kVK_LeftArrow: return REX_KEY_LEFT;
    case kVK_RightArrow: return REX_KEY_RIGHT;
    case kVK_UpArrow: return REX_KEY_UP;
    case kVK_DownArrow: return REX_KEY_DOWN;
    case kVK_F1: return REX_KEY_F1;
    case kVK_F2: return REX_KEY_F2;
    case kVK_F3: return REX_KEY_F3;
    case kVK_F4: return REX_KEY_F4;
    case kVK_F5: return REX_KEY_F5;
    case kVK_F6: return REX_KEY_F6;
    case kVK_F7: return REX_KEY_F7;
    case kVK_F8: return REX_KEY_F8;
    case kVK_F9: return REX_KEY_F9;
    case kVK_F10: return REX_KEY_F10;
    case kVK_F11: return REX_KEY_F11;
    case kVK_F12: return REX_KEY_F12;
    case kVK_F13: return REX_KEY_F13;
    case kVK_F14: return REX_KEY_F14;
    case kVK_F15: return REX_KEY_F15;
    case kVK_F16: return REX_KEY_F16;
    case kVK_F17: return REX_KEY_F17;
    case kVK_F18: return REX_KEY_F18;
    case kVK_F19: return REX_KEY_F19;
    case kVK_F20: return REX_KEY_F20;
    case kVK_F21: return REX_KEY_F21;
    case kVK_F22: return REX_KEY_F22;
    case kVK_F23: return REX_KEY_F23;
    case kVK_F24: return REX_KEY_F24;
    case kVK_ANSI_Keypad0: return REX_KEY_KP_0;
    case kVK_ANSI_Keypad1: return REX_KEY_KP_1;
    case kVK_ANSI_Keypad2: return REX_KEY_KP_2;
    case kVK_ANSI_Keypad3: return REX_KEY_KP_3;
    case kVK_ANSI_Keypad4: return REX_KEY_KP_4;
    case kVK_ANSI_Keypad5: return REX_KEY_KP_5;
    case kVK_ANSI_Keypad6: return REX_KEY_KP_6;
    case kVK_ANSI_Keypad7: return REX_KEY_KP_7;
    case kVK_ANSI_Keypad8: return REX_KEY_KP_8;
    case kVK_ANSI_Keypad9: return REX_KEY_KP_9;
    case kVK_ANSI_KeypadDecimal: return REX_KEY_KP_DECIMAL;
    case kVK_ANSI_KeypadDivide: return REX_KEY_KP_DIVIDE;
    case kVK_ANSI_KeypadMultiply: return REX_KEY_KP_MULTIPLY;
    case kVK_ANSI_KeypadMinus: return REX_KEY_KP_SUBTRACT;
    case kVK_ANSI_KeypadPlus: return REX_KEY_KP_ADD;
    case kVK_ANSI_KeypadEnter: return REX_KEY_KP_ENTER;
    case kVK_ANSI_KeypadEquals: return REX_KEY_KP_EQUAL;
    case kVK_Shift: return REX_KEY_LEFT_SHIFT;
    case kVK_RightShift: return REX_KEY_RIGHT_SHIFT;
    case kVK_Control: return REX_KEY_LEFT_CONTROL;
    case kVK_RightControl: return REX_KEY_RIGHT_CONTROL;
    case kVK_Option: return REX_KEY_LEFT_ALT;
    case kVK_RightOption: return REX_KEY_RIGHT_ALT;
    case kVK_Command: return REX_KEY_LEFT_SUPER;
    case kVK_RightCommand: return REX_KEY_RIGHT_SUPER;
    default: break;
  }
  return REX_KEY_UNKNOWN;
}

@interface RexView : NSView
@end

@implementation RexView
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
  if (!ui_pixels || ui_width <= 0 || ui_height <= 0) {
    return;
  }
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, ui_pixels, (size_t)ui_width * (size_t)ui_height * 4, NULL);
  CGImageRef img = CGImageCreate(
    ui_width,
    ui_height,
    8,
    32,
    ui_width * 4,
    cs,
    kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
    provider,
    NULL,
    false,
    kCGRenderingIntentDefault
  );
  CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
  CGContextDrawImage(ctx, CGRectMake(0, 0, ui_width, ui_height), img);
  CGImageRelease(img);
  CGDataProviderRelease(provider);
  CGColorSpaceRelease(cs);
}
- (void)mouseDown:(NSEvent*)event {
  ui_mouse_down = 1;
  ui_mouse_buttons[REX_MOUSE_LEFT] = 1;
}
- (void)mouseUp:(NSEvent*)event {
  ui_mouse_down = 0;
  ui_mouse_buttons[REX_MOUSE_LEFT] = 0;
}
- (void)rightMouseDown:(NSEvent*)event {
  ui_mouse_buttons[REX_MOUSE_RIGHT] = 1;
}
- (void)rightMouseUp:(NSEvent*)event {
  ui_mouse_buttons[REX_MOUSE_RIGHT] = 0;
}
- (void)otherMouseDown:(NSEvent*)event {
  NSInteger button = [event buttonNumber];
  if (button == 2) {
    ui_mouse_buttons[REX_MOUSE_MIDDLE] = 1;
  } else if (button == 3) {
    ui_mouse_buttons[REX_MOUSE_BUTTON_4] = 1;
  } else if (button == 4) {
    ui_mouse_buttons[REX_MOUSE_BUTTON_5] = 1;
  }
}
- (void)otherMouseUp:(NSEvent*)event {
  NSInteger button = [event buttonNumber];
  if (button == 2) {
    ui_mouse_buttons[REX_MOUSE_MIDDLE] = 0;
  } else if (button == 3) {
    ui_mouse_buttons[REX_MOUSE_BUTTON_4] = 0;
  } else if (button == 4) {
    ui_mouse_buttons[REX_MOUSE_BUTTON_5] = 0;
  }
}
- (void)mouseMoved:(NSEvent*)event {
  NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
  ui_mouse_x = (int)p.x;
  ui_mouse_y = (int)p.y;
}
- (void)mouseDragged:(NSEvent*)event {
  [self mouseMoved:event];
}
- (void)keyDown:(NSEvent*)event {
  NSString* chars = [event characters];
  NSUInteger len = [chars length];
  NSEventModifierFlags mods = [event modifierFlags];
  ui_update_modifiers(mods);
  NSString* ign = [event charactersIgnoringModifiers];
  unichar c = 0;
  if ([ign length] > 0) {
    c = [ign characterAtIndex:0];
  }
  int code = ui_keycode_from_char(c);
  if (code == REX_KEY_UNKNOWN) {
    code = ui_keycode_from_keycode([event keyCode]);
  }
  if (code >= 0 && code < REX_UI_KEY_MAX) {
    ui_key_states[code] = 1;
  }
  if ([ign length] > 0) {
    switch (c) {
      case NSTabCharacter: ui_key_tab = 1; break;
      case NSCarriageReturnCharacter: ui_key_enter = 1; break;
      case ' ': ui_key_space = 1; break;
      case NSBackspaceCharacter: ui_key_backspace = 1; break;
      case NSDeleteCharacter: ui_key_delete = 1; break;
      default: break;
    }
    if (c == NSLeftArrowFunctionKey) ui_key_left = 1;
    if (c == NSRightArrowFunctionKey) ui_key_right = 1;
    if (c == NSUpArrowFunctionKey) ui_key_up = 1;
    if (c == NSDownArrowFunctionKey) ui_key_down = 1;
    if (c == NSHomeFunctionKey) ui_key_home = 1;
    if (c == NSEndFunctionKey) ui_key_end = 1;
    if (ui_key_ctrl) {
      if (c == 'c' || c == 'C') ui_key_copy = 1;
      if (c == 'v' || c == 'V') ui_key_paste = 1;
      if (c == 'x' || c == 'X') ui_key_cut = 1;
      if (c == 'a' || c == 'A') ui_key_select_all = 1;
    }
  }
  for (NSUInteger i = 0; i < len; i++) {
    unichar c = [chars characterAtIndex:i];
    if (c >= 0xD800 && c <= 0xDBFF) {
      ui_surrogate = c;
      continue;
    }
    uint32_t codepoint = c;
    if (c >= 0xDC00 && c <= 0xDFFF) {
      if (ui_surrogate) {
        codepoint = 0x10000u + (((uint32_t)ui_surrogate - 0xD800u) << 10)
          + ((uint32_t)c - 0xDC00u);
        ui_surrogate = 0;
      } else {
        codepoint = 0xFFFD;
      }
    } else if (ui_surrogate) {
      ui_surrogate = 0;
    }
    ui_text_push_utf8(codepoint);
  }
}
- (void)keyUp:(NSEvent*)event {
  NSString* ign = [event charactersIgnoringModifiers];
  unichar c = 0;
  if ([ign length] > 0) {
    c = [ign characterAtIndex:0];
  }
  int code = ui_keycode_from_char(c);
  if (code == REX_KEY_UNKNOWN) {
    code = ui_keycode_from_keycode([event keyCode]);
  }
  if (code >= 0 && code < REX_UI_KEY_MAX) {
    ui_key_states[code] = 0;
  }
  if ([ign length] == 0) {
    return;
  }
  if (c == NSTabCharacter) ui_key_tab = 0;
  if (c == NSCarriageReturnCharacter) ui_key_enter = 0;
  if (c == ' ') ui_key_space = 0;
  if (c == NSBackspaceCharacter) ui_key_backspace = 0;
  if (c == NSDeleteCharacter) ui_key_delete = 0;
  if (c == NSLeftArrowFunctionKey) ui_key_left = 0;
  if (c == NSRightArrowFunctionKey) ui_key_right = 0;
  if (c == NSUpArrowFunctionKey) ui_key_up = 0;
  if (c == NSDownArrowFunctionKey) ui_key_down = 0;
  if (c == NSHomeFunctionKey) ui_key_home = 0;
  if (c == NSEndFunctionKey) ui_key_end = 0;
}
- (void)flagsChanged:(NSEvent*)event {
  NSEventModifierFlags mods = [event modifierFlags];
  ui_update_modifiers(mods);
  int code = ui_keycode_from_keycode([event keyCode]);
  int down = 0;
  if (code == REX_KEY_LEFT_SHIFT || code == REX_KEY_RIGHT_SHIFT) {
    down = (mods & NSEventModifierFlagShift) != 0;
  } else if (code == REX_KEY_LEFT_CONTROL || code == REX_KEY_RIGHT_CONTROL) {
    down = (mods & NSEventModifierFlagControl) != 0;
  } else if (code == REX_KEY_LEFT_ALT || code == REX_KEY_RIGHT_ALT) {
    down = (mods & NSEventModifierFlagOption) != 0;
  } else if (code == REX_KEY_LEFT_SUPER || code == REX_KEY_RIGHT_SUPER) {
    down = (mods & NSEventModifierFlagCommand) != 0;
  }
  if (code >= 0 && code < REX_UI_KEY_MAX) {
    ui_key_states[code] = down;
  }
}
- (void)scrollWheel:(NSEvent*)event {
  CGFloat delta = [event scrollingDeltaY];
  CGFloat deltaX = [event scrollingDeltaX];
  if (deltaX > 0) {
    ui_scroll_x += 1;
  } else if (deltaX < 0) {
    ui_scroll_x -= 1;
  }
  if (delta > 0) {
    ui_scroll_y += 1;
  } else if (delta < 0) {
    ui_scroll_y -= 1;
  }
}
@end

@interface RexWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation RexWindowDelegate
- (void)windowWillClose:(NSNotification*)notification {
  ui_running = 0;
}
- (void)windowDidResize:(NSNotification*)notification {
  ui_redraw = 1;
}
@end

int rex_ui_platform_init(const char* title, int width, int height) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    ui_width = width;
    ui_height = height;
    NSRect rect = NSMakeRect(0, 0, width, height);
    ui_window = [[NSWindow alloc] initWithContentRect:rect
                                            styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
    ui_view = [[RexView alloc] initWithFrame:rect];
    [ui_window setContentView:ui_view];
    [ui_window setTitle:[NSString stringWithUTF8String:title ? title : "Rex"]];
    [ui_window setAcceptsMouseMovedEvents:YES];
    ui_delegate = [[RexWindowDelegate alloc] init];
    [ui_window setDelegate:ui_delegate];
    [ui_window makeFirstResponder:ui_view];
    [ui_window makeKeyAndOrderFront:nil];
    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];
    ui_running = 1;
  }
  return 1;
}

void rex_ui_platform_shutdown(void) {
  @autoreleasepool {
    ui_running = 0;
    if (ui_window) {
      [ui_window close];
      ui_window = nil;
    }
    ui_view = nil;
    ui_delegate = nil;
  }
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
  @autoreleasepool {
    NSEvent* event = nil;
    do {
      event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                  untilDate:[NSDate distantPast]
                                     inMode:NSDefaultRunLoopMode
                                    dequeue:YES];
      if (event) {
        [NSApp sendEvent:event];
      }
    } while (event);

    if (ui_view && ui_window) {
      NSPoint p = [ui_view convertPoint:[ui_window mouseLocationOutsideOfEventStream] fromView:nil];
      ui_mouse_x = (int)p.x;
      ui_mouse_y = (int)p.y;
      NSRect bounds = [ui_view bounds];
      ui_width = (int)bounds.size.width;
      ui_height = (int)bounds.size.height;
      ui_dpi_scale = [ui_window backingScaleFactor];
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
  ui_pixels = pixels;
  ui_width = width;
  ui_height = height;
  if (ui_view) {
    [ui_view setNeedsDisplay:YES];
    [ui_view displayIfNeeded];
  }
}

int rex_ui_platform_get_clipboard(char* buffer, int capacity) {
  if (!buffer || capacity <= 0) {
    return 0;
  }
  NSPasteboard* pb = [NSPasteboard generalPasteboard];
  NSString* str = [pb stringForType:NSPasteboardTypeString];
  if (!str) {
    return 0;
  }
  const char* cstr = [str UTF8String];
  if (!cstr) {
    return 0;
  }
  int len = (int)strlen(cstr);
  if (len >= capacity) {
    len = capacity - 1;
  }
  memcpy(buffer, cstr, (size_t)len);
  buffer[len] = '\0';
  return len;
}

void rex_ui_platform_set_clipboard(const char* text) {
  if (!text) {
    text = "";
  }
  NSPasteboard* pb = [NSPasteboard generalPasteboard];
  [pb clearContents];
  NSString* str = [NSString stringWithUTF8String:text];
  if (str) {
    [pb setString:str forType:NSPasteboardTypeString];
  }
}

void rex_ui_platform_set_titlebar_dark(int dark) {
  (void)dark;
}

#endif
