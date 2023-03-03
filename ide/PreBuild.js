const {PNGFS} = require('./pngfs.js');
const {dom} = require('./dom.js');

const palette = [
    [255, 0, 255],
    [0, 0, 0],
    [15, 15, 15],
    [30, 30, 30],
    [61, 59, 0],
    [0, 2, 61],
    [45, 45, 45],
    [22, 68, 32],
    [32, 22, 68],
    [68, 32, 22],
    [91, 65, 0],
    [0, 91, 65],
    [65, 0, 91],
    [63, 63, 63],
    [42, 66, 84],
    [84, 42, 66],
    [66, 84, 42],
    [25, 105, 21],
    [21, 93, 105],
    [51, 21, 105],
    [105, 21, 59],
    [105, 84, 21],
    [127, 59, 0],
    [4, 127, 0],
    [0, 67, 127],
    [123, 0, 127],
    [79, 79, 79],
    [84, 59, 98],
    [98, 84, 59],
    [59, 98, 84],
    [39, 105, 118],
    [68, 39, 118],
    [118, 39, 73],
    [118, 100, 39],
    [42, 118, 39],
    [57, 138, 19],
    [19, 138, 84],
    [19, 90, 138],
    [51, 19, 138],
    [138, 19, 124],
    [138, 19, 23],
    [138, 118, 19],
    [158, 31, 0],
    [94, 158, 0],
    [0, 158, 94],
    [0, 31, 158],
    [158, 0, 158],
    [94, 94, 94],
    [113, 75, 79],
    [79, 113, 75],
    [75, 79, 113],
    [86, 56, 132],
    [132, 56, 86],
    [132, 116, 56],
    [56, 132, 56],
    [56, 116, 132],
    [37, 150, 141],
    [37, 64, 150],
    [109, 37, 150],
    [150, 37, 96],
    [150, 77, 37],
    [128, 150, 37],
    [37, 150, 45],
    [104, 169, 18],
    [18, 169, 33],
    [18, 169, 134],
    [18, 104, 169],
    [33, 18, 169],
    [134, 18, 169],
    [169, 18, 104],
    [169, 33, 18],
    [169, 134, 18],
    [188, 0, 9],
    [103, 188, 0],
    [0, 188, 179],
    [84, 0, 188],
    [109, 109, 109],
    [126, 131, 87],
    [87, 126, 131],
    [131, 87, 126],
    [153, 65, 97],
    [153, 138, 65],
    [65, 153, 68],
    [65, 133, 153],
    [103, 65, 153],
    [63, 43, 175],
    [175, 43, 175],
    [175, 43, 61],
    [175, 138, 43],
    [100, 175, 43],
    [43, 175, 100],
    [43, 138, 175],
    [21, 197, 138],
    [21, 138, 197],
    [21, 21, 197],
    [138, 21, 197],
    [197, 21, 138],
    [197, 21, 21],
    [197, 138, 21],
    [138, 197, 21],
    [21, 197, 21],
    [179, 219, 0],
    [32, 219, 0],
    [0, 219, 113],
    [0, 179, 219],
    [0, 32, 219],
    [113, 0, 219],
    [219, 0, 179],
    [219, 0, 32],
    [219, 113, 0],
    [127, 127, 127],
    [107, 147, 121],
    [121, 107, 147],
    [147, 121, 107],
    [169, 158, 85],
    [85, 169, 91],
    [85, 147, 169],
    [124, 85, 169],
    [169, 85, 113],
    [191, 63, 142],
    [191, 93, 63],
    [180, 191, 63],
    [70, 191, 63],
    [63, 191, 165],
    [63, 106, 191],
    [129, 63, 191],
    [43, 60, 211],
    [138, 43, 211],
    [211, 43, 172],
    [211, 43, 60],
    [211, 138, 43],
    [172, 211, 43],
    [60, 211, 43],
    [43, 211, 138],
    [43, 172, 211],
    [21, 233, 109],
    [21, 233, 222],
    [21, 127, 233],
    [32, 21, 233],
    [145, 21, 233],
    [233, 21, 205],
    [233, 21, 88],
    [233, 71, 21],
    [233, 183, 21],
    [166, 233, 21],
    [49, 233, 21],
    [255, 238, 0],
    [142, 142, 142],
    [120, 139, 165],
    [165, 120, 139],
    [139, 165, 120],
    [97, 187, 105],
    [97, 162, 187],
    [141, 97, 187],
    [187, 97, 126],
    [187, 177, 97],
    [210, 156, 75],
    [147, 210, 75],
    [75, 210, 118],
    [75, 185, 210],
    [79, 75, 210],
    [194, 75, 210],
    [210, 75, 109],
    [232, 53, 211],
    [232, 53, 91],
    [232, 133, 53],
    [211, 232, 53],
    [91, 232, 53],
    [53, 232, 133],
    [53, 211, 232],
    [53, 91, 232],
    [133, 53, 232],
    [30, 112, 255],
    [97, 30, 255],
    [247, 30, 255],
    [255, 30, 112],
    [255, 97, 30],
    [255, 247, 30],
    [112, 255, 30],
    [30, 255, 97],
    [30, 255, 247],
    [158, 158, 158],
    [169, 138, 177],
    [177, 169, 138],
    [138, 177, 169],
    [119, 172, 196],
    [159, 119, 196],
    [196, 119, 141],
    [196, 190, 119],
    [119, 196, 128],
    [119, 216, 99],
    [99, 216, 179],
    [99, 152, 216],
    [146, 99, 216],
    [216, 99, 185],
    [216, 113, 99],
    [216, 214, 99],
    [235, 134, 80],
    [233, 235, 80],
    [129, 235, 80],
    [80, 235, 134],
    [80, 233, 235],
    [80, 129, 235],
    [134, 80, 235],
    [235, 80, 233],
    [235, 80, 129],
    [229, 61, 255],
    [255, 132, 61],
    [87, 255, 61],
    [61, 183, 255],
    [173, 173, 173],
    [193, 154, 153],
    [153, 193, 154],
    [154, 153, 193],
    [177, 132, 214],
    [214, 132, 153],
    [214, 210, 132],
    [132, 214, 144],
    [132, 185, 214],
    [112, 226, 234],
    [112, 122, 234],
    [208, 112, 234],
    [234, 112, 157],
    [234, 171, 112],
    [191, 234, 112],
    [112, 234, 136],
    [159, 255, 91],
    [91, 255, 219],
    [91, 94, 255],
    [255, 91, 225],
    [255, 154, 91],
    [191, 191, 191],
    [201, 212, 170],
    [170, 201, 212],
    [212, 170, 201],
    [233, 149, 167],
    [233, 231, 149],
    [149, 233, 164],
    [149, 201, 233],
    [198, 149, 233],
    [165, 127, 255],
    [255, 127, 153],
    [216, 255, 127],
    [127, 255, 229],
    [206, 206, 206],
    [182, 230, 205],
    [205, 182, 230],
    [230, 205, 182],
    [255, 255, 158],
    [158, 255, 255],
    [255, 158, 255],
    [221, 221, 221],
    [188, 207, 255],
    [255, 236, 188],
    [237, 237, 237],
    [255, 255, 255]
];

