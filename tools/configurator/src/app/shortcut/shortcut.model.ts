export type ShortcutActionDataDescriptor = {
  name: string,
  dataSize: number,
  dataOffset: number,
  dataType: 'uint8',
  editor: 'computerSelector',
}

type ShortcutActionDefinition = {
  name: string,
  dataDescriptor: ShortcutActionDataDescriptor[],
  formatDescription?: (data: Uint8Array) => string,
};

export const shortcutActionDefinitions: Record<number, ShortcutActionDefinition> = {
  0: { name: 'Active next computer', dataDescriptor: [] },
  1: { name: 'Active previous computer', dataDescriptor: [] },
  2: {
    name: 'Active specific computer',
    dataDescriptor: [
      { name: 'computerId', dataSize: 1, dataType: 'uint8', dataOffset: 0, editor: 'computerSelector' }
    ],
    formatDescription: (data: Uint8Array): string => {
      return 'Active computer ' + data[0];
    }
  }
};

