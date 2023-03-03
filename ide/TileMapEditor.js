const {dom, index} = require('./dom.js');
const {convertToU8, palette} = require('./PreBuild.js');

function log(...msgs) {
    postMessage({build:msgs}, '*');
}

function assert(check, ...msgs) {
    if (!check) {
        log(...msgs);
        return true;
    }
    return false;
}

class TileMapEditor {
    #fs;
    #dom;
    path;
    #map;
    canvas;
    ctx;
    imgData;
    forceRedraw;

    #cache = {};

    constructor(parentElement, path, map, fs) {
        this.path = path;
        this.#fs = fs;

        this.#dom = index(dom('div', parentElement, {className:'imageEditor'}, [
            ['div', {id:'imgcontainer'}, [
                ['canvas', {id:'canvas'}]
            ]]
            // ['div', {id:'sidebar'}, [
            //     ['select', {id:'mode', onchange:this.changeMeta.bind(this)}, [
            //         ['option', {value:'image', textContent:'image'}],
            //         ['option', {value:'tilemap', textContent:'tilemap'}],
            //         ['option', {value:'tileset', textContent:'tileset'}]
            //     ]],
            //     [{id:'imageSidebar'}, [
            //         [{className:'row'}, [
            //             ['span', {textContent:'Width:'}],
            //             ['span', {id:'imgwidth'}]
            //         ]],
            //         [{className:'row'}, [
            //             ['span', {textContent:'Height:'}],
            //             ['span', {id:'imgheight'}]
            //         ]]
            //     ]],
            //     [{id:'tilemapSidebar'}, [
            //         [{className:'row'}, [
            //             ['span', {textContent:'Tile Set:'}],
            //             ['select', {id:'tileset', onchange:this.changeMeta.bind(this)}, this.getTilesets().map(path => [
            //                 'option', {value:path, textContent:path.replace(/.*\/|\..*/g, '')}
            //             ])],
            //             ['button', {id:'refresh', onclick:this.refreshTileSets.bind(this), textContent:'⟳'}]
            //         ]]
            //     ]],
            //     [{id:'tilesetSidebar'}, [
            //         [{className:'row'}, [
            //             ['span', {textContent:'Tile Width:'}],
            //             ['input', {
            //                 id:'tilewidth', type:"number", min:"8", max:"256",
            //                 onchange:this.changeMeta.bind(this),
            //                 value:this.metadata.tileset.tilewidth|0||8
            //             }]
            //         ]],
            //         [{className:'row'}, [
            //             ['span', {textContent:'Tile Height:'}],
            //             ['input', {
            //                 id:'tileheight', type:"number", min:"8", max:"256",
            //                 onchange:this.changeMeta.bind(this),
            //                 value:this.metadata.tileset.tileheight|0||8
            //             }]
            //         ]]
            //     ]]
            // ]]
        ]), {}, this);

        this.load(map);
    }

    async loadImage(path) {
        if (!(path in this.#cache)) {
            this.#cache[path] = new Promise((onload, onerror) => {
                let pathparts = this.path.split('/');
                pathparts[pathparts.length - 1] = path;
                let fullpath = pathparts.join('/');
                log("Loading image ", fullpath);
                const imgSrc = this.#fs.readFile(fullpath);
                const el = dom('img', {src:imgSrc, onload:_=>{
                    const canvas = dom('canvas', {
                        width: el.naturalWidth,
                        height: el.naturalHeight
                    });
                    const ctx = canvas.getContext("2d");
                    ctx.drawImage(el, 0, 0);
                    onload(ctx.getImageData(0, 0, canvas.width, canvas.height));
                }, onerror});
            });
        }
        return await this.#cache[path];
    }

    commit() {
    }

    reload() {
        this.load(this.#fs.readFile(this.path));
    }

    load(map) {
        this.#map = null;
        try {
            this.#map = JSON.parse(map);
        } catch (ex) {
            log(ex + '');
        }
        this.layout();
    }

    clear() {
        const canvas = this.#dom.canvas;
        if (!this.ctx)
            this.ctx = canvas.getContext('2d');
    }

    async layout() {
        this.clear();
        let map = this.#map;

        if (assert(map, "No map data"))
            return;

        if (assert(map.compressionlevel == -1, "Invalid compression in map:", map.compressionlevel))
            return;

        if (assert(!map.infinite, "Infinite map not supported"))
            return;

        if (assert(map.renderorder == "right-down", "Render order " + map.renderorder + " not supported"))
            return;

        if (assert(map.orientation == "orthogonal", "Map orientation not orthogonal"))
            return;

        if (assert(map.layers?.length, "No layers in map"))
            return;

        const {width, height, tilewidth, tileheight, layers} = map;

        const canvas = this.#dom.canvas;

        if (canvas.width != width * tilewidth) {
            canvas.width = width * tilewidth;
            this.imgData = null;
        }

        if (canvas.height != height * tileheight) {
            canvas.height = height * tileheight;
            this.imgData = null;
        }

        if (!this.imgData)
            this.imgData = this.ctx.getImageData(0, 0, canvas.width, canvas.height);

        const data = this.imgData.data;
        data.fill(0xFF);

        for (const layer of layers) {
            if (layer.visible && layer.opacity > 0.5) {
                await this[layer.type]?.(layer, data);
            }
        }

        this.ctx.putImageData(this.imgData, 0, 0);
    }

    async getTile(id) {
        id &= 0xFFFFFF;

        if (!id)
            return null;

        let tilesets = this.#map.tilesets;
        let set = null;
        for (let candidate of tilesets) {
            if (candidate.firstgid <= id && (candidate.firstgid + candidate.tilecount) > id) {
                set = candidate;
                break;
            }
        }

        if (!set)
            return null;

        let image = await this.loadImage(set.image);
        let index = (id|0) - (set.firstgid|0);
        let x = index % set.columns;
        let y = (index / set.columns) | 0;
        return {
            image,
            offsetX:x * set.tilewidth,
            offsetY:y * set.tileheight
        };
    }

    async tilelayer(layer, out) {
        const out32 = new Uint32Array(out.buffer);
        const outStride = this.imgData.width;

        const {data, width, height} = layer;
        const {tilewidth, tileheight} = this.#map;
        for (let y = 0; y < height; ++y) {
            for (let x = 0; x < width; ++x) {
                const tile = await this.getTile(data[y * width + x]);
                if (!tile || !tile.image)
                    continue;

                const img32 = new Uint32Array(tile.image.data.buffer);
                const imgStride = tile.image.width;

                let {offsetX, offsetY} = tile;
                for (let py = 0; py < tileheight; ++py) {
                    const outOffset = (y * tileheight + py) * outStride + x * tilewidth;
                    const imgOffset = (offsetY + py) * imgStride + offsetX;
                    for (let px = 0; px < tilewidth; ++px) {
                        let c = img32[imgOffset + px];
                        if (c)
                            out32[outOffset + px] = c;
                    }
                }
            }
        }
    }

    // autozoom() {
    //     const img = this.#dom.img;
    //     const canvas = this.#dom.canvas;
    //     const parent = img.parentElement;
    //     const zoom = Math.min(parent.clientWidth/img.naturalWidth, parent.clientHeight/img.naturalHeight) * 0.85;
    //     const width = img.naturalWidth * zoom | 0;
    //     const height = img.naturalHeight * zoom | 0;
    //     canvas.style.width  = img.style.width  = width + "px";
    //     canvas.style.height = img.style.height = height + "px";
    //     canvas.style.left   = img.style.left   = ((parent.clientWidth/2 - width/2)|0) + "px";
    //     canvas.style.top    = img.style.top    = ((parent.clientHeight/2 - height/2)|0) + "px";
    // }

    // layoutImage() {
    //     this.autozoom();

    //     const img = this.#dom.img;
    //     this.#dom.imgwidth.textContent = img.naturalWidth;
    //     this.#dom.imgheight.textContent = img.naturalHeight;

    //     if (!this.initCanvas(img.naturalWidth, img.naturalHeight, img)) {
    //         return;
    //     }
    //     const u8 = convertToU8(this.imgData);
    //     const data = this.imgData.data;
    //     let i = 0;
    //     for (let y = 0; y < img.naturalHeight; ++y) {
    //         for (let x = 0; x < img.naturalWidth; ++x) {
    //             let color = palette[u8[y + 1][x]];
    //             data[i++] = color[0];
    //             data[i++] = color[1];
    //             data[i++] = color[2];
    //             data[i++] = 255;
    //         }
    //     }
    //     this.ctx.putImageData(this.imgData, 0, 0);
    // }


    async layoutTilemap() {
        // this.autozoom();

        // let forceRedraw = this.forceRedraw;
        // this.forceRedraw = false;

        // if (this.tilesetPath != this.metadata.tilemap.tileset) {
        //     this.tilesetData = null;
        //     this.tilesetImg = null;
        // }

        // if (!this.tilesetData) {
        //     this.tilesetData = JSON.parse(this.#fs.readFile(this.metadata.tilemap.tileset));
        //     this.tilesetPath = this.metadata.tilemap.tileset;
        //     forceRedraw = true;
        // }

        // if (!this.tilesetImg) {
        //     forceRedraw = true;
        //     let imgName = this.metadata.tilemap.tileset.replace(/\.meta$/gi, '');
        //     let imgSrc = this.#fs.readFile(imgName);
        //     await new Promise((onload, onerror) => {
        //         this.tilesetImg = dom('img', {src:imgSrc, onload, onerror});
        //     });
        // }

        // if (!forceRedraw)
        //     return;

        // const {tilewidth, tileheight, tilesPerRow, tilesPerCol} = this.tilesetDimensions();
        // const img = this.#dom.img;
        // this.initCanvas(img.naturalWidth, img.naturalHeight, img);

        // const canvas = this.#dom.canvas;
        // canvas.width = img.naturalWidth * tilewidth;
        // canvas.height = img.naturalHeight * tileheight;

        // let i = 0, ntid = 1;
        // let assign = {};
        // for (let y = 0; y < this.imgData.height; ++y) {
        //     for (let x = 0; x < this.imgData.width; ++x) {
        //         let c = 0;
        //         c = this.imgData.data[i++];
        //         c <<= 8;
        //         c |= this.imgData.data[i++];
        //         c <<= 8;
        //         c |= this.imgData.data[i++];

        //         let A = this.imgData.data[i++];
        //         if (A < 128)
        //             continue;

        //         c = c.toString(16);

        //         let tdat = this.metadata.tilemap['#' + c];
        //         if (!tdat || typeof tdat != 'object')
        //             this.metadata.tilemap['#' + c] = tdat = {};
        //         assign[c] = tdat;

        //         let tid = tdat.id;
        //         if (tid === undefined)
        //             continue;

        //         tid |= 0;
        //         let tx = tid % tilesPerRow;
        //         let ty = (tid / tilesPerRow) | 0;
        //         if (ty >= tilesPerCol) {
        //             tdat.id = 0;
        //             continue;
        //         }

        //         this.ctx.drawImage(
        //             this.tilesetImg,
        //             tx * tilewidth, ty * tileheight, tilewidth, tileheight,
        //             x * tilewidth, y * tilewidth, tilewidth, tileheight
        //         );
        //     }
        // }

        // let old = this.#dom.tilemapSidebar.querySelectorAll('.assign');
        // for (let i = 0; i < old.length; ++i)
        //     old[i].remove();

        // for (let key in assign) {
        //     const row = dom(this.#dom.tilemapSidebar, {className:'row assign'}, [
        //         [{style:{backgroundColor:'#' + '0'.repeat(6 - key.length) + key, width:'32px', flexGrow:0}}],
        //         ['canvas', {width:tilewidth, height:tileheight, flexGrow:0, onclick:this.changeTileId.bind(this, key, assign[key])}],
        //         ['input', {type:'number', value:assign[key].info|0, style:{flexGrow:1}, onchange:this.changeTileInfo.bind(this, key, assign[key])}]
        //     ]);
        //     const tid = assign[key].id;
        //     const canvas = row.querySelector('canvas');
        //     const ctx = canvas.getContext('2d');
        //     let tx = tid % tilesPerRow;
        //     let ty = (tid / tilesPerRow) | 0;
        //     ctx.drawImage(this.tilesetImg, tx * tilewidth, ty * tileheight, tilewidth, tileheight, 0, 0, tilewidth, tileheight);
        // }
    }

    dirty() {
    }
};

module.exports.TileMapEditor = TileMapEditor;
