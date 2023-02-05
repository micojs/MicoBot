const {dom, index} = require('./dom.js');
const {convertToU8, palette} = require('./PreBuild.js');

class MusicEditor {
    #fs;
    #dom;
    path;
    metadata;

    constructor(parentElement, path, image, fs) {
        this.path = path;
        try {
            this.metadata = JSON.parse(fs.readFile(path + ".meta"));
        } catch(ex) {}

        if (!this.metadata || typeof this.metadata != "object") {
            this.metadata = {mode:"music"};
        }
        if (!this.metadata.music || typeof this.metadata.image != 'object')
            this.metadata.music = {};

        this.#fs = fs;
        this.#dom = index(dom('iframe', parentElement, {className:'imageEditor', src:'BassoonTracker/index.html'}), {}, this);
    }

    commit() {
    }

    onload() {
        this.layout();
    }

    layout() {
    }

    changeMeta() {
        const dom = this.#dom;
        this.dirty();
        this.layout();
    }

    dirty() {
        // this.#fs.writeFile(this.path + ".meta", JSON.stringify(this.metadata, 0, 4));
    }
};

module.exports.MusicEditor = MusicEditor;
