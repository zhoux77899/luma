# Battery

Battery 是一个 Settings category。它不是 Launcher 上的 App。

打开 [Settings](../settings.md)，选 Battery。Confirm 会聚焦历史那一栏。

![Battery](../../assets/luma-settings-battery.png)

读数有效时，这一栏显示当前百分比和电压。无效就显示 `--`。

图是过去一小时的 Battery history（电池历史）：每分钟一个样本，最多 60 根柱，右对齐到现在。左边空着的是还没采到的分钟。启动后的新一轮是一段空档。网格标 0、50、100 百分比，以及 -60、-30、0 经过的分钟。

柱的颜色和 Header 电池符号共用 Battery band：0-20、21-30、31-100。

LUMA 不显示 Charging。Cardputer ADV 报不了充电状态。

Footer：`Ent ok`，`Esc back`。
