export const nativePlatforms = [
  {
    platform: "darwin",
    arch: "arm64",
    packageName: "@lukks/wrtc-darwin-arm64",
  },
  {
    platform: "darwin",
    arch: "x64",
    packageName: "@lukks/wrtc-darwin-x64",
  },
  {
    platform: "linux",
    arch: "arm64",
    packageName: "@lukks/wrtc-linux-arm64",
  },
  {
    platform: "linux",
    arch: "x64",
    packageName: "@lukks/wrtc-linux-x64",
  },
  {
    platform: "win32",
    arch: "x64",
    packageName: "@lukks/wrtc-win32-x64",
  },
];

export function packageDirectoryName(packageName) {
  return packageName.replace("@lukks/", "");
}
