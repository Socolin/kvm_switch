import { Component, inject } from '@angular/core';
import { MatButton } from '@angular/material/button';
import { MatCard, MatCardActions, MatCardContent } from '@angular/material/card';
import { MatDialog } from '@angular/material/dialog';
import { MatIcon } from '@angular/material/icon';
import { KvmSwitchState } from '../kvm-switch-state';
import { ShortcutDefinition } from '../web-usb-service';
import { KeyboardShortcutComponent } from './keyboard-shortcut.component';
import {
  ShortcutEditDialogComponent,
  ShortcutEditDialogData,
  ShortcutEditDialogResult
} from './shortcut-edit-dialog.component';
import { ShortcutService } from './shortcut.service';

@Component({
  imports: [
    KeyboardShortcutComponent,
    MatButton,
    MatCard,
    MatCardActions,
    MatCardContent,
    MatIcon
  ],
  selector: 'app-keyboard-shortcuts-panel',
  styleUrl: './keyboard-shortcuts-panel.component.scss',
  templateUrl: './keyboard-shortcuts-panel.component.html'
})
export class KeyboardShortcutsPanelComponent {
  protected readonly shortcutService = inject(ShortcutService);
  protected readonly kvmSwitchState = inject(KvmSwitchState);
  protected readonly matDialog = inject(MatDialog);

  protected addShortcut() {
    let dialogRef = this.matDialog.open<ShortcutEditDialogComponent, ShortcutEditDialogData, ShortcutEditDialogResult>(
      ShortcutEditDialogComponent,
      {
        data: {
          shortcut: {
            data: new Uint8Array(),
            keys: [],
            shortcutId: -1,
            action: 0,
            enabled: false
          }
        }
      }
    );
    dialogRef.afterClosed().subscribe(async (result) => {
      if (!result) {
        return;
      }
      await this.shortcutService.addShortcut(result.shortcut);
    });
  }

  protected async editShortcut(shortcut: ShortcutDefinition) {
    await this.shortcutService.saveShortcut(shortcut);
  }

  protected async deleteShortcut(shortcut: ShortcutDefinition) {
    await this.shortcutService.deleteShortcut(shortcut);
  }
}
