const {dom, index} = require('./dom.js');
const {Model} = require('./Model.js');

const {TreeListNode} = require('./TreeListNode.js');
const {TabContainer} = require('./TabContainer.js');
const {Preview} = require('./Preview.js');
const {ProjectControls} = require('./ProjectControls.js');
const {Console} = require('./Console.js');

const {PNGFS} = require('./pngfs.js');
const {stdlib} = require('./stdlib.js');
const {loadExample} = require('./example.js');

const ESPboy = require('./ESPBoy.js');
const SAMD = require('./SAMD.js');
const Browser = require('./Browser.js');
const {PreBuild} = require('./PreBuild.js');
const {ImageEditor} = require('./ImageEditor.js');
// const {MusicEditor} = require('./MusicEditor.js');
const {FS2Zip, Zip2FS} = require('./FS2Zip.js');

const uploaders = {
    espboy:ESPboy,
    // meta:{upload:SAMD.upload.bind(null, 0x2000)},
    metro:{upload:SAMD.upload.bind(null, 0x4000)}
};

class IDE {
    el;
    parent;
    model;

    #view = null;
    #tabContainer = null;
    #consoleContainer = null;
    #fs = null;
    #projectModel = null;
    #editors = {};
    #treeNodes = {};
    #preview = null;
    #booted = false;

    constructor(el, parent, model) {
        this.el = el;
        this.parent = parent;
        this.model = model;
        model.watch('state', state => this.changeState(state));
        model.watch('windowSize', size => this.resize());
        model.watch('runSize', size => this.run(size));
    }

