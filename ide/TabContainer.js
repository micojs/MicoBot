const {dom} = require('./dom.js');

class TabContainer {
    constructor(node) {
        this.onClose = null;
        this.onActivate = null;
        this.tabs = {};
        this.node = node;
        this.tabset = dom(node, {className:'tabset'});
        this.container = dom(node, {className:'tabcontainer'});
    }

    close(name) {
        const tab = this.tabs[name];
        if (!tab)
            return;

        if (this.onClose) {
            if (this.onClose(tab.contents) === false)
                return;
        }

        let active = null;
        let beforeActive = null;
        let afterActive = null;

        for (let name in this.tabs) {
            const {tab} = this.tabs[name];
            const isActive = tab.className.indexOf("inactive") == -1;
            if (isActive) {
                active = name;
            } else if (!active) {
                beforeActive = name;
            } else if (!afterActive) {
                afterActive = name;
                break;
            }
        }

        if (active == name) {
            if (beforeActive) {
                this.activate(beforeActive);
            } else if (afterActive) {
                this.activate(afterActive);
            }
        }

        tab.tab.remove();
        tab.contents.remove();
        delete this.tabs[name];
    }

    active() {
        for (let name in this.tabs) {
            const {tab, contents} = this.tabs[name];
            if (tab.className.indexOf("inactive") == -1) {
                return {name, contents};
            }
        }
        return undefined;
    }

    add(name, contents) {
        let close = [];

        if (this.onClose) {
            close.push(dom('span', {className:'tabclose', textContent:'x', onclick:this.close.bind(this, name)}));
        }

        this.tabs[name] = {
            tab:dom(this.tabset, {textContent:name, className:'tab inactive', onclick:_=>{this.activate(name);}}, close),
            contents
        };
    }

    activate(name) {
        const tab = this.tabs[name];
        if (!tab)
            return false;

        while (this.container.lastChild)
            this.container.removeChild(this.container.lastChild);

        this.container.appendChild(tab.contents);

        for (let k in this.tabs)
            this.tabs[k].tab.className = 'tab ' + (k == name ? 'active' : 'inactive');

        if (this.onActivate)
            this.onActivate(tab.contents);

        return true;
    }
}

module.exports.TabContainer = TabContainer;
