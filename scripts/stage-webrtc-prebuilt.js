#!/usr/bin/env node

import { copyFileSync, existsSync, mkdirSync, readdirSync, statSync } from "node:fs";
import { dirname, extname, join, relative, resolve } from "node:path";

const root = resolve(process.cwd());
const source = join(root, "build/external/libwebrtc/download/src");
const output = join(root, ".webrtc-prebuilt");
const build = join(root, "build/external/libwebrtc/build/Release");
const headerExtensions = new Set([
  "", ".def", ".h", ".hh", ".hpp", ".inc", ".inl", ".ipp", ".tcc",
]);

function copyFile(from, to) {
  mkdirSync(dirname(to), { recursive: true });
  copyFileSync(from, to);
}

function copyHeaders(directory) {
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    if (entry.name === ".git") continue;
    const from = join(directory, entry.name);
    const sourceRelative = relative(source, from).replaceAll("\\", "/");
    if (entry.isDirectory()) {
      // Catapult's test tools include extensionless binaries such as d8.
      if (sourceRelative === 'third_party/catapult' ||
          sourceRelative === "third_party/llvm-build" ||
          (sourceRelative.startsWith("build/linux/") && sourceRelative.includes("sysroot"))) {
        continue;
      }
      copyHeaders(from);
    } else if (entry.isFile() && headerExtensions.has(extname(entry.name))) {
      copyFile(from, join(output, "download/src", relative(source, from)));
    }
  }
}

if (!existsSync(source) || !existsSync(build)) {
  throw new Error("WebRTC source or Release build output is missing");
}

const outputs = process.platform === "win32"
  ? [
      "obj/webrtc.lib",
      "obj/api/video/adapted_video_track_source/adapted_video_track_source.obj",
      "obj/api/video_codecs/builtin_video_decoder_factory/builtin_video_decoder_factory.obj",
      "obj/api/video_codecs/builtin_video_encoder_factory/builtin_video_encoder_factory.obj",
    ]
  : [
      "obj/libwebrtc.a",
      "obj/api/video/adapted_video_track_source/adapted_video_track_source.o",
      "obj/api/video_codecs/builtin_video_decoder_factory/builtin_video_decoder_factory.o",
      "obj/api/video_codecs/builtin_video_encoder_factory/builtin_video_encoder_factory.o",
    ];

for (const item of outputs) {
  const from = join(build, item);
  if (!existsSync(from) || !statSync(from).isFile()) {
    throw new Error(`Required WebRTC output is missing: ${item}`);
  }
}

const linker = process.platform === 'linux'
  ? 'third_party/llvm-build/Release+Asserts/bin/ld.lld'
  : null
if (linker && !existsSync(join(source, linker))) {
  throw new Error(`Required Chromium linker is missing: ${linker}`)
}

copyHeaders(source);

const generated = join(build, "gen");
if (existsSync(generated)) {
  const copyGenerated = directory => {
    for (const entry of readdirSync(directory, { withFileTypes: true })) {
      const from = join(directory, entry.name);
      if (entry.isDirectory()) copyGenerated(from);
      else if (entry.isFile() && headerExtensions.has(extname(entry.name))) {
        copyFile(from, join(output, "build/Release/gen", relative(generated, from)));
      }
    }
  };
  copyGenerated(generated);
}

for (const item of outputs) {
  copyFile(join(build, item), join(output, 'build/Release', item))
}

if (linker) {
  copyFile(join(source, linker), join(output, 'download/src', linker))
}

console.log(`Staged WebRTC prebuilt files in ${output}`);
