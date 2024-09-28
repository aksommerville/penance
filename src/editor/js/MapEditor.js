/* MapEditor.js
 * Top level of our custom map editor.
 */
 
import { Dom } from "./Dom.js";
import { Data } from "./Data.js";
import { MapRes } from "./MapRes.js";
import { MapToolbarUi } from "./MapToolbarUi.js";
import { MapCanvasUi } from "./MapCanvasUi.js";
import { MapBus } from "./MapBus.js";
import { MapPainter } from "./MapPainter.js";
import { MapStore } from "./MapStore.js";

export class MapEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Data, MapBus, MapPainter, MapStore, Window];
  }
  constructor(element, dom, data, mapBus, mapPainter, mapStore, window) {
    this.element = element;
    this.dom = dom;
    this.data = data;
    this.mapBus = mapBus;
    this.mapPainter = mapPainter;
    this.mapStore = mapStore;
    this.window = window;
    
    this.loc = null; // From MapStore: {plane,x,y,res,map}
    this.map = null;
    this.res = null;
    this.toolbar = null;
    this.canvas = null;
    
    this.mapBusListener = this.mapBus.listen(e => this.onBusEvent(e));
  }
  
  static checkResource({path,type,rid,serial}) {
    if (type === "map") return 2;
    return 0;
  }
  
  onRemoveFromDom() {
    this.mapBus.unlisten(this.mapBusListener);
  }
  
  setup(res) {
    this.res = res;
    this.maybeReady();
  }
  
  maybeReady() {
    if (!this.res) return;
    this.loc = this.mapStore.findPath(this.res.path);
    if (!this.loc) throw new Error(`Map ${this.res.path} not found in MapStore`);
    this.map = this.loc.map;
    this.mapBus.setLoc(this.loc);
    this.mapPainter.reset(this.map);
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.toolbar = this.dom.spawnController(this.element, MapToolbarUi);
    this.canvas = this.dom.spawnController(this.element, MapCanvasUi);
    this.toolbar.setMap(this.map);
  }
  
  onOpen(rid) {
    if (!rid) return;
    const loc = this.mapStore.entryByRid(rid);
    if (!loc) return;
    this.window.location = "#" + loc.res.path;
  }
  
  onBusEvent(e) {
    switch (e.type) {
      case "dirty": if (this.map) this.data.dirty(this.res.path, () => this.map.encode()); break;
      case "open": this.onOpen(e.rid); break;
    }
  }
}
