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

function convertToU8(img, settings){
    settings = settings || {bpp:8, header:1, isTransparent:1};
    let transparentIndex = settings.transparent|0;
    // let palette = settings.palette;
    let out = [];
    let bpp = (settings.bpp|0) || (Math.log(palette.length) / Math.log(2))|0;
    if (settings.header|0) {
        if (img.width < 256 && img.height < 256) {
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
                                  + (B-PB)*(B-PB)
                            ;

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

async function prebuild(fs) {
    let promises = [];
    let outfs;
    fs.recurse(entity => {
        if (!entity.isFile())
            return entity.name[0] != '.';
        let extension = entity.extension;
        if (extension == 'jpg' || extension == 'png') {
            init().push(convertImage.call(this, entity.name, entity.node.data));
        }
        return false;
    });

    if (!promises.length)
        return fs;

    await Promise.all(promises);

    return outfs;

    async function convertImage(name, data) {
        return new Promise((resolve, fail) => {
            const img = new Image();
            img.onload = _=>{
                const canvas = dom('canvas', {width:img.width, height:img.height});
                const ctx = canvas.getContext('2d');
                ctx.drawImage(img, 0, 0);
                const data = ctx.getImageData(0, 0, img.width, img.height);
                const arrays = convertToU8(data, {bpp:8, header:1, isTransparent:1});
                let out = '';
                for (let array of arrays) {
                    for (let v of array) {
                            out += v.toString(16).padStart(2, '0');
                    }
                }
                const outName = `/.R/${name.split('.')[0]}.raw`;
                outfs.writeFile(outName, out);
                resolve();
            }
            img.onerror = _=>{
                fail('Could not load image ' + name);
            };
            img.src = data;
        });
    }

    function init() {
        if (!outfs) {
            outfs = new PNGFS(fs.toJSON());
            outfs.mkdir('/.R');
        }
        return promises;
    }
}

module.exports.PreBuild = prebuild;
module.exports.palette = palette;
module.exports.convertToU8 = convertToU8;
