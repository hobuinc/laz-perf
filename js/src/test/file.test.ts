import { promises as fs } from "fs";
import { join } from "path";

import { createLazPerf } from "..";

const testdatadir = join(__dirname, "../../../cpp/test/raw-sets");

test("read file", async () => {
  const file = await fs.readFile(join(testdatadir, "autzen_trim.laz"));

  // Pluck out some necessary values from the header.  We could hard-code them,
  // but this way we ensure that this test will be re-examined if the test file
  // is ever updated.
  const pointDataRecordFormat = file.readUint8(104) & 0b1111;
  const pointDataRecordLength = file.readUint16LE(105);
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
  expect(pointDataRecordFormat).toEqual(3);
  expect(pointDataRecordLength).toEqual(34);
  expect(pointCount).toEqual(110_000);
  expect(scale).toEqual([0.01, 0.01, 0.01]);
  expect(min.every((v, i) => v < max[i])).toBe(true);

	// Create our Emscripten module.
  const LazPerf = await createLazPerf();
  const laszip = new LazPerf.LASZip();

  // Allocate our memory in the Emscripten heap: a filePtr buffer for our
  // compressed content and a single point's worth of bytes for our output.
  const dataPtr = LazPerf._malloc(pointDataRecordLength);
  const filePtr = LazPerf._malloc(file.byteLength);

  // Copy our data into the Emscripten heap so we can point at it in getPoint().
  LazPerf.HEAPU8.set(
    new Uint8Array(file.buffer, file.byteOffset, file.byteLength),
    filePtr
  );

  try {
    laszip.open(filePtr, file.byteLength);

    for (let i = 0; i < pointCount; ++i) {
      laszip.getPoint(dataPtr);

      // Now our dataPtr (in the Emscripten heap) contains our point data, copy
      // it out into our own Buffer.
      const pointbuffer = Buffer.from(
        LazPerf.HEAPU8.buffer,
        dataPtr,
        pointDataRecordLength
      );

      // Grab the scaled/offset XYZ values and reverse the scale/offset to get
      // their absolute positions.  It would be possible to add checks for
      // attributes other than XYZ here - our pointbuffer contains an entire
      // point whose format corresponds to the pointDataRecordFormat above.
      const point = [
        pointbuffer.readInt32LE(0),
        pointbuffer.readInt32LE(4),
        pointbuffer.readInt32LE(8),
      ].map((v, i) => v * scale[i] + offset[i]);

      // Doing 6 expect(point[n]).toBeGreaterThanOrEqual(min[n]) style checks in
      // this tight loop slows down the test by 50x, so do a quicker check.
      if (point.some((v, i) => v < min[i] || v > max[i])) {
        console.error(i, point, min, max);
        throw new Error(`Point ${i} out of expected range`);
      }
    }
  } finally {
    LazPerf._free(filePtr);
    LazPerf._free(dataPtr);
    laszip.delete();
  }
});
