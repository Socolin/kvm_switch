import { Component, input } from '@angular/core';
import { MatTableModule } from '@angular/material/table';
import { KvmLog, KvmLogLevel } from './web-usb-service';

@Component({
  imports: [
    MatTableModule,
  ],
  selector: 'app-kmv-logs',
  styleUrl: './kmv-logs.component.scss',
  templateUrl: './kmv-logs.component.html'
})
export class KmvLogsComponent {
  logs = input.required<KvmLog[]>();

  logLevelToString(logLevel: KvmLogLevel) {
    switch (logLevel) {
      case KvmLogLevel.Debug:
        return "Debug";
      case KvmLogLevel.Info:
        return "Info";
      case KvmLogLevel.Warning:
        return "Warning";
      case KvmLogLevel.Error:
        return "Error";
      case KvmLogLevel.Critical:
        return "Critical";
    }
  }
}
