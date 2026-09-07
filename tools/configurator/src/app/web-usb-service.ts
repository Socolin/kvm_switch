import { Service } from '@angular/core';
import { BinaryDataReader, BinaryDataReaderImpl, BinaryDataReaderSizeCalculator } from './utils/binary-data-reader';
import { BinaryDataWriter } from './utils/binary-data-writer';


export type KvmInfo = ReturnType<typeof kvmUsbOperations.GetInfo.deserializeData>;
export type HidState = ReturnType<typeof kvmUsbOperations.GetHidState.deserializeData>;
export type HidDeviceInfo = ReturnType<typeof kvmUsbOperations.GetHidDevice.deserializeData>;
export type ShortcutDefinition = ReturnType<typeof kvmUsbOperations.GetKeyboardShortcut.deserializeData>;
export type ComputerState = ReturnType<typeof kvmUsbOperations.GetComputerState.deserializeData>;

export enum KvmLogLevel {
  Debug,
  Info,
  Warning,
  Error,
  Critical,
}
export type KvmLog = {
  timestamp: bigint
  logLevel: KvmLogLevel,
  line: number,
  function: string,
  message: string,
};

const inCommandOpcode = (opCode: number): number => opCode;
const outCommandOpcode = (opCode: number): number => opCode | 0x80;

type BaseOperation = {
  readonly opCode: number;
  readonly dynamicData?: boolean;
}

type InOperation = BaseOperation & {
  deserializeData(reader: BinaryDataReader): unknown;
  readonly isIn: true;
}

type OutOperation = BaseOperation & {
  readonly isIn: false
  serializeData(data: unknown): BufferSource;
}

export const kvmUsbOperations = {
  GetInfo: {
    opCode: inCommandOpcode(0x01),
    isIn: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        version: reader.getNextUint16(),
        protocolVersion: reader.getNextUint16(),
        computerCount: reader.getNextUint8(),
        hidInterfaceCount: reader.getNextUint8(),
        hidDeviceCount: reader.getNextUint8(),
      };
    }
  } satisfies InOperation,
  GetComputerState: {
    opCode: inCommandOpcode(0x02),
    isIn: true,
    dynamicData: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        computerId: reader.getNextUint8(),
        state: reader.getNextUint8(),
        hidProtocolPerInterface: reader.getNextDynamicArrayOfUint8()
      };
    }
  } satisfies InOperation,
  GetHidDevice: {
    opCode: inCommandOpcode(0x03),
    isIn: true,
    dynamicData: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        devAddr: reader.getNextUint8(),
        isMounted: reader.getNextBool(),
        manufacturerName: reader.getNextDynamicUtf16String(),
        productName: reader.getNextDynamicUtf16String(),
      };
    }

  } satisfies InOperation,
  GetHidState: {
    opCode: inCommandOpcode(0x04),
    isIn: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        enabled: reader.getNextUint8(),
        devAddr: reader.getNextUint8(),
        hostHidIdx: reader.getNextUint8(),
        kvmHidIdx: reader.getNextUint8(),
        itfProtocol: reader.getNextUint8(),
        useReportId: reader.getNextBool(),
        hasKeyboardReport: reader.getNextBool(),
        vid: reader.getNextUint16(),
        pid: reader.getNextUint16()
      };
    }
  } satisfies InOperation,
  GetHidDescriptor: {
    opCode: inCommandOpcode(0x05),
    isIn: true,
    dynamicData: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        descriptor: reader.getNextStaticArrayOfUint8(reader.byteLength)
      };
    }
  } satisfies InOperation,
  GetGeneralConfig: {
    opCode: inCommandOpcode(0x06),
    isIn: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        vid: reader.getNextUint16(),
        pid: reader.getNextUint16()
      };
    }
  } satisfies InOperation,
  SetGeneralConfig: {
    opCode: outCommandOpcode(0x07),
    isIn: false,
    serializeData: (data: { use_custom_vid: boolean, vid: number, use_custom_pid: boolean, pid: number }) => {
      let writer = new BinaryDataWriter();
      writer.writeInt16(data.vid);
      writer.writeInt16(data.pid);
      return writer.getBuffer(true);
    }
  } satisfies OutOperation,
  GetKeyboardShortcuts: {
    opCode: inCommandOpcode(0x08),
    isIn: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        keyboardShortcutCount: reader.getNextUint8()
      };
    }
  } satisfies InOperation,
  GetKeyboardShortcut: {
    opCode: inCommandOpcode(0x09),
    isIn: true,
    dynamicData: true,
    deserializeData: (reader: BinaryDataReader) => {
      return {
        shortcutId: reader.getNextUint8(),
        enabled: reader.getNextBool(),
        action: reader.getNextUint8(),
        keys: reader.getNextDynamicArrayOfUint8(),
        data: reader.getNextDynamicArrayOfUint8()
      };
    }
  } satisfies InOperation,
  SetKeyboardShortcut: {
    opCode: outCommandOpcode(0x0A),
    isIn: false,
    serializeData: (data: {
      shortcut_id: number,
      enabled: boolean,
      action: number,
      key_count: number,
      keys: number[],
      data_len: number,
      data: number[]
    }) => {
      let writer = new BinaryDataWriter();
      writer.writeUint8(data.shortcut_id);
      writer.writeBool(data.enabled);
      writer.writeUint8(data.action);
      writer.writeUint8(data.key_count);
      writer.writeStaticArrayOfUint8(data.keys);
      writer.writeUint8(data.data_len);
      writer.writeStaticArrayOfUint8(data.data);
      return writer.getBuffer(true);
    }
  } satisfies OutOperation,
  GetLogs: {
    opCode: inCommandOpcode(0x0B),
    isIn: true,
    dynamicData: true,
    deserializeData: (reader: BinaryDataReader) => {
      let logs: KvmLog[] = [];
      while (!reader.eof()) {
        let timestamp = reader.getNextUint64();
        let logLevel = reader.getNextUint8();
        let line = reader.getNextUint16();
        let functionName = reader.getNextDynamicString();
        let log = {
          timestamp: timestamp,
          logLevel: logLevel,
          line: line,
          function: functionName,
          message: reader.getNextDynamicString()
        };
        logs.push(log);
      }
      return {
        logs: logs
      };
    }
  } satisfies InOperation
};


