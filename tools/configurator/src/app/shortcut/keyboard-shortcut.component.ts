import { Component, inject, input, model, output } from '@angular/core';
import { MatIconButton } from '@angular/material/button';
import { MatDialog } from '@angular/material/dialog';
import { MatIcon } from '@angular/material/icon';
import { ShortcutDefinition } from '../web-usb-service';
import {
  ShortcutEditDialogComponent,
  ShortcutEditDialogData,
  ShortcutEditDialogResult
} from './shortcut-edit-dialog.component';
import { ShortcutKeyComponent } from './shortcut-key.component';
import { shortcutActionDefinitions } from './shortcut.model';

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
  protected readonly matDialog = inject(MatDialog);
  readonly actions = shortcutActionDefinitions;
  readonly shortcut = input.required<ShortcutDefinition>();
  readonly editShortcut = output<ShortcutDefinition>();
  readonly deleteShortcut = output<ShortcutDefinition>();

  protected openEditShortcut() {
    let dialogRef = this.matDialog.open<ShortcutEditDialogComponent, ShortcutEditDialogData, ShortcutEditDialogResult>(
      ShortcutEditDialogComponent,
      {
        data: {
          shortcut: this.shortcut()
        }
      }
    );
    dialogRef.afterClosed().subscribe(async (result) => {
      if (!result) {
        return;
      }
      this.editShortcut.emit(result.shortcut);
    });
  }
}
