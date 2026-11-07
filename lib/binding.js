import { createRequire } from 'module';

// @ts-nocheck
const require$1 = createRequire(import.meta.url);
const nativePackages = {
    "darwin-arm64": "@lukks/wrtc-darwin-arm64",
    "darwin-x64": "@lukks/wrtc-darwin-x64",
    "linux-arm64": "@lukks/wrtc-linux-arm64",
    "linux-x64": "@lukks/wrtc-linux-x64",
    "win32-x64": "@lukks/wrtc-win32-x64",
};
let binding;
try {
    binding = require$1("../build/Debug/wrtc.node");
}
catch {
    try {
        binding = require$1("../build/Release/wrtc.node");
    }
    catch {
        const target = `${process.platform}-${process.arch}`;
        const packageName = nativePackages[target];
        if (!packageName) {
            throw new Error(`Unsupported native platform: ${target}`);
        }
        try {
            binding = require$1(packageName);
        }
        catch (cause) {
            throw new Error(`The optional native package ${packageName} is not installed`, { cause });
        }
    }
}
var binding$1 = binding;

export { binding$1 as default };
//# sourceMappingURL=binding.js.map
