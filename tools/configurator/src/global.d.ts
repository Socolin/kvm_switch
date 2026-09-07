interface KeyboardLayoutMap {
  readonly size: number;
  entries(): IterableIterator<[string, string]>;
  forEach(callbackfn: (value: string, key: string, map: KeyboardLayoutMap) => void, thisArg?: any): void;
  get(key: string): string | undefined;
  has(key: string): boolean;
  keys(): IterableIterator<string>;
  values(): IterableIterator<string>;
  [Symbol.iterator](): IterableIterator<[string, string]>;
}

interface Keyboard {
  getLayoutMap(): Promise<KeyboardLayoutMap>;
  lock(keyCodes?: string[]): Promise<void>;
  unlock(): void;
}

interface Navigator {
  readonly keyboard?: Keyboard;
}
