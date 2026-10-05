// view.h — EuclidianView 坐标系统（上游 同名类的 N1 子集）
#ifndef DREAMEMBER_KERNEL_VIEW_H
#define DREAMEMBER_KERNEL_VIEW_H

namespace dreamember {

// xZero/yZero：世界原点在画布上的像素位置；xscale/yscale：每世界单位的像素数。
// 像素系：左上为原点，y 向下。公式与 上游 一致：
//   px = xZero + wx*xscale ;  py = yZero - wy*yscale
// 缩放用 上游 CoordSystemAnimation 的闭式解（锚点像素处的世界点保持不动）：
//   xZero' = ax + f*(xZero - ax) ;  scale' = f*scale
class EuclidianView {
public:
    int width = 0, height = 0;
    double xZero = 0, yZero = 0;
    double xscale = 50, yscale = 50;   // 上游 SCALE_STANDARD = 50
    double uiScale = 1.0;              // 位图px/vp，点大小/刻度/线宽等"物理尺寸"随它放大

    void setCanvasSize(int w, int h, double uiScale = 1.0);
    double px(double wx) const { return xZero + wx * xscale; }
    double py(double wy) const { return yZero - wy * yscale; }
    double wx(double pxv) const { return (pxv - xZero) / xscale; }
    double wy(double pyv) const { return (yZero - pyv) / yscale; }
    double xmin() const { return wx(0); }
    double xmax() const { return wx(width); }
    double ymin() const { return wy(height); }
    double ymax() const { return wy(0); }

    void panBy(double dxPix, double dyPix)
    {
        xZero += dxPix;
        yZero += dyPix;
    }
    void zoomAt(double anchorPx, double anchorPy, double factor);
    void reset();

private:
    bool sizedOnce_ = false;
};

} // namespace dreamember
#endif // DREAMEMBER_KERNEL_VIEW_H
