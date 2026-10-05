// CasPanel.ets — CAS 视图（2026-09-30 第五包）：上游经典版 的 CAS 入口
// 在 ☰ 菜单"视图"分组（上游 无底部 CAS 栏——原 Index 底部冒烟面板系早期
// 集成 Giac 时的测试残留，本轮删除），本面板即照 上游 语义做的正式视图：
// 单行输入 + 求值 + 历史结果列表，直接调内核 Giac 包装 casCaseval
//（Solve/Expand/Simplify/Derivative/Integral 等由 giac 原生支持；Factor
// 类命令真机有已知闪退记录，内核侧有 stage 日志，UI 只做 try/catch 兜底）。
import { BusinessError } from '@kit.BasicServicesKit';
import dreamemberKernel from 'libdreamember.so';
import { Theme } from '../model/Theme';

interface CasResult {
  expr: string;
  value: string;
  ok: boolean;
}

@Component
export struct CasPanel {
  @Prop dark: boolean = false;
  onClose: () => void = () => {};
  @State private input: string = '';
  @State private results: CasResult[] = [];

  // 求值：结果插到列表头（最新在上），保留最近 30 条；输入行清空便于连发
  private runCas(): void {
    const expr = this.input.trim();
    if (expr.length === 0) {
      return;
    }
    let value = '';
    let ok = true;
    try {
      value = dreamemberKernel.casCaseval(expr);
      ok = !value.startsWith('error');
    } catch (err) {
      const e = err as BusinessError;
      value = 'error: ' + (e.message ?? '');
      ok = false;
    }
    const entry: CasResult = { expr: expr, value: value, ok: ok };
    this.results = [entry, ...this.results].slice(0, 30);
    this.input = '';
  }

  build() {
    Column() {
      Row() {
        Text('CAS 视图')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(Theme.text(this.dark))
          .layoutWeight(1)
        Text('符号计算（Giac 内核）')
          .fontSize(11)
          .fontColor(Theme.faint(this.dark))
          .margin({ right: 8 })
        Text('✕')
          .fontSize(14)
          .fontColor(Theme.sub(this.dark))
          .width(30)
          .height(30)
          .textAlign(TextAlign.Center)
          .onClick(() => this.onClose())
      }
      .width('100%')
      .padding({ left: 12, right: 8, top: 6, bottom: 2 })

      Row({ space: 6 }) {
        TextInput({ text: this.input,
          placeholder: '如 Solve(x^2=4)、Expand((x+1)^2)、Integral(cos(x))' })
          .layoutWeight(1)
          .height(32)
          .fontSize(13)
          .backgroundColor(Theme.inputBg(this.dark))
          .onChange((v: string) => {
            this.input = v;
          })
          .onSubmit(() => this.runCas())
        Text('求值')
          .fontSize(13)
          .fontColor('#FFFFFF')
          .backgroundColor(Theme.accent(this.dark))
          .borderRadius(8)
          .height(32)
          .padding({ left: 14, right: 14 })
          .textAlign(TextAlign.Center)
          .onClick(() => this.runCas())
      }
      .width('100%')
      .padding({ left: 12, right: 12, top: 2 })

      if (this.results.length > 0) {
        List({ space: 2 }) {
          ForEach(this.results, (r: CasResult, idx: number) => {
            ListItem() {
              Row() {
                Text(r.expr)
                  .fontSize(13)
                  .fontColor(Theme.sub(this.dark))
                  .flexShrink(1)
                  .maxLines(1)
                  .textOverflow({ overflow: TextOverflow.Ellipsis })
                Text('= ' + r.value)
                  .fontSize(13)
                  .fontColor(r.ok ? Theme.accent(this.dark) : Theme.error(this.dark))
                  .layoutWeight(1)
                  .textAlign(TextAlign.End)
                  .maxLines(1)
                  .textOverflow({ overflow: TextOverflow.Ellipsis })
                  .margin({ left: 8 })
              }
              .width('100%')
              .padding({ left: 12, right: 12, top: 5, bottom: 5 })
              .border({ width: { bottom: 1 }, color: { bottom: this.dark ? '#33363B' : '#EDEFF4' } })
            }
          }, (r: CasResult, idx: number) => idx.toString() + '|' + r.expr)
        }
        .width('100%')
        .layoutWeight(1)
        .scrollBar(BarState.Auto)
        .align(Alignment.Top)
        .margin({ top: 2 })
      } else {
        // 空态提示（上游 CAS 行为示例）
        Column({ space: 4 }) {
          Text('输入符号表达式后点"求值"')
            .fontSize(12)
            .fontColor(Theme.faint(this.dark))
          Text('例：Solve(x^2=4)　Expand((x+1)^2)　Derivative(x^3)　Integral(1/x)')
            .fontSize(11)
            .fontColor(Theme.faint(this.dark))
        }
        .width('100%')
        .alignItems(HorizontalAlign.Center)
        .margin({ top: 18 })
      }
    }
    .width('100%')
    .height('100%')
  }
}
