import createLazPerf from './laz-perf.js'
export { createLazPerf }

export type LazPerf = Awaited<ReturnType<typeof createLazPerf>>

export const LazPerf = { create: createLazPerf }
