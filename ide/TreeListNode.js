const {dom} = require('./dom.js');

class TreeListNode {
    constructor(entry, path, cb, index) {
        this.path = path + entry.name;
        this.entry = entry;
        index[this.path] = this;

        const depth = path.split("/").length - 2;
        const space = "&nbsp;".repeat(depth * 2);

        const children = Object.values(entry.node.children || {})
              .sort((a, b)=>(a.node.children && !b.node.children) || a.name > b.name);

        this.dom = dom('div', {
            className:`treenode ${this.entry.node.constructor.name}`,
            onclick:event=>{
                event.stopPropagation();
                event.preventDefault();
                cb(path + entry.name);
            }
        }, [
            ["div", {innerHTML:space + entry.name, className:"filelabel"}],
            ...children.map(child => (new TreeListNode(child, this.path + "/", cb, index)).dom)
        ]);
    }

    setActive(isActive) {
        this.dom.classList.toggle('active', !!isActive);
    }

    isActive() {
        return this.dom.classList.contains("active");
    }

    isFile() {
        return this.entry.isFile();
    }

    isDirectory() {
        return this.entry.isDirectory();
    }
}

module.exports.TreeListNode = TreeListNode;
