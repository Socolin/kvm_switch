import { Component, computed, inject, input } from '@angular/core';
import { MatButton } from '@angular/material/button';
import { MatCard, MatCardContent, MatCardHeader, MatCardSubtitle, MatCardTitle } from '@angular/material/card';
import { MatDialog } from '@angular/material/dialog';
import {
  MatAccordion,
  MatExpansionPanel,
  MatExpansionPanelHeader,
  MatExpansionPanelTitle
} from '@angular/material/expansion';
import { MatIcon } from '@angular/material/icon';
import { getUsbVendorById } from 'usb-vendor-ids';
import { KvmSwitchState } from '../kvm-switch-state';
import { HidState, kvmUsbOperations } from '../web-usb-service';
import {
  HidDescriptorInspectorDialogComponent,
  HidDescriptorInspectorDialogData
} from './hid-descriptor-inspector-dialog.component';

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
    MatIcon,
    MatButton
  ],
  selector: 'app-hid-device',
  styleUrl: './hid-device.component.scss',
  templateUrl: './hid-device.component.html'
})
export class HidDeviceComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);
  protected readonly matDialog = inject(MatDialog);

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

  protected async openHidInspector(kvmHidIdx: number) {
    let webUsbConnection = this.kvmSwitchState.webUsbConnection();
    if (!webUsbConnection) {
      return;
    }

    let hidDescriptorResult = await webUsbConnection.executeInOperation(kvmUsbOperations.GetHidDescriptor, kvmHidIdx);
    this.matDialog.open<HidDescriptorInspectorDialogComponent, HidDescriptorInspectorDialogData>(HidDescriptorInspectorDialogComponent, {
      data: {
        hidDescriptor: hidDescriptorResult.descriptor
      }
    });
  }
}
