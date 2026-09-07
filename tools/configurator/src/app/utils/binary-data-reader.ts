export interface BinaryDataReader {
  byteLength: number;

  eof(): boolean;

  getNextBool(): boolean;

  getNextUint8(): number;

  getNextUint16(): number;

  getNextUint32(): number;

  getNextUint64(): bigint;

  getNextInt8(): number;

  getNextInt16(): number;

  getNextInt32(): number;

  getNextInt64(): bigint;

  getNextStaticArrayOfUint8(length: number): number[];

  getNextDynamicArrayOfUint8(): number[];

  getNextDynamicBufferOfUint8(): Uint8Array;

  getNextDynamicString(): string;

  getNextDynamicUtf16String(): string;
}

export class BinaryDataReaderSizeCalculator implements BinaryDataReader {
  byteLength: number = 0;
  totalSize = 0;

  eof(): boolean {
    return false;
  }

  getNextBool(): boolean {
    this.totalSize += 1;
    return false;
  }

  getNextUint8(): number {
    this.totalSize += 1;
    return 0;
  }

  getNextUint16(): number {
    this.totalSize += 2;
    return 0;
  }

  getNextUint32(): number {
    this.totalSize += 4;
    return 0;
  }

  getNextUint64(): bigint {
    this.totalSize += 8;
    return 0n;
  }

  getNextInt8(): number {
    this.totalSize += 1;
    return 0;
  }

  getNextInt16(): number {
    this.totalSize += 2;
    return 0;
  }

  getNextInt32(): number {
    this.totalSize += 4;
    return 0;
  }

  getNextStaticArrayOfUint8(length: number): number[] {
    this.totalSize += length;
    return [];
  }

  getNextInt64(): bigint {
    this.totalSize += 8;
    return 0n;
  }

  getNextDynamicString(): string {
    throw new Error('Not supported.');
  }

  getNextDynamicUtf16String(): string {
    throw new Error('Not supported.');
  }

  getNextDynamicArrayOfUint8(): number[] {
    throw new Error('Not supported.');
  }

  getNextDynamicBufferOfUint8(): Uint8Array {
    throw new Error('Not supported.');
  }
}

export class BinaryDataReaderImpl implements BinaryDataReader {
  byteLength: number = 0;
  private offset: number = 0;

  constructor(
    private readonly data: DataView
  ) {
    this.byteLength = data.byteLength;
  }

  eof(): boolean {
    return this.offset >= this.byteLength;
  }

  getNextBool(): boolean {
    return !!this.data.getUint8(this.offset++);
  }

  getNextUint8(): number {
    return this.data.getUint8(this.offset++);
  }

  getNextUint16(): number {
    const value = this.data.getUint16(this.offset, true);
    this.offset += 2;
    return value;
  }

  getNextUint32(): number {
    const value = this.data.getUint32(this.offset, true);
    this.offset += 4;
    return value;
  }

  getNextUint64(): bigint {
    const value = this.data.getBigUint64(this.offset, true);
    this.offset += 8;
    return value;
  }

  getNextInt8(): number {
    return this.data.getInt8(this.offset++);
  }

  getNextInt16(): number {
    const value = this.data.getInt16(this.offset, true);
    this.offset += 2;
    return value;
  }

  getNextInt32(): number {
    const value = this.data.getInt32(this.offset, true);
    this.offset += 4;
    return value;
  }

  getNextInt64(): bigint {
    const value = this.data.getBigInt64(this.offset, true);
    this.offset += 8;
    return value;
  }

  getNextStaticArrayOfUint8(length: number): number[] {
    let result: number[] = [];
    for (let i = 0; i < length; i++) {
      result.push(this.data.getUint8(this.offset++));
    }
    return result;
  }

  getNextDynamicArrayOfUint8(): number[] {
    let length = this.getNextUint8();
    let result: number[] = [];
    for (let i = 0; i < length; i++) {
      result.push(this.data.getUint8(this.offset++));
    }
    return result;
  }

  getNextDynamicString(): string {
    let size = this.getNextUint8();
    let result = '';
    for (let i = 0; i < size; i++) {
      result += String.fromCharCode(this.getNextUint8());
    }
    return result;
  }

  getNextDynamicUtf16String(): string {
    let size = this.getNextUint8();
    let charLen = Math.floor(size / 2);
    let result = '';
    for (let i = 0; i < charLen; i++) {
      result += String.fromCharCode(this.getNextUint16());
    }
    return result;
  }

  getNextDynamicBufferOfUint8(): Uint8Array {
    let size = this.getNextUint8();
    let result = new Uint8Array(this.data.buffer, this.offset, size);
    this.offset += size;
    return result;
  }
}
