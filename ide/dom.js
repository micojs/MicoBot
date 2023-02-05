function dom(...args) {
    let tag = "div";
    let parent = null;
    let props = null;
    let children = null;
    for (let arg of args) {
        if (arg === undefined)
            continue;

        if (typeof arg == 'string') {
            tag = arg;
            continue;
        }
        if (arg && arg instanceof Element) {
            parent = arg;
            continue;
        }
        if (arg && Array.isArray(arg)) {
            children = arg;
            continue;
        }
        if (arg && typeof arg == 'object') {
            props = arg;
            continue;
        }
    }

    let node = document.createElement(tag);

    if (parent)
        parent.appendChild(node);

    if (props) {
        if (props.style) {
            Object.assign(node.style, props.style);
            props = Object.assign({}, props);
            delete props.style;
        }
        if (props.attr) {
            for (let key in props.attr) {
                node.setAttribute(key, props.attr[key]);
            }
            delete props.attr;
        }
        Object.assign(node, props);
    }

    if (children)
        children.forEach(child => {
            if (!child)
                return;
            else if (Array.isArray(child))
                dom(node, ...child);
            else if (child instanceof Element)
                node.appendChild(child);
        });

    return node;
}

function index(el, map = {}, bind = null) {
    if (el.className) {
        if (!Array.isArray(map[el.className]))
            map[el.className] = [el];
        else
            map[el.className].push(el);
    }
    if (el.id)
        map[el.id] = el;

    let recurse = true;

    if (bind) {
        const bindClazz = bind.constructor.name.toLowerCase();

        for (let i = 0; i < el.attributes.length; ++i) {
            const key = el.attributes[i].name;
            const value = el.attributes[i].value;
            const parts = key.split('-');

            if (parts.length != 2 || (parts[0] != 'index' && parts[0] != 'bind'))
                continue;

            if (parts[0] == 'index' && parts[1] != bindClazz)
                continue;


            const func = bind[value];

            if (typeof func != 'function') {
                console.warn(`No method ${value} in ${bind.constructor.name}.`, el);
                continue;
            }

            if (parts[0] == 'index') {
                recurse = func.call(bind, el) !== false;
            } else if (parts[0] == 'bind') {
                el.addEventListener(parts[1], bind[value].bind(bind));
            }
        }
    }

    if (recurse) {
        for (let i = 0, max = el.children.length; i < max; ++i)
            index(el.children[i], map, bind);
    }

    return map;
}

module.exports.dom = dom;
module.exports.index = index;
