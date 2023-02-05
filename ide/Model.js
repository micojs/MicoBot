const localforage = require('localforage');

localforage.config({name:"model"});

class Model {
    #storageId;
    #data;
    #timeout;
    #dirtyCount = 0;
    onBeforeSave;
    onAfterSave;

    constructor() {
        this.#data = {};
    }

    dirty(key){
        let entry = this.#data[key];
        if (entry)
            entry.listeners.forEach(listener => listener(entry.value));

        if (!this.#storageId)
            return;

        if (this.#timeout > 0) {
            this.#dirtyCount++;
            if (this.#dirtyCount > 100)
                return;
            // console.log("Delaying save ", this.#dirtyCount);
            clearTimeout(this.#timeout);
            this.#timeout = 0;
        } else if (this.#timeout == -1) {
            // console.log("Ignoring dirty");
            return;
        } else {
            // console.log("Schedule save");
        }

        this.#timeout = setTimeout(this.flush.bind(this), 1000);
    }

    flush() {
        clearTimeout(this.#timeout);
        this.#timeout = -1;
        console.log("Saving", this.#storageId);
        this.onBeforeSave?.();
        clearTimeout(this.#timeout);
        this.#timeout = 0;
        this.#dirtyCount = 0;
        localforage.setItem(this.#storageId, this.toJSON());
        this.onAfterSave?.();
    }

    toJSON() {
        const json = {};
        for (let k in this.#data) {
            let v = this.#data[k].value;
            if (v && typeof v == 'object' && v.constructor != Object && v.constructor != Array) {
                // console.log(`Skipping serialization of ${v.constructor.name}`);
                continue;
            }
            json[k] = v;
        }
        return json;
    }

    async useStorage(storageId){
        this.#storageId = null;
        const data = await localforage.getItem(storageId);
        if (data) {
            for (let k in data)
                this.set(k, data[k]);
        }
        this.#storageId = storageId;
    }

    async destroy(storageId) {
        if (storageId || this.#storageId)
            await localforage.removeItem(storageId || this.#storageId);
    }

    init(values) {
        const listeners = [];
        for (const key in values) {
            const value = values[key];
            let entry = this.#data[key];
            if (entry && ('value' in entry))
                continue;
            if (!entry) {
                entry = this.#data[key] = {
                    value,
                    listeners:[]
                };
            } else {
                entry.value = value;
                listeners.push(...entry.listeners.map(l => l.bind(null, value)));
            }
        }
        listeners.forEach(listener => listener());
    }

    set(key, value) {
        let entry = this.#data[key];
        if (!entry) {
            entry = this.#data[key] = {
                value,
                listeners:[]
            };
        } else if (entry.value != value || (value && typeof value == 'object')) {
            entry.value = value;
            this.dirty(key);
        }
    }

    get(key, defval) {
        let entry = this.#data[key];
        if (!entry) {
            if (defval !== undefined)
                this.set(key, defval)
            return defval;
        }
        return entry.value;
    }

    watch(key, callback) {
        if (typeof key == 'object') {
            const bound = [];
            for (let name in key) {
                const el = key[name];
                if (Array.isArray(el) || bound.indexOf(el) != -1)
                    continue;
                for (let i = 0; i < el.attributes.length; ++i) {
                    const attr = el.attributes[i];
                    const parts = attr.name.split('-');
                    if (parts.length != 2)
                        continue;

                    let prop = ({
                        text:'textContent',
                        html:'innerHTML'
                    })[parts[1]] || parts[1];

                    if (parts[0] == 'read' || parts[0] == 'readwrite')
                        this.watch(attr.value, read.bind(this, el, prop));
                    if (parts[0] == 'write' || parts[0] == 'readwrite')
                        el.addEventListener('change', write.bind(this, el, prop, attr.value));
                    if (parts[0] == 'for' && parts[1] == 'each')
                        this.watch(attr.value, foreach.bind(this, el, el.innerHTML));
                }
            }

            function foreach(el, template, arr) {
                if (!Array.isArray(arr))
                    return;
                const children = [];
                for (const desc of arr)
                    children.push(template.replace(/\$\{([^}]+)\}/g, (_, key) => desc[key]));
                el.innerHTML = children.join('');
            }

            function read(el, prop, val) {
                el[prop] = val;
                console.log('updating ', el, 'prop', prop, 'with val', val);
            }

            function write(el, prop, key) {
                this.set(key, el[prop]);
            }
        } else {
            let entry = this.#data[key];
            if (!entry) {
                entry = this.#data[key] = {
                    listeners:[callback]
                };
            } else {
                entry.listeners.push(callback);
            }
            callback(entry.value);
        }
    }
};

module.exports.Model = Model;
