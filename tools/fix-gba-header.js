#!/usr/bin/env node
const fs = require("fs");

const romPath = process.argv[2];
if (!romPath) {
  console.error("Usage: node tools/fix-gba-header.js path/to/rom.gba");
  process.exit(1);
}

let rom = fs.readFileSync(romPath);
if (rom.length < 0xc0) {
  console.error("ROM is too small to contain a GBA header.");
  process.exit(1);
}

let checksum = 0;
for (let i = 0xa0; i <= 0xbc; i++) {
  checksum = (checksum - rom[i]) & 0xff;
}

rom[0xbd] = (checksum - 0x19) & 0xff;

let paddedSize = 256 * 1024;
while (paddedSize < rom.length) {
  paddedSize *= 2;
}

if (rom.length !== paddedSize) {
  const paddedRom = Buffer.alloc(paddedSize, 0xff);
  rom.copy(paddedRom);
  rom = paddedRom;
}

fs.writeFileSync(romPath, rom);
