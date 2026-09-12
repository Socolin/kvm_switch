import { computed, effect, inject, resource, Service, signal } from '@angular/core';
import {
  ComputerState,
  HidDeviceInfo,
  HidState,
  KvmLog,
  kvmUsbOperations,
  ShortcutDefinition,
  WebUsbConnection,
  WebUsbService
} from './web-usb-service';

@Service()
export class KvmSwitchState {
  readonly webUsbService = inject(WebUsbService);
  readonly webUsbConnection = signal<WebUsbConnection | undefined>(undefined);
  readonly isConnected = computed(() => this.webUsbConnection()?.isConnected());

  constructor() {
    effect(() => {
      if (!this.isConnected()) {
        clearInterval(this.logIntervalId);
        this.logIntervalId = 0;
        this.logs.set([]);
      }
    });
  }

  async refresh() {
    this.hidInterfaces.reload();
    this.keyboardShortcuts.reload();
    this.computerStates.reload();
    this.hidDevicesInfo.reload();
  }

  private logIntervalId = 0;

  kvmStartTime() {
    let kvmInfo = this.kvmInfo.value();
    if (!kvmInfo) {
      return new Date();
    }
    return new Date(kvmInfo.now - Number(kvmInfo.currentTime / 1_000n))
  }

  async connectToKvm() {
    clearInterval(this.logIntervalId);
    this.logs.set([]);
    this.webUsbConnection.set(undefined);

    let connection = await this.webUsbService.connect();
    this.webUsbConnection.set(connection);
    let getLogsResult = await connection.executeInOperation(kvmUsbOperations.GetLogs);
    this.logs.set(getLogsResult.logs);
    this.logIntervalId = setInterval(async () => {
      let webUsbConnection = this.webUsbConnection();
      if (webUsbConnection) {
        let getLogsResult = await webUsbConnection.executeInOperation(kvmUsbOperations.GetLogs);
        this.logs.update((value) => [...value, ...getLogsResult.logs]);
      }
    }, 250);
  }

  readonly kvmInfo = resource({
    params: () => ({ webUsbConnection: this.webUsbConnection() }),
    loader: async ({ params }) => await params.webUsbConnection?.executeInOperation(kvmUsbOperations.GetInfo)
  });

  readonly generalConfig = resource({
    params: () => ({ webUsbConnection: this.webUsbConnection() }),
    loader: async ({ params }) => await params.webUsbConnection?.executeInOperation(kvmUsbOperations.GetGeneralConfig)
  });

  readonly hidInterfaces = resource({
    params: () => ({
      webUsbConnection: this.webUsbConnection(),
      kvmInfo: this.kvmInfo.value(),
      isConnected: this.isConnected()
    }),
    loader: async ({ params }) => {
      let result = { hidInterfaces: [] as HidState[] };
      if (!params.kvmInfo)
        return undefined;
      if (!params.isConnected) {
        return undefined;
      }

      for (let i = 0; i < params.kvmInfo.hidInterfaceCount; i++) {
        let hidState = await params.webUsbConnection?.executeInOperation(kvmUsbOperations.GetHidState, i);
        if (hidState && hidState.enabled)
          result.hidInterfaces.push(hidState);
      }

      return result;
    }
  });

  readonly hidDevicesInfo = resource({
    params: () => ({
      webUsbConnection: this.webUsbConnection(),
      kvmInfo: this.kvmInfo.value(),
      isConnected: this.isConnected()
    }),
    loader: async ({ params }) => {
      let result: Record<number, HidDeviceInfo> = {};
      if (!params.kvmInfo)
        return undefined;
      if (!params.isConnected) {
        return undefined;
      }

      for (let devAddr = 1; devAddr <= params.kvmInfo.hidDeviceCount; devAddr++) {
        let hidDeviceInfo = await params.webUsbConnection?.executeInOperation(kvmUsbOperations.GetHidDevice, devAddr);
        if (hidDeviceInfo && hidDeviceInfo.isMounted)
          result[hidDeviceInfo.devAddr] = hidDeviceInfo;
      }

      return result;
    }
  });

  readonly keyboardShortcuts = resource({
    params: () => ({
      webUsbConnection: this.webUsbConnection(),
      isConnected: this.isConnected()
    }),
    loader: async ({ params }) => {
      let result: ShortcutDefinition[] = [];
      if (!params.webUsbConnection) {
        return undefined;
      }
      if (!params.isConnected) {
        return undefined;
      }

      let shortcutsInfo = await params.webUsbConnection.executeInOperation(kvmUsbOperations.GetKeyboardShortcuts);
      for (let shortcutId = 0; shortcutId < shortcutsInfo.keyboardShortcutCount; shortcutId++) {
        let shortcut = await params.webUsbConnection.executeInOperation(kvmUsbOperations.GetKeyboardShortcut, shortcutId);
        result.push(shortcut);
      }

      return result;
    }
  });

  readonly computerStates = resource({
    params: () => ({
      webUsbConnection: this.webUsbConnection(),
      kvmInfo: this.kvmInfo.value(),
      isConnected: this.isConnected()
    }),
    loader: async ({ params }) => {
      if (!params.kvmInfo)
        return undefined;
      if (!params.webUsbConnection) {
        return undefined;
      }
      if (!params.isConnected) {
        return undefined;
      }

      let result: ComputerState[] = [];

      for (let computerId = 0; computerId < params.kvmInfo.computerCount; computerId++) {
        let computerState = await params.webUsbConnection.executeInOperation(kvmUsbOperations.GetComputerState, computerId);
        result.push(computerState);
      }

      return result;
    }
  });

  readonly logs = signal<KvmLog[]>([]);
}
