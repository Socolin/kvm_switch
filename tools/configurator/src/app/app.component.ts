import { Component, computed, inject } from '@angular/core';
import { MatButton } from '@angular/material/button';
import { MatCard, MatCardActions, MatCardContent, MatCardHeader, MatCardTitle } from '@angular/material/card';
import { MatDialog } from '@angular/material/dialog';
import { MatIcon } from '@angular/material/icon';
import { MatToolbar } from '@angular/material/toolbar';
import { DeviceInfo, HidDeviceComponent } from './hid-device.component';
import { KeyboardShortcutComponent } from './keyboard-shortcut.component';
import { KmvLogsComponent } from './kmv-logs.component';
import { KvmSwitchState } from './kvm-switch-state';
import {
  ShortcutEditDialogComponent,
  ShortcutEditDialogData,
  ShortcutEditDialogResult
} from './shortcut-edit-dialog.component';


@Component({
  imports: [MatButton, MatCard, MatCardHeader, MatCardContent, MatCardTitle, MatToolbar, HidDeviceComponent, KmvLogsComponent, KeyboardShortcutComponent, MatIcon, MatCardActions],
  selector: 'app-root',
  styleUrl: './app.component.scss',
  templateUrl: './app.component.html'
})
export class AppComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);
  protected readonly matDialog = inject(MatDialog);

  protected readonly hidDevices = computed(() => {
    let devices: Record<number, DeviceInfo> = {};

    if (!this.kvmSwitchState.hidInterfaces.hasValue())
      return devices;

    for (let hidInterface of this.kvmSwitchState.hidInterfaces.value().hidInterfaces) {
      let device = devices[hidInterface.devAddr];
      if (!device)
        device = {
          vid: hidInterface.vid,
          pid: hidInterface.pid,
          devAddr: hidInterface.devAddr,
          hidInterfaces: []
        };
      device.hidInterfaces.push(hidInterface);
      devices[hidInterface.devAddr] = device;

      if (this.kvmSwitchState.hidDevicesInfo.hasValue()) {
        let deviceInfo = this.kvmSwitchState.hidDevicesInfo.value()[device.devAddr];
        if (deviceInfo) {
          device.manufacturerName = deviceInfo.manufacturerName;
          device.productName = deviceInfo.productName;
        }
      }

    }

    return devices;
  });
  protected readonly Object = Object;

  protected addShortcut() {
    let dialogRef = this.matDialog.open<ShortcutEditDialogComponent, ShortcutEditDialogData, ShortcutEditDialogResult>(
      ShortcutEditDialogComponent,
      {
        data: {
          shortcut: {
            data: new Uint8Array(),
            keys: [],
            shortcutId: -1,
            action: 0,
            enabled: false
          }
        }
      }
    );
    dialogRef.afterClosed().subscribe((result) => {
      if (!result) {
        return;
      }
    });
  }
}
