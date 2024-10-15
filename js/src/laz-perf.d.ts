type Pointer = number

declare class LASZip {
  constructor()
  delete(): void

  open(data: Pointer, length: number): void
  getPoint(dest: Pointer): void
  getCount(): number
  getPointLength(): number
  getPointFormat(): number
}

declare class ChunkDecoder {
  constructor()
  delete(): void

  open(
    pointDataRecordFormat: number,
    pointDataRecordLength: number,
    pointer: Pointer
  ): void

  getPoint(pointer: Pointer): void
}
declare class LazWriter {
  constructor()
  delete(): void

  open(data: Pointer, length: number): void
  setPointFormat(format: number): LazWriter
  setTransform(offsetX: number, offsetY: number, offsetZ: number, scaleX: number, scaleY: number, scaleZ: number): LazWriter
  setChunkSize(chunkSize: number): LazWriter
  writePoint(inputBuf: Pointer): void
  newChunk(): bigint
  close(): bigint
}

export declare interface LazPerf extends EmscriptenModule {
  LASZip: typeof LASZip
  ChunkDecoder: typeof ChunkDecoder
  LazWriter: typeof LazWriter
}

declare const createLazPerf: EmscriptenModuleFactory<LazPerf>
export default createLazPerf
