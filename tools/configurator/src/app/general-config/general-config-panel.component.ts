import { Component, inject } from '@angular/core';
import { MatCard, MatCardContent, MatCardHeader, MatCardSubtitle, MatCardTitle } from '@angular/material/card';
import { KvmSwitchState } from '../kvm-switch-state';

@Component({
  imports: [
    MatCard,
    MatCardContent,
    MatCardHeader,
    MatCardTitle,
    MatCardSubtitle
  ],
  selector: 'app-general-config-panel',
  styleUrl: './general-config-panel.component.scss',
  templateUrl: './general-config-panel.component.html'
})
export class GeneralConfigPanelComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);
}
