const esptool = require('./esptool.js');
const {Flasher} = require('./Flasher.js');


class ESPBoyFlasher extends Flasher {
    stub;

    async disconnect() {
        if (!this.stub)
            return;
        await this.stub.disconnect();
        await this.stub.port.close();
        this.stub = undefined;
    }

    async connect() {
        if (this.stub)
            return;

        const esploader = await esptool.connect({
            log: this.log,
            debug: this.log,
            error: this.log
        });

        try {
            await esploader.initialize();

            this.log("Connected to " + esploader.chipName);

            this.stub = await esploader.runStub();
            this.stub.addEventListener("disconnect", () => {
                this.stub = false;
            });
        } finally {
            // esploader.disconnect();
        }
    }

    async upload(binary) {
        if (!this.stub) {
            await this.showConnectPopup();
            await this.connect();
        }
        if (this.stub) {
            await this.stub.flashData(
                binary,
                (bytesWritten, totalBytes) => {
                    postMessage({build:["Uploading: " + Math.floor((bytesWritten / totalBytes) * 100) + "%"]}, '*');
                },
                0
            );
            await this.disconnect();
        }
    }
}

module.exports.upload = async function(binary) {
    const flasher = new ESPBoyFlasher();
    await flasher.upload(binary);
}
