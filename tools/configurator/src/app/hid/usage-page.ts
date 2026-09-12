// https://usb.org/document-library/hid-usage-tables-17

export const keyCodes: Record<number, { name: string, text: string, jsKeyCode: string }> = {
  0x01: { name: 'KEYBOARD_ERROR_ROLL_OVER', text: 'ERROR_ROLL_OVER', jsKeyCode: 'unk' },
  0x02: { name: 'KEYBOARD_POST_FAIL', text: 'POST_FAIL', jsKeyCode: 'unk' },
  0x03: { name: 'KEYBOARD_ERROR_UNDEFINED', text: 'ERROR_UNDEFINED', jsKeyCode: 'unk' },

  0x04: { name: 'KEYBOARD_A', text: 'A', jsKeyCode: 'KeyA' },
  0x05: { name: 'KEYBOARD_B', text: 'B', jsKeyCode: 'KeyB' },
  0x06: { name: 'KEYBOARD_C', text: 'C', jsKeyCode: 'KeyC' },
  0x07: { name: 'KEYBOARD_D', text: 'D', jsKeyCode: 'KeyD' },
  0x08: { name: 'KEYBOARD_E', text: 'E', jsKeyCode: 'KeyE' },
  0x09: { name: 'KEYBOARD_F', text: 'F', jsKeyCode: 'KeyF' },
  0x0A: { name: 'KEYBOARD_G', text: 'G', jsKeyCode: 'KeyG' },
  0x0B: { name: 'KEYBOARD_H', text: 'H', jsKeyCode: 'KeyH' },
  0x0C: { name: 'KEYBOARD_I', text: 'I', jsKeyCode: 'KeyI' },
  0x0D: { name: 'KEYBOARD_J', text: 'J', jsKeyCode: 'KeyJ' },
  0x0E: { name: 'KEYBOARD_K', text: 'K', jsKeyCode: 'KeyK' },
  0x0F: { name: 'KEYBOARD_L', text: 'L', jsKeyCode: 'KeyL' },
  0x10: { name: 'KEYBOARD_M', text: 'M', jsKeyCode: 'KeyM' },
  0x11: { name: 'KEYBOARD_N', text: 'N', jsKeyCode: 'KeyN' },
  0x12: { name: 'KEYBOARD_O', text: 'O', jsKeyCode: 'KeyO' },
  0x13: { name: 'KEYBOARD_P', text: 'P', jsKeyCode: 'KeyP' },
  0x14: { name: 'KEYBOARD_Q', text: 'Q', jsKeyCode: 'KeyQ' },
  0x15: { name: 'KEYBOARD_R', text: 'R', jsKeyCode: 'KeyR' },
  0x16: { name: 'KEYBOARD_S', text: 'S', jsKeyCode: 'KeyS' },
  0x17: { name: 'KEYBOARD_T', text: 'T', jsKeyCode: 'KeyT' },
  0x18: { name: 'KEYBOARD_U', text: 'U', jsKeyCode: 'KeyU' },
  0x19: { name: 'KEYBOARD_V', text: 'V', jsKeyCode: 'KeyV' },
  0x1A: { name: 'KEYBOARD_W', text: 'W', jsKeyCode: 'KeyW' },
  0x1B: { name: 'KEYBOARD_X', text: 'X', jsKeyCode: 'KeyX' },
  0x1C: { name: 'KEYBOARD_Y', text: 'Y', jsKeyCode: 'KeyY' },
  0x1D: { name: 'KEYBOARD_Z', text: 'Z', jsKeyCode: 'KeyZ' },

  0x1E: { name: 'KEYBOARD_1', text: '1', jsKeyCode: 'Digit1' },
  0x1F: { name: 'KEYBOARD_2', text: '2', jsKeyCode: 'Digit2' },
  0x20: { name: 'KEYBOARD_3', text: '3', jsKeyCode: 'Digit3' },
  0x21: { name: 'KEYBOARD_4', text: '4', jsKeyCode: 'Digit4' },
  0x22: { name: 'KEYBOARD_5', text: '5', jsKeyCode: 'Digit5' },
  0x23: { name: 'KEYBOARD_6', text: '6', jsKeyCode: 'Digit6' },
  0x24: { name: 'KEYBOARD_7', text: '7', jsKeyCode: 'Digit7' },
  0x25: { name: 'KEYBOARD_8', text: '8', jsKeyCode: 'Digit8' },
  0x26: { name: 'KEYBOARD_9', text: '9', jsKeyCode: 'Digit9' },
  0x27: { name: 'KEYBOARD_0', text: '0', jsKeyCode: 'Digit0' },

  0x28: { name: 'KEYBOARD_ENTER', text: 'ENTER', jsKeyCode: 'Enter' },
  0x29: { name: 'KEYBOARD_ESCAPE', text: 'ESCAPE', jsKeyCode: 'Escape' },
  0x2A: { name: 'KEYBOARD_BACKSPACE', text: 'BACKSPACE', jsKeyCode: 'Backspace' },
  0x2B: { name: 'KEYBOARD_TAB', text: 'TAB', jsKeyCode: 'Tab' },
  0x2C: { name: 'KEYBOARD_SPACE', text: 'SPACE', jsKeyCode: 'Space' },
  0x2D: { name: 'KEYBOARD_MINUS', text: 'MINUS', jsKeyCode: 'Minus' },
  0x2E: { name: 'KEYBOARD_EQUAL', text: 'EQUAL', jsKeyCode: 'Equal' },
  0x2F: { name: 'KEYBOARD_LEFT_BRACKET', text: 'LEFT_BRACKET', jsKeyCode: 'BracketLeft' },
  0x30: { name: 'KEYBOARD_RIGHT_BRACKET', text: 'RIGHT_BRACKET', jsKeyCode: 'BracketRight' },
  0x31: { name: 'KEYBOARD_BACKSLASH', text: 'BACKSLASH', jsKeyCode: 'Backslash' },
  0x33: { name: 'KEYBOARD_SEMICOLON', text: 'SEMICOLON', jsKeyCode: 'Semicolon' },
  0x34: { name: 'KEYBOARD_APOSTROPHE', text: 'APOSTROPHE', jsKeyCode: 'Quote' },
  0x35: { name: 'KEYBOARD_GRAVE', text: 'GRAVE', jsKeyCode: 'Backquote' },
  0x36: { name: 'KEYBOARD_COMMA', text: 'COMMA', jsKeyCode: 'Comma' },
  0x37: { name: 'KEYBOARD_PERIOD', text: 'PERIOD', jsKeyCode: 'Period' },
  0x38: { name: 'KEYBOARD_SLASH', text: 'SLASH', jsKeyCode: 'Slash' },
  0x39: { name: 'KEYBOARD_CAPS_LOCK', text: 'CAPS_LOCK', jsKeyCode: 'CapsLock' },

  0x3A: { name: 'KEYBOARD_F1', text: 'F1', jsKeyCode: 'F1' },
  0x3B: { name: 'KEYBOARD_F2', text: 'F2', jsKeyCode: 'F2' },
  0x3C: { name: 'KEYBOARD_F3', text: 'F3', jsKeyCode: 'F3' },
  0x3D: { name: 'KEYBOARD_F4', text: 'F4', jsKeyCode: 'F4' },
  0x3E: { name: 'KEYBOARD_F5', text: 'F5', jsKeyCode: 'F5' },
  0x3F: { name: 'KEYBOARD_F6', text: 'F6', jsKeyCode: 'F6' },
  0x40: { name: 'KEYBOARD_F7', text: 'F7', jsKeyCode: 'F7' },
  0x41: { name: 'KEYBOARD_F8', text: 'F8', jsKeyCode: 'F8' },
  0x42: { name: 'KEYBOARD_F9', text: 'F9', jsKeyCode: 'F9' },
  0x43: { name: 'KEYBOARD_F10', text: 'F10', jsKeyCode: 'F10' },
  0x44: { name: 'KEYBOARD_F11', text: 'F11', jsKeyCode: 'F11' },
  0x45: { name: 'KEYBOARD_F12', text: 'F12', jsKeyCode: 'F12' },

  0x46: { name: 'KEYBOARD_PRINT_SCREEN', text: 'PRINT_SCREEN', jsKeyCode: 'PrintScreen' },
  0x47: { name: 'KEYBOARD_SCROLL_LOCK', text: 'SCROLL_LOCK', jsKeyCode: 'ScrollLock' },
  0x48: { name: 'KEYBOARD_PAUSE', text: 'PAUSE', jsKeyCode: 'Pause' },
  0x49: { name: 'KEYBOARD_INSERT', text: 'INSERT', jsKeyCode: 'Insert' },
  0x4A: { name: 'KEYBOARD_HOME', text: 'HOME', jsKeyCode: 'Home' },
  0x4B: { name: 'KEYBOARD_PAGE_UP', text: 'PAGE_UP', jsKeyCode: 'PageUp' },
  0x4C: { name: 'KEYBOARD_DELETE', text: 'DELETE', jsKeyCode: 'Delete' },
  0x4D: { name: 'KEYBOARD_END', text: 'END', jsKeyCode: 'End' },
  0x4E: { name: 'KEYBOARD_PAGE_DOWN', text: 'PAGE_DOWN', jsKeyCode: 'PageDown' },
  0x4F: { name: 'KEYBOARD_RIGHT', text: 'RIGHT', jsKeyCode: 'ArrowRight' },
  0x50: { name: 'KEYBOARD_LEFT', text: 'LEFT', jsKeyCode: 'ArrowLeft' },
  0x51: { name: 'KEYBOARD_DOWN', text: 'DOWN', jsKeyCode: 'ArrowDown' },
  0x52: { name: 'KEYBOARD_UP', text: 'UP', jsKeyCode: 'ArrowUp' },

  0x53: { name: 'KEYPAD_NUM_LOCK', text: 'NUM_LOCK', jsKeyCode: 'NumpadNumLock' },
  0x54: { name: 'KEYPAD_DIVIDE', text: 'DIVIDE', jsKeyCode: 'NumpadDivide' },
  0x55: { name: 'KEYPAD_MULTIPLY', text: 'MULTIPLY', jsKeyCode: 'NumpadMultiply' },
  0x56: { name: 'KEYPAD_MINUS', text: 'MINUS', jsKeyCode: 'NumpadSubtract' },
  0x57: { name: 'KEYPAD_PLUS', text: 'PLUS', jsKeyCode: 'NumpadAdd' },
  0x58: { name: 'KEYPAD_ENTER', text: 'ENTER', jsKeyCode: 'NumpadEnter' },
  0x59: { name: 'KEYPAD_1', text: '1', jsKeyCode: 'Numpad1' },
  0x5A: { name: 'KEYPAD_2', text: '2', jsKeyCode: 'Numpad2' },
  0x5B: { name: 'KEYPAD_3', text: '3', jsKeyCode: 'Numpad3' },
  0x5C: { name: 'KEYPAD_4', text: '4', jsKeyCode: 'Numpad4' },
  0x5D: { name: 'KEYPAD_5', text: '5', jsKeyCode: 'Numpad5' },
  0x5E: { name: 'KEYPAD_6', text: '6', jsKeyCode: 'Numpad6' },
  0x5F: { name: 'KEYPAD_7', text: '7', jsKeyCode: 'Numpad7' },
  0x60: { name: 'KEYPAD_8', text: '8', jsKeyCode: 'Numpad8' },
  0x61: { name: 'KEYPAD_9', text: '9', jsKeyCode: 'Numpad9' },
  0x62: { name: 'KEYPAD_0', text: '0', jsKeyCode: 'Numpad0' },
  0x63: { name: 'KEYPAD_PERIOD', text: 'PERIOD', jsKeyCode: 'NumpadDecimal' },

  0x64: { name: 'KEYBOARD_NON_US_BACKSLASH', text: 'NON_US_BACKSLASH', jsKeyCode: 'NonUsBackslash' },
  0x65: { name: 'KEYBOARD_APPLICATION', text: 'APPLICATION', jsKeyCode: 'Application' },
  0x66: { name: 'KEYBOARD_POWER', text: 'POWER', jsKeyCode: 'Power' },
  0x67: { name: 'KEYBOARD_EQUAL_KEYPAD', text: 'EQUAL_KEYPAD', jsKeyCode: 'NumpadEqual' },

  0x68: { name: 'KEYBOARD_F13', text: 'F13', jsKeyCode: 'KeyF13' },
  0x69: { name: 'KEYBOARD_F14', text: 'F14', jsKeyCode: 'KeyF14' },
  0x6A: { name: 'KEYBOARD_F15', text: 'F15', jsKeyCode: 'KeyF15' },
  0x6B: { name: 'KEYBOARD_F16', text: 'F16', jsKeyCode: 'KeyF16' },
  0x6C: { name: 'KEYBOARD_F17', text: 'F17', jsKeyCode: 'KeyF17' },
  0x6D: { name: 'KEYBOARD_F18', text: 'F18', jsKeyCode: 'KeyF18' },
  0x6E: { name: 'KEYBOARD_F19', text: 'F19', jsKeyCode: 'KeyF19' },
  0x6F: { name: 'KEYBOARD_F20', text: 'F20', jsKeyCode: 'KeyF20' },
  0x70: { name: 'KEYBOARD_F21', text: 'F21', jsKeyCode: 'KeyF21' },
  0x71: { name: 'KEYBOARD_F22', text: 'F22', jsKeyCode: 'KeyF22' },
  0x72: { name: 'KEYBOARD_F23', text: 'F23', jsKeyCode: 'KeyF23' },
  0x73: { name: 'KEYBOARD_F24', text: 'F24', jsKeyCode: 'KeyF24' },

  0x74: { name: 'KEYBOARD_EXECUTE', text: 'EXECUTE', jsKeyCode: 'Execute' },
  0x75: { name: 'KEYBOARD_HELP', text: 'HELP', jsKeyCode: 'Help' },
  0x76: { name: 'KEYBOARD_MENU', text: 'MENU', jsKeyCode: 'Menu' },
  0x77: { name: 'KEYBOARD_SELECT', text: 'SELECT', jsKeyCode: 'Select' },
  0x78: { name: 'KEYBOARD_STOP', text: 'STOP', jsKeyCode: 'Stop' },
  0x79: { name: 'KEYBOARD_AGAIN', text: 'AGAIN', jsKeyCode: 'Again' },
  0x7A: { name: 'KEYBOARD_UNDO', text: 'UNDO', jsKeyCode: 'Undo' },
  0x7B: { name: 'KEYBOARD_CUT', text: 'CUT', jsKeyCode: 'Cut' },
  0x7C: { name: 'KEYBOARD_COPY', text: 'COPY', jsKeyCode: 'Copy' },
  0x7D: { name: 'KEYBOARD_PASTE', text: 'PASTE', jsKeyCode: 'Paste' },
  0x7E: { name: 'KEYBOARD_FIND', text: 'FIND', jsKeyCode: 'Find' },
  0x7F: { name: 'KEYBOARD_MUTE', text: 'MUTE', jsKeyCode: 'Mute' },
  0x80: { name: 'KEYBOARD_VOLUME_UP', text: 'VOLUME_UP', jsKeyCode: 'VolumeUp' },
  0x81: { name: 'KEYBOARD_VOLUME_DOWN', text: 'VOLUME_DOWN', jsKeyCode: 'VolumeDown' },
  0x82: { name: 'KEYBOARD_LOCKING_CAPS_LOCK', text: 'LOCKING_CAPS_LOCK', jsKeyCode: 'CapsLock' },
  0x83: { name: 'KEYBOARD_LOCKING_NUM_LOCK', text: 'LOCKING_NUM_LOCK', jsKeyCode: 'NumLock' },
  0x84: { name: 'KEYBOARD_LOCKING_SCROLL_LOCK', text: 'LOCKING_SCROLL_LOCK', jsKeyCode: 'ScrollLock' },
  0x85: { name: 'KEYPAD_COMMA', text: 'COMMA', jsKeyCode: 'Comma' },
  0x86: { name: 'KEYPAD_EQUAL_SIGN', text: 'EQUAL_SIGN', jsKeyCode: 'EqualSign' },

  0x87: { name: 'KEYBOARD_INTERNATIONAL1', text: 'INTERNATIONAL1', jsKeyCode: 'International1' },
  0x88: { name: 'KEYBOARD_INTERNATIONAL2', text: 'INTERNATIONAL2', jsKeyCode: 'International2' },
  0x89: { name: 'KEYBOARD_INTERNATIONAL3', text: 'INTERNATIONAL3', jsKeyCode: 'International3' },
  0x8A: { name: 'KEYBOARD_INTERNATIONAL4', text: 'INTERNATIONAL4', jsKeyCode: 'International4' },
  0x8B: { name: 'KEYBOARD_INTERNATIONAL5', text: 'INTERNATIONAL5', jsKeyCode: 'International5' },
  0x8C: { name: 'KEYBOARD_INTERNATIONAL6', text: 'INTERNATIONAL6', jsKeyCode: 'International6' },
  0x8D: { name: 'KEYBOARD_INTERNATIONAL7', text: 'INTERNATIONAL7', jsKeyCode: 'International7' },
  0x8E: { name: 'KEYBOARD_INTERNATIONAL8', text: 'INTERNATIONAL8', jsKeyCode: 'International8' },
  0x8F: { name: 'KEYBOARD_INTERNATIONAL9', text: 'INTERNATIONAL9', jsKeyCode: 'International9' },

  0x90: { name: 'KEYBOARD_LANG1', text: 'LANG1', jsKeyCode: 'Lang1' },
  0x91: { name: 'KEYBOARD_LANG2', text: 'LANG2', jsKeyCode: 'Lang2' },
  0x92: { name: 'KEYBOARD_LANG3', text: 'LANG3', jsKeyCode: 'Lang3' },
  0x93: { name: 'KEYBOARD_LANG4', text: 'LANG4', jsKeyCode: 'Lang4' },
  0x94: { name: 'KEYBOARD_LANG5', text: 'LANG5', jsKeyCode: 'Lang5' },
  0x95: { name: 'KEYBOARD_LANG6', text: 'LANG6', jsKeyCode: 'Lang6' },
  0x96: { name: 'KEYBOARD_LANG7', text: 'LANG7', jsKeyCode: 'Lang7' },
  0x97: { name: 'KEYBOARD_LANG8', text: 'LANG8', jsKeyCode: 'Lang8' },
  0x98: { name: 'KEYBOARD_LANG9', text: 'LANG9', jsKeyCode: 'Lang9' },

  0x99: { name: 'KEYBOARD_ALTERNATE_ERASE', text: 'ALTERNATE_ERASE', jsKeyCode: 'AlternateErase' },
  0x9A: { name: 'KEYBOARD_SYSREQ_ATTENTION', text: 'SYSREQ_ATTENTION', jsKeyCode: 'SysreqAttention' },
  0x9B: { name: 'KEYBOARD_CANCEL', text: 'CANCEL', jsKeyCode: 'Cancel' },
  0x9C: { name: 'KEYBOARD_CLEAR', text: 'CLEAR', jsKeyCode: 'Clear' },
  0x9D: { name: 'KEYBOARD_PRIOR', text: 'PRIOR', jsKeyCode: 'Prior' },
  0x9E: { name: 'KEYBOARD_RETURN', text: 'RETURN', jsKeyCode: 'Return' },
  0x9F: { name: 'KEYBOARD_SEPARATOR', text: 'SEPARATOR', jsKeyCode: 'Separator' },
  0xA0: { name: 'KEYBOARD_OUT', text: 'OUT', jsKeyCode: 'Out' },
  0xA1: { name: 'KEYBOARD_OPER', text: 'OPER', jsKeyCode: 'Oper' },
  0xA2: { name: 'KEYBOARD_CLEAR_AGAIN', text: 'CLEAR_AGAIN', jsKeyCode: 'ClearAgain' },
  0xA3: { name: 'KEYBOARD_CRSEL_PROPS', text: 'CRSEL_PROPS', jsKeyCode: 'CrselProps' },
  0xA4: { name: 'KEYBOARD_EXSEL', text: 'EXSEL', jsKeyCode: 'Exsel' },

  0xE0: { name: 'KEYBOARD_LEFT_CTRL', text: 'LEFT_CTRL', jsKeyCode: 'ControlLeft' },
  0xE1: { name: 'KEYBOARD_LEFT_SHIFT', text: 'LEFT_SHIFT', jsKeyCode: 'ShiftLeft' },
  0xE2: { name: 'KEYBOARD_LEFT_ALT', text: 'LEFT_ALT', jsKeyCode: 'AltLeft' },
  0xE3: { name: 'KEYBOARD_LEFT_GUI', text: 'LEFT_GUI', jsKeyCode: 'MetaLeft' },
  0xE4: { name: 'KEYBOARD_RIGHT_CTRL', text: 'RIGHT_CTRL', jsKeyCode: 'ControlRight' },
  0xE5: { name: 'KEYBOARD_RIGHT_SHIFT', text: 'RIGHT_SHIFT', jsKeyCode: 'ShiftRight' },
  0xE6: { name: 'KEYBOARD_RIGHT_ALT', text: 'RIGHT_ALT', jsKeyCode: 'AltRight' },
  0xE7: { name: 'KEYBOARD_RIGHT_GUI', text: 'RIGHT_GUI', jsKeyCode: 'ContextMenu' }
};

