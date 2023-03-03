const {Flasher} = require('./Flasher.js');

async function getport() {
    console.log('Looking for port');
    return navigator.serial.getPorts()
        .then(ports => {
            for (var port of ports) {
                console.log("pre-authorized");
                return port;
            }
            const filters = [
                { usbVendorId: 0x0483, usbProductId: 0x5740 },
            ];
            return navigator.serial.requestPort({filters});
        })
}


function timeout(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function run(data) {
    const port = await getport();
    await port.open({'baudRate': 115200, 'flowControl': 'hardware'});
    console.log("port open");
    const writer = await port.writable.getWriter();
    await writer.write(data);
    await timeout(1000);
    await writer.close();
    await port.close();
    console.log("port closed");
}

class BlitFlasher extends Flasher {
    async upload() {
        const binary = this.data;
        await this.showConnectPopup();
        const enc = new TextEncoder("ascii");
        const cmd = "32BLPROG" + this.name + "\x00" + binary.byteLength + "\x00";
        const header = enc.encode(cmd);
        const data = new Uint8Array(header.byteLength + binary.byteLength);
        data.set(header, 0);
        data.set(binary, header.byteLength);
        await run(data);
    }
}

module.exports.upload = async function(binary, name = "micojs") {
    const flasher = new BlitFlasher(name + ".blit", new Uint8Array(binary));
    await flasher.upload();
}
