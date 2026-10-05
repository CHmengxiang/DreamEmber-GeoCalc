import { router } from '@kit.ArkUI';
import { bundleManager } from '@kit.AbilityKit';

/**
 * 关于页：展示应用信息与声明
 * N3：深色模式（跟随系统，AppStorage 'ui.dark'）
 * 三十包②：版本号改读 bundle 信息（AppScope versionName）——此前硬编码
 * 1.0.0 已与实际构建（1.0.1→1.0.2）脱节两轮
 */
@Entry
@Component
struct About {
  @StorageProp('ui.dark') private dark: boolean = false;
  @State private verText: string = '版本 1.0.2';

  aboutToAppear(): void {
    try {
      const info = bundleManager.getBundleInfoForSelfSync(
        bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT);
      if (info.versionName && info.versionName.length > 0) {
        this.verText = '版本 ' + info.versionName;
      }
    } catch (_e) {
    }
  }

  private cPage(): string {
    return this.dark ? '#17181A' : '#FFFFFF';
  }

  private cCard(): string {
    return this.dark ? '#232529' : '#F5F6FA';
  }

  private cText(): string {
    return this.dark ? '#E8EAED' : '#333333';
  }

  private cSub(): string {
    return this.dark ? '#9AA0A6' : '#555555';
  }

  private cFaint(): string {
    return this.dark ? '#6E747D' : '#888888';
  }

  build() {
    Column() {
      // 顶栏
      Row() {
        Text('←')
          .fontSize(22)
          .fontColor(this.cText())
          .onClick(() => {
            router.back();
          })
          .padding({ right: 12 })
        Text('关于')
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .fontColor(this.cText())
      }
      .width('100%')
      .height(52)
      .padding({ left: 16 })
      .alignItems(VerticalAlign.Center)

      Scroll() {
        Column() {
          // 应用标识
          Column() {
            Text('梦烬几何')
              .fontSize(26)
              .fontWeight(FontWeight.Bold)
              .fontColor(this.cText())
            Text('DreamEmber GeoCalc')
              .fontSize(13)
              .fontColor(this.cFaint())
              .margin({ top: 4 })
            Text(this.verText)
              .fontSize(13)
              .fontColor(this.cFaint())
              .margin({ top: 4 })
          }
          .width('100%')
          .alignItems(HorizontalAlign.Center)
          .margin({ top: 40, bottom: 32 })

          // 说明卡片
          Column() {
            Text('声明')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('梦烬几何（作者：CH梦想 / CH_mengxiang）为原创独立开发：ArkTS 界面与自研 C++ 几何内核，' +
              '未使用 GeoGebra 源代码（开发过程中参考了其公开源码与交互设计）；' +
              '计算机代数引擎采用开源库 giac（GPL-3.0+，© Bernard Parisse / Institut Fourier）。' +
              '本项目源码按 GPL-3.0 协议开源，应用面向非商业的教育学习场景免费提供。' +
              'GeoGebra 是 GeoGebra GmbH 的注册商标，本应用与 GeoGebra 官方无隶属关系。')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
              .textAlign(TextAlign.Start)
          }
          .width('100%')
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())
          .margin({ bottom: 16 })

          Column() {
            Text('功能说明')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('梦烬几何提供交互式数学绘图：函数图像、几何对象、方程求解与 CAS 符号计算。' +
              '支持触屏拖拽、缩放与命令输入。')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
          }
          .width('100%')
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())
          .margin({ bottom: 16 })

          // 三十八轮：开源仓库地址（GPL-3.0 合规——源码可得入口）
          Column() {
            Text('开源仓库')
              .fontSize(15)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.cText())
              .margin({ bottom: 8 })
            Text('Gitee：gitee.com/CH_mengxiang/DreamEmber-GeoCalc')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
            Text('获取源码、提交问题反馈均在上述仓库进行。')
              .fontSize(13)
              .fontColor(this.cFaint())
              .lineHeight(20)
              .margin({ top: 6 })
          }
          .width('100%')
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())

          Blank()
        }
        .constraintSize({ minHeight: '100%' })
        .padding({ left: 16, right: 16, bottom: 24 })
      }
      .layoutWeight(1)
      .align(Alignment.Top)
      .scrollBar(BarState.Off)
      .edgeEffect(EdgeEffect.Spring)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(this.cPage())
  }
}