function getImageHeaderSize(img) {
    if (img.width < 256 && img.height < 256)
        return 4;
    return 7;
}

function convertToU8(img, settings){
    settings = settings || {bpp:8, header:1, isTransparent:1};
    let transparentIndex = settings.transparent|0;
    // let palette = settings.palette;
    let out = [];
    let bpp = (settings.bpp|0) || (Math.log(palette.length) / Math.log(2))|0;
    if (settings.header|0) {
        if (getImageHeaderSize(img) === 4) {
            out.push([img.width, img.height, bpp, 0]);
        } else {
            out.push([
                0,
                (img.width >> 8) & 0xFF, img.width & 0xFF,
                (img.height >> 8) & 0xFF, img.height & 0xFF,
                bpp,
                0
            ]);
        }
    }
    let i=0, len, bytes, data = img.data;
    let ppb = 8 / bpp;
    let run = [],
        min = settings.paloffset|0,
        max = Math.min(palette.length, min+(1<<bpp));
    let lowest = 255;
    let highest = 0;

    let transparent = settings.isTransparent;

    if (transparent === undefined){
        for( i=3; !transparent && i<data.length; i+=4 ){
            transparent = data[i] < 128;
        }
    } else transparent = transparent|0;
    settings.isTransparent = transparent;

    i=0;
    let PC = undefined, PCC = 0;

    for( let y=0; y<img.height; ++y ){

        run = [];

        for( let x=0; x<img.width; ++x ){
            let closest = 0;
            let closestDist = Number.POSITIVE_INFINITY;
            let R = data[i++]|0;
            let G = data[i++]|0;
            let B = data[i++]|0;
            let A = data[i++]|0;
            if(bpp == 16) {
                let C = (R>>3<<11) | (G>>2<<5) | (B>>3);
                run.push(C&0xFF, C>>8);
            } else if(bpp == 1) {
                if (transparent) {
                    closest = A > 128;
                } else {
                    closest = (R + G + B) / 3 > 128;
                }

                run[x>>3|0] = (run[x>>3]||0) + (closest<<(7 - (x&7)));
            } else {
                let C = (R<<16) + (G<<8) + B;
                if( A > 128 || !transparent ) {
                    if(C === PC){
                        closest = PCC;
                    } else {

                        for( let c=min; c<max; ++c ){
                            if( transparent && c == transparentIndex )
                                continue;
                            const ca = palette[c];
                            const PR = ca[0]|0;
                            const PG = ca[1]|0;
                            const PB = ca[2]|0;
		            const dist = (R-PR)*(R-PR)
                                  + (G-PG)*(G-PG)
                                  + (B-PB)*(B-PB);

                            if( dist < closestDist ){
                                closest = c;
                                closestDist = dist;
                            }
                        }

                        PC = C;
                        PCC = closest;

                    }

                }else{
                    closest = transparentIndex;
                }

                if (closest < lowest)
                    lowest = closest;
                if (closest > highest)
                    highest = closest;

                let shift = (ppb - 1 - x%ppb) * bpp;
                run[(x/ppb)|0] = (run[(x/ppb)|0]||0) + ((closest-min)<<shift);
            }
        }

        out.push(run);
    }

    let range = (highest - lowest) + 1;


    return out;
}

