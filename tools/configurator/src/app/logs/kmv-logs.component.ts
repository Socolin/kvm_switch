import { DatePipe } from '@angular/common';
import { Component, computed, input, signal } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MatCheckbox } from '@angular/material/checkbox';
import { MatFormField, MatLabel } from '@angular/material/input';
import { MatOption, MatSelect } from '@angular/material/select';
import { MatTableModule } from '@angular/material/table';
import { MatTooltip } from '@angular/material/tooltip';
import { BigIntPipe } from '../utils/big-int-pipe';
import { KvmLog, KvmLogLevel, KvmSwitchInfo } from '../web-usb-service';

@Component({
  imports: [
    MatTableModule,
    BigIntPipe,
    MatTooltip,
    MatFormField,
    MatSelect,
    MatOption,
    FormsModule,
    MatLabel,
    MatCheckbox,
    DatePipe
  ],
  selector: 'app-kmv-logs',
  styleUrl: './kmv-logs.component.scss',
  templateUrl: './kmv-logs.component.html'
})
export class KmvLogsComponent {
  logs = input.required<KvmLog[]>();
  kvmStartTime = input.required<Date>();

  minLogLevel = signal<KvmLogLevel>(KvmLogLevel.Debug);
  filteredLogs = computed(() => {
    return this.logs().filter(x => x.logLevel >= this.minLogLevel());
  });

  showTimestamp = signal<boolean>(false);

  convertTimestampToDate(timestamp: bigint): Date {
    return new Date(this.kvmStartTime().getTime() + Number(timestamp / 1_000n));
  }

  logLevelToString(logLevel: KvmLogLevel) {
    switch (logLevel) {
      case KvmLogLevel.Debug:
        return 'Debug';
      case KvmLogLevel.Info:
        return 'Info';
      case KvmLogLevel.Warning:
        return 'Warning';
      case KvmLogLevel.Error:
        return 'Error';
      case KvmLogLevel.Critical:
        return 'Critical';
    }
  }

  protected readonly KvmLogLevel = KvmLogLevel;
}
