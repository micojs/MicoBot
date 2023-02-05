const {dom, index} = require('./dom.js');

class Flasher {
    constructor() {}

    log(...args) {
        postMessage({log:[args]}, '*')
    }

    async showConnectPopup() {
        return new Promise((resolve, fail) => {
            const root = dom(document.body,
                             {
                                 style: {
                                     position: "absolute",
                                     width: "100%",
                                     height: "100%",
                                     margin:0,
                                     padding:0,
                                 }
                             }, [
                                 ['div', {
                                     style:{
                                         position: "absolute",
                                         width: "210px",
                                         height: "100px",
                                         margin: "auto",
                                         padding: "18px",
                                         boxSizing: "border-box",
                                         left: 0,
                                         right: 0,
                                         top: 0,
                                         bottom: 0,
                                         background: '#333',
                                         borderRadius: '10px',
                                         border: '1px solid #555'
                                     },
                                 }, [
                                     ['h1', {
                                         textContent:'Ready!',
                                         style:{
                                             textAlign:'center',
                                             fontFamily:'Menlo, Monaco, "Courier New", monospace',
                                             color:'white'
                                         }
                                     }],
                                     ['div', [
                                         ['button', {
                                             textContent:'Cancel',
                                             onclick:_=>{
                                                 root.remove();
                                                 fail();
                                             }
                                         }],
                                         ['button', {
                                             textContent:'Flash',
                                             onclick:_=>{
                                                 root.remove();
                                                 resolve();
                                             }

                                         }]
                                     ]]
                                 ]]
                             ]);
        });
    }

};

module.exports.Flasher = Flasher;
