import { Component, inject } from '@angular/core';
import { MAT_DIALOG_DATA, MatDialogContent, MatDialogTitle } from '@angular/material/dialog';

export type HidDescriptorInspectorDialogData = {
  hidDescriptor: Uint8Array
}

@Component({
  imports: [
    MatDialogTitle,
    MatDialogContent
  ],
  selector: 'app-hid-descriptor-inspector-dialog',
  styleUrl: './hid-descriptor-inspector-dialog.component.scss',
  templateUrl: './hid-descriptor-inspector-dialog.component.html'
})
export class HidDescriptorInspectorDialogComponent {
  readonly data = inject<HidDescriptorInspectorDialogData>(MAT_DIALOG_DATA);
}
