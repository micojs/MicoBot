const { sleep, Uint8Buffer, toByteArray } = require('./Uint8Buffer.js');

// Timeouts
const DEFAULT_TIMEOUT = 6000; // timeout for most flash operations
const CHIP_ERASE_TIMEOUT = 300000; // timeout for full chip erase
const MAX_TIMEOUT = CHIP_ERASE_TIMEOUT * 2; // longest any command can run
const SYNC_TIMEOUT = 200; // timeout for syncing with bootloader
const ERASE_REGION_TIMEOUT_PER_MB = 30000; // timeout (per megabyte) for erasing a region
const MEM_END_ROM_TIMEOUT = 50;

const TIMEOUT_QUICK   = 200;
const TIMEOUT_NORMAL  = 3000;
const TIMEOUT_LONG    = 5000;

class SamBAError extends Error {
  constructor(msg) {
    super(msg);
  }
}

module.exports.SamBAError = SamBAError;

const emptyByteArray = new Uint8Array();

class SamBA {

  options;

  serialPort;

  // readLoop state
  closed = true;
  readLoopPromise = undefined;
  serialReader = undefined;
  inputBuffer = new Uint8Buffer(64);

  _canChipErase = false;
  _canWriteBuffer = false;
  _canChecksumBuffer = false;
  _canProtect = false;

  _readBufferSize;

  get canChecksumBuffer() {
    return this._canChecksumBuffer;
  }

  get canProtect() {
    return this._canProtect;
  }

  get canChipErase() {
    return this._canChipErase;
  }

  get canWriteBuffer() {
    return this._canWriteBuffer;
  }

  get writeBufferSize() { return 4096; }

  constructor(serialPort, options) {
    this.options = Object.assign(
      {
        flashSize: 4 * 1024 * 1024,
        logger: console,
        debug: false,
        trace: false
      },
      options || {}
    );

    this.serialPort = serialPort;

    this._canChipErase = false;
    this._canWriteBuffer = false;
    this._canChecksumBuffer = false;
    this._canProtect = false;
    this._readBufferSize = 0;
  }

  get logger() {
    return this.options.logger;
  }

  async checksumBuffer(start_addr, size) {

    if (!this._canWriteBuffer)
        throw new SamBAError('Cannot write buffer');

    if (size > this.checksumBufferSize())
      throw new SamBAError('Size too large for checksum buffer');

    if (this.options.debug)
      this.options.logger.debug('checksumBuffer(start_addr=0x', this.hex(start_addr), ',size=0x', this.hex(size),')');

    let result = await this.sendCommand('Z' + this.hex(start_addr) + ',' + this.hex(size), 12, undefined, 0, TIMEOUT_LONG );

    if (!result || result[0] != 0x5A /* 'Z' */) // Expects "Z00000000#\n\r"
        throw new SamBAError('Board response for \'Z\' command wrong');

    let value = this.decodeResponse(result);
    value = value.substr(1, 8);

    let num = parseInt(value, 16);

    if (num == NaN) {
      throw new SamBAError('Invalid checksum returned');
    }

    return num;
  }

  checksumBufferSize() { return 4096; }

  checksumCalc(c, crc) {
    return -1;
  }

  async chipErase(start_addr) {

    if (!this._canChipErase)
        throw new SamBAError('Chip erase not supported');

    if (this.options.debug)
      this.options.logger.debug('chipErase(start_addr=0x', this.hex(start_addr),')');

    let result = await this.sendCommand('X' + this.hex(start_addr), 3, undefined, 0, TIMEOUT_LONG);

    if (!result || result[0] != 0x58 /* 'X' */)
      throw new SamBAError('Board response for \'X\' command wrong');
  }

  async writeBuffer(src_addr, dst_addr, size) {

    if (!this._canWriteBuffer)
        throw new SamBAError('Cannot write buffer');

    if (size > this.checksumBufferSize())
      throw new SamBAError('Size too large for checksum buffer');

    if (this.options.debug)
      this.options.logger.debug('writeBuffer(src_addr=0x', this.hex(src_addr), ',dst_addr=0x', this.hex(dst_addr), ',size=0x', this.hex(size),')');

    let result = await this.sendCommand('Y' + this.hex(src_addr) + ',0', 3, undefined, 0, TIMEOUT_QUICK );

    if (!result || result[0] != 0x59 /* 'Y' */)
        throw new SamBAError('Board response for \'Y\' command wrong');

//    await sleep(50);
    result = await this.sendCommand('Y' + this.hex(dst_addr) + ',' + this.hex(size), 3, undefined, 0, TIMEOUT_LONG * 2);
    await sleep(50);


    if (!result || result[0] != 0x59 /* 'Y' */)
        throw new SamBAError('Board response for \'Y\' command wrong');
  }

