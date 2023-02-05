
import { SamBA } from "./samba.js";

class Applet {

  /**
   * Create a flasher
   *
   * @param samba SamBA instance handling IO with board
   * @param addr Flash base address
   * @param size Page size in bytes
   * @param user Address in SRAM where the applet and buffers will be placed
   */
  constructor(
    samba,
    addr,
    code,
    size,
    start,
    stack,
    reset) {

    this._samba = samba;
    this._addr = addr;
    this._size = size;
    this._start = start;
    this._stack = stack;
    this._reset = reset;
    this._code = code;

    this._installed = false;
  }

  get size()  { return this._size; }
  get addr()  { return this._addr; }

  _samba ;
  _addr ; // Address in device SRAM where will be placed the applet
  _size ; // Applet size
  _start ; //
  _stack ; // Applet stack address in device SRAM
  _reset ;
  _code;

  _installed ;

  async checkInstall() {
    if (!this._installed) {
      await this._samba.write(this._addr, this._code, this._size);

      this._installed = true;
    }
  }
  async setStack(stack) {
    // Check if applet is already on the board and install if not
    await this.checkInstall();
    await this._samba.writeWord(this._stack, stack);
  }

  // To be used for Thumb-1 based devices (ARM7TDMI, ARM9)
  async run() {
    // Check if applet is already on the board and install if not
    await this.checkInstall();
    // Add one to the start address for Thumb mode
    await this._samba.go(this._start + 1);
  }

  // To be used for Thumb-2 based devices (Cortex-Mx)
  async runv() {
    // Check if applet is already on the board and install if not
    await this.checkInstall();
    // Add one to the start address for Thumb mode
    await this._samba.writeWord(this._reset, this._start + 1);

    // The stack is the first reset vector
    await this._samba.go(this._stack);
  }
}
module.exports.Applet = Applet;
