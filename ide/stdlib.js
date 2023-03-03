module.exports.stdlib = `

/**
 * Prints messages into the console for debugging
 */
declare function debug(...args):void;

/**
 * Returns the current timestamp in milliseconds
 */
declare function getTime():number;

/**
 * Returns a floating-point value between 0 (inclusive) and 1 (exclusive)
 */
declare function rand():number;

/**
 * Returns an integer value between 0 (inclusive) and max (exclusive)
 */
declare function rand(max:number):number;

/**
 * Returns a floating-point value between min (inclusive) and max (exclusive)
 */
declare function rand(min:number, max:number):number;

/**
 * Returns an integer value between min (inclusive) and max (exclusive)
 */
declare function rand(min:number, max:number, ignored:any):number;

/**
 * Returns the absolute value of value
 */
declare function abs(value:number):number;

/**
 * Returns value rounded down to the nearest integer
 */
declare function floor(value:number):number;

/**
 * Returns value rounded to the nearest integer
 */
declare function round(value:number):number;

/**
 * Returns value rounded up to the nearest integer
 */
declare function ceil(value:number):number;

/**
 * Returns the square root of value
 */
declare function sqrt(value:number):number;

/**
 * Returns the cosine of angle, given in radians
 */
declare function cos(angle:number):number;

/**
 * Returns the sine of angle, given in radians
 */
declare function sin(angle:number):number;

/**
 * Returns the arc tangent of y/x
 */
declare function atan2(y:number, x:number):number;

/**
 * Returns the arc tangent of angle, given in radians
 */
declare function tan(angle:number):number;


/**
 * Returns the lowest of the given values
 */
declare function min(...value):number;

/**
 * Returns the highest of the given values
 */
declare function max(...value):number;

/**
 * Returns the root of the sum of each argument squared
 */
declare function vectorLength(...args):number;

/**
 * Returns the difference between two angles given in radians
 */
declare function angleDifference(angleA, angleB):number

/**
 * Sets the target framerate that the game should update at.
 */
declare function setFPS(fps:number):void;

/**
 * Sets current tilemap. See wiki for further info.
 */
declare function setTileMap(map:TileMapResource):void;

/**
 * Sets the current drawing color.
 * Returns a color.
 */
declare function setPen(R:number, G:number, B:number):number;

/**
 * Sets the current drawing color.
 * Returns a color.
 */
declare function setPen(color:number):number;

/**
 * Sets the font used by the text() function.
 */
declare function setFont(font:FontResource):void;

/**
 * Sets the LED color on supported hardware.
 */
declare function setLED(R:number, G:number, B:number):void;

/**
 * Fills a rectangle on the screen using the current pen color.
 */
declare function rect(x:number, y:number, width:number, height:number):void;

/**
 * Sets the texture to be used by the image() function.
 */
declare function setTexture(texture:ImageResource):void;

/**
 * Sets if the following image() calls should be flipped horizontally.
 */
declare function setMirrored(mirror:boolean):void;

/**
 * Sets if the following image() calls should be flipped vertically.
 */
declare function setFlipped(flip:boolean):void;

/**
 * Sets if the following image() calls should support transparency.
 */
declare function setTransparent(transparent:boolean):void;

/**
 * Returns the screen's width in pixels.
 */
declare function getWidth():number;

/**
 * Returns the screen's height in pixels.
 */
declare function getHeight():number;

/**
 * Reads a byte from a resource at the specified offset.
 */
declare function readByte(resource:Resource, offset?:number):number;

/**
 * Returns the image's width in pixels.
 */
declare function getWidth(image:ImageResource):number;

/**
 * Returns the image's height in pixels.
 */
declare function getHeight(image:ImageResource):number;

/**
 * Fills the entire screen with the current pen's color.
 */
declare function clear():void;

/**
 * Draws the active texture onto the screen at position 0, 0.
 */
declare function image():void;

/**
 * Draws texture onto the screen at position 0, 0.
 */
declare function image(texture:ImageResource):void;

/**
 * Draws the active texture onto the screen at position X, Y.
 */
declare function image(X:number, Y:number):void;

/**
 * Draws texture onto the screen at position X, Y.
 */
declare function image(texture:ImageResource, X:number, Y:number, rotation?:number, scale?:number):void;

/**
 * Draws text onto the screen.
 */
declare function text(message:string, x?:number, y?:number):void;

/**
 * True while button is pressed, false otherwise.
 */
var UP:boolean;

/**
 * True while button is pressed, false otherwise.
 */
var DOWN:boolean;

/**
 * True while button is pressed, false otherwise.
 */
var LEFT:boolean;

/**
 * True while button is pressed, false otherwise.
 */
var RIGHT:boolean;

/**
 * True while button is pressed, false otherwise.
 */
var A:boolean;

/**
 * True while button is pressed, false otherwise.
 */
var B:boolean;

/**
 * True while button is pressed, false otherwise.
 */
var C:boolean;

/**
 * True while button is pressed, false otherwise.
 */
var D:boolean;

`;
