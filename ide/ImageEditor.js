const {dom, index} = require('./dom.js');
const {convertToU8, palette} = require('./PreBuild.js');

class ImageEditor {
    #fs;
    #dom;
    path;
    metadata;
    img;
    canvas;
    ctx;
    imgData;
    forceRedraw;

    selectedTID;

    tilesetPath;
    tilesetMeta;
    tilesetImg;

    constructor(parentElement, path, image, fs) {
        this.path = path;
        try {
            this.metadata = JSON.parse(fs.readFile(path + ".meta"));
        } catch(ex) {}

        if (!this.metadata || typeof this.metadata != "object") {
            this.metadata = {mode:"image"};
        }
        if (!this.metadata.image || typeof this.metadata.image != 'object')
            this.metadata.image = {};
        if (!this.metadata.tilemap || typeof this.metadata.tilemap != 'object')
            this.metadata.tilemap = {tileset:''};
        if (!this.metadata.tileset || typeof this.metadata.tileset != 'object')
            this.metadata.tileset = {tilewidth:8, tileheight:8};

        this.#fs = fs;
        this.#dom = index(dom('div', parentElement, {className:'imageEditor'}, [
            ['div', {id:'imgcontainer'}, [
                ['canvas', {id:'canvas', src:image, onload:this.onload.bind(this)}],
                ['img', {id:'img', src:image, onload:this.onload.bind(this)}]
            ]],
            ['div', {id:'sidebar'}, [
                // ['select', {id:'mode', onchange:this.changeMeta.bind(this)}, [
                //     ['option', {value:'image', textContent:'image'}],
                //     ['option', {value:'tilemap', textContent:'tilemap'}],
                //     ['option', {value:'tileset', textContent:'tileset'}]
                // ]],
                [{id:'imageSidebar'}, [
                    [{className:'row'}, [
                        ['span', {textContent:'Width:'}],
                        ['span', {id:'imgwidth'}]
                    ]],
                    [{className:'row'}, [
                        ['span', {textContent:'Height:'}],
                        ['span', {id:'imgheight'}]
                    ]]
                ]],
                // [{id:'tilemapSidebar'}, [
                //     [{className:'row'}, [
                //         ['span', {textContent:'Tile Set:'}],
                //         ['select', {id:'tileset', onchange:this.changeMeta.bind(this)}, this.getTilesets().map(path => [
                //             'option', {value:path, textContent:path.replace(/.*\/|\..*/g, '')}
                //         ])],
                //         ['button', {id:'refresh', onclick:this.refreshTileSets.bind(this), textContent:'⟳'}]
                //     ]]
                // ]],
                // [{id:'tilesetSidebar'}, [
                //     [{className:'row'}, [
                //         ['span', {textContent:'Tile Width:'}],
                //         ['input', {
                //             id:'tilewidth', type:"number", min:"8", max:"256",
                //             onchange:this.changeMeta.bind(this),
                //             value:this.metadata.tileset.tilewidth|0||8
                //         }]
                //     ]],
                //     [{className:'row'}, [
                //         ['span', {textContent:'Tile Height:'}],
                //         ['input', {
                //             id:'tileheight', type:"number", min:"8", max:"256",
                //             onchange:this.changeMeta.bind(this),
                //             value:this.metadata.tileset.tileheight|0||8
                //         }]
                //     ]]
                // ]]
            ]]
        ]), {}, this);
        this.#dom.tileset.value = this.metadata.tilemap.tileset;
        this.#dom.mode.value = this.metadata.mode;
    }

    commit() {
    }

    reload() {
        this.#dom.img.src = this.#fs.readFile(this.path);
    }

    refreshTileSets() {
        const tileset = this.#dom.tileset;
        let oldValue = tileset.value;

        while (tileset.children.length)
            tileset.children[0].remove();

        for (let path of this.getTilesets())
            dom(tileset, 'option', {value:path, textContent:path.replace(/.*\/|\..*/g, '')});

        tileset.value = oldValue;
        this.tilesetData = null;
        this.layout();
    }

    onload() {
        this.layout();
    }

    initCanvas(width, height, img) {
        if (!width || !height)
            return false;
        const canvas = this.#dom.canvas;
        if (canvas.width != width || canvas.height != height) {
            canvas.width = width;
            canvas.height = height;
            this.imgData = null;
        }
        if (!this.ctx) {
            this.ctx = canvas.getContext('2d');
        }
        if (!this.imgData) {
            if (img)
                this.ctx.drawImage(img, 0, 0);
            this.imgData = this.ctx.getImageData(0, 0, width, height);
            return true;
        }
        return false;
    }

    layout() {
        const mode = this.metadata.mode;
        const editor = this.#dom.imageEditor[0];
        editor.classList.toggle('image', mode == 'image');
        editor.classList.toggle('tilemap', mode == 'tilemap');
        editor.classList.toggle('tileset', mode == 'tileset');

        if (mode == 'image')
            this.layoutImage();

        if (mode == 'tilemap')
            this.layoutTilemap();

        if (mode == 'tileset')
            this.layoutTileset();
    }

    autozoom() {
        const img = this.#dom.img;
        const canvas = this.#dom.canvas;
        const parent = img.parentElement;
        const zoom = Math.min(parent.clientWidth/img.naturalWidth, parent.clientHeight/img.naturalHeight) * 0.85;
        const width = img.naturalWidth * zoom | 0;
        const height = img.naturalHeight * zoom | 0;
        canvas.style.width  = img.style.width  = width + "px";
        canvas.style.height = img.style.height = height + "px";
        canvas.style.left   = img.style.left   = ((parent.clientWidth/2 - width/2)|0) + "px";
        canvas.style.top    = img.style.top    = ((parent.clientHeight/2 - height/2)|0) + "px";
    }

    layoutImage() {
        this.autozoom();

        const img = this.#dom.img;
        this.#dom.imgwidth.textContent = img.naturalWidth;
        this.#dom.imgheight.textContent = img.naturalHeight;

        if (!this.initCanvas(img.naturalWidth, img.naturalHeight, img)) {
            return;
        }
        const u8 = convertToU8(this.imgData);
        const data = this.imgData.data;
        let i = 0;
        for (let y = 0; y < img.naturalHeight; ++y) {
            for (let x = 0; x < img.naturalWidth; ++x) {
                let color = palette[u8[y + 1][x]];
                data[i++] = color[0];
                data[i++] = color[1];
                data[i++] = color[2];
                data[i++] = 255;
            }
        }
        this.ctx.putImageData(this.imgData, 0, 0);
    }

    getTilesets() {
        const fs = this.#fs;
        let tilesets = []
        let path = [];

        recurse(fs.root, (entity) => {
            if (!entity.isFile())
                return entity.name[0] != '.';
            let extension = entity.extension;
            if (extension == 'meta') {
                try {
                    let meta = JSON.parse(entity.node.data);
                    if (meta.mode == 'tileset')
                        tilesets.push(path.join('/'));
                } catch (ex){}
            }
            return false;
        });

        return tilesets;

        function recurse(entity, callback) {
            path.push(entity.name);
            if (callback(entity) !== false && entity.isDirectory()) {
                for (let child of Object.values(entity.node.children))
                    recurse(child, callback);
            }
            path.pop();
        }
    }

    tilesetDimensions() {
        const tilewidth = (this.tilesetData.tileset.tilewidth|0) || 8;
        const tileheight = (this.tilesetData.tileset.tileheight|0) || 8;
        return {
            tilewidth,
            tileheight,
            tilesPerRow: ((this.tilesetImg.naturalWidth / tilewidth) | 0) || 1,
            tilesPerCol: ((this.tilesetImg.naturalWidth / tileheight) | 0) || 1
        };
    }

    async layoutTilemap() {
        this.autozoom();

        let forceRedraw = this.forceRedraw;
        this.forceRedraw = false;

        if (this.tilesetPath != this.metadata.tilemap.tileset) {
            this.tilesetData = null;
            this.tilesetImg = null;
        }

        if (!this.tilesetData) {
            this.tilesetData = JSON.parse(this.#fs.readFile(this.metadata.tilemap.tileset));
            this.tilesetPath = this.metadata.tilemap.tileset;
            forceRedraw = true;
        }

        if (!this.tilesetImg) {
            forceRedraw = true;
            let imgName = this.metadata.tilemap.tileset.replace(/\.meta$/gi, '');
            let imgSrc = this.#fs.readFile(imgName);
            await new Promise((onload, onerror) => {
                this.tilesetImg = dom('img', {src:imgSrc, onload, onerror});
            });
        }

        if (!forceRedraw)
            return;

        const {tilewidth, tileheight, tilesPerRow, tilesPerCol} = this.tilesetDimensions();
        const img = this.#dom.img;
        this.initCanvas(img.naturalWidth, img.naturalHeight, img);

        const canvas = this.#dom.canvas;
        canvas.width = img.naturalWidth * tilewidth;
        canvas.height = img.naturalHeight * tileheight;

        let i = 0, ntid = 1;
        let assign = {};
        for (let y = 0; y < this.imgData.height; ++y) {
            for (let x = 0; x < this.imgData.width; ++x) {
                let c = 0;
                c = this.imgData.data[i++];
                c <<= 8;
                c |= this.imgData.data[i++];
                c <<= 8;
                c |= this.imgData.data[i++];

                let A = this.imgData.data[i++];
                if (A < 128)
                    continue;

                c = c.toString(16);

                let tdat = this.metadata.tilemap['#' + c];
                if (!tdat || typeof tdat != 'object')
                    this.metadata.tilemap['#' + c] = tdat = {};
                assign[c] = tdat;

                let tid = tdat.id;
                if (tid === undefined)
                    continue;

                tid |= 0;
                let tx = tid % tilesPerRow;
                let ty = (tid / tilesPerRow) | 0;
                if (ty >= tilesPerCol) {
                    tdat.id = 0;
                    continue;
                }

                this.ctx.drawImage(
                    this.tilesetImg,
                    tx * tilewidth, ty * tileheight, tilewidth, tileheight,
                    x * tilewidth, y * tilewidth, tilewidth, tileheight
                );
            }
        }

        let old = this.#dom.tilemapSidebar.querySelectorAll('.assign');
        for (let i = 0; i < old.length; ++i)
            old[i].remove();

        for (let key in assign) {
            const row = dom(this.#dom.tilemapSidebar, {className:'row assign'}, [
                [{style:{backgroundColor:'#' + '0'.repeat(6 - key.length) + key, width:'32px', flexGrow:0}}],
                ['canvas', {width:tilewidth, height:tileheight, flexGrow:0, onclick:this.changeTileId.bind(this, key, assign[key])}],
                ['input', {type:'number', value:assign[key].info|0, style:{flexGrow:1}, onchange:this.changeTileInfo.bind(this, key, assign[key])}]
            ]);
            const tid = assign[key].id;
            const canvas = row.querySelector('canvas');
            const ctx = canvas.getContext('2d');
            let tx = tid % tilesPerRow;
            let ty = (tid / tilesPerRow) | 0;
            ctx.drawImage(this.tilesetImg, tx * tilewidth, ty * tileheight, tilewidth, tileheight, 0, 0, tilewidth, tileheight);
        }
    }

    changeTileId(key, obj) {
        if (!this.tilesetImg)
            return;
        const {tilewidth, tileheight, tilesPerRow, tilesPerCol} = this.tilesetDimensions();
        const canvases = [];
        const parent = this.#dom.imageEditor[0];
        const aspect = this.tilesetImg.naturalWidth / this.tilesetImg.naturalHeight;
        const height = parent.clientHeight * 0.9 | 0;
        const width = height * aspect | 0;
        const popup = dom(parent, {
            className:'overlay',
            onclick:event=>{
                if (event.target == popup)
                    popup.remove();
            }
        }, [
            [{className:'popup', style:{height:height + 'px', width:width + "px"}}]
        ]);
        const container = popup.querySelector('.popup');
        for (let ty = 0; ty < tilesPerCol; ++ty) {
            for (let tx = 0; tx < tilesPerRow; ++tx) {
                const canvas = dom('canvas', {width:tilewidth, height:tileheight, className:'tile', onclick:setTID.bind(this, tx, ty)}, container);
                const ctx = canvas.getContext('2d');
                ctx.drawImage(this.tilesetImg, tx * tilewidth, ty * tileheight, tilewidth, tileheight, 0, 0, tilewidth, tileheight);
                canvases.push(canvas);
            }
        }

        setTID.call(this, obj.id % tilesPerRow, obj.id / tilesPerRow|0);

        function setTID(tx, ty) {
            const id = ty * tilesPerRow + tx;
            canvases.forEach((canvas, i) => {
                canvas.classList.toggle('selected', i == id)
            });
            if (id != obj.id) {
                obj.id = id;
                this.forceRedraw = true;
                this.changeMeta();
            }
        }
    }

    changeTileInfo(key, obj, event) {
        obj.info = event.target.value | 0;
        this.dirty();
    }

    layoutTileset() {
        this.autozoom();

        const dom = this.#dom;
        const imgwidth  = dom.img.naturalWidth;
        const imgheight = dom.img.naturalHeight;
        const tw = this.metadata.tileset.tilewidth || 8;
        const th = this.metadata.tileset.tileheight || 8;

        if (!this.initCanvas(imgwidth/tw, imgheight/th)) {
            return;
        }

        let i = 0;
        const data = this.imgData.data;
        for (let y = 0; y < this.imgData.height; ++y) {
            for (let x = 0; x < this.imgData.width; ++x) {
                let c = (x + y) & 1 ? 0xAA : 0x77;
                data[i++] = c;
                data[i++] = c;
                data[i++] = c;
                data[i++] = 255;
            }
        }

        this.ctx.putImageData(this.imgData, 0, 0);
    }

    changeMeta() {
        const dom = this.#dom;

        const imgwidth = dom.img.naturalWidth;
        const imgheight = dom.img.naturalHeight;

        let tilewidth = (dom.tilewidth.value | 0) || 8;
        let tileheight = (dom.tileheight.value | 0) || 8;

        const tilesPerRow = Math.round(imgwidth / tilewidth) || 1;
        const tilesPerCol = Math.round(imgheight / tileheight) || 1;

        if ((imgwidth / tilesPerRow)|0 != tilewidth)
            dom.tilewidth.value = tilewidth = (imgwidth / tilesPerRow)|0;
        if ((imgheight / tilesPerCol)|0 != tileheight)
            dom.tileheight.value = tileheight = (imgheight / tilesPerCol)|0;

        this.metadata = {
            mode:dom.mode.value,
            image:{},
            tilemap:Object.assign(this.metadata.tilemap, {
                tileset:dom.tileset.value
            }),
            tileset:Object.assign(this.metadata.tileset, {
                tilewidth:tilewidth,
                tileheight:tileheight
            })
        };

        this.dirty();

        this.layout();
    }

    dirty() {
        this.#fs.writeFile(this.path + ".meta", JSON.stringify(this.metadata, 0, 4));
    }
};

module.exports.ImageEditor = ImageEditor;
