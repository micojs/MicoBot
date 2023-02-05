const {dom} = require('./dom.js');

class Console {
    maxLines = 1000;
    #prevLine = {
        text:null,
        count:0,
        el:null
    };
    onAppend;

    #tweens = 0;
    #interval;

    constructor(filter) {
        this.el = dom('div', {className:'console'});

        if (typeof filter == 'string')
            filter = ((filter, msg) => msg[filter]).bind(this, filter);

        window.addEventListener('message', event => {
            let msgs = filter(event.data);
            if (!Array.isArray(msgs)) {
                if (typeof msgs == 'object' && msgs) {
                    for (let key in msgs) {
                        this[key](msgs[key]);
                    }
                }
                return;
            }

            let top = this.el.scrollTop;

            for (let msg of msgs) {
                this.append(msg);
            }

            setTimeout(_=>this.el.scrollTop = top, 0);
            this.tween();

            if (this.onAppend)
                this.onAppend();
        });
    }

    clear() {
        while (this.el.children.length)
            this.el.children[0].remove();
    }

    tween() {
        this.#tweens = 30;
        if (this.#interval)
            return;
        this.#interval = setInterval(_=>{
            this.el.scrollTop = (this.el.scrollTop * 13 + this.el.scrollHeight) / 14;
            this.#tweens--;
            if (this.#tweens <= 0) {
                clearInterval(this.#interval);
                this.#interval = 0;
            }
        }, 1000 / this.#tweens);
    }

    append(msg) {
        if (Array.isArray(msg))
            msg = msg.join(' ');

        const line = (msg + '');

        if (line === this.#prevLine.text) {
            this.#prevLine.count++;
            this.#prevLine.el.classList.add('repeat');
            this.#prevLine.el.setAttribute('repeat', this.#prevLine.count);
        } else {
            while (this.el.children.length >= this.maxLines)
                this.el.children[0].remove();
            this.#prevLine.text = line;
            this.#prevLine.count = 1;
            this.#prevLine.el = dom(this.el, 'pre', {
                className:'line',
                textContent:line,
                attr:{
                    time:(new Date()).toLocaleString()
                }
            });
        }
    }
}

module.exports.Console = Console;
