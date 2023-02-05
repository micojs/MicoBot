const {dom, index} = require('./dom.js');

class ProjectControls {
    constructor(el, model) {
        this.el = el;
        this.view = index(el, {}, this);
        this.model = model;
        model.watch(this.view);
    }

    goToTOP() {
        this.model.set('state', 'TOP');
    }
}

module.exports.ProjectControls = ProjectControls;
