import { Component, computed, inject } from '@angular/core';
import { MatButton } from '@angular/material/button';
import { MatCard, MatCardContent, MatCardHeader, MatCardSubtitle, MatCardTitle } from '@angular/material/card';
import { MatDivider } from '@angular/material/list';
import { MatToolbar } from '@angular/material/toolbar';
import { DeviceInfo, HidDeviceComponent } from './hid-device.component';
import { KeyboardShortcutComponent } from './keyboard-shortcut.component';
import { KmvLogsComponent } from './kmv-logs.component';
import { KvmSwitchState } from './kvm-switch-state';


@Component({
  imports: [MatButton, MatCard, MatCardHeader, MatCardContent, MatCardTitle, MatToolbar, MatCardSubtitle, HidDeviceComponent, KmvLogsComponent, MatDivider, KeyboardShortcutComponent],
  selector: 'app-root',
  styleUrl: './app.component.scss',
  templateUrl: './app.component.html'
})
export class AppComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);

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
}
