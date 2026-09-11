type HidDescriptorField = {
  mainItemFlags: {
    value: number;
    parsedValue: {
      // FIXME:
    }
  }

  usage_page: number;
  usage: {
    isRange: true,
    range: {
      min: number,
      max: number
    }
  } | {
    isRange: false,
    value: number
  }

  logical: {
    min: number;
    max: number;
  };
  physical: {
    min: number;
    max: number;
  };

  unitExponent: number;
  unit: number;

  bitSize: number;
}

export enum HidReportType {
  Input,
  Output,
  Feature
}

export enum HidItemType {
  Main = 0,
  Global = 1,
  Local = 2,
}

export enum MainItemTag {
  Input = 8,
  Output = 9,
  Collection = 10,
  Feature = 11,
  CollectionEnd = 12,
}

export enum GlobalItemTag {
  UsagePage = 0,
  LogicalMin = 1,
  LogicalMax = 2,
  PhysicalMin = 3,
  PhysicalMax = 4,
  UnitExponent = 5,
  Unit = 6,
  ReportSize = 7,
  ReportId = 8,
  ReportCount = 9,
  Push = 10,
  Pop = 11
}

export enum LocalItemTag {
  Usage = 0,
  UsageMin = 1,
  UsageMax = 2,
  DesignatorIndex = 3,
  DesignatorMin = 4,
  DesignatorMax = 5,
  StringIndex = 6,
  StringMin = 7,
  StringMax = 8,
  Delimiter = 9,
}

export type RawHidItem = {
  dataSize: 1 | 2 | 4;
  rawData: DataView;
  rawItem: DataView;
} & ({
  itemType: HidItemType.Main;
  tag: MainItemTag;
} | {
  itemType: HidItemType.Local;
  tag: LocalItemTag;
} | {
  itemType: HidItemType.Global;
  tag: GlobalItemTag;
});

export type HidDescriptorItem = RawHidItem & {
  depth: number,
  text: string,
  formattedData?: string,
  data?: number,
};

export enum HidCollectionType {
  Physical,
  Application,
  Logical,
  Report,
  NamedArray,
  UsageSwitch,
  UsageModifier,
}

export type HidCollection = {
  usage: number,
  usagePage: number,
  type: HidCollectionType,
  parentCollection?: HidCollection,
}

export type HidReportDefinition = {
  reportId: number,
  reportType: HidReportType,
  fields: HidDescriptorField[],
}

export type HidReportDescriptor = {
  items: HidDescriptorItem[],
  reports: HidReportDefinition[],
  collections: HidCollection[]
}

type GlobalState = {
  usagePage: number,
  logicalMin: number,
  logicalMax: number,
  physicalMin: number,
  physicalMax: number,
  unitExponent: number,
  unit: number,
  reportSize: number,
  reportCount: number,
  reportId: number
}

type HidUsage = {
  usage: number;
  usagePage?: number;
}

type LocalState = {
  useRange: boolean;
  usageCount: number;
  usages: HidUsage[];
  usageMin?: HidUsage;
  usageMax?: HidUsage;
  designatorIndex: number;
  designatorMin: number;
  designatorMax: number;
  stringIndex: number;
  stringMin: number;
  stringMax: number;
  delimiter: number;
}

type MainItemFlags = {
  dataConstant: boolean;
  arrayVariable: boolean;
  absoluteRelative: boolean;
  noWrapWrap: boolean;
  linearNonLinear: boolean;
  preferredStateNoPreferred: boolean;
  noNullPositionNullState: boolean;
  nonVolatileVolatile: boolean;
  bitFieldBufferedBytes: boolean;
}

class DecodeContext {
  globalState: GlobalState;
  globalStatesStack: GlobalState[] = [];
  localState: LocalState;
  collectionStack: HidCollection[] = [];

  constructor() {
    this.globalState = {
      usagePage: 0,
      logicalMin: 0,
      logicalMax: 0,
      physicalMin: 0,
      physicalMax: 0,
      unitExponent: 0,
      unit: 0,
      reportSize: 0,
      reportCount: 0,
      reportId: 0
    };
    this.localState = this.initLocalStorage();
  }