export const usagePages: Record<number, { name: string, usages: Record<number, { name: string }> }> = {
  0: {
    name: 'Undefined',
    usages: {}
  },
  1: {
    name: 'Generic Desktop Page',
    usages: {
      0x00: { name: 'Undefined' },
      0x01: { name: 'Pointer' },
      0x02: { name: 'Mouse' },
      0x04: { name: 'Joystick' },
      0x05: { name: 'Gamepad' },
      0x06: { name: 'Keyboard' },
      0x07: { name: 'Keypad' },
      0x08: { name: 'Multi-axis Controller' },
      0x09: { name: 'Tablet PC System Controls' },
      0x0A: { name: 'Water Cooling Device' },
      0x0B: { name: 'Computer Chassis Device' },
      0x0C: { name: 'Wireless Radio Controls' },
      0x0D: { name: 'Portable Device Control' },
      0x0E: { name: 'System Multi-Axis Controller' },
      0x0F: { name: 'Spatial Controller' },
      0x10: { name: 'Assistive Control' },
      0x11: { name: 'Device Dock' },
      0x12: { name: 'Dockable Device' },
      0x13: { name: 'Call State Management Control' },
      0x30: { name: 'X' },
      0x31: { name: 'Y' },
      0x32: { name: 'Z' },
      0x33: { name: 'Rx' },
      0x34: { name: 'Ry' },
      0x35: { name: 'Rz' },
      0x36: { name: 'Slider' },
      0x37: { name: 'Dial' },
      0x38: { name: 'Wheel' },
      0x39: { name: 'Hat Switch' },
      0x3A: { name: 'Counted Buffer' },
      0x3B: { name: 'Byte Count' },
      0x3C: { name: 'Motion Wakeup' },
      0x3D: { name: 'Start' },
      0x3E: { name: 'Select' },
      0x40: {name: 'Vx'},
      0x41: {name: 'Vy'},
      0x42: {name: 'Vz'},
      0x43: {name: 'Vbrx'},
      0x44: {name: 'Vbry'},
      0x45: {name: 'Vbrz'},
      0x46: {name: 'Vno'},
      0x47: {name: 'Feature Notification'},
      0x48: {name: 'Resolution Multiplier'},
      0x49: {name: 'Qx'},
      0x4A: {name: 'Qy'},
      0x4B: {name: 'Qz'},
      0x4C: {name: 'Qw'},
      0x80: {name: 'System Control'},
      0x81: {name: 'System Power Down'},
      0x82: {name: 'System Sleep'},
      0x83: {name: 'System Wake Up'},
      0x84: {name: 'System Context Menu'},
      0x85: {name: 'System Main Menu'},
      0x86: {name: 'System App Menu'},
      0x87: {name: 'System Menu Help'},
      0x88: {name: 'System Menu Exit'},
      0x89: {name: 'System Menu Select'},
      0x8A: {name: 'System Menu Right'},
      0x8B: {name: 'System Menu Left'},
      0x8C: {name: 'System Menu Up'},
      0x8D: {name: 'System Menu Down'},
      0x8E: {name: 'System Cold Restart'},
      0x8F: {name: 'System Warm Restart'},
      0x90: {name: 'D-pad Up'},
      0x91: {name: 'D-pad Down'},
      0x92: {name: 'D-pad Right'},
      0x93: {name: 'D-pad Left'},
      0x94: {name: 'Index Trigger'},
      0x95: {name: 'Palm Trigger'},
      0x96: {name: 'Thumbstick'},
      0x97: {name: 'System Function Shift'},
      0x98: {name: 'System Function Shift Lock'},
      0x99: {name: 'System Function Shift Lock Indicator'},
      0x9A: {name: 'System Dismiss Notification'},
      0x9B: {name: 'System Do Not Disturb'},
      0xA0: {name: 'System Dock'},
      0xA1: {name: 'System Undock'},
      0xA2: {name: 'System Setup'},
      0xA3: {name: 'System Break'},
      0xA4: {name: 'System Debugger Break'},
      0xA5: {name: 'Application Break'},
      0xA6: {name: 'Application Debugger Break'},
      0xA7: {name: 'System Speaker Mute'},
      0xA8: {name: 'System Hibernate'},
      0xA9: {name: 'System Microphone Mute'},
      0xAA: {name: 'System Accessibility Binding'},
      0xB0: {name: 'System Display Invert'},
      0xB1: {name: 'System Display Internal'},
      0xB2: {name: 'System Display External'},
      0xB3: {name: 'System Display Both'},
      0xB4: {name: 'System Display Dual'},
      0xB5: {name: 'System Display Toggle Int/Ext Mode'},
      0xB6: {name: 'System Display Swap Primary/Secondary'},
      0xB7: {name: 'System Display Toggle LCD Autoscale'},
      0xC0: {name: 'Sensor Zone'},
      0xC1: {name: 'RPM'},
      0xC2: {name: 'Coolant Level'},
      0xC3: {name: 'Coolant Critical Level'},
      0xC4: {name: 'Coolant Pump'},
      0xC5: {name: 'Chassis Enclosure'},
      0xC6: {name: 'Wireless Radio Button'},
      0xC7: {name: 'Wireless Radio LED'},
      0xC8: {name: 'Wireless Radio Slider Switch'},
      0xC9: {name: 'System Display Rotation Lock Button'},
      0xCA: {name: 'System Display Rotation Lock Slider Switch'},
      0xCB: {name: 'Control Enable'},
      0xD0: {name: 'Dockable Device Unique ID'},
      0xD1: {name: 'Dockable Device Vendor ID'},
      0xD2: {name: 'Dockable Device Primary Usage Page'},
      0xD3: {name: 'Dockable Device Primary Usage ID'},
      0xD4: {name: 'Dockable Device Docking State'},
      0xD5: {name: 'Dockable Device Display Occlusion'},
      0xD6: {name: 'Dockable Device Object Type'},
      0xE0: {name: 'Call Active LED'},
      0xE1: {name: 'Call Mute Toggle'},
      0xE2: {name: 'Call Mute LED'},
    }
  },
  2: {
    name: 'Simulation Controls Page',
    usages: {}
  },
  0x03: {
    name: 'VR Controls Page',
    usages: {}
  },
  0x04: {
    name: 'Sport Controls Page',
    usages: {}
  },
  0x05: {
    name: 'Game Controls Page',
    usages: {}
  },
  0x06: {
    name: 'Generic Device Controls Page',
    usages: {}
  },
  0x07: {
    name: 'Keyboard/Keypad Page',
    usages: keyCodes
  },
  0x08: {
    name: 'LED Page',
    usages: {}
  },
  0x09: {
    name: 'Button Page',
    usages: {}
  },
  0x0A: {
    name: 'Ordinal Page',
    usages: {}
  },
  0x0B: {
    name: 'Telephony Device Page',
    usages: {}
  },
  0x0C: {
    name: 'Consumer Page',
    usages: {}
  },
  0x0D: {
    name: 'Digitizers Page',
    usages: {}
  },
  0x0E: {
    name: 'Haptics Page',
    usages: {}
  },
  0x0F: {
    name: 'Physical Input Device Page',
    usages: {}
  },
  0x10: {
    name: 'Unicode Page',
    usages: {}
  },
  0x11: {
    name: 'SoC Page',
    usages: {}
  },
  0x12: {
    name: 'Eye and Head Trackers Page',
    usages: {}
  },
  0x14: {
    name: 'Auxiliary Display Page',
    usages: {}
  },
  0x20: {
    name: 'Sensors Page',
    usages: {}
  },
  0x40: {
    name: 'Medical Instrument Page',
    usages: {}
  },
  0x41: {
    name: 'Braille Display Page',
    usages: {}
  },
  0x59: {
    name: 'Lighting And Illumination Page',
    usages: {}
  },
  0x80: {
    name: 'Monitor Page',
    usages: {}
  },
  0x81: {
    name: 'Monitor Enumerated Page',
    usages: {}
  },
  0x82: {
    name: 'VESA Virtual Controls Page',
    usages: {}
  },
  0x84: {
    name: 'Power Page',
    usages: {}
  },
  0x85: {
    name: 'Battery System Page',
    usages: {}
  },
  0x8C: {
    name: 'Barcode Scanner Page',
    usages: {}
  },
  0x8D: {
    name: 'Scales Page',
    usages: {}
  },
  0x8E: {
    name: 'Magnetic Stripe Reader Page',
    usages: {}
  },
  0x90: {
    name: 'Camera Control Page',
    usages: {}
  },
  0x91: {
    name: 'Arcade Page',
    usages: {}
  },
  0x92: {
    name: 'Gaming Device Page',
    usages: {}
  },
  0xF1D0: {
    name: 'FIDO Alliance Page',
    usages: {}
  }
};
