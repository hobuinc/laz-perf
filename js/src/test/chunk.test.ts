import { promises as fs } from 'fs'
import { join } from 'path'

import { createLazPerf } from '..'
import { parseHeader } from './utils'

const testdatadir = join(__dirname, '../../../cpp/test/raw-sets')

test('chunk decoder', async () => {
  const file = await fs.readFile(join(testdatadir, 'autzen_trim.laz'))

  const {
    pointDataRecordFormat,
    pointDataRecordLength,
    pointDataOffset,
    pointCount,
    scale,
    offset,
    min,
    max,
  } = parseHeader(file)

  expect(pointDataRecordFormat).toEqual(3)
  expect(pointDataRecordLength).toEqual(34)
  expect(pointCount).toBeGreaterThanOrEqual(2)
  expect(scale).toEqual([0.01, 0.01, 0.01])
  expect(min.every((v, i) => v < max[i])).toBe(true)

  // Create our Emscripten module.
  const LazPerf = await createLazPerf()
  const decoder = new LazPerf.ChunkDecoder()

  // To make it clear that we are only operating on the compressed data chunk
  // rather than a full LAZ file, strip off the header/VLRs here.  The magic
  // offset of 8 is to skip past the chunk table offset.  We are only going to
  // decompress 2 points here so we don't have to worry about potential resets
  // of the chunk table, so our only assumption here will be that points 0 and 1
  // are in the same chunk.
  const chunk = file.subarray(pointDataOffset + 8)

  // Allocate our memory in the Emscripten heap: a filePtr buffer for our
  // compressed content and a single point's worth of bytes for our output.
  const dataPtr = LazPerf._malloc(pointDataRecordLength)
  const chunkPtr = LazPerf._malloc(chunk.byteLength)

  // Copy our chunk into the Emscripten heap.
  LazPerf.HEAPU8.set(
    new Uint8Array(chunk.buffer, chunk.byteOffset, chunk.byteLength),
    chunkPtr
  )

  try {
    decoder.open(pointDataRecordFormat, pointDataRecordLength, chunkPtr)

    for (let i = 0; i < 2; ++i) {
      decoder.getPoint(dataPtr)

      // Now our dataPtr (in the Emscripten heap) contains our point data, copy
      // it out into our own Buffer.
      const pointbuffer = Buffer.from(
        LazPerf.HEAPU8.buffer,
        dataPtr,
        pointDataRecordLength
      )

      // Grab the scaled/offset XYZ values and reverse the scale/offset to get
      // their absolute positions.  It would be possible to add checks for
      // attributes other than XYZ here - our pointbuffer contains an entire
      // point whose format corresponds to the pointDataRecordFormat above.
      const point = [
        pointbuffer.readInt32LE(0),
        pointbuffer.readInt32LE(4),
        pointbuffer.readInt32LE(8),
      ].map((v, i) => v * scale[i] + offset[i])

      // Doing 6 expect(point[n]).toBeGreaterThanOrEqual(min[n]) style checks in
      // this tight loop slows down the test by 50x, so do a quicker check.
      if (point.some((v, i) => v < min[i] || v > max[i])) {
        console.error(i, point, min, max)
        throw new Error(`Point ${i} out of expected range`)
      }
    }
  } finally {
    LazPerf._free(chunkPtr)
    LazPerf._free(dataPtr)
    decoder.delete()
  }
})
