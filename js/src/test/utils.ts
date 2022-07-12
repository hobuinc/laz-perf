/**
 * Parse out a small subset of the LAS header that we will use to verify point
 * data contents.
 */
export function parseHeader(file: Buffer) {
  if (file.byteLength < 227) throw new Error("Invalid file length");

  const pointDataRecordFormat = file.readUint8(104) & 0b1111;
  const pointDataRecordLength = file.readUint16LE(105);
  const pointDataOffset = file.readUint32LE(96);
  const pointCount = file.readUint32LE(107);

  const scale = [
    file.readDoubleLE(131),
    file.readDoubleLE(139),
    file.readDoubleLE(147),
  ];
  const offset = [
    file.readDoubleLE(155),
    file.readDoubleLE(163),
    file.readDoubleLE(171),
  ];
  const min = [
    file.readDoubleLE(187),
    file.readDoubleLE(203),
    file.readDoubleLE(219),
  ];
  const max = [
    file.readDoubleLE(179),
    file.readDoubleLE(195),
    file.readDoubleLE(211),
  ];
  return {
    pointDataRecordFormat,
    pointDataRecordLength,
		pointDataOffset,
    pointCount,
    scale,
    offset,
    min,
    max,
  };
}
