/** ECharts按需引入折线、坐标轴和提示框；只展示本次页面会话，不伪造实机历史。 */
import * as echarts from 'echarts/core';
import { LineChart } from 'echarts/charts';
import { GridComponent, TooltipComponent } from 'echarts/components';
import { CanvasRenderer } from 'echarts/renderers';

echarts.use([LineChart, GridComponent, TooltipComponent, CanvasRenderer]);
const config = {
  temp: { name: '温度', unit: '°C', color: '#b88b62' },
  humidity: { name: '湿度', unit: '%RH', color: '#789ba4' },
  light: { name: '光照', unit: 'lux', color: '#94a572' },
  sound: { name: '声音', unit: '/100', color: '#a496ac' },
  gasIndex: { name: '气体指数', unit: '/100', color: '#ba9177' },
};

export function createTrends() {
  const chart = echarts.init(document.getElementById('trend-chart'), null, { renderer: 'canvas' });
  const history = [];
  let selected = 'temp';
  function render() {
    const item = config[selected];
    chart.setOption({
      animationDuration: 650,
      animation: !matchMedia('(prefers-reduced-motion: reduce)').matches,
      backgroundColor: 'transparent',
      grid: { top: 24, right: 16, bottom: 25, left: 37 },
      tooltip: { trigger: 'axis', backgroundColor: '#fffdf3f0', borderColor: '#d9dfcc',
        textStyle: { color: '#738361', fontSize: 12 }, valueFormatter: value => `${value ?? '--'} ${item.unit}` },
      xAxis: { type: 'category', boundaryGap: false, data: history.map(point => point.time),
        axisLine: { lineStyle: { color: '#e0e4d6' } }, axisTick: { show: false },
        axisLabel: { color: '#9aab8b', fontSize: 10, hideOverlap: true } },
      yAxis: { type: 'value', scale: true, splitNumber: 3,
        axisLabel: { color: '#9aab8b', fontSize: 10 },
        splitLine: { lineStyle: { color: '#aaba8e20', type: 'dashed' } } },
      series: [{ name: item.name, type: 'line', smooth: 0.35, showSymbol: history.length === 1, symbolSize: 5,
        data: history.map(point => Number.isFinite(point[selected]) ? Number(point[selected].toFixed(1)) : null),
        lineStyle: { color: item.color, width: 2, shadowColor: item.color + '55', shadowBlur: 8 },
        itemStyle: { color: item.color },
        areaStyle: { color: new echarts.graphic.LinearGradient(0, 0, 0, 1,
          [{ offset: 0, color: item.color + '30' }, { offset: 1, color: item.color + '00' }]) } }],
    });
  }
  document.querySelectorAll('[data-series]').forEach(button => {
    button.addEventListener('click', () => {
      selected = button.dataset.series;
      document.querySelectorAll('[data-series]').forEach(node => {
        node.classList.toggle('selected', node === button);
        node.setAttribute('aria-pressed', String(node === button));
      });
      render();
    });
  });
  const observer = new ResizeObserver(() => chart.resize());
  observer.observe(document.getElementById('trend-chart'));
  render();
  return {
    clear() { history.length = 0; render(); },
    add(state, time = new Date()) {
      history.push({ ...state, time: time.toLocaleTimeString('zh-CN', { hour12: false }) });
      if (history.length > 40) history.shift();
      render();
    },
    destroy() { observer.disconnect(); chart.dispose(); },
  };
}
