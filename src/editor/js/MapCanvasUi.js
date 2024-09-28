/* MapCanvasUi.js
 * Main section of editor.
 */
 
import { Dom } from "./Dom.js";
import { CommandModal } from "./CommandModal.js";
import { MapBus } from "./MapBus.js";
import { Data } from "./Data.js";
import { MapRes } from "./MapRes.js";
import { MapStore } from "./MapStore.js";

const RENDER_DEBOUNCE_TIME = 50;

export class MapCanvasUi {
  static getDependencies() {
    return [HTMLCanvasElement, Dom, Window, MapBus, Data, MapStore];
  }
  constructor(element, dom, window, mapBus, data, mapStore) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.mapBus = mapBus;
    this.data = data;
    this.mapStore = mapStore;
    
    this.map = null;
    this.loc = null;
    this.ctx = this.element.getContext("2d");
    this.ctx.imageSmoothingEnabled = false;
    this.renderTimeout = null;
    this.geometry = { // Gets rewritten at each render, where we drew things.
      x0: 0, // Left edge of main output.
      y0: 0,
      w: 0, // Extent of main map's image.
      h: 0,
      colw: 0, // Distance between columns, including spacing if present.
      rowh: 0,
    };
    this.neighbors = []; // [nw,n,ne,w,null,e,sw,s,se]: {path,rid,image}
    this.entrances = []; // From MapStore: {srcrid,srcx,srcy,dstrid,dstx,dsty}, we are "dst"
    this.mapcmdicon = {}; // key: command keyword, value: Image or null
    
    this.mapBusListener = this.mapBus.listen(e => this.onBusEvent(e));
    
    this.resizeObserver = new this.window.ResizeObserver(e => this.renderSoon());
    this.resizeObserver.observe(this.element);
    
    this.motionListener = e => this.onMotion(e);
    this.window.addEventListener("mousemove", this.motionListener);
    this.element.addEventListener("mousedown", e => this.onMouseDown(e));
    this.mouseUpListener = null; // attached to window
    
