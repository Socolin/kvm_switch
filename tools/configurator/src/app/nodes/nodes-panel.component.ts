import { Component, inject } from '@angular/core';
import { MatCard, MatCardContent, MatCardHeader, MatCardTitle } from '@angular/material/card';
import { KvmSwitchState } from '../kvm-switch-state';

@Component({
  imports: [
    MatCard,
    MatCardContent,
    MatCardHeader,
    MatCardTitle
  ],
  selector: 'app-nodes-panel',
  styleUrl: './nodes-panel.component.scss',
  templateUrl: './nodes-panel.component.html'
})
export class NodesPanelComponent {
  protected readonly kvmSwitchState = inject(KvmSwitchState);
}
