import { Component, computed, input } from '@angular/core';
import { MatCard, MatCardContent, MatCardHeader, MatCardSubtitle, MatCardTitle } from '@angular/material/card';
import {
  MatAccordion,
  MatExpansionPanel,
  MatExpansionPanelHeader,
  MatExpansionPanelTitle
} from '@angular/material/expansion';
import { MatIcon } from '@angular/material/icon';
import { getUsbVendorById } from 'usb-vendor-ids';
import { HidState } from './web-usb-service';

export type DeviceInfo = {
  devAddr: number,
  vid: number,
  pid: number,
  hidInterfaces: HidState[],
  manufacturerName?: string,
  productName?: string
};

@Component({
  imports: [
    MatAccordion,
    MatExpansionPanel,
    MatExpansionPanelHeader,
    MatExpansionPanelTitle,
    MatCard,
    MatCardContent,
    MatCardHeader,
    MatCardTitle,
    MatCardSubtitle,
    MatIcon
  ],
  selector: 'app-hid-device',
  styleUrl: './hid-device.component.scss',
  templateUrl: './hid-device.component.html'
})
export class HidDeviceComponent {
  device = input.required<DeviceInfo>();
  vendorName = computed(() => {
    return getUsbVendorById(this.device().vid);
  });

  getItfProtocolName(itfProtocol: number): string {
    switch (itfProtocol) {
      case 0:
        return 'None';
      case 1:
        return 'Keyboard';
      case 2:
        return 'Mouse';
      default:
        return 'Unknown';
    }
  }
}