    this.onLocChanged(this.mapBus.loc);
  }
  
  onRemoveFromDom() {
    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
      this.resizeObserver = null;
    }
    this.mapBus.unlisten(this.mapBusListener);
    if (this.motionListener) {
      this.window.removeEventListener("mousemove", this.motionListener);
      this.motionListener = null;
    }
    if (this.mouseUpListener) {
      this.window.removeEventListener("mouseup", this.mouseUpListener);
      this.mouseUpListener = null;
    }
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, RENDER_DEBOUNCE_TIME);
  }
  
  renderNow() {
    const bounds = this.element.getBoundingClientRect();
    this.element.width = bounds.width;
    this.element.height = bounds.height;
    this.ctx.clearRect(0, 0, bounds.width, bounds.height);
    if (this.map) this.renderMap();
  }
  
  renderMap() {
    let colw = 16;
    let rowh = colw;
    const xscale = Math.floor((this.element.width * 0.75) / (colw * this.map.w));
    const yscale = Math.floor((this.element.height * 0.75) / (rowh * this.map.h));
    const scale = Math.max(1, Math.min(xscale, yscale));
    colw *= scale;
    rowh *= scale;
    let colstep = colw;
    let rowstep = rowh;
    if (this.mapBus.visibility.grid) {
      colstep++;
      rowstep++;
    }
    const dstw = colstep * this.map.w;
    const dsth = rowstep * this.map.h;
    const dstx = (this.element.width >> 1) - (dstw >> 1);
    const dsty = (this.element.height >> 1) - (dsth >> 1);
    this.geometry.x0 = dstx;
    this.geometry.y0 = dsty;
    this.geometry.w = dstw;
    this.geometry.h = dsth;
    this.geometry.colw = colstep;
    this.geometry.rowh = rowstep;
    let drawColorCells = true;
    if (this.mapBus.visibility.image) {
      const imageName = this.map.getCommandByKeyword("image") || "";
      const image = this.data.getImage(imageName);
      if (image) {
        if (image.complete) {
          this.ctx.imageSmoothingEnabled = false;
          const srctilesize = Math.floor(image.naturalWidth / 16);
          for (let vp=0, y=dsty, yi=this.map.h; yi-->0; y+=rowstep) {
            for (let x=dstx, xi=this.map.w; xi-->0; x+=colstep, vp++) {
              const srcx = (this.map.v[vp] & 15) * srctilesize;
              const srcy = (this.map.v[vp] >> 4) * srctilesize;
              this.ctx.drawImage(image, srcx, srcy, srctilesize, srctilesize, x, y, colw, rowh);
            }
          }
          drawColorCells = false;
        } else {
          image.addEventListener("load", () => {
            this.renderSoon();
          });
        }
      }
    }
    if (drawColorCells) {
      for (let vp=0, y=dsty, yi=this.map.h; yi-->0; y+=rowstep) {
        for (let x=dstx, xi=this.map.w; xi-->0; x+=colstep, vp++) {
          this.ctx.fillStyle = MapCanvasUi.ctab[this.map.v[vp]];
          this.ctx.fillRect(x, y, colw, rowh);
        }
      }
    }
    if (this.mapBus.visibility.neighbors) {
      const margin = 10;
      for (let ny=-1, np=0; ny<=1; ny++) {
        for (let nx=-1; nx<=1; nx++, np++) {
          if (!this.neighbors[np]) continue;
          const srcbits = this.neighbors[np].image;
          if (!srcbits) continue;
          let ndstx = dstx + nx * (dstw + margin);
          let ndsty = dsty + ny * (dsth + margin);
          this.ctx.drawImage(srcbits, 0, 0, srcbits.width, srcbits.height, ndstx, ndsty, dstw, dsth);
        }
      }
    }
    if (this.mapBus.visibility.regions) {
      this.renderRegions();
    }
    if (this.mapBus.visibility.points) {
      this.renderPoints();
    }
  }
  
  renderRegions() {
    this.ctx.fillStyle = "#f80";
    this.ctx.globalAlpha = 0.5;
    const margin = this.geometry.colw >> 2;
    for (const command of this.map.commands) {
      const c = this.map.parseRegionCommand(command);
      if (!c) continue;
      const dstx = this.geometry.x0 + this.geometry.colw * c.x + margin;
      const dsty = this.geometry.y0 + this.geometry.rowh * c.y + margin;
      const dstw = this.geometry.colw * c.w - (margin << 1);
      const dsth = this.geometry.rowh * c.h - (margin << 1);
      this.ctx.fillRect(dstx, dsty, dstw, dsth);
    }
    this.ctx.globalAlpha = 1;
  }
  
  renderPoints() {
    const countById = {};
    const aoff = Math.round(this.geometry.colw * 0.333);
    const boff = Math.round(this.geometry.colw * 0.666);
    const ro = Math.max(4, this.geometry.colw * 0.125);
    const ri = ro - 1;
    for (const command of this.map.commands) {
      const c = this.map.parsePointCommand(command);
      if (!c) continue;
      // We'll allow four positions within each cell for POI handles. Above four, they'll occlude each other.
      const id = c.x + "," + c.y;
      let subp = countById[id];
      if (!subp) {
        subp = 0;
        countById[id] = 1;
      } else {
        countById[id]++;
      }
      subp &= 3;
      let dstx = this.geometry.x0 + this.geometry.colw * c.x;
      let dsty = this.geometry.y0 + this.geometry.rowh * c.y;
      if (subp & 1) dstx += boff; else dstx += aoff;
      if (subp & 2) dsty += boff; else dsty += aoff;
      const srcbits = this.getIconForPointCommand(c.kw);
      if (srcbits) {
        this.ctx.drawImage(srcbits, 0, 0, srcbits.naturalWidth, srcbits.naturalHeight, dstx - 8, dsty - 8, 16, 16);
      } else {
        this.ctx.beginPath();
        this.ctx.ellipse(dstx, dsty, ro, ro, 0, 0, Math.PI * 2);
        this.ctx.fillStyle = "#fff";
        this.ctx.fill();
        this.ctx.beginPath();
        this.ctx.ellipse(dstx, dsty, ri, ri, 0, 0, Math.PI * 2);
        const opcode = this.evalOpcode(c.kw);
        this.ctx.fillStyle = MapCanvasUi.ctab[opcode];
        this.ctx.fill();
      }
    }
    // Entrances render just like point commands, user doesn't need to care about the difference.
    // Only render if the icon is ready, because I'm lazy.
    if (this.entrances.length > 0) {
      const srcbits = this.getIconForPointCommand("door-dst");
      if (srcbits) {
        for (const entrance of this.entrances) {
          const id = entrance.dstx + "," + entrance.dsty;
          let subp = countById[id];
          if (!subp) {
            subp = 0;
            countById[id] = 1;
          } else {
            countById[id]++;
          }
          subp &= 3;
          let dstx = this.geometry.x0 + this.geometry.colw * entrance.dstx;
          let dsty = this.geometry.y0 + this.geometry.rowh * entrance.dsty;
          if (subp & 1) dstx += boff; else dstx += aoff;
          if (subp & 2) dsty += boff; else dsty += aoff;
          this.ctx.drawImage(srcbits, 0, 0, srcbits.naturalWidth, srcbits.naturalHeight, dstx - 8, dsty - 8, 16, 16);
        }
      }
    }
  }
  
  getIconForPointCommand(kw) {
    let image = this.mapcmdicon[kw];
    if (image === undefined) {
      if (!isNaN(+kw)) return null; // Don't try to look up numeric keywords.
      image = new Image();
      this.mapcmdicon[kw] = image;
      image.addEventListener("load", () => {
        this.renderSoon();
      });
      image.addEventListener("error", () => {
        this.mapcmdicon[kw] = null;
      });
      image.src = `/mapcmdicon/${kw}.png`;
    } else if (image?.complete) {
      return image;
    }
    return null;
  }
  
  redrawNeighbors() {
    if (!this.mapBus.visibility.neighbors || !this.map) {
      this.neighbors = [];
      return;
    }
    this.neighbors = [null, null, null, null, null, null, null, null, null];
    for (let np=0, dy=-1; dy<=1; dy++) {
      for (let dx=-1; dx<=1; dx++, np++) {
        if (!dx && !dy) continue;
        const nei = this.mapStore.entryByCoords(this.loc.x + dx, this.loc.y + dy);
        if (!nei) continue;
        this.neighbors[np] = {
          path: nei.res.path,
          rid: nei.res.rid,
          image: this.drawMapImage(nei.map, 0.5, np),
        };
      }
    }
  }
  
  // Return a new canvas with an exact-size image of the given map.
  // (fadeOut) in 0..1 to blend it gray a little.
  drawMapImage(map, fadeOut, neighborsp) {
    const tilesheet = this.data.getImage(map.getCommandByKeyword("image"));
    const tilesize = tilesheet ? (tilesheet.naturalWidth >> 4) : 16;
    const canvas = document.createElement("CANVAS");
    if (tilesheet && !tilesheet.complete) {
      tilesheet.addEventListener("load", () => {
        this.neighbors[neighborsp].image = canvas;
        this.drawMapImageNow(map, fadeOut, tilesheet, tilesheet.naturalWidth >> 4, canvas);
        this.renderSoon();
      }, { once: true });
      return null;
    }
    this.drawMapImageNow(map, fadeOut, tilesheet, tilesize, canvas);
    return canvas;
  }
  
  drawMapImageNow(map, fadeOut, tilesheet, tilesize, canvas) {
    canvas.width = map.w * tilesize;
    canvas.height = map.h * tilesize;
    const ctx = canvas.getContext("2d");
    for (let y=0, vp=0, yi=map.h; yi-->0; y+=tilesize) {
      for (let x=0, xi=map.w; xi-->0; x+=tilesize, vp++) {
        if (tilesheet) {
          const srcx = (map.v[vp] & 0xf) * tilesize;
          const srcy = (map.v[vp] >> 4) * tilesize;
          ctx.drawImage(tilesheet, srcx, srcy, tilesize, tilesize, x, y, tilesize, tilesize);
        } else {
          ctx.fillStyle = MapCanvasUi.ctab[map.v[vp]];
          ctx.fillRect(x, y, tilesize, tilesize);
        }
      }
    }
    if (fadeOut) {
      ctx.globalAlpha = 0.5;
      ctx.fillStyle = "#444";
      ctx.fillRect(0, 0, canvas.width, canvas.height);
    }
  }
  
  evalOpcode(src) {
    let v = +src;
    if (!isNaN(v)) return v;
    for (const [opcode, name] of CommandModal.mapcmd) {
      if (name === src) return opcode;
    }
    return 0;
  }
  
  /* For (x,y) straight off a mouse event, return:
   * {
   *   col, row: number; Cell coordinates in main map, can be OOB.
   *   which: string;    "" for main, or "nw","n","ne","w","e","sw","s","se","oob", which map.
   *   x, y: number;     Coords in our canvas.
   *   subx, suby: number; (0..1) mouse position in cell (eg POI selection needs this)
   * }
   * (which) will be a neighbor indicator even if no neighbor present there, we don't check.
   * If neighbor visibility is off, (which) will only be "" or "oob".
   */
  assessMousePosition(x, y) {
    const bounds = this.element.getBoundingClientRect();
    x -= bounds.x;
    y -= bounds.y;
    const rx = (x - this.geometry.x0) / this.geometry.colw;
    const ry = (y - this.geometry.y0) / this.geometry.rowh;
    const col = Math.floor(rx);
    const row = Math.floor(ry);
    const subx = rx % 1;
    const suby = ry % 1;
    let which = "";
    if (!this.map) {
      which = "oob";
    } else if (this.mapBus.visibility.neighbors) {
      if (col < 0) {
        if (row < 0) which = "nw";
        else if (row >= this.map.h) which = "sw";
        else which = "w";
      } else if (col >= this.map.w) {
        if (row < 0) which = "ne";
        else if (row >= this.map.h) which = "se";
        else which = "e";
      } else {
        if (row < 0) which = "n";
        else if (row >= this.map.h) which = "s";
      }
    } else if ((col < 0) || (col >= this.map.w) || (row < 0) || (row >= this.map.h)) {
      which = "oob";
    }
    return { x, y, col, row, which, subx, suby };
  }
  
  onMotion(e) {
    const loc = this.assessMousePosition(e.x, e.y);
    this.mapBus.setMouse(loc.col, loc.row, loc.subx, loc.suby);
  }
  
  clickNeighbor(p, dx, dy) {
    const neighbor = this.neighbors[p];
    if (neighbor) {
      const path = neighbor.path;
      this.window.location = "#" + path;
      return;
    }
    const nx = this.loc.x + dx;
    const ny = this.loc.y + dy;
    if ((nx < 0) || (ny < 0) || (nx >= MapRes.LONG_LIMIT) || (ny >= MapRes.LAT_LIMIT)) {
      this.dom.modalMessage(`New map coords out of bounds. (${nx},${ny}), limit (0,0)..(${MapRes.LONG_LIMIT-1},${MapRes.LAT_LIMIT}-1)`);
      return;
    }
    const proposeId = this.mapStore.unusedId();
    this.dom.modalInput(`'ID' or 'ID-NAME' for new map at (${dx},${dy}) to current:`, proposeId.toString()).then(basename => {
      if (!basename) throw null;
      const entry = this.mapStore.createMap(nx, ny, this.map, basename);
      if (!entry) return;
      this.window.location = "#" + entry.res.path;
    }).catch(e => e ? console.error(e) : null);
  }
  
  onMouseDown(e) {
    const loc = this.assessMousePosition(e.x, e.y);
    switch (loc.which) {
      case "nw": this.clickNeighbor(0, -1, -1); break;
      case "n":  this.clickNeighbor(1,  0, -1); break;
      case "ne": this.clickNeighbor(2,  1, -1); break;
      case "w":  this.clickNeighbor(3, -1,  0); break;
      case "e":  this.clickNeighbor(5,  1,  0); break;
      case "sw": this.clickNeighbor(6, -1,  1); break;
      case "s":  this.clickNeighbor(7,  0,  1); break;
      case "se": this.clickNeighbor(8,  1,  1); break;
      case "": {
          this.mapBus.setMouse(loc.col, loc.row, loc.subx, loc.suby);
          this.mapBus.beginPaint();
          if (!this.mouseUpListener) {
            this.mouseUpListener = e => this.onMouseUp(e);
            this.window.addEventListener("mouseup", this.mouseUpListener);
          }
        } break;
    }
  }
  
  onMouseUp(e) {
    this.mapBus.endPaint();
    this.window.removeEventListener("mouseup", this.mouseUpListener);
    this.mouseUpListener = null;
  }
  
  onLocChanged() {
    this.loc = this.mapBus.loc;
    this.map = this.loc.map;
    this.redrawNeighbors();
    this.renderSoon();
  }
  
  onBusEvent(e) {
    switch (e.type) {
      case "visibility": this.redrawNeighbors(); this.renderSoon(); break;
      case "dirty": this.renderSoon(); break;
      case "commandsChanged": this.redrawNeighbors(); this.renderSoon(); break;
      case "loc": this.onLocChanged(); break;
      case "render": this.renderSoon(); break;
    }
  }
}