    resize() {
        if (!this.el.classList.contains('hidden')) {
            this.boot();
            for (let key in this.#editors)
                this.#editors[key]?.editor?.layout?.();
        }
    }

    async changeState(newState) {
        this.el.classList.toggle('hidden', newState != this.constructor.name);
        await this.closeProject();
        if (newState == this.constructor.name) {
            this.resize();
            this.openProject();
        }
        this.run(null);
    }

    async closeProject() {
        [...Object.values(this.#editors)].forEach(({editor}) => editor.commit());
        if (this.model) {
            this.model.set('previewProject', '');
            this.model.set('previewHTML', '');
        }
        if (this.#projectModel)
            this.#projectModel.flush();
        this.#fs = null;
        this.#projectModel = null;
        [...Object.values(this.#editors)].forEach(({shortPath}) => this.#tabContainer.close(shortPath));
    }

    async openProject() {
        const projectId = this.model.get("projectId");
        const modelId = "project-" + projectId;
        let model = this.model.get(modelId, null);

        if (!model) {
            model = new Model();
            await model.useStorage(modelId);
            this.model.set(modelId, model);
        }

        let fs = new PNGFS(model.get('fs', null));

        fs.rm('.R');

        model.onBeforeSave = _ => {
            model.set('fs', fs.toJSON());
        };

        this.#projectModel = model;
        window.projectFS = fs;
        this.#fs = fs;
        this.#fs.onDirty = _ => model.dirty();

        if (this.#fs.ls().length == 0)
            loadExample(this.#fs);
        this.emitStd(this.model.get('platform'));

        this.refreshTree();

        const activeFile = model.get('activeFile', '/source/main.js');
        const openFileList = model.get('openFileList', []);
        const copy = [...openFileList];
        openFileList.length = 0;
        copy.forEach(name => this.openFile(name));
        this.openFile(activeFile);
    }

    boot() {
        if (this.#booted)
            return;
        this.#booted = true;

        this.setupMonaco();

        this.#view = index(this.el, {}, this);
        this.model.watch('platform', platform => this.emitStd(platform));
        this.model.watch('projectName', name => {
            if (name != this.model.get('previewProject', ''))
                this.model.set('previewHTML', '')
        });
    }

    setupPreview(el) {
        this.#preview = new Preview(el, this, this.model);
        return false;
    }

    setupConsoleContainer(el) {
        const cc = new TabContainer(el);
        this.#consoleContainer = cc;

        const logcon = new Console('log');
        logcon.onAppend = _ => cc.activate('debug');
        cc.add('debug', logcon.el);
        cc.activate('debug');

        const buildcon = new Console('build');
        buildcon.onAppend = _ => cc.activate('compilation');
        buildcon.maxLines = 1;
        cc.add('compilation', buildcon.el);
        return false;
    }

    setupTabContainer(el) {
        this.#tabContainer = new TabContainer(el);
        this.#tabContainer.onClose = this.onCloseEditor.bind(this);
        this.#tabContainer.onActivate = this.onActivateFile.bind(this);
        return false;
    }

    setupProjectControls(el) {
        new ProjectControls(el, this.model);
        return false;
    }

    setupMonaco() {
        monaco.languages.typescript.javascriptDefaults.setEagerModelSync(true);

        monaco.languages.typescript.javascriptDefaults.setCompilerOptions({
	    target: monaco.languages.typescript.ScriptTarget.ES6,
	    allowNonTsExtensions: true,
            noLib: true,
            allowJs: true
        });

        var libUri = 'ts:filename/stdlib.d.ts';
        monaco.languages.typescript.javascriptDefaults.addExtraLib(stdlib, libUri);
        monaco.editor.createModel(stdlib, 'typescript', monaco.Uri.parse(libUri));
    }

    refreshTree() {
        const treelist = document.querySelector('#treelist');
        this.#treeNodes = {};

        while (treelist.childNodes.length)
            treelist.removeChild(treelist.childNodes[0]);

        for (let entry of this.#fs.ls("/"))
            treelist.appendChild((new TreeListNode(entry, "/", this.openFile.bind(this), this.#treeNodes)).dom);
    }

    cancelEvent(event) {
        event.stopPropagation();
        event.preventDefault();
    }

    dropFile(event) {
        event.stopPropagation();
        event.preventDefault();

        let parentElement = event.target.closest('.treenode.Directory');
        if (!parentElement) {
            return;
        }

        let parentPath = null;
        for (let path in this.#treeNodes) {
            let node = this.#treeNodes[path];
            if (node.dom == parentElement) {
                parentPath = path;
                break;
            }
        }
        if (!parentPath) {
            return;
        }

        let files = event.dataTransfer?.files;
        if (!files?.length) {
            return;
        }

        let pending = files.length;

        for( let i=0; i<files.length; ++i ){
	    let file = files[i];
	    let fr = new FileReader();
            if (/\.zip$/i.test(file.name)) {
	        fr.onload = loadProject.bind(this, fr);
                fr.readAsArrayBuffer(file);
            } else {
	        fr.onload = loadFile.bind(this, fr, file);
                if (/\.js$/i.test(file.name))
                    fr.readAsText(file);
                else
                    fr.readAsDataURL(file);
            }
        }

        async function loadProject(fr) {
            await Zip2FS(fr, this.#fs);
            for (let key in this.#editors)
                this.#editors[key].editor?.reload?.();
            this.refreshTree();
        }

        function loadFile(fr, file) {
            // console.log(parentPath + '/' + file.name, fr.result);
            this.#fs.writeFile(parentPath + '/' + file.name, fr.result);
            if (!--pending)
                this.refreshTree();
        }

    }

    editorFactories = {
        js(parentElement, path, contents) {
            const editor = monaco.editor.create(
                parentElement,
                {
	            value: contents,
	            language: 'javascript',
	            theme: 'vs-dark'
	        });

            let fs = this.#fs;

            editor.commit = _ => {
                let newval = editor.getValue();
                if (newval != contents) {
                    contents = newval;
                    fs.writeFile(path, contents);
                    // console.log("NEWVAL", newval, newval == contents);
                }
            };

            editor.reload = _ => {
                editor.setValue(fs.readFile(path));
            };

            editor.onDidChangeModelContent(editor.commit);

            editor.addAction({
	        id: 'save-action',
	        label: 'Save File',
	        keybindings: [
		    monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS
	        ],
	        precondition: null,
	        keybindingContext: null,
	        // contextMenuGroupId: 'navigation',
	        // contextMenuOrder: 1.5,
	        run(editor) {}
            });

            editor.addAction({
	        id: 'execute',
	        label: 'Execute',
	        keybindings: [
		    monaco.KeyMod.CtrlCmd | monaco.KeyCode.Enter
	        ],
	        precondition: null,
	        keybindingContext: null,
	        // contextMenuGroupId: 'navigation',
	        // contextMenuOrder: 1.5,
	        run: () => {
                    this.#preview.toggleRun();
                }
            });

            return editor;
        },

        meta:"js",
        json:"js",

        raw(parentElement, path, contents) {

        },

        png(parentElement, path, contents) {
            return new ImageEditor(parentElement, path, contents, this.#fs);
        },

        jpg:'png',

        // mod(parentElement, path, contents) {
        //     return new MusicEditor(parentElement, path, contents, this.#fs);
        // },

        // xm:'mod'
    }

    openFile(path) {
        const shortPath = path.split('/').pop();
        if (this.#tabContainer.activate(shortPath)) {
            this.#projectModel.set('activeFile', path);
            return;
        }

        let contents = this.#fs.readFile(path);
        if (contents === false)
            return;

        let editorNode = dom('div', {style:{width:"100%", height:"100%", position:"absolute"}});

        const entry = {
            node:editorNode,
            shortPath,
            path,
            editor:null
        };

        this.#editors[path] = entry;

        const openFileList = this.#projectModel.get('openFileList', []);
        openFileList.push(path);
        this.#projectModel.dirty('openFileList');

        this.#tabContainer.add(shortPath, editorNode);
        this.#tabContainer.activate(shortPath);

        let factory = (shortPath.match(/\.([^./]+)$/i)?.[1] + '').toLowerCase();

        while (typeof factory == 'string')
            factory = this.editorFactories[factory];

        if (!factory)
            factory = this.editorFactories.raw;

        entry.editor = factory.call(this, editorNode, path, contents);
    }

    getEditor(container) {
        for (let path in this.#editors) {
            const entry = this.#editors[path];
            if (entry.node == container)
                return entry;
        }
        return null;
    }

    getActiveTreeNode() {
        for (let key in this.#treeNodes) {
            if (this.#treeNodes[key].isActive())
                return this.#treeNodes[key];
        }
        return undefined;
    }

    onActivateFile(container) {
        if (!this.#projectModel)
            return;
        const path = this.getEditor(container)?.path;
        this.#editors[path]?.editor?.layout?.();
        this.#projectModel.set('activeFile', path);
        for (let key in this.#treeNodes) {
            this.#treeNodes[key].setActive(key == path);
        }
    }

    onCloseEditor(container) {
        for (let path in this.#editors) {
            const {node, editor} = this.#editors[path];
            if (node != container)
                continue;

            if (editor && editor.dispose)
                editor.dispose();

            delete this.#editors[path];

            if (this.#projectModel) {
                const openFileList = this.#projectModel.get('openFileList', []);
                openFileList.splice(openFileList.indexOf('path'), 1);
                this.#projectModel.dirty('openFileList');
            }

            break;
        }
        return true;
    }

    emitStd(platform) {
        if (!this.#fs || !platform)
            return;
        const code = `// Autogenerated file - DO NOT EDIT!
"set platform ${platform}";
"addSysCall setScreenMode setFPS setPen setFont setLED setTexture";
"addSysCall setMirrored setFlipped setTransparent";
"addSysCall getWidth getHeight readByte getTime";
"addSysCall clear image text rect";
"push globals UP DOWN LEFT RIGHT A B C D FRAMETIME PI HALF_PI TWO_PI";
"registerBuiltinResource fontMini fontTIC806x6 fontZXSpec fontAdventurer fontDonut fontDragon fontC64 fntC64UIGfx fontMonkey fontKarateka fontKoubit fontRunes fontTight fontTiny";
"include /source/main.js"
`;
        this.#fs?.writeFile('/std.js', code);
        this.#editors['/std.js']?.editor.setValue(code);
    }

    exportProject() {
        FS2Zip(this.#fs);
    }

    deleteFile() {
        const reference = this.getActiveTreeNode();
        if (!confirm(`Are you sure you want to delete ${reference.path}?`)) {
            return;
        }

        if (this.#fs.rm(reference.path)) {
            for (let path in this.#editors) {
                if (path == reference.path || path.startsWith(reference.path + '/')) {
                    const editor = this.#editors[path];
                    if (editor)
                        this.#tabContainer.close(editor.shortPath);
                }
            }
            let node = this.#treeNodes[reference.path];
            if (node) {
                node.remove();
                delete this.#treeNodes[reference.path];
            }
        }
    }

    createFile() {
        const fs = this.#fs;
        const name = prompt("File Name:");
        if (!name)
            return;
        const reference = this.getActiveTreeNode();

        const path = reference.path.split('/');
        if (!reference.isDirectory())
            path.pop();
        path.push(name);
        const strpath = path.join('/');

        if (fs.lookup(strpath)) {
            alert(`File ${name} already exists.`);
            return;
        }

        const [fileName, ext] = strpath.split('/').pop().split('.');

        if (!fileName || !ext) {
            alert("Invalid file name: " + name);
            return;
        }

        const source = ext.toLowerCase() !== 'js' ? '' : `// ${name}

class ${fileName} {
    constructor() {
    }

    update(time) {
    }

    render() {
    }
}

`;

        if (!fs.writeFile(strpath, source)) {
            alert(`Could not write file ${name}.`);
            return;
        }

        this.refreshTree();
        this.openFile(strpath);
    }

    createDir() {
        const fs = this.#fs;
        const name = prompt("Directory Name:");
        if (!name)
            return;
        const reference = this.getActiveTreeNode();

        const path = reference.path.split('/');
        if (!reference.isDirectory())
            path.pop();
        path.push(name);
        const strpath = path.join('/');
        if (!fs.mkdir(strpath)) {
            alert(`Could not create directory ${name}.`);
        }
        this.refreshTree();

        if (reference.isFile()) {
            this.openFile(reference.path);
        }
    }

    async run(size) {
        if (!this.#fs)
            return;
        if (!size) {
            this.model.set('previewProject', '');
            this.model.set('previewHTML', '');
        } else {
            try {
                this.#projectModel.flush();
                const fs = await PreBuild(this.#fs);
                const html = await Browser.build(fs, size);
                postMessage({log:{clear:true}}, '*');
                this.model.set('previewProject', this.model.get('projectName'));
                this.model.set('previewHTML', html);
            } catch(ex) {
                console.log(ex);
                if (ex && typeof ex == 'object') {
                    let msg = ex.description || ex.error || ex.message || ex + '';
                    postMessage({build:[msg]}, '*');
                 } else if (typeof ex == 'string') {
                    postMessage({build:[ex]}, '*');
                }
            }
        }
    }

    async export() {
        this.#preview.setExportVisibility(false);
        let img;
        try {
            postMessage({build:["Building"]}, '*');
            const fs = await PreBuild(this.#fs);
            await Browser.build(fs, [0, 0]);

            const remote = this.parent.remote;
            let key = this.#projectModel.get('RPIN');
            if (!key) {
                key = await remote.requestKey();
                this.#projectModel.set('RPIN', key);
            }

            const result = await remote.requestBuild(key, {fs:fs.toJSON()});
            if (!result || typeof result != 'object') {
                console.log(result);
                throw 'Unexpected error';
            }

            if (result.error)
                throw result;

            if (result.url) {
                const url = result.url;
                const uploader = uploaders[this.model.get('platform')];
                if (uploader && navigator.serial) {
                    const rsp = await fetch(url);
                    if (rsp.headers.get('content-type') == "application/octet-stream") {
                        uploader.upload(await rsp.arrayBuffer());
                    } else {
                        const errmsg = await rsp.text();
                        console.error(errmsg);
                    }
                } else {
                    location.href = url;
                }
            }

            postMessage({build:["Build complete"]}, '*');
        } catch(ex) {
            console.log(ex);
            if (ex && typeof ex == 'object') {
                if (ex.description) // esprima exception
                    postMessage({build:[ex.description]}, '*');
                if (ex.error)
                    postMessage({build:[ex.error]}, '*');
            } else if (typeof ex == 'string') {
                postMessage({build:[ex]}, '*');
            }
        } finally {
            if (img)
                img.remove();
            this.#preview.setExportVisibility(true);
        }
    }

}
module.exports.IDE = IDE;
