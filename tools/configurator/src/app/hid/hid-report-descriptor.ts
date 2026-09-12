import { usagePages } from './usage-page';

type HidDescriptorReportField = {
  collection: HidCollection | undefined;
  mainItemFlags: MainItemFlags;

  usagePage: number;
  usage: {
    kind: 'range',
    range: {
      min: number,
      max: number
    }
  } | {
    kind: 'value',
    value: number
  } | {
    kind: 'array',
    values: number[],
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
  bitOffset: number;
}

export enum HidReportType {
  Input = 'Input',
  Output = 'Output',
  Feature = 'Feature'
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
  fields: HidDescriptorReportField[],
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
  designatorIndex: number;
  designatorMin: number;
  designatorMax: number;
  stringIndex: number;
  stringMin: number;
  stringMax: number;
  delimiter: number;
  usageMin?: HidUsage;
  usageMax?: HidUsage;
  usages?: HidUsage[];
};

class MainItemFlags {
  dataConstant: boolean;
  arrayVariable: boolean;
  absoluteRelative: boolean;
  noWrapWrap: boolean;
  linearNonLinear: boolean;
  preferredStateNoPreferred: boolean;
  noNullPositionNullState: boolean;
  nonVolatileVolatile: boolean;
  bitFieldBufferedBytes: boolean;

  constructor(uData: number) {
    this.dataConstant = ((uData >> 0) & 1) == 1;
    this.arrayVariable = ((uData >> 1) & 1) == 1;
    this.absoluteRelative = ((uData >> 2) & 1) == 1;
    this.noWrapWrap = ((uData >> 3) & 1) == 1;
    this.linearNonLinear = ((uData >> 4) & 1) == 1;
    this.preferredStateNoPreferred = ((uData >> 5) & 1) == 1;
    this.noNullPositionNullState = ((uData >> 6) & 1) == 1;
    this.nonVolatileVolatile = ((uData >> 7) & 1) == 1;
    this.bitFieldBufferedBytes = ((uData >> 8) & 1) == 1;
  }

  isConstant() {
    return this.dataConstant;
  }

  isVariable() {
    return this.arrayVariable;
  }
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
      designatorIndex: 0,
      designatorMin: 0,
      designatorMax: 0,
      stringIndex: 0,
      stringMin: 0,
      stringMax: 0,
      delimiter: 0
    } as LocalState;
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
    if (!this.localState.usages || this.localState.usages.length === 0) {
      throw new Error('No usage found before the collection');
    }
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

  private getOrCreateReport(
    reportDescriptor: HidReportDescriptor,
    reportId: number,
    reportType: HidReportType
  ): HidReportDefinition {
    let report = reportDescriptor.reports.find(x => x.reportId == reportId && x.reportType == reportType);
    if (!report) {
      report = {
        reportId: reportId,
        reportType: reportType,
        fields: []
      };
      reportDescriptor.reports.push(report);
    }
    return report;
  }

  private createReportField(
    reportDescriptor: HidReportDescriptor,
    reportId: number,
    reportType: HidReportType,
    mainItemFlags: MainItemFlags,
    collection: HidCollection | undefined
  ): HidDescriptorReportField {
    let report = this.getOrCreateReport(reportDescriptor, reportId, reportType);
    let previousField = report.fields.length === 0 ? undefined : report.fields[report.fields.length - 1];
    let field: HidDescriptorReportField = {
      collection: collection,
      mainItemFlags,
      bitOffset: previousField ? (previousField.bitOffset + previousField.bitSize) : 0,
      bitSize: 0,
      usagePage: 0,
      usage: {
        kind: 'value',
        value: 0
      },
      logical: {
        min: 0,
        max: 0
      },
      physical: {
        min: 0,
        max: 0
      },
      unit: 0,
      unitExponent: 0,
    };
    report.fields.push(field);
    return field;
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

              decodeContext.updateLocalState(s => {
                s.usages ??= [];
                s.usages.push(decodedHidUsage);
              });
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
              let mainItemFlags: MainItemFlags = new MainItemFlags(uData);
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

              let reportType: HidReportType;
              switch (hidItem.tag) {
                case MainItemTag.Input:
                  reportType = HidReportType.Input;
                  break;
                case MainItemTag.Output:
                  reportType = HidReportType.Output;
                  break;
                case MainItemTag.Feature:
                  reportType = HidReportType.Feature;
                  break;
              }
              let reportId = decodeContext.globalState.reportId;
              if (mainItemFlags.isConstant()) {
                let field: HidDescriptorReportField = this.createReportField(result, reportId, reportType, mainItemFlags, activeCollection);
                field.bitSize = decodeContext.globalState.reportSize * decodeContext.globalState.reportCount;
              } else {
                if (mainItemFlags.isVariable()) {
                  for (let i = 0; i < decodeContext.globalState.reportCount; i++) {
                    let usage: number = 0;
                    let usagePage: number = decodeContext.globalState.usagePage;
                    if (decodeContext.localState.usageMin && decodeContext.localState.usageMax) {
                      usage = Math.min(decodeContext.localState.usageMin.usage + i, decodeContext.localState.usageMax.usage);
                    } else if (decodeContext.localState.usages) {
                      let contextUsage = decodeContext.localState.usages[Math.min(i, decodeContext.localState.usages.length - 1)];
                      usage = contextUsage.usage;
                      if (contextUsage.usagePage) {
                        usagePage = contextUsage.usagePage;
                      }
                    }

                    let field: HidDescriptorReportField = this.createReportField(result, reportId, reportType, mainItemFlags, activeCollection);
                    field.bitSize = decodeContext.globalState.reportSize;

                    field.usagePage = usagePage;
                    field.usage = {
                      kind: 'value',
                      value: usage,
                    }
                    field.logical = {
                      min: decodeContext.globalState.logicalMin,
                      max: decodeContext.globalState.logicalMax
                    };
                    field.physical = {
                      min: decodeContext.globalState.physicalMin,
                      max: decodeContext.globalState.physicalMax
                    };
                    field.unit = decodeContext.globalState.unit;
                    field.unitExponent = decodeContext.globalState.unitExponent;
                  }
                } else {
                  for (let i = 0; i < decodeContext.globalState.reportCount; i++) {
                    let field: HidDescriptorReportField = this.createReportField(result, reportId, reportType, mainItemFlags, activeCollection);
                    field.bitSize = decodeContext.globalState.reportSize;
                    if (decodeContext.localState.usageMin && decodeContext.localState.usageMax) {
                      field.usage = {
                        kind: 'range',
                        range: {
                          min: decodeContext.localState.usageMin.usage,
                          max: decodeContext.localState.usageMax.usage
                        }
                      };
                      field.usagePage = decodeContext.localState.usageMin.usagePage ?? decodeContext.globalState.usagePage;
                    } else if (decodeContext.localState.usages) {
                      let contextUsage = decodeContext.localState.usages[0];
                      field.usagePage = contextUsage.usagePage ?? decodeContext.globalState.usagePage;
                      field.usage = {
                        kind: 'array',
                        values: [...decodeContext.localState.usages.map(x => x.usage)],
                      }
                    } else {
                      // If no usage, it's used as padding
                      field.usage = {
                        kind: 'value',
                        value: 0
                      }
                    }
                    field.logical = {
                      min: decodeContext.globalState.logicalMin,
                      max: decodeContext.globalState.logicalMax
                    };
                    field.physical = {
                      min: decodeContext.globalState.physicalMin,
                      max: decodeContext.globalState.physicalMax
                    };
                    field.unitExponent = decodeContext.globalState.unitExponent;
                    field.unit = decodeContext.globalState.unit;
                  }
                }
              }
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