MapCanvasUi.ctab = [
  "#000000","#800000","#008000","#808000","#000080","#800080","#008080","#c0c0c0",
  "#808080","#ff0000","#00ff00","#ffff00","#0000ff","#ff00ff","#00ffff","#ffffff",
  "#000000","#00005f","#000087","#0000af","#0000d7","#0000ff","#005f00","#005f5f",
  "#005f87","#005faf","#005fd7","#005fff","#008700","#00875f","#008787","#0087af",
  "#0087d7","#0087ff","#00af00","#00af5f","#00af87","#00afaf","#00afd7","#00afff",
  "#00d700","#00d75f","#00d787","#00d7af","#00d7d7","#00d7ff","#00ff00","#00ff5f",
  "#00ff87","#00ffaf","#00ffd7","#00ffff","#5f0000","#5f005f","#5f0087","#5f00af",
  "#5f00d7","#5f00ff","#5f5f00","#5f5f5f","#5f5f87","#5f5faf","#5f5fd7","#5f5fff",
  "#5f8700","#5f875f","#5f8787","#5f87af","#5f87d7","#5f87ff","#5faf00","#5faf5f",
  "#5faf87","#5fafaf","#5fafd7","#5fafff","#5fd700","#5fd75f","#5fd787","#5fd7af",
  "#5fd7d7","#5fd7ff","#5fff00","#5fff5f","#5fff87","#5fffaf","#5fffd7","#5fffff",
  "#870000","#87005f","#870087","#8700af","#8700d7","#8700ff","#875f00","#875f5f",
  "#875f87","#875faf","#875fd7","#875fff","#878700","#87875f","#878787","#8787af",
  "#8787d7","#8787ff","#87af00","#87af5f","#87af87","#87afaf","#87afd7","#87afff",
  "#87d700","#87d75f","#87d787","#87d7af","#87d7d7","#87d7ff","#87ff00","#87ff5f",
  "#87ff87","#87ffaf","#87ffd7","#87ffff","#af0000","#af005f","#af0087","#af00af",
  "#af00d7","#af00ff","#af5f00","#af5f5f","#af5f87","#af5faf","#af5fd7","#af5fff",
  "#af8700","#af875f","#af8787","#af87af","#af87d7","#af87ff","#afaf00","#afaf5f",
  "#afaf87","#afafaf","#afafd7","#afafff","#afd700","#afd75f","#afd787","#afd7af",
  "#afd7d7","#afd7ff","#afff00","#afff5f","#afff87","#afffaf","#afffd7","#afffff",
  "#d70000","#d7005f","#d70087","#d700af","#d700d7","#d700ff","#d75f00","#d75f5f",
  "#d75f87","#d75faf","#d75fd7","#d75fff","#d78700","#d7875f","#d78787","#d787af",
  "#d787d7","#d787ff","#d7af00","#d7af5f","#d7af87","#d7afaf","#d7afd7","#d7afff",
  "#d7d700","#d7d75f","#d7d787","#d7d7af","#d7d7d7","#d7d7ff","#d7ff00","#d7ff5f",
  "#d7ff87","#d7ffaf","#d7ffd7","#d7ffff","#ff0000","#ff005f","#ff0087","#ff00af",
  "#ff00d7","#ff00ff","#ff5f00","#ff5f5f","#ff5f87","#ff5faf","#ff5fd7","#ff5fff",
  "#ff8700","#ff875f","#ff8787","#ff87af","#ff87d7","#ff87ff","#ffaf00","#ffaf5f",
  "#ffaf87","#ffafaf","#ffafd7","#ffafff","#ffd700","#ffd75f","#ffd787","#ffd7af",
  "#ffd7d7","#ffd7ff","#ffff00","#ffff5f","#ffff87","#ffffaf","#ffffd7","#ffffff",
  "#080808","#121212","#1c1c1c","#262626","#303030","#3a3a3a","#444444","#4e4e4e",
  "#585858","#626262","#6c6c6c","#767676","#808080","#8a8a8a","#949494","#9e9e9e",
  "#a8a8a8","#b2b2b2","#bcbcbc","#c6c6c6","#d0d0d0","#dadada","#e4e4e4","#eeeeee",
];
