// @ts-nocheck
"use strict";

import { createRequire } from "module";

const require = createRequire(import.meta.url);

const nativePackages: Record<string, string> = {
  "darwin-arm64": "@lukks/wrtc-darwin-arm64",
  "darwin-x64": "@lukks/wrtc-darwin-x64",
  "linux-arm64": "@lukks/wrtc-linux-arm64",
  "linux-x64": "@lukks/wrtc-linux-x64",
  "win32-x64": "@lukks/wrtc-win32-x64",
};

let binding: any;

try {
  binding = require("../build/Debug/wrtc.node");
} catch {
  try {
    binding = require("../build/Release/wrtc.node");
  } catch {
    const target = `${process.platform}-${process.arch}`;
    const packageName = nativePackages[target];
    if (!packageName) {
      throw new Error(`Unsupported native platform: ${target}`);
    }
    try {
      binding = require(packageName);
    } catch (cause) {
      throw new Error(
        `The optional native package ${packageName} is not installed`,
        { cause },
      );
    }
  }
}

export default binding;
