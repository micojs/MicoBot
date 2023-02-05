
const { WordCopyApplet } = require('./wordcopyapplet.js');
const { SamBA } = require('./samba.js');

class FlashConfigError extends Error {
  constructor(msg) {
    super(msg);
  }
}
module.exports.FlashConfigError = FlashConfigError;

class FlashRegionError extends Error {
  constructor(msg) {
    super(msg);
  }
}
module.exports.FlashRegionError = FlashRegionError;

class FlashEraseError extends Error {
  constructor(msg) {
    super(msg);
  }
}
module.exports.FlashEraseError = FlashEraseError;

class FlashCmdError extends Error {
  constructor(msg) {
    super(msg);
  }
}
module.exports.FlashCmdError = FlashCmdError;

class FlashPageError extends Error {
  constructor(msg) {
    super(msg);
  }
}
module.exports.FlashPageError = FlashPageError;

class FlashOption
{
    constructor(value) {
      this._dirty = false;
      this._value = value;
    }

    set(value)  {
      this._value = value;
      this._dirty = true;
    }

    get()  {
      return this._value;
    }

    isDirty()  { return this._dirty; }

    _value;
    _dirty;
};

/**
 *
 */
class Flash {

  /**
   * Create a flasher
   *
   * @param samba SamBA instance handling IO with board
   * @param name Name of the board
   * @param addr Flash base address
   * @param pages Number of pages
   * @param size Page size in bytes
   * @param planes Number of flash planes
   * @param lockRegions Number of flash lock regions
   * @param user Address in SRAM where the applet and buffers will be placed
   * @param stack Address in SRAM where the applet stack will be placed
   */
  constructor(
          samba,
          name,
          addr,
          pages,
          size,
          planes,
          lockRegions,
          user,
          stack) {

    this._samba = samba;
    this._name = name;

    this._addr = addr;
    this._pages = pages;
    this._size = size;
    this._planes = planes;
    this._lockRegions = lockRegions;
    this._user = user;
    this._stack = stack;

    this._bootFlash = new FlashOption(true);
    this._bod = new FlashOption(true);
    this._bor = new FlashOption(true);
    this._security = new FlashOption(true);

    this._regions = new FlashOption(new Array(0));

    this._wordCopy = new WordCopyApplet(samba, user);

    if (!((size & (size - 1)) == 0)) {
      throw new FlashConfigError();
    }

    if (!((pages & (pages - 1)) == 0)) {
      throw new FlashConfigError();
    }

    if (!((lockRegions & (lockRegions - 1)) == 0)) {
      throw new FlashConfigError();
    }

    this._onBufferA = true;

    // page buffers will have the size of a physical page and will be situated right after the applet
    this._pageBufferA = Math.trunc((this._user + this._wordCopy.size + 3) / 4) * 4; // we need to avoid non 32bits aligned access on Cortex-M0+
    this._pageBufferB = this._pageBufferA + size;

  }

  _samba;
  _name;
  _addr;
  _pages;
  _size;
  _planes;
  _lockRegions;
  _user;
  _stack;

  _prepared  = false;

  // // abstract set eraseAuto(enable);

  get address() { return this._addr; }
  get pageSize() { return this._size; }
  get numPages() { return this._pages; }
  get numPlanes() { return this._planes; }
  get totalSize() { return this._size * this._pages; }
  get lockRegions() { return this._lockRegions; }

  // abstract eraseAll(offset: number) : void;

  // abstract getLockRegions() : Promise<Array<boolean>>;
  setLockRegions(regions) {
    if (regions.length > this._lockRegions)
        throw new FlashRegionError();

    this._regions.set(regions);
  }

  // abstract getSecurity() : Promise<boolean>;
  setSecurity() {
    this._security.set(true);
  }

  // abstract getBod() : Promise<boolean>;
  setBod(enable) {
    if (this.canBod())
      this._bod.set(enable);
  }
  // abstract canBod() : boolean;

  // abstract getBor() : Promise<boolean>;
  setBor(enable) {
    if (this.canBor())
      this._bor.set(enable);
  }
  // abstract canBor() : boolean;

  // abstract getBootFlash() : boolean;
  setBootFlash(enable) {
    if (this.canBootFlash())
      this._bootFlash.set(enable);
  }
  // abstract canBootFlash() : boolean;

  // abstract writeOptions() : void;

  // abstract writePage(page: number) : void;
  // abstract readPage(page: number, buf: Uint8Array) : Promise<void>;

  async writeBuffer(dst_addr, size) {
    await this._samba.writeBuffer(this._onBufferA ? this._pageBufferA : this._pageBufferB, dst_addr + this._addr, size);
  }

  async loadBuffer(data, offset = 0, bufferSize = data.length) {

    if (offset > 0) {
      data = data.subarray(offset);
    }

    await this._samba.write(this._onBufferA ? this._pageBufferA : this._pageBufferB, data, bufferSize);
  }

  async prepareApplet() {

    if (!this._prepared) {
      await this._wordCopy.setWords(this._size / 4 /* sizeof(uint32_t) */);
      await this._wordCopy.setStack(this._stack);

      this._prepared = true;
    }
  }

  _wordCopy;
  _bootFlash;
  _regions;
  _bod;
  _bor;
  _security;
  _onBufferA = true;
  _pageBufferA  = 0;
  _pageBufferB  = 0;
}

module.exports.Flash = Flash;
