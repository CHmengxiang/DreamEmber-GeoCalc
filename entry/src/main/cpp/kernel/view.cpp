// view.cpp — 坐标系统实现
#include "kernel/view.h"

namespace dreamember {

void EuclidianView::setCanvasSize(int w, int h, double uiScale)
{
    int nw = w > 0 ? w : 1;
    int nh = h > 0 ? h : 1;
    if (uiScale > 0.0) this->uiScale = uiScale;
    if (!sizedOnce_) {
        // 首次知道画布尺寸：原点居中（上游 的 getXZeroStandard/.getYZeroStandard）
        width = nw;
        height = nh;
        xZero = width / 2.0;
        yZero = height / 2.0;
        sizedOnce_ = true;
        return;
    }
    if (nw != width) {
        // 二十五包⑦：宽屏(PC UI)启动画布先按全宽居中，左侧视图栏+停靠面板
        // 随后加载把画布推窄——xZero 钉在旧像素中心，原点就被推离画布中心
        // （真机反馈⑦，须手动重置视图）。宽度变化时保持「视野水平中心的
        // 世界点」不动（xZero += Δw/2，上游 桌面拖分栏时视野居中不动同观感，
        // 启动逐帧累积恰好把原点送回新中心）。高度有意不补：窄屏抽屉把手
        // 拖高逐帧改高度，补了内容会跟着把手滑。
        xZero += (nw - width) / 2.0;
    }
    width = nw;
    height = nh;
}

void EuclidianView::zoomAt(double anchorPx, double anchorPy, double factor)
{
    if (!(factor > 0.0) || factor == 1.0) return;
    double ns = xscale * factor;
    if (ns < 1e-9 || ns > 1e9) return;   // 实用缩放界限（上游 允许 1e-15..1e15，触控场景用不到）
    xZero = anchorPx + factor * (xZero - anchorPx);
    yZero = anchorPy + factor * (yZero - anchorPy);
    xscale = ns;
    yscale *= factor;
}

void EuclidianView::reset()
{
    xscale = yscale = 50.0;
    xZero = width / 2.0;
    yZero = height / 2.0;
}

} // namespace dreamember
