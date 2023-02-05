const {JSC} = require('../JSC/jsc.js');

function buildHTML(code, [width, height]) {
    return `<!doctype html>
<html>
<head>
<style>
html, body, canvas {
width: 100%;
height: 100%;
position: absolute;
overflow: hidden;
margin: 0;
border: 0;
padding: 0;
left: 0;
right: 0;
top: 0;
bottom: 0;
}
canvas {
margin: auto;
image-rendering: pixelated;
}
</style>
<script src="assets.js"></script>
<script src="browser.js?nc=6"></script>
<script>
${code}
</script>
</head>
<body>
<canvas id=canvas width=${width} height=${height}></canvas>
</body>
</html>`
}

async function build(fs, size) {
    const compiler = new JSC(path => fs.readFile(path));
    compiler.include('/std.js');
    fs.ls('/.R/')?.forEach?.(entry => {
        if (entry.isFile())
            compiler.include('/.R/' + entry.name);
    });
    compiler.process();
    return buildHTML(compiler.write('js'), size);
}

module.exports.build = build;
