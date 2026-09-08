import { Component, inject } from '@angular/core';
import { MatButton } from '@angular/material/button';
import { MatToolbar } from '@angular/material/toolbar';
import { GeneralConfigPanelComponent } from './general-config/general-config-panel.component';
import { HidDevicesPanelComponent } from './hid/hid-devices-panel.component';
import { KvmSwitchState } from './kvm-switch-state';
import { KvmLogsPanelComponent } from './logs/kvm-logs-panel.component';
import { NodesPanelComponent } from './nodes/nodes-panel.component';
import { KeyboardShortcutsPanelComponent } from './shortcut/keyboard-shortcuts-panel.component';


@Component({
  imports: [MatButton, MatToolbar, KeyboardShortcutsPanelComponent, HidDevicesPanelComponent, NodesPanelComponent, KvmLogsPanelComponent, GeneralConfigPanelComponent],
  selector: 'app-root',
  styleUrl: './app.component.scss',
  templateUrl: './app.component.html'
})
export class AppComponent {
  protected readonly Object = Object;
  protected readonly kvmSwitchState = inject(KvmSwitchState);

}
