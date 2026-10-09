// canvasUtil.ets — Canvas 通用工具（从 MathField 拆出，五十二包：供
// RatexView 等新组件复用，避免 views 内部循环依赖）。
import display from '@ohos.display';

// 屏幕密度（px/vp，如 3x 设备 = 3.0）：Canvas 坐标与 measureText 为 vp
// 口径、唯独 font-size 为物理 px（第十五包③四轮证据链终案）——字号
// ×k 换算、几何与宽度 vp 原值直用。第十七包②起 CanvasView 键盘避让的
// Area.globalPosition（px）换算也复用本函数
export function canvasDensity(): number {
  try {
    return display.getDefaultDisplaySync().densityPixels;
  } catch (e) {
    return 3;
  }
}
