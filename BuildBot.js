const http = require('http');
const fs = require('fs');
const {PNGFS} = require('./ide/pngfs.js');
const builders = require('./builders.js');
const express = require('express');
const {JSC} = loadESM('./JSC/jsc.min.js');
const port = 8081;
const app = express();
const {cwd} = require('process');
const admin = require("firebase-admin");
const serviceAccount = require("./microjsServiceAccountCred.json");
const settings = require("./settings.json");

admin.initializeApp({
    credential: admin.credential.cert(serviceAccount),
    storageBucket: settings.storageBucket,
    databaseURL: settings.databaseURL
});

// As an admin, the app has access to read and write all data, regardless of Security Rules
const db = admin.database();
const storage = admin.storage().bucket();
let cacheBuster = 0;
const queue = [];

db.ref('/request').on('child_added', (snapshot) => {
    const cmd = snapshot.val();
    const projectId = snapshot.key;
    queue.push({cmd, projectId});
    if (queue.length == 1)
        pop();
});

async function pop() {
    if (!queue.length)
        return;

    const {cmd, projectId} = queue.shift();

    (new Promise((resolve, fail) => {
        db.ref(`/project/${projectId}/fs`).once('value', snapshot => {
            try {
                const fs = snapshot.val();
                if (!fs || typeof fs != 'object' || fs.pngfs != 1) {
                    fail("Got invalid fs for " + projectId);
                    return;
                }

                const {cpp, platform, compiler} = js2cpp(fs);
                if (!(platform in builders)) {
                    fail('Invalid platform ' + platform);
                    return;
                }

                let builder = new builders[platform](cpp, compiler);
                const ext = builder.ext();
                builder.say();
                builder.run()
                    .then(output => storage.upload(output, {destination: `${platform}/${projectId}.${ext}`}))
                    .then(_ => {
                        resolve({
                            url:`https://storage.googleapis.com/storage/v1/b/microjs-7d00a.appspot.com/o/${platform}%2F${projectId}.${ext}?alt=media&nc=${++cacheBuster}`
                        });
                    })
                    .catch(ex => fail(ex));
            } catch(ex) {
                fail(ex);
            }
        });
    }))
        .then(result => {
            db.ref(`/request/${projectId}`).remove();
            db.ref(`/build/${projectId}`).set(result);
            pop();
        })
        .catch(error => {
            console.error(projectId, error);
            if (typeof error == 'object') {
                if ("stderr" in error)
                    error = error.stderr;
                else if ('code' in error)
                    error = error.code;
                else
                    error = error + '';
            }
            db.ref(`/request/${projectId}`).remove();
            db.ref(`/build/${projectId}`).set({error});
            pop();
        });
}

function js2cpp(json) {
    const fs = new PNGFS(typeof json == 'string' ? JSON.parse(json) : json);
    const compiler = new JSC(path => fs.readFile(path));
    compiler.include('/std.js');
    fs.ls('/.R/')?.forEach?.(entry => {
        if (entry.isFile())
            compiler.include('/.R/' + entry.name);
    });
    compiler.process();
    cpp = compiler.write('cpp');
    platform = compiler.getOpt('platform');
    return {cpp, platform, compiler};
}


function loadESM(path) {
    const src = fs.readFileSync(path, 'utf-8');
    const entry = parseInt(src.substr(src.lastIndexOf('[') + 1));
    return new Function('return ' + src)()(entry);
}
