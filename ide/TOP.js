const {dom, index} = require('./dom.js');
const {Model} = require('./Model.js');

class TOP {
    #el;
    #view;
    #parent;
    #model;

    constructor(el, parent, model) {
        this.#el = el;
        this.#view = index(el, {}, this);
        this.#parent = parent;
        this.#model = model;
        model.watch(this.#view);
        model.watch('state', state => this.changeState(state));
    }

    changeState(newState) {
        this.#el.classList.toggle('hidden', newState != this.constructor.name);
    }

    clickProject(event) {
        if (event.target != event.currentTarget)
            this[event.target.getAttribute('bind-click')]?.(event);
    }

    checkName() {
        const old = this.#view.newprojectname.value;
        const name = old
              .trim()
              .replace(/[^a-zA-Z 0-9]+/, '')
              .substr(0, 18);
        // if (old != name)
        //     this.#view.newprojectname.value = name;
        return name;
    }

    async newProject() {
        const rawName = this.#view.newprojectname.value.trim();
        const name = this.checkName();
        const newprojectname = this.#view.newprojectname;
        if (name.length == 0){
            newprojectname.focus();
            newprojectname.placeholder = 'Enter Your Project\'s name here.';
            return;
        }

        const projectList = this.#model.get('projectList', []);

        let id = 0;
        for (let project of projectList) {
            if (id <= (project.id|0))
                id = (project.id|0) + 1;
            if (project.name.toLowerCase() == name.toLowerCase()) {
                newprojectname.focus();
                newprojectname.placeholder = 'Name already taken by another project.';
                newprojectname.value = '';
                return;
            }
        }

        const remote = this.#parent.remote;
        const remoteProject = await remote.loadProject(rawName);
        if (remoteProject) {
            const modelId = 'project-' + id;
            const projectModel = new Model();
            projectModel.useStorage(modelId);
            projectModel.set('fs', remoteProject.fs);
            projectModel.set('RPIN', rawName);
            this.#model.set(modelId, projectModel);
        }

        const project = {id, name};
        projectList.push(project);
        this.#model.set('projectId', project.id);
        this.#model.set('projectName', project.name);
        this.#model.set('state', 'IDE');
        this.#model.set('projectList', projectList);
    }

    findProject(pred) {
        const projectList = this.#model.get('projectList', []);
        nextProject: for (const project of projectList) {
            for (const key in pred) {
                if (pred[key] != project[key])
                    continue nextProject;
            }
            return project;
        }
        return undefined;
    }

    openProject(event) {
        const project = this.findProject({id:event.target.getAttribute('project-id')});
        this.#model.set('projectId', project.id);
        this.#model.set('projectName', project.name);
        this.#model.set('state', 'IDE');
    }

    deleteProject() {
        const project = this.findProject({id:event.target.getAttribute('project-id')});
        const projectList = this.#model.get('projectList', []);
        const index = projectList.indexOf(project);
        projectList.splice(index, 1);
        this.#model.dirty('projectList');

        const projectId = project.id;
        const modelId = "project-" + projectId;
        let model = this.#model.get(modelId, null);
        if (!model)
            model = new Model();
        model.destroy(modelId);
        this.#model.set(modelId, null);
    }
}

module.exports.TOP = TOP;
