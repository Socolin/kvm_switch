import { Component, HostListener, inject, signal } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MatButton, MatIconButton } from '@angular/material/button';
import {
  MAT_DIALOG_DATA,
  MatDialogActions,
  MatDialogClose,
  MatDialogContent,
  MatDialogRef,
  MatDialogTitle
} from '@angular/material/dialog';
import { MatIcon } from '@angular/material/icon';
import { MatInput } from '@angular/material/input';
import { MatFormField, MatLabel, MatOption, MatSelect } from '@angular/material/select';
import { ShortcutKeyComponent } from './shortcut-key.component';
import { keyCodes, ShortcutActionDataDescriptor, shortcutActionDefinitions } from './shortcut.model';
import { ShortcutDefinition } from '../web-usb-service';

export type ShortcutEditDialogData = {
  shortcut: ShortcutDefinition
}
export type ShortcutEditDialogResult = {
  shortcut: ShortcutDefinition
}

@Component({
  imports: [
    MatDialogTitle,
    MatDialogContent,
    MatDialogActions,
    MatDialogClose,
    MatButton,
    MatSelect,
    MatOption,
    FormsModule,
    MatFormField,
    MatLabel,
    MatIconButton,
    MatIcon,
    ShortcutKeyComponent,
    MatInput
  ],
  selector: 'app-shortcut-edit-dialog',
  styleUrl: './shortcut-edit-dialog.component.scss',
  templateUrl: './shortcut-edit-dialog.component.html'
})
export class ShortcutEditDialogComponent {
  protected readonly Object = Object;

  private dialogRef = inject<MatDialogRef<ShortcutEditDialogComponent, ShortcutEditDialogResult>>(
    MatDialogRef<ShortcutEditDialogComponent, ShortcutEditDialogResult>
  );

  readonly data = inject<ShortcutEditDialogData>(MAT_DIALOG_DATA);
  readonly shortcutActionDefinitions = shortcutActionDefinitions;

  actionId = signal(this.data.shortcut.action);
  keys = signal(this.data.shortcut.keys);
  editingKeyIdx = signal(-1);
  actionData = signal(this.data.shortcut.data);

  @HostListener('document:keydown', ['$event']) onKeydownHandler(event: KeyboardEvent) {
    if (this.editingKeyIdx() == -1) {
      return;
    }

    let pressedKey = Object.entries(keyCodes).find(([keyCode, keyInfo]) => keyInfo.jsKeyCode === event.code);
    if (!pressedKey) {
      console.warn('Unknown key pressed: ', event.code);
      return;
    }

    event.preventDefault();
    event.stopImmediatePropagation();
    event.stopPropagation();

    this.keys.update(keys => {
      let newKeys = [...keys];
      newKeys[this.editingKeyIdx()] = +pressedKey[0];
      return newKeys;
    });
    this.editingKeyIdx.set(-1);

    setTimeout(() => {
      this.dialogRef.disableClose = false;
    });

  }

  save() {
    this.dialogRef.close({
      shortcut: {
        shortcutId: this.data.shortcut.shortcutId,
        action: this.actionId(),
        enabled: true,
        keys: this.keys(),
        data: this.actionData()
      }
    });
  }

  editKey(keyIndex: number) {
    this.dialogRef.disableClose = true;
    this.editingKeyIdx.set(keyIndex);
  }

  removeKey(keyIndex: number) {
    this.keys.update((keys) => {
      let newKeys = [...keys];
      newKeys.splice(keyIndex, 1);
      return newKeys;
    });
  }

  addKey() {
    this.keys.update((keys) => [...keys, 0]);
    this.editKey(this.keys().length - 1);
  }

  protected getData(dataDescriptor: ShortcutActionDataDescriptor) {
    let dataView = new DataView(this.actionData().buffer);
    switch (dataDescriptor.dataType) {
      case 'uint8':
        return dataView.getUint8(dataDescriptor.dataOffset);
    }
  }

  protected setData(dataDescriptor: ShortcutActionDataDescriptor, $event: any) {
    this.actionData.update(b => {
      let newBuffer = b.buffer.slice();
      let dataView = new DataView(newBuffer, b.byteOffset, b.byteLength);
      dataView.setUint8(dataDescriptor.dataOffset, $event);
      return new Uint8Array(newBuffer);
    });
  }

  protected setAction(actionId: number) {
    this.actionId.set(actionId);

    let totalDataSize = 0;
    for (let dataDescriptor of this.shortcutActionDefinitions[actionId].dataDescriptor) {
      totalDataSize += dataDescriptor.dataSize;
    }
    this.actionData.set(new Uint8Array(totalDataSize))
  }
}
