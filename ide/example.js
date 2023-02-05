module.exports.loadExample = function loadExample(fs) {
    fs.mkdir('/images');

    fs.writeFile('/images/wheel.png', "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAOpJREFUOI1jZMAN/qPxGbEpwirIwMDwf5uJE4qA15l9WNVjMwBDMz5D0A3AqRmXIcgGENSMzRAmonTgASzoAp7LvsLZ26O4GRgYGBjUfHzgYre2bEFRj+KCyKxPqIad3svgeXoviliWcjhWA/7zzzNhYGBgYBA48henc12uQ1ykED6TgQGaThiRNSODD4mnUfiKEbMw1DxYmY4ZBjBQduwiTpcgA4pjAZYOsHojTX0OAwMDA8PqSScx5B6sTGdgYGBghLmA8WPSGaJthWlGdgEMoLgEZig01DE0YzOAgQE1GzPiEWNgYGBgAABEAkFa5rPi7QAAAABJRU5ErkJggg==");

    fs.mkdir('/sounds');
    fs.mkdir('/source');

    fs.writeFile('/source/main.js', `
"push title Game Title";
"push author Your Name";
"push description A MicoJS game";
"set version v0.0.1";
"set category game";
"set url https://micojs.github.com";

const bgColor = setPen(0, 0, 0);
const txtColor = setPen(64, 128, 255);

let screenWidth, screenHeight;

class Wheel {
    init() {
        this.x = screenWidth/2;
        this.y = screenHeight/2;
        this.speedX = rand(-3, 3);
        this.speedY = rand(-3, 3);
        this.color = 0;
    }

    update() {
        if (A)
            this.color++;
        if (B)
            this.color = 0;

        this.x += this.speedX;
        this.y += this.speedY;

        let bounce = false;
        while (!this.speedX || (this.x > screenWidth && this.speedX > 0) || (this.x < 0 && this.speedX < 0)) {
            this.speedX = rand(-5, 5);
            bounce = true;
        }
        while (!this.speedY || (this.y > screenHeight && this.speedY > 0) || (this.y < 0 && this.speedY < 0)) {
            this.speedY = rand(-5, 5);
            bounce = true;
        }
        if (bounce)
            debug("Bounce");
    }

    render() {
        setMirrored(this.speedX < 0);
        setFlipped(this.speedY < 0);
        setPen(this.color);
        image(R.wheel, this.x, this.y);
    }
}

const wheel = new Wheel();

function init() {
    screenWidth = getWidth();
    screenHeight = getHeight();
    wheel.init();
}

function update(time) {
    wheel.update();
}

function render() {
    setPen(bgColor);
    clear();
    wheel.render();
    setPen(txtColor);
    text("Hello world!", 5, 5);
}
`);
};
