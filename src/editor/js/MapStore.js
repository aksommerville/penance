/* MapStore.js
 * Global singleton that manages all the map resources.
 * Maps are laid out in a larger plane, unrelated to their rid.
 * Each map must contain "location LONG LAT" in 0..255, and each must be unique.
 * The map editor should not interact with Data directly (for map resources), should all go through MapStore.
 */
 
import { Data } from "./Data.js";
import { MapBus } from "./MapBus.js";
import { MapRes } from "./MapRes.js";

export class MapStore {
  static getDependencies() {
    return [Data, MapBus];
  }
  constructor(data, mapBus) {
    this.data = data;
    this.mapBus = mapBus;
    
    this.loaded = false;
    this.plane = []; // {res,map}, 1k slots and very sparse. Indexed by (lat*MapRes.LONG_LIMIT)+long
    for (let i=MapRes.LAT_LIMIT*MapRes.LONG_LIMIT; i-->0; ) this.plane.push(null);
    this.resv = []; // Another copy of all the resources, we need it during construction.
  }
  
  require() {
    if (this.loaded) return;
    this.loaded = true;
    this.plane = [];
    this.resv = [];
    for (const res of this.data.resv) {
      if (res.type !== "map") continue;
      if (!res.rid) throw new Error(`map resources must have an explicit id in 1..65535 (${JSON.stringify(res.path)})`);
      if (this.resv.indexOf(res) >= 0) continue; // Already visited via neighbor link or something.
      this.addMap(null, null, res, null);
    }
  }
  
  mapByPath(path) {
    return this.entryByPath(path)?.map;
  }
  
  resByPath(path) {
    // We happen to have a flat list of resource objects -- more efficient than searching for live maps.
    return this.resv.find(r => r.path === path);
  }
  
  // => {res,map} or null
  entryByPath(path) {
    return this.plane.find(e => e?.res.path === path);
  }
  
  // => {x,y,res,map}
  findPath(path) {
    this.require();
    const p = this.plane.findIndex(e => e?.res.path === path);
    if (p < 0) return null;
    const { res, map } = this.plane[p];
    const x = p % MapRes.LONG_LIMIT;
    const y = Math.floor(p / MapRes.LAT_LIMIT);
    return { x, y, res, map };
  }
  
  // => {res,map} or null
  entryByRid(rid) {
    return this.plane.find(e => e?.res.rid === rid);
  }
  
  resByWhatever(whatever) {
    if (whatever?.startsWith?.("map:")) whatever = whatever.substring(4);
    if (!whatever) return null;
    const rid = +whatever;
    const slashWhatever = "/" + whatever;
    for (const res of this.resv) {
      if (res.rid === rid) return res;
      if (res.name === whatever) return res;
      if (res.path === whatever) return res;
      if (res.path.endsWith(slashWhatever)) return res;
    }
    return null;
  }
  
  entryByCoords(x, y) {
    if ((x < 0) || (y < 0) || (x >= MapRes.LONG_LIMIT) || (y >= MapRes.LAT_LIMIT)) return null;
    return this.plane[y * MapRes.LONG_LIMIT + x];
  }
  
  /* (ref) is a MapRes object to copy from. We'll automatically declare the same image, maybe other things.
   * The live state is fully updated synchronously, and we queue in Data for the remote write.
   */
  createMap(x, y, ref, basename) {
    if (!basename) basename = this.unusedId().toString();
    const path = "map/" + basename;
    if (this.resv.find(r => r.path === path)) throw new Error(`Map path ${JSON.stringify(path)} already in use.`);
    if ((x < 0) || (y < 0) || (x >= MapRes.LONG_LIMIT) || (y >= MapRes.LAT_LIMIT)) throw new Error(`Invalid map location ${x},${y}`);
    if (this.plane[y * MapRes.LONG_LIMIT + x]) throw new error(`Map location ${x},${y} already in use.`);
    
    // Create the new map object.
    let map;
    if (ref) {
      map = new MapRes(ref.w, ref.h);
      this.copyDefaultCommands(map, ref);
    } else {
      map = new MapRes();
    }
    map.replaceSingleCommand("location", `${x} ${y}`);
    
    // Tell Data about the new file, and acquire the resource record.
    const res = this.data.newResourceSync(path, map.encode());
    
    // Insert in plane and resv.
    const entry = this.addMap(x, y, res, map);
    
    return entry;
  }
  
  unusedId() {
    // No doubt there are better ways to do this but we'll be naive:
    // Build a Set of all rid, then iterate from 1 until we find something not in the Set.
    const all = new Set();
    for (const res of this.resv) all.add(res.rid);
    for (let rid=1; ; rid++) if (!all.has(rid)) return rid;
  }
  
  /* Everything below is private.
   ************************************************************************************/
   
  copyDefaultCommands(dst, src) {
    for (const command of src.commands) {
      if (command.startsWith("image ")) {
        dst.commands.push(command);
      }
    }
  }
  
  /* Put this map in our plane.
   * If (map) null, we decode it from (res.serial).
   * If (x,y) null, we read those from (map).
   * (x,y) must name a valid location and must not yet be in use.
   * (res) must not yet be in (this.resv) -- we'll put it there.
   * Returns the entry from (this.plane) on success: {res,map}
   */
  addMap(x, y, res, map) {
    if (!res) throw new Error(`Resource required`);
    if (!map) map = new MapRes(res.serial);
    if (x === null) {
      if (y !== null) throw new Error(`MapStore.addMap: x and y must both be null, or both valid`);
      const loc = map.getLocation();
      x = loc[0];
      y = loc[1];
    } else {
      const loc = map.getLocation(true);
      if (loc) {
        if ((x !== loc[0]) || (y !== loc[1])) throw new Error(`map:${res.rid} internally declares loc ${loc[0]},${loc[1]}, but you're adding at ${x},${y}`);
      } else {
        map.replaceSingleCommand("location", `${x} ${y}`);
      }
    }
    if (this.resv.indexOf(res) >= 0) throw new Error(`map:${res.rid} already included somewhere`);
    if ((x < 0) || (y < 0) || (x >= MapRes.LONG_LIMIT) || (y >= MapRes.LAT_LIMIT)) throw new Error(`Invalid map location ${x},${y}`);
    const p = y * MapRes.LONG_LIMIT + x;
    if (this.plane[p]) throw new Error(`Map location ${x},${y} already occupied by map:${this.plane[p].res.rid}, attempting to put map:${res.rid} there too`);
    const entry = { res, map };
    this.plane[p] = entry;
    this.resv.push(res);
    return entry;
  }
  
  parsePosition(src) {
    const match = src.match(/^@(\d+),(\d+)$/);
    if (!match) return [0, 0];
    return [+match[1] || 0, +match[2] || 0];
  }
}

MapStore.singleton = true;