function bytesToStr(arr) {
    let out = '';
    if (!Array.isArray(arr))
        throw "Expected array of bytes";
    for (let data of arr) {
        if (typeof data === 'number') {
            data = data >>> 0;
            do {
                const v = data & 0xFF;
                data >>>= 8;
                out += v.toString(16).padStart(2, '0');
            } while (data > 0);
        } else {
            out += bytesToStr(data);
        }
    }
    return out;
}

function normalizeBytes(arr) {
    let out = [];
    if (!Array.isArray(arr))
        throw "Expected array of bytes";
    for (let data of arr) {
        if (typeof data === 'number') {
            data = data >>> 0;
            do {
                const v = data & 0xFF;
                data >>>= 8;
                out.push(v);
            } while (data > 0);
        } else {
            for (let b of normalizeBytes(data))
                out.push(b);
        }
    }
    return out;
}

function bytesToLen(arr) {
    let out = 0;
    if (!Array.isArray(arr))
        return 0;
    for (let data of arr) {
        if (typeof data === 'number') {
            data = data >>> 0;
            do {
                const v = data & 0xFF;
                data >>>= 8;
                out += 2;
            } while (data > 0);
        } else {
            out += bytesToLen(data);
        }
    }
    return out;
}

function u32(x) {
    return [
        (x) & 0xFF,
        (x >>> 8) & 0xFF,
        (x >>> 16) & 0xFF,
        (x >>> 24) & 0xFF
    ];
}

function u16(x) {
    return [
        (x) & 0xFF,
        (x >>> 8) & 0xFF
    ];
}

async function loadImage(data) {
    return new Promise((resolve, fail) => {
        const img = new Image();
        img.onload = _=>{
            const canvas = dom('canvas', {width:img.width, height:img.height});
            const ctx = canvas.getContext('2d');
            ctx.drawImage(img, 0, 0);
            resolve(ctx.getImageData(0, 0, img.width, img.height));
        };
        img.onerror = _=>{
            fail('Could not load image ' + name);
        };
        img.src = data;
    });
}

async function convertImage(name, data, outfs, tileTable) {
    const recname = name.split('.')[0];
    const imgdata = await loadImage(data);
    tileTable.setTileImage(name, imgdata);
    const out = bytesToStr(convertToU8(imgdata, {bpp:8, header:1, isTransparent:1}));
    const outName = `/.R/${recname}.raw`;
    outfs.writeFile(outName, out);
}

function getTile(map, id) {
    id &= 0xFFFFFF;

    if (!id)
        return null;

    if (!map._cache)
        map._cache = {};
    const cache = map._cache;

    if (id in cache)
        return cache[id];

    let tilesets = map.tilesets;
    let set = null;
    for (let candidate of tilesets) {
        if (candidate.firstgid <= id && (candidate.firstgid + candidate.tilecount) > id) {
            set = candidate;
            break;
        }
    }

    if (!set) {
        cache[id] = null;
        return null;
    }

    let index = (id|0) - (set.firstgid|0);
    let x = index % set.columns;
    let y = (index / set.columns) | 0;
    let properties = null;

    if (set.tiles) {
        for (let entry of set.tiles) {
            if (entry.id != index)
                continue;
            properties = Object.assign(Object.create(null), entry);
            delete properties.id;

            for (let prop in properties) {
                let value = prop.value;

                if (typeof value == 'string')
                    value = {h:value};
                else
                    value |= 0;

                if (!value)
                    delete properties[prop];
                else
                    properties[prop] = value;
            }

            if (entry.properties) {
                delete properties.properties;
                for (let prop of entry.properties) {
                    let value = prop.value;

                    if (typeof value == "string")
                        value = {h:value};
                    else
                        value |= 0;

                    if (value)
                        properties[prop.name] = value;
                }
            }

            break;
        }
    }

    const tile = {
        image:set.image,
        properties,
        offsetX:x * set.tilewidth,
        offsetY:y * set.tileheight
    };

    cache[id] = tile;

    return tile;
}

