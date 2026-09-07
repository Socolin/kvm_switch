import { inject, Service } from '@angular/core';
import { KvmSwitchState } from '../kvm-switch-state';
import { kvmUsbOperations, ShortcutDefinition } from '../web-usb-service';

@Service()
export class ShortcutService {
  readonly kvmSwitchState = inject(KvmSwitchState);

  async addShortcut(shortcut: ShortcutDefinition) {
    let webUsbConnection = this.kvmSwitchState.webUsbConnection();
    if (!webUsbConnection) {
      return;
    }

    if (!this.kvmSwitchState.keyboardShortcuts.hasValue()) {
      return;
    }

    let shortcuts = this.kvmSwitchState.keyboardShortcuts.value();
    let freeShortcutId = shortcuts.find(x => !x.enabled)?.shortcutId;
    if (freeShortcutId == undefined) {
      return;
    }

    await webUsbConnection.executeOutOperation(kvmUsbOperations.SetKeyboardShortcut, 0, {
      shortcutId: freeShortcutId,
      action: shortcut.action,
      enabled: shortcut.enabled,
      keys: shortcut.keys,
      data: shortcut.data
    });
    this.kvmSwitchState.keyboardShortcuts.reload();
  }

  async saveShortcut(shortcut: ShortcutDefinition) {
    let webUsbConnection = this.kvmSwitchState.webUsbConnection();
    if (!webUsbConnection) {
      return;
    }

    await webUsbConnection.executeOutOperation(kvmUsbOperations.SetKeyboardShortcut, 0, {
      shortcutId: shortcut.shortcutId,
      action: shortcut.action,
      enabled: shortcut.enabled,
      keys: shortcut.keys,
      data: shortcut.data
    });
    this.kvmSwitchState.keyboardShortcuts.reload();
  }

  async deleteShortcut(shortcut: ShortcutDefinition) {
    let webUsbConnection = this.kvmSwitchState.webUsbConnection();
    if (!webUsbConnection) {
      return;
    }

    await webUsbConnection.executeOutOperation(kvmUsbOperations.SetKeyboardShortcut, 0, {
      shortcutId: shortcut.shortcutId,
      action: shortcut.action,
      enabled: false,
      keys: shortcut.keys,
      data: shortcut.data
    });
    this.kvmSwitchState.keyboardShortcuts.reload();
  }
}