  initLocalStorage() {
    return {
      useRange: false,
      usageCount: 0,
      usages: [],
      designatorIndex: 0,
      designatorMin: 0,
      designatorMax: 0,
      stringIndex: 0,
      stringMin: 0,
      stringMax: 0,
      delimiter: 0
    };
  }

  pushGlobalState() {
    this.globalStatesStack.push({ ...this.globalState });
  }

  popGlobalState() {
    let globalState = this.globalStatesStack.pop();
    if (!globalState) {
      throw new Error('Cannot pop global state, no state left in stack');
    }
    this.globalState = globalState;
  }

  updateGlobalState(updater: (state: GlobalState) => void): void {
    updater(this.globalState);
  }

  updateLocalState(updater: (state: LocalState) => void): void {
    updater(this.localState);
  }

  createCollection(type: HidCollectionType): HidCollection {

    let parentCollection = this.collectionStack.length > 0 ? this.collectionStack[this.collectionStack.length - 1] : undefined;
    let collection: HidCollection = {
      usagePage: this.localState.usages[0].usagePage ?? this.globalState.usagePage,
      usage: this.localState.usages[0].usage,
      parentCollection: parentCollection,
      type: type
    };
    this.collectionStack.push(collection);
    return collection;
  }

  endCollection(): HidCollection | undefined {
    let collection = this.collectionStack.pop();
    return collection?.parentCollection;
  }

  clearLocalContext() {
    this.localState = this.initLocalStorage();
  }
}

// https://usb.org/document-library/hid-usage-tables-17
const usagePages: Record<number, { name: string, usages: Record<number, { name: string }> }> = {
  0: {
    name: 'Undefined',
    usages: {},
  },
  1: {
    name: 'Generic Desktop Page',
    usages: {
      0x00: { name: 'Undefined' },
      0x01: { name: 'Pointer' },
      0x02: { name: 'Mouse' },
      0x04: { name: 'Joystick' },
      0x05: { name: 'Gamepad' },
      0x06: { name: 'Keyboard' },
      0x07: { name: 'Keypad' },
    },
  },
  2: {
    name: 'Simulation Controls Page',
    usages: {},
  },
  0x03: {
    name: 'VR Controls Page',
    usages: {},
  },
  0x04: {
    name: 'Sport Controls Page',
    usages: {},
  },
  0x05: {
    name: 'Game Controls Page',
    usages: {},
  },
  0x06: {
    name: 'Generic Device Controls Page',
    usages: {},
  },
  0x07: {
    name: 'Keyboard/Keypad Page',
    usages: {},
  },
  0x08: {
    name: 'LED Page',
    usages: {},
  },
  0x09: {
    name: 'Button Page',
    usages: {},
  },
  0x0A: {
    name: 'Ordinal Page',
    usages: {},
  },
  0x0B: {
    name: 'Telephony Device Page',
    usages: {},
  },
  0x0C: {
    name: 'Consumer Page',
    usages: {},
  },
  0x0D: {
    name: 'Digitizers Page',
    usages: {},
  },
  0x0E: {
    name: 'Haptics Page',
    usages: {},
  },
  0x0F: {
    name: 'Physical Input Device Page',
    usages: {},
  },
  0x10: {
    name: 'Unicode Page',
    usages: {},
  },
  0x11: {
    name: 'SoC Page',
    usages: {},
  },
  0x12: {
    name: 'Eye and Head Trackers Page',
    usages: {},
  },
  0x14: {
    name: 'Auxiliary Display Page',
    usages: {},
  },
  0x20: {
    name: 'Sensors Page',
    usages: {},
  },
  0x40: {
    name: 'Medical Instrument Page',
    usages: {},
  },
  0x41: {
    name: 'Braille Display Page',
    usages: {},
  },
  0x59: {
    name: 'Lighting And Illumination Page',
    usages: {},
  },
  0x80: {
    name: 'Monitor Page',
    usages: {},
  },
  0x81: {
    name: 'Monitor Enumerated Page',
    usages: {},
  },
  0x82: {
    name: 'VESA Virtual Controls Page',
    usages: {},
  },
  0x84: {
    name: 'Power Page',
    usages: {},
  },
  0x85: {
    name: 'Battery System Page',
    usages: {},
  },
  0x8C: {
    name: 'Barcode Scanner Page',
    usages: {},
  },
  0x8D: {
    name: 'Scales Page',
    usages: {},
  },
  0x8E: {
    name: 'Magnetic Stripe Reader Page',
    usages: {},
  },
  0x90: {
    name: 'Camera Control Page',
    usages: {},
  },
  0x91: {
    name: 'Arcade Page',
    usages: {},
  },
  0x92: {
    name: 'Gaming Device Page',
    usages: {},
  },
  0xF1D0: {
    name: 'FIDO Alliance Page',
    usages: {},
  },
};


