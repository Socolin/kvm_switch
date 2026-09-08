import { Component, inject, signal } from '@angular/core';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { MatButton } from '@angular/material/button';
import {
  MAT_DIALOG_DATA,
  MatDialogActions,
  MatDialogClose,
  MatDialogContent,
  MatDialogRef,
  MatDialogTitle
} from '@angular/material/dialog';
import { MatFormField, MatHint, MatInput, MatLabel } from '@angular/material/input';
import { GeneralConfig } from '../web-usb-service';

export type GenericConfigEditorDialogData = {
  generalConfig: GeneralConfig
}

export type GenericConfigEditorDialogResult = {
  generalConfig: GeneralConfig
}

@Component({
  imports: [
    MatButton,
    MatDialogActions,
    MatDialogClose,
    MatDialogContent,
    MatDialogTitle,
    MatFormField,
    MatInput,
    ReactiveFormsModule,
    FormsModule,
    MatLabel,
    MatHint
  ],
  selector: 'app-generic-config-editor-dialog',
  styleUrl: './generic-config-editor-dialog.component.scss',
  templateUrl: './generic-config-editor-dialog.component.html'
})
export class GenericConfigEditorDialogComponent {
  private dialogRef = inject<MatDialogRef<GenericConfigEditorDialogComponent, GenericConfigEditorDialogResult>>(
    MatDialogRef<GenericConfigEditorDialogComponent, GenericConfigEditorDialogResult>
  );
  readonly data = inject<GenericConfigEditorDialogData>(MAT_DIALOG_DATA);

  vid = signal<number>(this.data.generalConfig.vid);
  pid = signal<number>(this.data.generalConfig.pid);

  protected save() {
    this.dialogRef.close({
      generalConfig: {
        vid: this.vid(),
        pid: this.pid()
      }
    });
  }
}
