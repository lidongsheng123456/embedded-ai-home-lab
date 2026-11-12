/** 视频中的一室一厅一厨一卫一阳台。单位为建模米，尺寸按画面比例估计，未作测量。 */
export const HOME_LAYOUT = {
  bounds: {width: 7, depth: 6.9, center: [-0.1, 0.05]},
  wallHeight: 2.6,
  cutawayHeight: 0.68,
  // 客厅范围包含厨房和入户通道；厨房不借用左侧卧室、卫生间的面积。
  // 坐标方向只服务于建模，不代表真实东南西北。
  connections: [['living', 'balcony'], ['living', 'bedroom'], ['living', 'kitchen'], ['bedroom', 'bathroom']],
  doors: {
    // 两扇门都沿横向墙面布置，厨房推拉门与入户门平行，不开在左侧隔墙上。
    entry: {center: [2.93, 3.54], width: 0.82},
    kitchen: {center: [1.2, 1.6], width: 1.65},
  },
  rooms: [
    {id: 'living', name: '客厅', center: [1.7, 0.8], size: [3.4, 5.4], color: '#e9dfca',
      // 地砖分两块铺，给内部厨房留出位置，避免地面重叠闪烁。
      floorSections: [{center: [1.7, -0.15], size: [3.4, 3.5]}, {center: [2.9, 2.55], size: [1, 1.9]}]},
    {id: 'balcony', name: '阳台', center: [1.7, -2.65], size: [3.4, 1.5], color: '#dce4dc'},
    {id: 'bedroom', name: '卧室', center: [-1.8, -1.35], size: [3.6, 4.1], color: '#e8d7c6'},
    {id: 'kitchen', name: '厨房', center: [1.2, 2.55], size: [2.4, 1.9], color: '#d8e0cf'},
    {id: 'bathroom', name: '卫生间', center: [-3, 2.1], size: [1.2, 2.8], color: '#d8e3e5'},
  ],
  // 节点放在视频里的客厅桌面上，位置仅为预设，未声称设备已经安装。
  device: [1.42, 1.02, -0.45],
  presenceMarker: [2.18, 0.055, 0.45],
  views: {
    overview: [[11, 10, 14], [-0.1, 0.25, 0.05]],
    top: [[-0.1, 17, 0.2], [-0.1, 0, 0.05]],
    detail: [[5, 4.2, 5], [1.42, 0.65, -0.45]],
  },
};

export const homeRoom = id => HOME_LAYOUT.rooms.find(room => room.id === id);