export class HidReportDescriptorDecoder {

  private itemTypeIdToHidItemType(itemType: number): HidItemType {
    switch (itemType) {
      case 0:
        return HidItemType.Main;
      case 1:
        return HidItemType.Global;
      case 2:
        return HidItemType.Local;
      default:
        throw new Error('Invalid item type');
    }
  }

  private itemTypeIdToHidCollectionType(itemType: number): HidCollectionType {
    switch (itemType) {
      case 0:
        return HidCollectionType.Physical;
      case 1:
        return HidCollectionType.Application;
      case 2:
        return HidCollectionType.Logical;
      case 3:
        return HidCollectionType.Report;
      case 4:
        return HidCollectionType.NamedArray;
      case 5:
        return HidCollectionType.UsageSwitch;
      case 6:
        return HidCollectionType.UsageModifier;
      default:
        throw new Error('Invalid collection type');
    }
  }

  private listHidReportItem(rawReportDescriptor: Uint8Array): RawHidItem[] {
    let items: RawHidItem[] = [];

    let data = new DataView(rawReportDescriptor.buffer);
    let position = 0;

    while (position < data.byteLength) {
      let prefix = data.getUint8(position++);

      // Parsing and skipping long items
      if (prefix == 0xFE) {
        if (position + 2 > data.byteLength) {
          throw new Error('Error while parsing HID Descriptor: Trying to read data out of range');
        }

        let dataSize = data.getUint8(position++);
        let longTag = data.getUint8(position++);

        if (position + dataSize > data.byteLength) {
          throw new Error('Error while parsing HID Descriptor: Trying to read data out of range');
        }

        position += dataSize;
        continue;
      }

      // Parsing short items

      let sizeCode = prefix & 0x3;
      let itemType = (prefix >> 2) & 0x3;
      let tag = (prefix >> 4) & 0xf;
      let dataSize = (1 << sizeCode) >> 1;

      if (position + dataSize > data.byteLength) {
        throw new Error('Error while parsing HID Descriptor: Trying to read data out of range');
      }

      items.push({
        itemType: this.itemTypeIdToHidItemType(itemType),
        tag: tag,
        dataSize: dataSize,
        rawData: new DataView(data.buffer, data.byteOffset + position, dataSize),
        rawItem: new DataView(data.buffer, data.byteOffset + position - 1, dataSize + 1)
      } as RawHidItem);

      position += dataSize;
    }

    return items;
  }

  private decodeHidUsage(dataSize: number, data: number): HidUsage {
    if (dataSize === 4) {
      return {
        usage: data >> 16,
        usagePage: (data & 0xFFFF)
      };
    } else {
      return {
        usage: data
      };
    }
  }

  collectionTypeToString(hidCollectionType: HidCollectionType): string {
    switch (hidCollectionType) {
      case HidCollectionType.Physical:
        return 'Physical';
      case HidCollectionType.Application:
        return 'Application';
      case HidCollectionType.Logical:
        return 'Logical';
      case HidCollectionType.Report:
        return 'Report';
      case HidCollectionType.NamedArray:
        return 'NamedArray';
      case HidCollectionType.UsageSwitch:
        return 'UsageSwitch';
      case HidCollectionType.UsageModifier:
        return 'UsageModifier';
    }
  }

