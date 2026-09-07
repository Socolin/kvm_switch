import { Component, inject, model } from '@angular/core';
import { MatIconButton } from '@angular/material/button';
import { MatDialog } from '@angular/material/dialog';
import { MatIcon } from '@angular/material/icon';
import {
  ShortcutEditDialogComponent,
  ShortcutEditDialogData,
  ShortcutEditDialogResult
} from './shortcut-edit-dialog.component';
import { ShortcutKeyComponent } from './shortcut-key.component';
import { shortcutActionDefinitions } from './shortcut.model';
import { ShortcutDefinition } from './web-usb-service';

@Component({
  imports: [
    MatIconButton,
    MatIcon,
    ShortcutKeyComponent
  ],
  selector: 'app-keyboard-shortcut',
  styleUrl: './keyboard-shortcut.component.scss',
  templateUrl: './keyboard-shortcut.component.html'
})
export class KeyboardShortcutComponent {
  shortcut = model.required<ShortcutDefinition>();

  protected readonly matDialog = inject(MatDialog);

  actions = shortcutActionDefinitions;

  protected editShortcut() {

    let dialogRef = this.matDialog.open<ShortcutEditDialogComponent, ShortcutEditDialogData, ShortcutEditDialogResult>(
      ShortcutEditDialogComponent,
      {
        data: {
          shortcut: this.shortcut()
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
