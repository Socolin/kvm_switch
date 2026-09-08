import { Component, computed, inject } from '@angular/core';
import { KvmSwitchState } from '../kvm-switch-state';
import { DeviceInfo, HidDeviceComponent } from './hid-device.component';

@Component({
  imports: [
    HidDeviceComponent
  ],
  selector: 'app-hid-devices-panel',
  styleUrl: './hid-devices-panel.component.scss',
  templateUrl: './hid-devices-panel.component.html'
})
export class HidDevicesPanelComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);
  protected readonly Object = Object;
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

}
