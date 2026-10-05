// Theme.ets — 轮 A 防乱重构：画布/代数区的深浅两套配色与类型标记收敛到单处。
// 原 CanvasView、AlgebraRowView、CanvasView.typeColorFor 三份逐字拷贝合一；
// Index/About/MathKeyboard 是页面级配色，不属画布域，仍各自持有。

export class Theme {
  static canvas(dark: boolean): string {
    return dark ? '#1E2023' : '#FFFFFF';
  }

  static drawer(dark: boolean): string {
    return dark ? '#222428' : '#F0F1F5';
  }

  static panel(dark: boolean): string {
    return dark ? '#2A2D31' : '#F5F6FA';
  }

  static rowBg(dark: boolean): string {
    return dark ? '#2C2F34' : '#FFFFFF';
  }

  static text(dark: boolean): string {
    return dark ? '#E8EAED' : '#333333';
  }

  static sub(dark: boolean): string {
    return dark ? '#9AA0A6' : '#666666';
  }

  static faint(dark: boolean): string {
    return dark ? '#6E747D' : '#999999';
  }

  static error(dark: boolean): string {
    return dark ? '#FF7B72' : '#F44336';
  }

  static accent(dark: boolean): string {
    return dark ? '#8FB6FF' : '#007DFF';
  }

  static del(dark: boolean): string {
    return dark ? '#E58A8A' : '#CC6666';
  }

  // 浮层描边（缩放按钮/新建行/提示框共用）
  static border(dark: boolean): string {
    return dark ? '#3A3D42' : '#D8DCE4';
  }

  // 输入框底色（行内修改/新建/透明度手改框）
  static inputBg(dark: boolean): string {
    return dark ? '#1E2023' : '#FFFFFF';
  }

  // ✕/✓ 自绘按钮底色
  static keyBg(dark: boolean): string {
    return dark ? '#33373D' : '#E9ECF3';
  }

  // 类型标记色（代数区左侧小圆点/实心按钮，近似 上游 对象配色；深色下提亮）
  static typeColor(t: string, dark: boolean): string {
    if (t === 'point') {
      return dark ? '#8B8BFF' : '#4D4DFF';
    }
    if (t === 'numeric' || t === 'slider') {
      return dark ? '#B9946F' : '#8D6E63';
    }
    if (t === 'list') {
      return dark ? '#C9A6FF' : '#6A4FBF';
    }
    if (t === 'text') {
      return dark ? '#9FB8C9' : '#4A6572';
    }
    if (t === 'vector') {
      return dark ? '#B28DFF' : '#333CCC';
    }
    if (t === 'polygon') {
      return dark ? '#7FB0FF' : '#2C6BD4';
    }
    if (t === 'angle') {
      return dark ? '#7FD1A8' : '#1C7E54';
    }
    return dark ? '#D5D7DB' : '#1C1C1F';
  }

  static typeName(t: string): string {
    if (t === 'point') {
      return '点';
    }
    if (t === 'segment') {
      return '线段';
    }
    if (t === 'line') {
      return '直线';
    }
    if (t === 'ray') {
      return '射线';
    }
    if (t === 'vector') {
      return '向量';
    }
    if (t === 'circle') {
      return '圆';
    }
    if (t === 'polygon') {
      return '多边形';
    }
    if (t === 'angle') {
      return '角';
    }
    if (t === 'text') {
      return '文本';
    }
    if (t === 'integral') {
      return '积分';
    }
    if (t === 'slider') {
      return '滑动条';
    }
    if (t === 'function') {
      return '函数';
    }
    return '数值';
  }
}
