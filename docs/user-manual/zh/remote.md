# REMOTE

REMOTE 一次显示一台 Remote，并通过 Infrared 发射它的 Key。Cardputer ADV 只能发射，不能接收，所以没有学习界面。从 [Launcher](launcher.md) 打开 REMOTE。

主画面保留 Header，没有 Footer。屏幕上是一台 Remote：有品牌标记时显示标记，以及六个 Key。

## 按键

按下对应键就会发射。不用先选中一行再按 Ent。

| 键 | Cardputer |
| --- | --- |
| Power | `P` |
| Mute | `M` |
| Vol+ | `;`（上） |
| Vol- | `.`（下） |
| Ch- | `,`（左） |
| Ch+ | `/`（右） |

`[` 是上一台 Remote，`]` 是下一台。按住不连发。

七个 Brand remote 排在前面，不能改，也不能删：LG、TCL、海信、Samsung、Sony、Panasonic、Philips。没有命令的键不会发射。

## 自己的 Remote

`N` 可以新建空 Remote，或复制一台 Brand remote。REMOTE 最多十六台，所以自己的最多九台。满了以后 `N` 显示 `FULL`。

`E` 编辑你新建的 Remote：名字、协议、共用地址，以及六个命令。自定义协议的时序只存在这一台 Remote 上。可选 Distance、Width 或 Manchester，再填数字。主画面上 `Del` 会先问一句，再删掉整台 Remote。主画面按 Esc 回到 Launcher。

SDL preview 只记录 `[IR]`，不会真的发射。
