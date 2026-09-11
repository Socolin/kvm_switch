import { Component, inject } from '@angular/core';
import { MatButton } from '@angular/material/button';
import {
  MAT_DIALOG_DATA,
  MatDialogActions,
  MatDialogClose,
  MatDialogContent,
  MatDialogTitle
} from '@angular/material/dialog';
import { MatTooltip } from '@angular/material/tooltip';
import { HidItemType, HidReportDescriptorDecoder, MainItemTag } from './hid-report-descriptor';

export type HidDescriptorInspectorDialogData = {
  hidDescriptor: Uint8Array
}

@Component({
  imports: [
    MatDialogTitle,
    MatDialogContent,
    MatTooltip,
    MatButton,
    MatDialogActions,
    MatDialogClose
  ],
  selector: 'app-hid-descriptor-inspector-dialog',
  styleUrl: './hid-descriptor-inspector-dialog.component.scss',
  templateUrl: './hid-descriptor-inspector-dialog.component.html'
})
export class HidDescriptorInspectorDialogComponent {
  protected readonly HidItemType = HidItemType;
  protected readonly MainItemTag = MainItemTag;
  readonly data = inject<HidDescriptorInspectorDialogData>(MAT_DIALOG_DATA);
  readonly hidReportDescriptor = new HidReportDescriptorDecoder().decodeHidReportDescriptor(this.data.hidDescriptor);

  * bufferAsUint8Numbers(buffer: DataView) {
    for (let i = 0; i < buffer.byteLength; i++) {
      yield buffer.getUint8(i);
    }
  }

  hidItemTypeToString(hidItemType: HidItemType): string {
    switch (hidItemType) {
      case HidItemType.Global:
        return 'Global';
      case HidItemType.Local:
        return 'Local';
      case HidItemType.Main:
        return 'Main';
    }
  }

  protected range(start: number, count: number) {
    let array = [];
    for (let i = start; i < start + count; i++) {
      array.push(i);
    }
    return array;
  }
}
