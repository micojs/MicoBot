// Copyright (C) 2021 Toitware ApS. All rights reserved.
// Use of this source code is governed by an MIT-style license that can be
// found in the LICENSE file.

async function sleep(ms){
  return new Promise((resolve) => setTimeout(resolve, ms));
}

module.exports.sleep = sleep;

export class Uint8Buffer {
  readOffset = 0;
  writeOffset = 0;
  size;

  _buffer;
  _view;

  constructor(size = 64) {
    this.size = size;
    this._buffer = new ArrayBuffer(this.size);
    this._view = new Uint8Array(this._buffer);
  }

  get length() {
    return this.writeOffset - this.readOffset;
  }

  shift() {
    if (this.length <= 0) {
      return undefined;
    }
    return this._view[this.readOffset++];
  }

  grow(newSize) {
    const newBuffer = new ArrayBuffer(newSize);
    const newView = new Uint8Array(newBuffer);
    this._view.forEach((v, i) => (newView[i] = v));
    this.size = newSize;
    this._buffer = newBuffer;
    this._view = newView;
  }

  fill(element, length = 1) {
    this.ensure(length);
    this._view.fill(element, this.writeOffset, this.writeOffset + length);
    this.writeOffset += length;
  }

  ensure(length) {
    if (this.size - this.writeOffset < length) {
      const newSize = this.size + Math.max(length, this.size);
      this.grow(newSize);
    }
  }

  pushBytes(value, byteCount, littleEndian) {
    for (let i = 0; i < byteCount; i++) {
      if (littleEndian) {
        this.push((value >> (i * 8)) & 0xff);
      } else {
        this.push((value >> ((byteCount - i) * 8)) & 0xff);
      }
    }
  }

  reset() {
    this.writeOffset = 0;
    this.readOffset = 0;
  }

  push(...bytes) {
    this.ensure(bytes.length);
    this._view.set(bytes, this.writeOffset);
    this.writeOffset += bytes.length;
  }

  copy(bytes) {
    this.ensure(bytes.length);
    this._view.set(bytes, this.writeOffset);
    this.writeOffset += bytes.length;
  }

  view() {
    return new Uint8Array(this._buffer, this.readOffset, this.writeOffset);
  }
}

export function toByteArray(str) {
  const byteArray = new Uint8Array(str.length);
  for (let i = 0; i < str.length; i++) {
    const charcode = str.charCodeAt(i);
    byteArray[i] = charcode & 0xff;
  }
  return byteArray;
}

export function toHex(value, size = 2) {
  return "0x" + (value >>> 0).toString(16).toUpperCase().padStart(size, "0");
}
