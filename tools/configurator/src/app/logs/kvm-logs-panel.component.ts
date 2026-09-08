import { Component, inject } from '@angular/core';
import { MatCard, MatCardContent } from '@angular/material/card';
import { KmvLogsComponent } from './kmv-logs.component';
import { KvmSwitchState } from '../kvm-switch-state';

@Component({
  imports: [
    MatCard,
    MatCardContent,
    KmvLogsComponent
  ],
  selector: 'app-kvm-logs-panel',
  styleUrl: './kvm-logs-panel.component.scss',
  templateUrl: './kvm-logs-panel.component.html'
})
export class KvmLogsPanelComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);
}
