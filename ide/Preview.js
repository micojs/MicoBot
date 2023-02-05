const {index} = require('./dom.js');

class Preview {
    constructor(node, parent, model) {
        this.view = index(node, {}, this);
        this.model = model;
        this.parent = parent;
        model.watch(this.view);
        model.watch('platform', _ => this.resize());
        model.watch('previewHTML', html => this.showHTML(html));
        this.resize();
    }

    getPlatformSize() {
        const platform = this.model.get('platform');
        const lcd = {
            pokitto:[220, 176],
            blit:[320, 240],
            espboy:[128, 128],
            meta:[160, 128],
            metro:[320, 240],
            pico:[240, 240]
        };
        return lcd[platform];
    }

    setExportVisibility(visible) {
        this.view.export.classList.toggle('hidden', !visible);
    }

    resize() {
        const container = this.view.screencontainer;
        const size = this.getPlatformSize();
        const zoom = this.view.zoom.value;
        if (!size) {
            container.display = 'none';
            return;
        }

        const [width, height] = size;
        const screen = this.view.screencontainer.querySelector('iframe');
        if (screen) {
            screen.style.width = (width * zoom) + "px";
            screen.style.height = (height * zoom) + "px";
        }
        container.style.display = 'block';
        container.style.height = (height * zoom) + "px";
        container.style["min-width"] = (width * zoom) + "px";
        container.parentElement.style["min-width"] = (width * zoom) + "px";
    }

    showHTML(html) {
        this.view.screen.remove();
        this.view.screencontainer.innerHTML = this.view.screen.outerHTML;
        const screen = this.view.screencontainer.querySelector('iframe');
        screen.src = "about:blank";

        if (html) {
            screen.contentWindow.document.open();
            screen.contentWindow.document.write(html);
            screen.contentWindow.document.close();
            screen.focus();
        }

        this.resize();
        this.view.run.textContent = html ? 'Stop' : 'Run';
    }

    toggleRun() {
        this.model.set('runSize', this.model.get('previewHTML') ? null : this.getPlatformSize());
    }

    export() {
        this.parent.export();
    }
}

module.exports.Preview = Preview;