@Service()
export class WebUsbService {
  private defaultVendorId = 0x1209;
  private vendorId = this.defaultVendorId;

  async connect() {
    let device = await navigator.usb.requestDevice({ filters: [{ vendorId: this.vendorId }] });
    console.log(device.productName);
    console.log(device.manufacturerName);
    await device.open();
    if (!device.configuration) {
      await device.selectConfiguration(1);
    }
    let interface_index = device.configuration!.interfaces.find((itf) => itf.alternate.interfaceClass === 255)?.interfaceNumber;
    if (!interface_index) {
      throw new Error('No interface found with class 255');
    }
    await device.claimInterface(interface_index);
    return new WebUsbConnection(device, interface_index);
  }
}

export class WebUsbConnection {
  constructor(
    public readonly device: USBDevice,
    public readonly interface_index: number
  ) {
  }

  async executeInOperation<TOperation extends InOperation>(
    operation: TOperation,
    value?: number
  ): Promise<ReturnType<TOperation['deserializeData']>> {
    let size = 0;
    if (operation.dynamicData) {
      size = 4096;
    } else {
      let sizeCalculator = new BinaryDataReaderSizeCalculator();
      operation.deserializeData(sizeCalculator);
      size = sizeCalculator.totalSize;
    }
    let result = await this.device.controlTransferIn(
      {
        requestType: 'vendor',
        recipient: 'interface',
        request: operation.opCode,
        value: value ?? 0x00,
        index: this.interface_index
      },
      size
    );
    if (result.status !== 'ok') {
      throw new Error('Control transfer failed: status is ' + result.status);
    }
    if (!result.data) {
      throw new Error('Control transfer failed: no data');
    }

    return operation.deserializeData(new BinaryDataReaderImpl(result.data)) as ReturnType<TOperation['deserializeData']>;
  }

  async executeOutOperation<TOperation extends OutOperation>(
    operation: TOperation,
    value: number,
    data: Parameters<TOperation['serializeData']>[0]
  ): Promise<void> {
    let serializeData = operation.serializeData(data);

    let result = await this.device.controlTransferOut(
      {
        requestType: 'vendor',
        recipient: 'interface',
        request: operation.opCode,
        value: value ?? 0x00,
        index: this.interface_index
      },
      serializeData
    );

    if (result.status !== 'ok') {
      throw new Error('Control transfer failed');
    }
  }
}