  /**
   * Send a byte stream to the device
   */
  async writeToStream(msg) {

    if (this.serialPort.writable) {

      const writer = this.serialPort.writable.getWriter();
      let chunkSize = 63;

      try {
        await sleep(50);
        await writer.write(msg);
      }
      finally {
        writer.releaseLock();
      }
    }
  }

  hex(value, digits = 8) {
    var result = value.toString(16);

    while (result.length < digits) {
      result = '0' + result;
    }

    return result;
  }

  writeByte(addr, value) {

    if (this.options.debug)
      this.options.logger.debug('writeByte(addr=0x', this.hex(addr), ',value=0x', this.hex(value,2),')');

    this.sendCommand('O' + this.hex(addr) + ',' + this.hex(value, 2), 0);
  }

  async readByte(addr) {

    if (this.options.debug)
      this.options.logger.debug('readByte(addr=0x', this.hex(addr),')');

    let result = await this.sendCommand('o' + this.hex(addr) + ",4", 1);

    if (result) {
      let value = result[0];

      if (this.options.debug)
        this.options.logger.debug('readByte(addr=0x', this.hex(addr),')=0x',this.hex(value,2));

      return value;
    }

    throw new SamBAError('Reading');
  }


  async writeWord(addr, value) {

    if (this.options.debug)
      this.options.logger.debug('writeWord(addr=0x', this.hex(addr), ',value=0x', this.hex(value),')');

    await this.sendCommand('W' + this.hex(addr) + ',' + this.hex(value, 8), 0);
  }

  async readWord(addr) {

    if (this.options.debug)
      this.options.logger.debug('readWord(addr=0x', this.hex(addr),')');

    let result = await this.sendCommand('w' + this.hex(addr) + ',4', 4);

    if (result) {
      let value = (result[3] << 24 | result[2] << 16 | result[1] << 8 | result[0] << 0);

      if (this.options.debug)
        this.options.logger.debug('readByte(addr=0x', this.hex(addr),')=0x',this.hex(value,8));

      return value;
    }

    throw new SamBAError('Reading');
  }

  async write(addr, buffer, size = buffer.length) {

    if (this.options.debug)
      this.options.logger.debug('write(addr=0x', this.hex(addr), ',size=0x', this.hex(size), ')');

    await this.sendCommand('S' + this.hex(addr) + ',' + this.hex(size, 8), 0, buffer, size, TIMEOUT_LONG);
  }

  async read(addr, buffer, size) {

    if (this.options.debug)
        this.options.logger.debug('read(addr=0x', this.hex(addr), ',size=0x', this.hex(size), ')');

    var start = 0;

    // The SAM firmware has a bug reading powers of 2 over 32 bytes
    // via USB.  If that is the case here, then read the first byte
    // with a readByte and then read one less than the requested size.
    if (this._readBufferSize == 0 && size > 32 && !(size & (size - 1))) {
        buffer[start] = await this.readByte(addr);
        addr++;
        start++;
        size--;
    }

    while (size > 0)
    {
        var chunk = size;

        // Handle any limitations on the size of the read
        if (this._readBufferSize > 0 && size > this._readBufferSize)
            chunk = this._readBufferSize;

        var result = await this.sendCommand('R' + this.hex(addr) + ',' + this.hex(chunk), chunk);

        if (result) {
          for (var i = 0; i < chunk; i++) {
            buffer[start++] = result[i];
          }
        }
        else
          throw new SamBAError('Reading binary');

        size -= chunk;
        addr += chunk;
        start += chunk;
    }
  }


  async go(addr) {

    if (this.options.debug)
      this.options.logger.debug('go(addr=0x', this.hex(addr), ')');

    await this.sendCommand('G' + this.hex(addr), 0);
  }

  /**
   * @param rebootWaitMs how long it may take to reboot
   * Start the read loop up.
   */
  async connect(preopened = false, rebootWaitMs = 1000) {

    if (this.readLoopPromise) {
      throw "already open";
    }

    if (!preopened) {
        await this.serialPort.open({
            dataBits: 8,
            stopBits: 1,
            parity: 'none',
            bufferSize: 63,
            flowControl: 'hardware',
            baudRate: 921600 });
        await sleep(50);
    }

    await this._connect();
  }

  async _connect() {

    this.closed = false;

    this.readLoopPromise = (async () => {
      this.readLoop()
        .catch( reason => {
          if (reason.name == 'NetworkError' && reason.code == 19) {
            console.log("readLoop terminated because the connection was closed.");
          }
        })
      this.readLoopPromise = undefined;
    })();

    // Clear the pipe
    await this.readBuffer(SYNC_TIMEOUT);

    await this.setBinaryMode();

    let version = await this.readVersion();
    var extIndex = version.indexOf('[Arduino:');

    if (this.options.debug)
      this.options.logger.debug('Version-Info from bootloader: ' + version);

    if (extIndex != -1)
    {
        extIndex += 9;
        while (extIndex < version.length && version[extIndex] != ']')
        {
            switch (version[extIndex]) {
                case 'X': this._canChipErase = true; break;
                case 'Y': this._canWriteBuffer = true; break;
                case 'Z': this._canChecksumBuffer = true; break;
                case 'P': this._canProtect = true; break;            }
            extIndex++;
        }

        // All SAMD-based Arduino/AdaFruit boards have a bug in their bootloader
        // that trying to read 64 bytes or more over USB corrupts the data.
        // We must limit these boards to read chunks of 63 bytes.
        this._readBufferSize = 63;
    }
  }

