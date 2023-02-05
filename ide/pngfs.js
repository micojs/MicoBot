let nextId = 1;

class Node {
    constructor(id = nextId++) {
        this.id = id;
    }
}

class Directory extends Node {
    constructor() {
        super();
        this.children = {};
    }

    addChild(entry) {
        this.children[entry.name] = entry;
    }

    unlink(entry) {
        if (this.children[entry.name] == entry)
            delete this.children[entry.name];
    }

    lookup(name) {
        return this.children[name];
    }

    unserialize(tree, ids) {
        ids.forEach(id => {
            let entry = tree[id];
            this.children[entry.name] = entry;
        });
    }

    serialize(tree) {
        const ids = [];
        for (let key in this.children) {
            let child = this.children[key];
            ids.push(child.id);
            child.serialize(tree);
        }
        tree[this.id] = {Directory:[ids]}
    }
}

class File extends Node {
    constructor(data = null) {
        super();
        this.data = data;
    }

    serialize(tree) {
        tree[this.id] = {File:[this.data]};
    }
}

class Entry extends Node {
    constructor(name, node) {
        super();
        this.name = name;
        this.node = node;
    }

    is(type) {
        return this.node instanceof type;
    }

    isFile() {return this.is(File);}
    isDirectory() {return this.is(Directory);}

    get extension () {
        return (this.name.match(/\.([^./]+)$/i)?.[1] || '').toLowerCase();
    }

    unserialize(tree, name, node) {
        this.name = name;
        this.node = tree[node];
        if (!this.node) {
            this.node = new Directory();
            this.node.id = node;
        }
    }

    serialize(tree) {
        this.node.serialize(tree);
        tree[this.id] = {Entry:[this.name, this.node.id]};
    }
}

module.exports.PNGFS = class PNGFS {
    #jsonCache;

    onDirty;

    constructor(image) {
        this.root = new Entry("", new Directory());
        this.root.id = "root";
        this.path = [this.root];
        this.version = 1;
        if (image && image.pngfs <= this.version)
            this.loadJSON(image);
    }

    dirty() {
        this.#jsonCache = null;
        this.onDirty?.();
    }

    loadJSON(tree) {
        const classes = {Entry, File, Directory};
        const index = {};
        const args = {};
        for (let id in tree) {
            if (id == "pngfs")
                continue;
            const meta = tree[id];
            const ctorName = Object.keys(meta)[0];
            const ctor = classes[ctorName];
            args[id] = meta[ctorName];
            const obj = new ctor(...args[id]);
            obj.id = id;
            index[id] = obj;
            if (nextId <= id)
                nextId = id + 1;
        }
        for (const id in index) {
            const obj = index[id];
            if (obj.unserialize)
                obj.unserialize(index, ...args[id]);
        }
        this.root = index.root;
        this.path = [this.root];
    }

    toJSON(){
        if (this.#jsonCache) {
            // console.log("Skipping serialization of cached PNGFS");
            return this.#jsonCache;
        }
        const tree = {pngfs:this.version};
        this.root.serialize(tree);
        this.#jsonCache = tree;
        // console.log("Serialized PNGFS");
        return tree;
    }

    ls(p = "") {
        const path = [...this.path];

        const parts = p.split('/');
        if (p && parts.length) {
            if (parts[0] == '') {
                path.length = 1;
                parts.shift();
            }

            while (parts.length) {
                const part = parts.shift();
                if (part == '.' || part == '')
                    continue;

                if (part == '..') {
                    if (path.length == 1)
                        return false;
                    path.pop();
                    continue;
                }

                const top = path[path.length - 1];
                const child = top.node.lookup(part);

                if (!child || !child.is(Directory))
                    return false;
                path.push(child);
            }
        }

        const top = path[path.length - 1];

        return Object.values(top.node.children);
    }

    cd(p) {
        const parts = p.split('/');
        if (!parts.length)
            return true;

        if (parts[0] == '') {
            this.path.length = 1;
            parts.shift();
        }

        while (parts.length) {
            const part = parts.shift();
            if (part == '.' || part == '')
                continue;

            if (part == '..') {
                if (this.path.length == 1)
                    return false;
                this.path.pop();
                continue;
            }

            const top = this.path[this.path.length - 1];
            const child = top.node.lookup(part);
            if (!child || !child.is(Directory))
                return false;

            this.path.push(child);
        }

        return true;
    }

    mkdir(p) {
        const path = [...this.path];
        const parts = p.split('/');
        if (!parts.length)
            return true;

        if (parts[0] == '') {
            path.length = 1;
            parts.shift();
        }

        while (parts.length) {
            const part = parts.shift();
            if (part == '.' || part == '')
                continue;

            if (part == '..') {
                if (path.length == 1)
                    return false;
                path.pop();
                continue;
            }

            const top = path[path.length - 1];
            const child = top.node.lookup(part);

            if (!child) {
                const entry = new Entry(part, new Directory());
                top.node.addChild(entry);
                path.push(entry);
                this.dirty();
            } else {
                if (!child.is(Directory))
                    return false;
                path.push(child);
            }
        }

        return true;
    }

    link(p, node) {
        const path = [...this.path];
        const parts = p.split('/');
        if (!parts.length)
            return true;

        if (parts[0] == '') {
            path.length = 1;
            parts.shift();
        }

        while (parts.length) {
            const part = parts.shift();
            if (part == '.' || part == '')
                continue;

            if (part == '..') {
                if (path.length == 1)
                    return false;
                path.pop();
                continue;
            }

            const top = path[path.length - 1];

            if (!parts.length) {
                this.dirty();
                return top.node.addChild(new Entry(part, node));
            }

            const child = top.node.lookup(part);
            if (!child || !child.is(Directory))
                return false;

            path.push(child);
        }

        return true;
    }

    writeFile(name, data) {
        let {child} = this.lookup(name);
        if (!child) {
            this.link(name, new File(data));
            return true;
        }
        if (child.is(File)) {
            child.node.data = data;
            this.dirty();
            return true;
        }
        return false;
    }

    readFile(name) {
        let {child} = this.lookup(name);
        if (!child)
            throw `Could not read inexistant file ${name}.`;
        if (!child.is(File))
            throw `Could not read ${child.constructor.name} ${name} as file.`;

        return child.node.data;
    }

    rm(p) {
        let {parent, child} = this.lookup(p);
        if (!parent || !child)
            return false;
        this.dirty();
        parent.node.unlink(child);
        return true;
    }

    lookup(p) {
        const path = [...this.path];
        const parts = p.split('/');
        if (!parts.length)
            return true;

        if (parts[0] == '') {
            path.length = 1;
            parts.shift();
        }

        while (parts.length) {
            const part = parts.shift();
            if (part == '.' || part == '')
                continue;

            if (part == '..') {
                if (path.length == 1)
                    return false;
                path.pop();
                continue;
            }

            const top = path[path.length - 1];
            const child = top.node.lookup(part);

            if (!child) {
                return false;
            }

            if (!parts.length) {
                return {parent:top, child};
            }

            if (!child.is(Directory))
                return false;

            path.push(child);
        }

        return true;
    }

    cwd() {
        return this.path.map(entry => entry.name).join('/');
    }

    recurse(callback) {
        const path = [];
        recurse(this.root, callback);

        function recurse(entity, callback) {
            path.push(entity.name);
            if (callback(entity, path) !== false && entity.isDirectory()) {
                for (let child of Object.values(entity.node.children))
                    recurse(child, callback);
            }
            path.pop();
        }
    }
}
