export type ShortcutActionDataDescriptor = {
  name: string,
  dataSize: number,
  dataOffset: number,
  dataType: 'uint8',
  editor: 'computerSelector' | 'deviceSelector',
}

type ShortcutActionDefinition = {
  name: string,
  dataDescriptor: ShortcutActionDataDescriptor[],
  formatDescription?: (data: Uint8Array) => string,
};

export enum ShortcutActionId {
  ChangeActiveComputerNext = 0,
  ChangeActiveComputerPrevious = 1,
  ChangeActiveComputerSet = 2,
  ChangeDeviceActiveComputerSet = 3,
}

export const shortcutActionDefinitions: Record<number, ShortcutActionDefinition> = {
  [ShortcutActionId.ChangeActiveComputerNext]: { name: 'Active next computer', dataDescriptor: [] },
  [ShortcutActionId.ChangeActiveComputerPrevious]: { name: 'Active previous computer', dataDescriptor: [] },
  [ShortcutActionId.ChangeActiveComputerSet]: {
    name: 'Active specific computer',
    dataDescriptor: [
      { name: 'computerId', dataSize: 1, dataType: 'uint8', dataOffset: 0, editor: 'computerSelector' }
    ],
    formatDescription: (data: Uint8Array): string => {
      return 'Active computer ' + data[0];
    }
  },
  [ShortcutActionId.ChangeDeviceActiveComputerSet]: {
    name: 'Forward a specific device to a computer',
    dataDescriptor: [
      { name: 'Computer Id', dataSize: 1, dataType: 'uint8', dataOffset: 0, editor: 'computerSelector' },
      { name: 'Device Id', dataSize: 1, dataType: 'uint8', dataOffset: 1, editor: 'deviceSelector' }
    ],
    formatDescription: (data: Uint8Array): string => {
      return `Forward device ${data[1]} to computer ${data[0]}`;
    }
  }
};