  async setBinaryMode() {
    return this.sendCommand( 'N', 2);
  }

  /**
   * Read the Arduino version information from the board
   *
   * @returns A promise providing the version string
   */
  async readVersion() {

    let buffer = await this.sendCommand( 'V', 256);

    if (buffer) {
      return this.decodeResponse(buffer);
    }

    throw new SamBAError('No data received')
  }

  decodeResponse(buffer) {

    if (buffer.length > 2) {
      // Strip CR/LF if found
      if ((buffer[buffer.length - 1] = 0x0c) && (buffer[buffer.length - 2] = 0x0a)) {
        buffer = buffer.subarray(0, buffer.length - 2);
      }
    }

    return new TextDecoder("ascii").decode(buffer);
  }

  async sendCommand(cmd, responseSize = 2, data = undefined, size = 0, timeout = DEFAULT_TIMEOUT) {
    this.inputBuffer.reset();

    const packet = this._sendCommandBuffer;
    packet.reset();
    packet.copy(toByteArray(cmd));
    packet.push( 0x23 ); // #

    const res = packet.view();
    if (this.options.trace) {
      this.logger.debug("Writing ", this.hex(res.length), " byte" + (res.length == 1 ? "" : "s") + ":", res.slice(0, packet.length));
    }

    await this.writeToStream(res);

    if (data) {
      // if (this.options.debug) {
      //   this.logger.debug("writing buffer", this.hex(data.length), " byte" + (res.length == 1 ? "" : "s"));
      // }
      await sleep(50);
      await this.writeToStream(data);
    }

    // if (this.options.debug) {
    //   this.logger.debug("done writing");
    // }

    if (responseSize > 0)
      return await this.readBuffer(timeout, responseSize );
    else
      return null;
  }

  /**
   * Change the baud rate for the serial port.
   */
  async setBaudRate(baud) {

    this.logger.log("Attempting to change baud rate to", baud, "...");

    // Close the read loop and port
    await this.disconnect();
    await this.serialPort.close();

    // Reopen the port and read loop
    await this.serialPort.open({ baudRate: baud });
    await sleep(50);
    this._connect();

    // Baud rate was changed
    this.logger.log("Changed baud rate to", baud);
  }

  /**
   * Shutdown the read loop.
   */
  async disconnect() {
    const p = this.readLoopPromise;
    const reader = this.serialReader;

    if (!p || !reader) {
      throw "not open";
    }

    this.closed = true;
    await reader.cancel();
    await p;
    return;
  }

  _sendCommandBuffer = new Uint8Buffer();

  async readBuffer(timeout = DEFAULT_TIMEOUT, responseSize = undefined) {
    let reply = [];

    const stamp = Date.now();
    while (Date.now() - stamp < timeout) {
      if (this.inputBuffer.length > 0) {
        const c = this.inputBuffer.shift() || 0;
        if (this.options.debug) {
        }
        reply.push(c);
      }
      else {
        await sleep(10);
      }

      if (!responseSize && reply.length > 1 && (reply[reply.length - 1] == 0x0)) {
        break;
      }

      if (responseSize && reply.length == responseSize) {
        break;
      }
    }

    // Check to see if we have a complete packet. If not, we timed out.
    if (reply.length == 0) {
      this.logger.log("Timed out after", timeout, "milliseconds");
      return null;
    }

    if (this.options.trace) {
      this.logger.debug("Reading", reply.length, "byte" + (reply.length == 1 ? "" : "s") + ":", reply);
    }

    return Uint8Array.from(reply);
  }

  async readLoop() {

    this.inputBuffer.reset();

    if (this.serialPort.readable) {

      const appReadable = this.serialPort.readable;

      this.serialReader = appReadable.getReader();
      try {
        while (!this.closed) {
          const { value, done } = await this.serialReader.read();
          if (done) {
            break;
          }
          if (value) {
            if (this.options.trace)
              this.logger.debug("Received " + value);

            this.inputBuffer.copy(value);
          }
        }
      }
      finally {
        await this.serialReader.cancel();
        this.serialReader.releaseLock();
        this.serialReader = undefined;
        this.closed = true;
      }
    }
  }

}

module.exports.SamBA = SamBA;
