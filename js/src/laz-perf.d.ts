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

export declare interface LazPerf extends EmscriptenModule {
  LASZip: typeof LASZip
  ChunkDecoder: typeof ChunkDecoder
}

declare const createLazPerf: EmscriptenModuleFactory<LazPerf>
export default createLazPerf
