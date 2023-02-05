
const { Flash } = require('./SAMDFlash.js');
const { SamBA } = require('./samba.js');

// CMDEX field should be 0xA5 to allow execution of any command.
const CMDEX_KEY                 =    0xa500;

const NVM_REG_BASE              =    0x41004000;

const NVM_REG_CTRLA             =    0x00;
const NVM_REG_CTRLB             =    0x04;
const NVM_REG_INTFLAG           =    0x10;
const NVM_REG_STATUS            =    0x12;
const NVM_REG_ADDR              =    0x14;
const NVM_REG_RUNLOCK           =    0x18;

const NVM_CMD_EP                =    0x00;
const NVM_CMD_EB                =    0x01;
const NVM_CMD_WP                =    0x03;
const NVM_CMD_WQW               =    0x04;
const NVM_CMD_LR                =    0x11;
const NVM_CMD_UR                =    0x12;
const NVM_CMD_SSB               =    0x16;
const NVM_CMD_PBC               =    0x15;

const ERASE_BLOCK_PAGES         =    16;   // pages

// NVM User Row
const NVM_UP_ADDR                 =    0x804000;
const NVM_UP_BOD33_DISABLE_OFFSET =    0x0;
const NVM_UP_BOD33_DISABLE_MASK   =    0x1;
const NVM_UP_BOD33_RESET_OFFSET   =    0x1;
const NVM_UP_BOD33_RESET_MASK     =    0x2;
const NVM_UP_NVM_LOCK_OFFSET      =    0x8;

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

class D5xNvmFlash extends Flash {

  _eraseAuto;

  constructor(
    samba,
    name,
    pages,
    size,
    user,
    stack) {

    super(samba, name, 0, pages, size, 1, 32, user, stack);

    this._eraseAuto = true;
  }

  get NVM_UP_SIZE() {
    return this._size;
  }

  async erase(offset, size) {
    let eraseSize = this._size * ERASE_BLOCK_PAGES;

    // Offset must be a multiple of the erase size
    if (offset % eraseSize)
      throw new FlashEraseError();

    // Offset and size must be in range
    if (offset + size > this.totalSize)
      throw new FlashEraseError();

    let eraseEnd = (offset + size + eraseSize - 1) / eraseSize;

    // Erase each erase size set of pages
    for (var eraseNum = offset / eraseSize; eraseNum < eraseEnd; eraseNum++) {

      // Issue erase command
      let wordAddr = (eraseNum * eraseSize);
      await this.writeRegU32(NVM_REG_ADDR, wordAddr);
      await this.command(NVM_CMD_EB);
    }
  }

  async waitReady() {
    while ((await this.readRegU16(NVM_REG_STATUS) & 0x1) == 0);
  }

  async eraseAll(offset ) {
    // Use the extended Samba command if available
    if (this._samba.canChipErase)
    {
      await this._samba.chipErase(offset);
    }
    else
    {
      await this.erase(offset, this.totalSize - offset);
    }
  }

  get eraseAuto() {
    return this._eraseAuto;
  }

  set eraseAuto(enable) {
    this._eraseAuto = enable;
  }

  async getLockRegions() {
    var lockBits = 0;
    let addr = NVM_UP_ADDR + NVM_UP_NVM_LOCK_OFFSET;
    var regions = new Array(this._lockRegions);

    for (var region = 0; region < this._lockRegions; region++) {
      if (region % 8 == 0)
        lockBits = await this._samba.readByte(addr++);

      regions[region] = (lockBits & (1 << (region % 8))) == 0;
    }

    return regions;
  }

  async getSecurity() {
    // There doesn't seem to be a way to read this
    return false;
  }

  async getBod() {
    let byte = await this._samba.readByte(NVM_UP_ADDR + NVM_UP_BOD33_DISABLE_OFFSET);
    return (byte & NVM_UP_BOD33_DISABLE_MASK) == 0;
  }

  canBod() { return true; }

  async getBor() {
    let byte = await this._samba.readByte(NVM_UP_ADDR + NVM_UP_BOD33_RESET_OFFSET);
    return (byte & NVM_UP_BOD33_RESET_MASK) != 0;
  }

  canBor() { return true; }

  getBootFlash() { return true; }
  canBootFlash() { return false; }

  async readUserPage(userRow ) {
    if (userRow.length != this.NVM_UP_SIZE)
      throw new Error('Invalid row buffer size');

    await this._samba.read(NVM_UP_ADDR, userRow, this.NVM_UP_SIZE);
  }

