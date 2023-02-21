const fs = require('fs').promises;
const {spawn} = require('child_process');
const {cwd} = require('process');
const path = require('path');
const {PokittoBuilder} = require('./PokittoBuilder.js');

const queue = [];

function stropt(opt, def) {
    if (typeof opt == 'undefined')
        opt = def;
    if (Array.isArray(opt))
        return opt.join(' ');
    return opt;
}

class Builder {
    constructor() {
        this.promise = new Promise(ok=>queue.push(ok));
        this.base = [cwd(), 'slugs', this.constructor.name];
        this.phonetic = this.constructor.name;
    }

    say() {
        // this.spawn("/usr/bin/say", [this.phonetic]);
    }

    path(...post) {
        return path.resolve([...this.base, ...post].join('/'));
    }

    ext() {
        return "bin";
    }

    async run() {
        if (queue.length > 1) {
            console.log("Pending ", this.constructor.name);
            await this.promise;
        }

        console.log("Running builder ", this.constructor.name);

        let ret;
        try {
            ret = await this.start();
        } finally {
            queue.shift();
            if (queue.length)
                queue[0]();
        }

        return ret;
    }

    cwd() {
        return this.path();
    }

    spawn(cmd, args) {
        return new Promise((ok, nok) => {
            let err = '', out = '';
            console.log('Spawning:', cmd, ...args);
            const cp = spawn(cmd, args, {cwd:this.cwd()});
            cp.stdout.on('data', (data) => {out += data; console.log("" + data);});
            cp.stderr.on('data', (data) => {err += data; console.error("" + data);});

            cp.on('close', (code) => {
                console.log(`${cmd} exited with code ${code}`);
                if (code == 0) {
                    ok(out);
                } else {
                    nok(code, err);
                }
            });

        });
    }
}


module.exports.pokitto = class pokitto extends Builder {
    constructor(cpp) {
        super();
        this.cpp = cpp;
        this.phonetic = "poe key toe";
    }

    async start() {
        await fs.writeFile(this.path('game.cpp'), this.cpp);
        const out = await PokittoBuilder(false);
        return this.path('build.bin');
    }
};

module.exports.blit = class blit extends Builder {
    constructor(cpp, compiler) {
        super();
        this.cpp = cpp;
        this.compiler = compiler;
    }

    cwd() {
        return this.path('build-stm32');
    }

    ext() {
        return "blit";
    }

    async start() {
        const compiler = this.compiler;
        await fs.writeFile(this.path('game.cpp'), this.cpp);
        await fs.writeFile(this.path('metadata.yml'), `title: ${stropt(compiler.getOpt('title'), 'game title')}
description: ${stropt(compiler.getOpt('description'), 'game description')}
author: ${stropt(compiler.getOpt('author'), 'you')}
splash:
  file: assets/no-image.png
icon:
  file: assets/no-icon.png
version: ${compiler.getOpt('version') || 'v0.0.1'}
category: ${compiler.getOpt('category') || 'game'}
url: ${compiler.getOpt('url') || 'https://github.com/32blit/32blit-boilerplate'}
`);

        const out = await this.spawn('make', []);
        // console.log("<<OUT[", out, "]OUT>>");
        return this.path('build-stm32/game.blit');
    }
};

module.exports.pico = class pico extends Builder {
    constructor(cpp, compiler) {
        super();
        this.cpp = cpp;
        this.compiler = compiler;
    }

    cwd() {
        return this.path('build');
    }

    ext() {
        return "uf2";
    }

    async start() {
        const compiler = this.compiler;
        await fs.writeFile(this.path('game.cpp'), this.cpp);
        await fs.writeFile(this.path('metadata.yml'), `title: ${stropt(compiler.getOpt('title'), 'game title')}
description: ${stropt(compiler.getOpt('description'), 'game description')}
author: ${stropt(compiler.getOpt('author'), 'you')}
splash:
  file: assets/no-image.png
icon:
  file: assets/no-icon.png
version: ${compiler.getOpt('version') || 'v0.0.1'}
category: ${compiler.getOpt('category') || 'game'}
url: ${compiler.getOpt('url') || 'https://github.com/32blit/32blit-boilerplate'}
`);
        const out = await this.spawn('make', []);
        // console.log("<<OUT[", out, "]OUT>>");
        return this.path('build/game.uf2');
    }
};

module.exports.espboy = class espboy extends Builder {
    constructor(cpp) {
        super();
        this.cpp = cpp;
        this.phonetic = "ESP boy";
    }

    async start() {
        await fs.writeFile(this.path('game.cpp'), this.cpp);
        const out = await this.spawn('arduino-cli', [
            "compile",
            "-b",
            "esp8266:esp8266:d1_mini:xtal=160,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,mmu=3232,non32xfer=fast,eesz=4M2M,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=921600",
            "-v",
            "--libraries",
            "../ESPboy_ArduinoIDE_Libraries",
            "--build-cache-path",
            this.path("../espboycore"),
            "--build-path",
            this.path("../espboybuild")
        ]);

        return this.path('../espboybuild/espboy.ino.bin');
    }
};

module.exports.meta = class meta extends Builder {
    constructor(cpp) {
        super();
        this.cpp = cpp;
    }

    async start() {
        await fs.writeFile(this.path('game.cpp'), this.cpp);
        const out = await this.spawn('arduino-cli', [
            "compile",
            "-b",
            "gamebuino:samd:gamebuino_meta_native",
            "-v",
            "--build-cache-path",
            this.path("../metacore"),
            "--build-path",
            this.path("../metabuild")
        ]);

        return this.path('../metabuild/meta.ino.bin');
    }
};

module.exports.metro = class metro extends Builder {
    constructor(cpp) {
        super();
        this.cpp = cpp;
    }

    async start() {
        await fs.writeFile(this.path('game.cpp'), this.cpp);
        // const out = await this.spawn('arduino-cli', [
        //     "compile",
        //     "-b",
        //     "adafruit:samd",
        //     "-v",
        //     "--build-cache-path",
        //     this.path("../metrocore"),
        //     "--build-path",
        //     this.path("../metrobuild")
        // ]);

        return this.path('../metabuild/Blink.ino.bin');
    }
};

module.exports.arduboy = class arduboy extends Builder {
    constructor(cpp) {
        super();
        this.cpp = cpp;
    }

    async start() {
        await fs.writeFile(this.path('game.cpp'), this.cpp);

        const out = await this.spawn('arduino-cli', [
            "compile",
            "-b",
            "arduino:avr:samd",
            "-v",
            // "--build-cache-path",
            // this.path("../arduboycore"),
            "--build-path",
            this.path("../arduboybuild")
        ]);

        return this.path('../metabuild/Blink.ino.bin');
    }
};
