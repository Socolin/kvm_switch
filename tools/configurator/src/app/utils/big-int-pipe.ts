import { Pipe, PipeTransform } from '@angular/core';

@Pipe({
  name: 'bigint',
})
export class BigIntPipe implements PipeTransform {
  transform(value: bigint | string | number): string {
    if (value === null || value === undefined) return '';
    return Intl.NumberFormat('en-US').format(BigInt(value));
  }
}
