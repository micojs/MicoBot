const {dom, index} = require('./dom.js');

const {Model} = require('./Model.js');
const {Remote} = require('./Remote.js');

const {IDE} = require('./IDE.js');
const {TOP} = require('./TOP.js');

class Main {
    #view;
    #ide;
    #top;
    #model = new Model();
    #remote;

    constructor() {
        addEventListener('DOMContentLoaded', this.loaded.bind(this));
        addEventListener('resize', this.resize.bind(this));
    }

    get remote() {
        if (!this.#remote) {
            this.#remote = new Remote();
            this.#remote.boot();
        }
        return this.#remote;
    }

    setupIDE(el) {
        this.#ide = new IDE(el, this, this.#model);
        return false;
    }

    setupTOP(el) {
        this.#top = new TOP(el, this, this.#model);
        return false;
    }

    loaded() {
        if (!window.monaco) {
            setTimeout(this.loaded.bind(this), 1000);
        } else {
            this.boot();
        }
    }

    resize() {
        this.#model.set("windowSize", [window.clientWidth, window.clientHeight]);
    }

    async boot() {
        await this.#model.useStorage("main");
        this.#model.init({
            windowSize: [0, 0],
            state:'TOP',
            platform:'espboy',
            projectId:null,
            projectName:null,
            projectList:[
                {id:1, name:"Bats"}
            ]
        });
        this.#view = index(document.body, {}, this);
    }
};

window.main = new Main();

