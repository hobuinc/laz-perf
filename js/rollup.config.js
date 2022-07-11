import commonjs from "@rollup/plugin-commonjs";
import copy from "rollup-plugin-copy";
import includePaths from "rollup-plugin-includepaths";
import typescript from "@rollup/plugin-typescript";

export default {
  input: "src/index.ts",
  output: {
    file: "lib/index.js",
    format: "umd",
    name: "LazPerf",
    globals: { fs: "fs", path: "path" },
  },
  external: ["fs", "path"],
  plugins: [
    commonjs({ include: "src/laz-perf.js" }),
    typescript({
      tsconfig: "./tsconfig.json",
      // Put declarations at the top level of the output dir.
      // declarationDir: ".",
    }),
    includePaths({ paths: "src", extensions: [".ts", ".js"] }),
    copy({
      targets: [
        { src: "src/laz-perf.wasm", dest: "lib" },
        { src: "src/laz-perf.d.ts", dest: "lib" },
      ],
    }),
  ],
};