function convertTileLayer(layer, data, tileTable) {
    const out = [0];
    for (let cell of layer.data) {
        let tile = getTile(data, cell);
        if (!tile) {
            out.push(0);
            continue;
        }
        let id = tileTable.getId(cell, tile, false, false);
        out.push(id);
    }
    return out;
}

function convertObjectLayer(layer, data, tileTable) {
    const out = [1];
    return out;
}

async function convertTMJ(name, datasrc, outfs, tileTable) {
    const data = JSON.parse(datasrc);

    const header = [
        u16(0),                                   // map resource indicator
        u32(0),                                   // tile set offset
        data.layers.length & 0xFF,                // layer count
        u16(data.width), u16(data.height),        // map width, height
        u16(data.tilewidth), u16(data.tileheight) // tile width, height
    ];

    const layers = [];
    const tiles = [];
    const out = [header, layers, tiles];
    const outName = `/.R/${name.split('.')[0]}.u8`;

    for (let layer of data.layers) {
        if (layer.type == "tilelayer") {
            layers.push(convertTileLayer(layer, data, tileTable));
        } else if (layer.type == "objectgroup") {
            layers.push(convertObjectLayer(layer, data, tileTable));
        }
    }

    const tileStart = bytesToLen(out);
    // header[1] = u32(tileStart);
    tileTable.tileStart = tileStart;

    // outfs.writeFile(outName, bytesToStr(out));
    const normalized = normalizeBytes(out);
    normalized.splice(2, 4, {r:'TileTable'});

    outfs.writeFile(outName, JSON.stringify(normalized));
}

class TileTable {
    index = [];
    cache = {};
    tileStart = 0;

    images = Object.create(null);

    setTileImage(name, data) {
        this.images[name] = data;
    }

    serialize(out) {
        let tiles = [];
        let props = [];
        out.push(tiles, props);
    }

    write(fs) {
        const tiles = [];
        const props = [];
        let propind = this.index.length * 2;

        for (let entry of this.index) {
            let img = this.images[entry.tile.image];
            if (!img)
                throw "Could not find " + entry.tile.image + " for tileset";

            const tile = entry.tile;
            let propcount = tile.properties ? Object.keys(tile.properties).length : 0;

            tiles.push(
                {r:tile.image, o:getImageHeaderSize(img) + tile.offsetY * img.width + tile.offsetX},
                (propcount ? propind : 0) | (img.width << 16)
            );

            if (tile.properties) {
                props.push(propcount); propind++;
                for (let key in tile.properties) {
                    props.push({h:key}, tile.properties[key]);
                    propind += 2;
                }
            }
        }

        const out = [...tiles, ...props];
        fs.writeFile('/.R/TileTable.u32', JSON.stringify(out));
    }

    getTile(id) {
        return this.index[id - 1];
    }

    getId(gid, tile, mirror, flip) {
        if (gid in this.cache)
            return this.cache[gid];

        let key = JSON.stringify([tile, mirror, flip]);
        for (let i = 0; i < this.index.length; ++i) {
            const entry = this.index[i];
            if (entry.key == key) {
                this.cache[gid] = i + 1;
                return i + 1;
            }
        }

        this.index.push({
            key,
            tile,
            mirror, flip
        });

        this.cache[gid] = this.index.length;
        return this.cache[gid];
    }
}

async function prebuild(fs) {
    let promises = [];
    let outfs;
    let tileTable = new TileTable();

    fs.recurse(entity => {
        if (!entity.isFile())
            return entity.name[0] != '.';
        let extension = entity.extension;
        if (extension == 'jpg' || extension == 'png') {
            init().push(convertImage.call(this, entity.name, entity.node.data, outfs, tileTable));
        }
        if (extension == 'tmj') {
            init().push(convertTMJ.call(this, entity.name, entity.node.data, outfs, tileTable));
        }
        return false;
    });

    if (!promises.length)
        return fs;

    await Promise.all(promises);

    tileTable.write(outfs);

    return outfs;

    function init() {
        if (!outfs) {
            outfs = new PNGFS(fs.toJSON());
            outfs.mkdir('/.R');
            outfs.mkdir('/.SD');
        }
        return promises;
    }
}

module.exports.PreBuild = prebuild;
module.exports.palette = palette;
module.exports.convertToU8 = convertToU8;
