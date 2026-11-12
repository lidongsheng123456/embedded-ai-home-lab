/** 这里只组装不同空间的家具，摆放细节分别由客厅、卧室和湿区模块管理。 */
import {createLiving} from './home-living.js';
import {createBedroom} from './home-bedroom.js';
import {createWetRooms} from './home-wet-rooms.js';

export function createFurniture(room, materials) {
  createLiving(room, materials);
  createBedroom(room, materials);
  createWetRooms(room, materials);
}
