"setOpt platform pokitto";
"addSysCall setScreenMode setPen setFont setLED setTexture setMirrored setFlipped";
"addSysCall getWidth getHeight";
"addSysCall clear image text";

let c = 0;
let sw;
let sh;
let w = getWidth(R.asset_no_image);
let h = getHeight(R.asset_no_image);
const bounce = new Array(5);

class Bounce {
    constructor(i) {
        this.x = i * 23;
        this.y = i * 5;
        this.vx = (i - 2.5) * 0.7;
        this.vy = 0;
        setTexture(R.asset_no_image);
    }

    update() {
        this.x += this.vx;
        this.y += this.vy;

        if ((this.x + w > sw && this.vx > 0) || (this.x < 0 && this.vx < 0)) {
            this.vx = -this.vx;
        }

        if ((this.y + h > sh && this.vy > 0) || (this.y < 0 && this.vy < 0)) {
            this.vy = -this.vy;
            if (this.y + h > sh)
                this.y = sh - h;
            setLED(c * 13, c * 81, c * 991);
        }

        this.vy += 0.13;
    }

    render() {
        setMirrored(this.vx < 0);
        setFlipped(this.vy < 0);
        image(this.x, this.y);
    }
}

function init() {
    sw = getWidth();
    sh = getHeight();
    for (let i = 0, max = bounce.length; i < max; ++i)
        bounce[i] = new Bounce(i);
}

function update(tick) {
    for (let b of bounce)
        b.update();
}

function render() {
    for (let b of bounce)
        b.render();
    setPen();
    text("Hello Pokitto!", 5, 4);
}