  async writeOptions() {

    var userPage = new Uint8Array(this.NVM_UP_SIZE);

    if (this.canBor() && this._bor.isDirty() && this._bor.get() != await this.getBor())
    {
      await this.readUserPage( userPage );

      if (this._bor.get())
        userPage[NVM_UP_BOD33_RESET_OFFSET] |= NVM_UP_BOD33_RESET_MASK;
      else
        userPage[NVM_UP_BOD33_RESET_OFFSET] &= ~NVM_UP_BOD33_RESET_MASK;
    }

    if (this.canBod() && this._bod.isDirty() && this._bod.get() != await this.getBod())
    {
      await this.readUserPage(userPage);

      if (this._bod.get())
        userPage[NVM_UP_BOD33_DISABLE_OFFSET] &= ~NVM_UP_BOD33_DISABLE_MASK;
      else
        userPage[NVM_UP_BOD33_DISABLE_OFFSET] |= NVM_UP_BOD33_DISABLE_MASK;
    }

    if (this._regions.isDirty()) {

      // Check if any lock bits are different from the current set
      var current = await this.getLockRegions();

      var regions = this._regions.get();
      var equal = true;
      for (var i = 0; i < regions.length && equal; i++) {
        equal &= (regions[ i ] == current[ i ]);
      }
      if (!equal) {
        await this.readUserPage(userPage);

        for (var region = 0; region < this._regions.get().length; region++) {
          if (this._regions.get()[region])
            userPage[NVM_UP_NVM_LOCK_OFFSET + region / 8] &= ~(1 << (region % 8));
          else
            userPage[NVM_UP_NVM_LOCK_OFFSET + region / 8] |= (1 << (region % 8));
        }
      }
    }

    // Erase and write the user row if modified
    if (userPage)
    {
      // Disable cache and configure manual page write
        // Configure manual page write and disable caches
      await this.writeRegU16(NVM_REG_CTRLA, (await this.readRegU16(NVM_REG_CTRLA) | (0x3 << 14)) & 0xffcf);

      // Erase user row
      await this.writeRegU32(NVM_REG_ADDR, NVM_UP_ADDR);
      await this.command(NVM_CMD_EP);

      // Write user page in quad-word chunks
      for (var offset = 0; offset < this.NVM_UP_SIZE; offset += this._size)
      {
        // Load the buffer with the quad word
        await this.loadBuffer(userPage, offset, 16);

        // Clear page buffer
        await this.command(NVM_CMD_PBC);

        // Copy quad word to page buffer
        await this.prepareApplet();
        await this._wordCopy.setDstAddr(NVM_UP_ADDR + offset);
        await this._wordCopy.setSrcAddr(this._onBufferA ? this._pageBufferA : this._pageBufferB);
        await this._wordCopy.setWords(4);
        this._onBufferA = !this._onBufferA;
        await this.waitReady();
        await this._wordCopy.runv();

        // Write the quad word
        await this.writeRegU32(NVM_REG_ADDR, (NVM_UP_ADDR + offset));
        await this.command(NVM_CMD_WQW);
      }
    }

    // Always do security last
    if (this._security.isDirty() && this._security.get() == true && this._security.get() != await this.getSecurity())
    {
      await this.command(NVM_CMD_SSB);
    }
  }

  async writePage(page) {

    if (page >= this._pages) {
      throw new FlashPageError();
    }

    // Disable cache and configure manual page write
    await this.writeRegU16(NVM_REG_CTRLA, (await this.readRegU16(NVM_REG_CTRLA) | (0x3 << 14)) & 0xffcf);

    // Auto-erase if writing at the start of the erase page
    if (this.eraseAuto && page % ERASE_BLOCK_PAGES == 0)
      await this.erase(page * this._size, ERASE_BLOCK_PAGES * this._size);

    // Clear page buffer
    await this.command(NVM_CMD_PBC);

    // Compute the start address.
    let addr = this._addr + (page * this._size);

    await this.prepareApplet();
    await this._wordCopy.setDstAddr(addr);
    await this._wordCopy.setSrcAddr(this._onBufferA ? this._pageBufferA : this._pageBufferB);
    await this._wordCopy.setWords(this._size / 4); // sizeof(uint32_t)
    this._onBufferA = !this._onBufferA;
    await this.waitReady();
    await this._wordCopy.runv();

    await this.writeRegU32(NVM_REG_ADDR, addr / 2);
    await this.command(NVM_CMD_WP);
  }

  async readPage(page, buf) {

    if (page >= this._pages) {
      throw new FlashPageError();
    }

    await this._samba.read(this._addr + (page * this._size), buf, this._size);
  }

  async readRegU16(reg) {

    return await this._samba.readByte(NVM_REG_BASE + reg)
        | (await this._samba.readByte(NVM_REG_BASE + reg + 1) << 8);
  }

  async writeRegU16(reg, value) {
    await this._samba.writeByte(NVM_REG_BASE + reg, value & 0xff);
    await this._samba.writeByte(NVM_REG_BASE + reg, (value >> 8) & 0xff);
  }

  async readRegU32(reg) {
    return await this._samba.readWord(NVM_REG_BASE + reg);
  }

  async writeRegU32(reg, value) {
    await this._samba.writeWord(NVM_REG_BASE + reg, value);
  }


  async command(cmd) {

    await this.waitReady();

    await this.writeRegU32(NVM_REG_CTRLB, CMDEX_KEY | cmd);

    await this.waitReady();

    if ((await this.readRegU16(NVM_REG_INTFLAG)) & 0xce) {
      // Clear the error bit
      await this.writeRegU16(NVM_REG_INTFLAG, 0xce);
      throw new FlashCmdError();
    }
  }

  async writeBuffer(dst_addr, size) {
    // Auto-erase if enabled
    if (this.eraseAuto && ((dst_addr / this._size) % ERASE_BLOCK_PAGES == 0))
      await this.erase(dst_addr, size);

    // Call the base class method
    await super.writeBuffer(dst_addr, size);
  }
}
module.exports.D5xNvmFlash = D5xNvmFlash;
