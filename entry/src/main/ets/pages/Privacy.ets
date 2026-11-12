// Privacy.ets — 应用内隐私政策页（1.1.0.6①：原「网页链接」改「次级界面
// 链接」——开鸿系统实测 openLink 打不开系统浏览器（真机反馈），政策全文
// 随包内置；首次启动告知弹窗（Index.ets）与关于页（About.ets）均跳本页）。
// 内容与仓库 docs/privacy-policy.md 同源——政策内容有任何变更时两处同步，
// 并同步 Index.ets 的 PRIVACY_POLICY_VERSION（日期式）触发全量重弹。
import { router } from '@kit.ArkUI';

interface PolicySection {
  h: string;      // 小节标题
  ps: string[];   // 小节段落（有序列表/要点并入同序段落）
}

// 与 docs/privacy-policy.md 同源全文（生效 2026-10-07）
const POLICY_SECTIONS: PolicySection[] = [
  {
    h: '一、我们不收集任何个人信息',
    ps: [
      '1. 本应用不收集、不存储、不上传、不共享您的任何个人信息或使用数据；',
      '2. 本应用不包含任何第三方统计、广告或追踪组件；',
      '3. 本应用不申请任何系统权限——不含网络、存储、定位、相机、麦克风、'
        + '通讯录等任何权限。应用安装包内不含网络访问能力，不存在任何向外部'
        + '服务器传输数据的通道。'
    ]
  },
  {
    h: '二、创作内容的存储方式',
    ps: [
      '1. 本应用不自动保存您在应用内创建的图形与表达式；未保存的作图内容'
        + '仅存在于应用运行内存中，关闭应用即消失；',
      '2. 当您主动保存 .ggb 文件时，应用会唤起系统文件选择器，由您自行选择'
        + '保存位置；文件保存在您指定的位置（如设备"文件管理"中的目录），'
        + '完全由您自己管理；',
      '3. 您通过文件选择器保存的文件位于应用沙箱之外，卸载应用不会删除'
        + '这些文件；',
      '4. 您通过文件选择器打开本地 .ggb 文件时，应用仅在该次访问中获得您'
        + '所选文件的读取授权，不会访问您设备上的其他文件。'
    ]
  },
  {
    h: '三、应用本地存储的数据',
    ps: [
      '应用沙箱内仅保存少量偏好设置（如主题外观、默认输入方式），不包含'
        + '任何个人身份信息。卸载应用后，这些设置即随之删除。'
    ]
  },
  {
    h: '四、数据的共享与披露',
    ps: [
      '由于本应用不收集任何数据，我们不会向任何第三方共享、转让或披露您的'
        + '个人信息，也不存在委托处理、合并收购情形下的数据转移。'
    ]
  },
  {
    h: '五、未成年人保护',
    ps: [
      '本应用不收集任何用户信息（包括未成年人信息），适用于包括儿童在内的'
        + '所有用户。监护人可放心让未成年人使用。'
    ]
  },
  {
    h: '六、您的权利',
    ps: [
      '- 您可以随时在设备系统中卸载本应用，卸载后应用沙箱内的偏好设置即被'
        + '删除；',
      '- 您保存到自选位置的 .ggb 文件归您所有，可随时自行查看、复制、删除；',
      '- 如您对本政策的执行有任何疑问或异议，可通过下文联系方式与我们沟通。'
    ]
  },
  {
    h: '七、政策变更',
    ps: [
      '如本政策发生变化，将在应用版本更新时一并说明，并在本页面更新生效'
        + '日期。重大变更会以显著方式提示。'
    ]
  },
  {
    h: '八、开源信息',
    ps: [
      '本应用按 GPL-3.0-or-later 协议开源，源码地址：'
        + 'https://gitee.com/CH_mengxiang/DreamEmber-GeoCalc。'
    ]
  },
  {
    h: '九、联系我们',
    ps: [
      '如您对本政策或本应用的数据处理方式有任何疑问、意见或建议，请联系：',
      '邮箱：qdyangfw@qq.com'
    ]
  }
];

@Entry
@Component
struct Privacy {
  @StorageProp('ui.dark') private dark: boolean = false;

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
      // 顶栏（与关于页同款）
      Row() {
        Text('←')
          .fontSize(22)
          .fontColor(this.cText())
          .onClick(() => {
            router.back();
          })
          .padding({ right: 12 })
        Text('隐私政策')
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
          Column() {
            Text('梦烬几何隐私政策')
              .fontSize(17)
              .fontWeight(FontWeight.Bold)
              .fontColor(this.cText())
            Text('生效日期：2026-10-07　|　最近更新：2026-10-07')
              .fontSize(12)
              .fontColor(this.cFaint())
              .margin({ top: 6 })
            Text('梦烬几何（英文名 DreamEmber GeoCalc，下称"本应用"）是一款'
              + '完全离线运行的本地几何与代数工具。我们非常重视您的隐私。'
              + '请您在使用本应用前仔细阅读本政策，以了解我们如何处理'
              + '（实际上是不收集、不上传）与您相关的信息。')
              .fontSize(14)
              .fontColor(this.cSub())
              .lineHeight(22)
              .margin({ top: 12 })
          }
          .width('100%')
          .alignItems(HorizontalAlign.Start)
          .padding(16)
          .borderRadius(12)
          .backgroundColor(this.cCard())

          ForEach(POLICY_SECTIONS, (sec: PolicySection) => {
            Column() {
              Text(sec.h)
                .fontSize(15)
                .fontWeight(FontWeight.Medium)
                .fontColor(this.cText())
                .margin({ bottom: 8 })
              ForEach(sec.ps, (p: string) => {
                Text(p)
                  .fontSize(14)
                  .fontColor(this.cSub())
                  .lineHeight(22)
                  .margin({ top: 4 })
              }, (p: string, i: number) => `${sec.h}-${i}`)
            }
            .width('100%')
            .alignItems(HorizontalAlign.Start)
            .padding(16)
            .borderRadius(12)
            .backgroundColor(this.cCard())
            .margin({ top: 16 })
          }, (sec: PolicySection) => sec.h)

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
