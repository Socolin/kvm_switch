import { Component, input, OnInit, signal } from '@angular/core';
import { MatTooltip } from '@angular/material/tooltip';
import { keyCodes } from './shortcut.model';

@Component({
  imports: [
    MatTooltip
  ],
  selector: 'app-shortcut-key',
  styleUrl: './shortcut-key.component.scss',
  templateUrl: './shortcut-key.component.html'
})
export class ShortcutKeyComponent implements OnInit {
  keyCode = input.required<number>();
  protected readonly keyCodes = keyCodes;

  layoutMap = signal<KeyboardLayoutMap | undefined>(undefined);

  async ngOnInit() {
    if (navigator.keyboard) {
      this.layoutMap.set(await navigator.keyboard.getLayoutMap());
    }
  }
}
