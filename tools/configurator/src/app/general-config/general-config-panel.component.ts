import { Component, inject } from '@angular/core';
import { MatIconButton } from '@angular/material/button';
import { MatCard, MatCardContent, MatCardHeader, MatCardTitle } from '@angular/material/card';
import { MatDialog } from '@angular/material/dialog';
import { MatIcon } from '@angular/material/icon';
import { KvmSwitchState } from '../kvm-switch-state';
import { GeneralConfig, kvmUsbOperations } from '../web-usb-service';
import {
  GenericConfigEditorDialogComponent,
  GenericConfigEditorDialogData,
  GenericConfigEditorDialogResult
} from './generic-config-editor-dialog.component';

@Component({
  imports: [
    MatCard,
    MatCardContent,
    MatCardHeader,
    MatCardTitle,
    MatIconButton,
    MatIcon
  ],
  selector: 'app-general-config-panel',
  styleUrl: './general-config-panel.component.scss',
  templateUrl: './general-config-panel.component.html'
})
export class GeneralConfigPanelComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);
  protected readonly matDialog = inject(MatDialog);

  protected openEditGeneralConfig(config: GeneralConfig) {
    let dialogRef = this.matDialog.open<GenericConfigEditorDialogComponent, GenericConfigEditorDialogData, GenericConfigEditorDialogResult>(
      GenericConfigEditorDialogComponent, {
        data: {
          generalConfig: config
        }
      });

    dialogRef.afterClosed().subscribe(async (result) => {
      if (!result) {
        return;
      }

      let webUsbConnection = this.kvmSwitchState.webUsbConnection();
      if (!webUsbConnection) {
        return;
      }

      await webUsbConnection.executeOutOperation(kvmUsbOperations.SetGeneralConfig, 0, {
        pid: result?.generalConfig.pid,
        vid: result?.generalConfig.vid
      });
    });
  }
}
