import { initializeApp } from "firebase/app";
import { getDatabase, push, set, update, ref, child, get, onValue, off } from "firebase/database";

class Remote {
    #db;
    #listen;

    constructor() {
    }

    async requestKey() {
        return push(child(ref(this.#db), 'project')).key;
    }

    async saveProject(key, data) {
        await set(ref(this.#db, 'project/' + key), data);
    }

    async loadProject(key) {
        const snapshot = await get(ref(this.#db, 'project/' + key));
        return snapshot.exists() ? snapshot.val() : undefined;
    }

    async requestBuild(key, project) {
        await update(ref(this.#db), {
            [`/project/${key}`]: project,
            [`/request/${key}`]: 'build',
            [`/build/${key}`]: null
        });
        return new Promise((ok, fail) => {
            const listen = ref(this.#db, `/build/${key}`);
            onValue(listen, snapshot => {
                if (!snapshot.exists())
                    return;
                off(listen);
                ok(snapshot.val());
            });
        });
    }

    boot() {
        const FBAPP = initializeApp({
            apiKey: "AIzaSyDsjCMDUl0e50zO_L_g7gWJyfLENuMv2a8",
            authDomain: "microjs-7d00a.firebaseapp.com",
            projectId: "microjs-7d00a",
            storageBucket: "microjs-7d00a.appspot.com",
            messagingSenderId: "850079858570",
            appId: "1:850079858570:web:f71216680a1a6c624c1273",
            databaseURL: "https://microjs-7d00a-default-rtdb.europe-west1.firebasedatabase.app"
        });
        this.#db = getDatabase(FBAPP);
    }
}

module.exports.Remote = Remote;
