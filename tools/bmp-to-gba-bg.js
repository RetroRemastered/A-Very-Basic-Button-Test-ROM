#!/usr/bin/env node
const fs = require("fs");

const [bmpPath, headerPath, symbolName = "sp_buttons_bg", mode = "normalize-fill"] = process.argv.slice(2);
if (!bmpPath || !headerPath) {
  console.error("Usage: node tools/bmp-to-gba-bg.js input.bmp output.h [symbol_name]");
  process.exit(1);
}

const bmp = fs.readFileSync(bmpPath);
if (bmp.toString("ascii", 0, 2) !== "BM") {
  throw new Error("Input is not a BMP file.");
}

const pixelOffset = bmp.readUInt32LE(10);
const width = bmp.readInt32LE(18);
const rawHeight = bmp.readInt32LE(22);
const bpp = bmp.readUInt16LE(28);
const height = Math.abs(rawHeight);
const topDown = rawHeight < 0;

if (width !== 240 || height !== 160 || (bpp !== 24 && bpp !== 32)) {
  throw new Error(`Expected a 240x160 24-bit or 32-bit BMP, got ${width}x${height} ${bpp}-bit.`);
}

const bytesPerPixel = bpp / 8;
const rowStride = Math.ceil((width * bytesPerPixel) / 4) * 4;
const values = [];
const normalizeFill = (value) => {
  if (mode === "raw") {
    return value;
  }

  const r = value & 31;
  const g = (value >> 5) & 31;
  const b = (value >> 10) & 31;
  const fillDistance = Math.abs(r - 23) + Math.abs(g - 25) + Math.abs(b - 26);
  const pressedDistance = Math.abs(r - 13) + Math.abs(g - 14) + Math.abs(b - 15);

  if (fillDistance <= 5) {
    return 0x6b37;
  }

  if (pressedDistance <= 4) {
    return 0x3dcd;
  }

  return value;
};

for (let y = 0; y < height; y++) {
  const sourceY = topDown ? y : height - 1 - y;
  const row = pixelOffset + sourceY * rowStride;
  for (let x = 0; x < width; x++) {
    const i = row + x * bytesPerPixel;
    const b = bmp[i];
    const g = bmp[i + 1];
    const r = bmp[i + 2];
    const value = ((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)) & 0x7fff;
    values.push(normalizeFill(value));
  }
}

const lines = [];
const guard = `${symbolName.toUpperCase()}_H`;
lines.push(`#ifndef ${guard}`);
lines.push(`#define ${guard}`);
lines.push("");
lines.push("#include <stdint.h>");
lines.push("");
lines.push(`#define ${symbolName.toUpperCase()}_WIDTH 240`);
lines.push(`#define ${symbolName.toUpperCase()}_HEIGHT 160`);
lines.push("");
lines.push(`static const uint16_t ${symbolName}[240 * 160] = {`);
for (let i = 0; i < values.length; i += 12) {
  const chunk = values.slice(i, i + 12).map((v) => `0x${v.toString(16).padStart(4, "0")}`);
  lines.push(`    ${chunk.join(", ")},`);
}
lines.push("};");
lines.push("");
lines.push("#endif");
lines.push("");

fs.writeFileSync(headerPath, lines.join("\n"));
