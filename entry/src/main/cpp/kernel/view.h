// view.h — EuclidianView 坐标系统（上游 同名类的 N1 子集）
#ifndef DREAMEMBER_KERNEL_VIEW_H
#define DREAMEMBER_KERNEL_VIEW_H

#include <cmath>

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

// 1.0.8.1 三维视图坐标系统（1.0.8.X 3D 一期；上游 Ggb3D 的正交轨道相机子集）。
// 世界系 z 朝上。相机由方位角 yaw（绕 z）与仰角 pitch 定义，正交投影：
//   right r = (−sinα, cosα, 0)
//   up    u = (−cosα·sinβ, −sinα·sinβ, cosβ)
//   fwd   f = (cosα·cosβ, sinα·cosβ, sinβ)
// 屏幕坐标（像素系左上原点 y 向下，与 EuclidianView 同约定）：
//   px = cx + (p·r − r0)·scale
//   py = cy − (p·u − u0)·scale
// 深度 depth = p·f（越大越远，供画家算法排序）。
// 视窗世界范围固定 [−5,5]³（上游 3D 视图默认 x/y/z 范围），缩放改 scale、
// 拖动改 yaw/pitch（轨道旋转：拖 x 转方位、拖 y 转仰角，仰角钳位 ±89.9°）。
// r0/u0 为视图中心的世界投影（1.0.8.3 平移：双指拖动改 r0/u0，project 全
// 场景随动；拖点 dragDelta 按位移向量解算，与中心无关不用改）。
class EuclidianView3D {
public:
    int width = 0, height = 0;
    double uiScale = 1.0;       // 位图px/vp（点大小/线宽等物理尺寸）
    double yaw = 30.0;          // 方位角（度）：绕 z，30° 时 x 轴朝左下、y 朝右下
    double pitch = 20.0;        // 仰角（度）：>0 自上往下看
    double scale = 50.0;        // 每世界单位像素数（上游 SCALE_STANDARD 同值）
    double r0 = 0.0, u0 = 0.0;  // 1.0.8.3 平移：视图中心在 right/up 轴上的世界投影

    void setCanvasSize(int w, int h, double uiScale2 = 1.0)
    {
        width = w;
        height = h;
        uiScale = uiScale2;
    }
    // 屏幕中心像素（世界原点投影处）
    double cx() const { return width / 2.0; }
    double cy() const { return height / 2.0; }

    // 投影：世界 (x,y,z) → 屏幕 (px,py) + 深度（返回值）
    double project(double x, double y, double z, double &px, double &py) const
    {
        const double sa = std::sin(yaw * M_PI / 180.0);
        const double ca = std::cos(yaw * M_PI / 180.0);
        const double sb = std::sin(pitch * M_PI / 180.0);
        const double cb = std::cos(pitch * M_PI / 180.0);
        const double X = -sa * x + ca * y;                 // p·r
        const double Y = -ca * sb * x - sa * sb * y + cb * z;   // p·u
        const double D = ca * cb * x + sa * cb * y + sb * z;    // p·f
        px = cx() + (X - r0) * scale;
        py = cy() - (Y - u0) * scale;
        return D;
    }
    // 视平面拖动 → 世界位移（屏幕 dx,dy 像素 → 世界 dx·r − dy·u 方向）：
    // 拖 3D 点用（点在垂直视向、过点自身的平面内移动，深度不变）
    void dragDelta(double dxPx, double dyPx, double &wx, double &wy, double &wz) const
    {
        const double sa = std::sin(yaw * M_PI / 180.0);
        const double ca = std::cos(yaw * M_PI / 180.0);
        const double sb = std::sin(pitch * M_PI / 180.0);
        const double cb = std::cos(pitch * M_PI / 180.0);
        // world = (dx/scale)·r + (dy/scale)·u（py 向下，dy 已按屏幕增量传入）
        wx = (-sa * dxPx + -ca * sb * dyPx) / scale;
        wy = (ca * dxPx + -sa * sb * dyPx) / scale;
        wz = (cb * dyPx) / scale;
    }
    // 轨道旋转：拖像素增量（dx 转方位、dy 转仰角，仰角钳位防翻转）。
    // 1.1.0.6③（用户实测反馈）：拖右画面须跟手右移——固定点屏幕 X=p·r，
    // 近端点（X>0 侧）随 yaw 增大向左移（如 +y 端 X=3cosα 递减），故方位
    // 取负号；仰角原语义保留（拖上画面上移，已跟手）
    void rotateBy(double dxPx, double dyPx)
    {
        yaw -= dxPx * 0.4;
        pitch += dyPx * 0.4;
        if (pitch > 89.9) pitch = 89.9;
        if (pitch < -89.9) pitch = -89.9;
        while (yaw >= 360.0) yaw -= 360.0;
        while (yaw < 0.0) yaw += 360.0;
    }
    // 1.0.8.3 平移：屏幕像素增量（右/下为正）→ 中心沿视平面反向移动
    //（world = (−dx/scale)·r + (dy/scale)·u，屏幕内容跟随手指）
    void panBy(double dxPx, double dyPx)
    {
        r0 -= dxPx / scale;
        u0 += dyPx / scale;
    }
    void zoomBy(double factor)
    {
        if (factor <= 0.0) return;
        const double ns = scale * factor;
        scale = ns < 4.0 ? 4.0 : (ns > 2000.0 ? 2000.0 : ns);   // 钳位 [4,2000]
    }
    void reset()
    {
        yaw = 30.0;
        pitch = 20.0;
        scale = 50.0;
        r0 = 0.0;
        u0 = 0.0;
    }
};

} // namespace dreamember
#endif // DREAMEMBER_KERNEL_VIEW_H
