export class BinaryDataWriter {
  private offset: number = 0;
  private values: { size: 1 | 2 | 4, unsigned: boolean, value: number }[] = [];

  constructor() {
  }

  public writeUint8(value: number): void {
    this.values.push({ size: 1, unsigned: true, value });
    this.offset += 1;
  }

  public writeUint16(value: number): void {
    this.values.push({ size: 2, unsigned: true, value });
    this.offset += 2;
  }

  public writeUint32(value: number): void {
    this.values.push({ size: 4, unsigned: true, value });
    this.offset += 4;
  }

  public writeInt8(value: number): void {
    this.values.push({ size: 1, unsigned: false, value });
    this.offset += 1;
  }

  public writeBool(value: boolean): void {
    this.values.push({ size: 1, unsigned: false, value: value ? 1 : 0 });
    this.offset += 1;
  }

  public writeInt16(value: number): void {
    this.values.push({ size: 2, unsigned: false, value });
    this.offset += 2;
  }

  public writeInt32(value: number): void {
    this.values.push({ size: 4, unsigned: false, value });
    this.offset += 4;
  }

  public writeStaticArrayOfUint8(array: number[]): void {
    for (let value of array) {
      this.values.push({ size: 1, unsigned: true, value });
      this.offset += 1;
    }
  }

  public getBuffer(littleEndian: boolean): ArrayBuffer {
    let dataArray = new ArrayBuffer(this.offset);
    const buffer = new DataView(dataArray);
    let offset = 0;
    for (let value of this.values) {
      switch (value.size) {
        case 1:
          if (value.unsigned)
            buffer.setUint8(offset, value.value);
          else
            buffer.setInt8(offset, value.value);
          offset += 1;
          break;
        case 2:
          if (value.unsigned)
            buffer.setUint16(offset, value.value, littleEndian);
          else
            buffer.setInt16(offset, value.value, littleEndian);
          offset += 2;
          break;
        case 4:
          if (value.unsigned)
            buffer.setUint32(offset, value.value, littleEndian);
          else
            buffer.setInt32(offset, value.value, littleEndian);
          offset += 4;
          break;
      }
    }

    return dataArray;
  }
}
