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

// 公式字号宽窗补偿（六十六包①）：vp 密度无关，同 vp 在手机与 PC 物理
// 尺寸相同，但 PC/平板观看距离约为手机两倍，公式显小——58 包的 1.1 视
// 觉补偿按手机标定（真机 57 包反馈），开鸿PC 实机反馈 PC 上仍偏小。按
// 十七包①口径不按设备类型、只看窗口实际宽度：复用 ui.isWide（≥640vp
// 断点，CanvasView 根 onAreaChange 发布），宽窗公式字号 ×1.35；窄窗
//（含 PC 锁手机比例）与手机完全一致。RaTeX 全链路（静态行 RatexView、
// 编辑行 MathField）与输入行占位符同用本系数；旧 painter 回退路径与图
// 形区标签不变
export const MATH_WIDE_FONT_SCALE: number = 1.35;

export function mathFontScale(isWide: boolean): number {
  return isWide ? MATH_WIDE_FONT_SCALE : 1;
}