  decodeHidReportDescriptor(rawReportDescriptor: Uint8Array): HidReportDescriptor {
    let result: HidReportDescriptor = {
      items: [],
      collections: [],
      reports: []
    };
    let activeCollection: HidCollection | undefined = undefined;
    let decodeContext = new DecodeContext();
    let hidItems = this.listHidReportItem(rawReportDescriptor);
    let itemDepth = 0;
    for (let hidItem of hidItems) {
      let formattedData: string | undefined;
      let data: number | undefined = undefined;
      let text: string;
      let uData = 0;
      let sData = 0;
      switch (hidItem.dataSize) {
        case 1:
          uData = hidItem.rawData.getUint8(0);
          sData = hidItem.rawData.getInt8(0);
          break;
        case 2:
          uData = hidItem.rawData.getUint16(0, true);
          sData = hidItem.rawData.getInt16(0, true);
          break;
        case 4:
          uData = hidItem.rawData.getUint32(0, true);
          sData = hidItem.rawData.getInt32(0, true);
          break;
      }
      switch (hidItem.itemType) {
        case HidItemType.Global: {
          switch (hidItem.tag) {
            case GlobalItemTag.UsagePage: {
              data = uData;
              text = 'UsagePage';
              formattedData = usagePages[uData]?.name;
              decodeContext.updateGlobalState(s => s.usagePage = uData);
              break;
            }
            case GlobalItemTag.LogicalMin:
              text = 'LogicalMin';
              data = sData;
              decodeContext.updateGlobalState(s => s.logicalMin = sData);
              break;
            case GlobalItemTag.LogicalMax:
              text = 'LogicalMax';
              data = sData;
              decodeContext.updateGlobalState(s => s.logicalMax = sData);
              break;
            case GlobalItemTag.PhysicalMin:
              text = 'PhysicalMin';
              data = sData;
              decodeContext.updateGlobalState(s => s.physicalMin = sData);
              break;
            case GlobalItemTag.PhysicalMax:
              text = 'PhysicalMax';
              data = sData;
              decodeContext.updateGlobalState(s => s.physicalMax = sData);
              break;
            case GlobalItemTag.UnitExponent: {
              const code = sData & 0xf;
              let unitExponent = 0;
              if (code >= 0x8) {
                unitExponent = -16 + code;
              } else {
                unitExponent = code;
              }
              text = 'UnitExponent';
              data = unitExponent;
              decodeContext.updateGlobalState(s => s.unitExponent = unitExponent);
              break;
            }
            case GlobalItemTag.Unit:
              text = 'Unit';
              data = uData;
              decodeContext.updateGlobalState(s => s.unit = uData);
              break;
            case GlobalItemTag.ReportSize:
              text = 'ReportSize';
              data = sData;
              decodeContext.updateGlobalState(s => s.reportSize = sData);
              break;
            case GlobalItemTag.ReportId:
              text = 'ReportId';
              data = sData;
              decodeContext.updateGlobalState(s => s.reportId = sData);
              break;
            case GlobalItemTag.ReportCount:
              text = 'ReportCount';
              data = sData;
              decodeContext.updateGlobalState(s => s.reportCount = sData);
              break;
            case GlobalItemTag.Push:
              text = 'Push';
              decodeContext.pushGlobalState();
              break;
            case GlobalItemTag.Pop:
              text = 'Pop';
              decodeContext.popGlobalState();
              break;
          }
          break;
        }
        case HidItemType.Local: {
          switch (hidItem.tag) {
            case LocalItemTag.Usage:
              text = 'Usage';
              data = uData;
              let decodedHidUsage = this.decodeHidUsage(hidItem.dataSize, uData);
              formattedData = usagePages[decodedHidUsage.usagePage ?? decodeContext.globalState.usagePage]?.usages[decodedHidUsage.usage]?.name;
              decodeContext.updateLocalState(s => s.usages.push(decodedHidUsage));
              break;
            case LocalItemTag.UsageMin:
              text = 'UsageMin';
              data = uData;
              decodeContext.updateLocalState(s => s.usageMin = this.decodeHidUsage(hidItem.dataSize, uData));
              break;
            case LocalItemTag.UsageMax:
              text = 'UsageMax';
              data = uData;
              decodeContext.updateLocalState(s => s.usageMax = this.decodeHidUsage(hidItem.dataSize, uData));
              break;
            case LocalItemTag.DesignatorIndex:
              text = 'DesignatorIndex';
              data = sData;
              decodeContext.updateLocalState(s => s.designatorIndex = sData);
              break;
            case LocalItemTag.DesignatorMin:
              text = 'DesignatorMin';
              data = sData;
              decodeContext.updateLocalState(s => s.designatorMin = sData);
              break;
            case LocalItemTag.DesignatorMax:
              text = 'DesignatorMax';
              data = sData;
              decodeContext.updateLocalState(s => s.designatorMax = sData);
              break;
            case LocalItemTag.StringIndex:
              text = 'StringIndex';
              data = sData;
              decodeContext.updateLocalState(s => s.stringIndex = sData);
              break;
            case LocalItemTag.StringMin:
              text = 'StringMin';
              data = sData;
              decodeContext.updateLocalState(s => s.stringMin = sData);
              break;
            case LocalItemTag.StringMax:
              text = 'StringMax';
              data = sData;
              decodeContext.updateLocalState(s => s.stringMax = sData);
              break;
            case LocalItemTag.Delimiter:
              text = 'Delimiter';
              data = sData;
              decodeContext.updateLocalState(s => s.delimiter = sData);
              break;
          }
          break;
        }
        case HidItemType.Main: {
          switch (hidItem.tag) {
            case MainItemTag.Input:
            case MainItemTag.Output:
            case MainItemTag.Feature: {
              let mainItemFlags: MainItemFlags = {
                dataConstant: ((uData >> 0) & 1) == 1,
                arrayVariable: ((uData >> 1) & 1) == 1,
                absoluteRelative: ((uData >> 2) & 1) == 1,
                noWrapWrap: ((uData >> 3) & 1) == 1,
                linearNonLinear: ((uData >> 4) & 1) == 1,
                preferredStateNoPreferred: ((uData >> 5) & 1) == 1,
                noNullPositionNullState: ((uData >> 6) & 1) == 1,
                nonVolatileVolatile: ((uData >> 7) & 1) == 1,
                bitFieldBufferedBytes: ((uData >> 8) & 1) == 1
              };
              formattedData = '';
              formattedData += !mainItemFlags.dataConstant ? 'Data' : 'Constant';
              formattedData += ', ' + (!mainItemFlags.arrayVariable ? 'Array' : 'Variable');
              formattedData += ', ' + (!mainItemFlags.absoluteRelative ? 'Absolute' : 'Relative');
              formattedData += ', ' + (!mainItemFlags.noWrapWrap ? 'NoWrap' : 'Wrap');
              formattedData += ', ' + (!mainItemFlags.linearNonLinear ? 'Linear' : 'Non Linear');
              formattedData += ', ' + (!mainItemFlags.preferredStateNoPreferred ? 'Preferred State' : 'No Preferred');
              formattedData += ', ' + (!mainItemFlags.noNullPositionNullState ? 'NoNullPosition' : 'NullState');
              formattedData += ', ' + (!mainItemFlags.nonVolatileVolatile ? 'NonVolatile' : 'Volatile');
              formattedData += ', ' + (!mainItemFlags.bitFieldBufferedBytes ? 'BitField' : 'BufferedBytes');
              break;
            }
          }

          switch (hidItem.tag) {
            case MainItemTag.Input:
              text = 'Input';
              data = uData;
              break;
            case MainItemTag.Output:
              text = 'Output';
              data = uData;
              break;
            case MainItemTag.Feature:
              text = 'Feature';
              data = uData;
              break;
            case MainItemTag.Collection:
              text = 'Collection';
              activeCollection = decodeContext.createCollection(this.itemTypeIdToHidCollectionType(uData));
              data = uData;
              formattedData = this.collectionTypeToString(activeCollection.type);
              break;
            case MainItemTag.CollectionEnd:
              activeCollection = decodeContext.endCollection();
              itemDepth--;
              text = 'CollectionEnd';
              break;
          }
          decodeContext.clearLocalContext();
          break;
        }
      }

      result.items.push({
        ...hidItem,
        text,
        data,
        formattedData,
        depth: itemDepth
      });

      if (hidItem.itemType === HidItemType.Main && hidItem.tag === MainItemTag.Collection) {
        itemDepth++;
      }
    }

    return result;
  }
}
