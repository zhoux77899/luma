# NOTES

Notes 列出各条 Note，一次只编辑一份 Notes document（笔记正文）。从 [Launcher](launcher.md) 打开。

## Note list

![Note list](../assets/luma-notes-menu.png)

已保存的 Note 按最近修改排在前面，每条是一个序号 chip 和一个 Title chip。New note 钉在底部。

Note title（标题）是正文的第一行。第一行为空时显示 `Untitled`。

上下键移动选择。在一条 Note 上 Confirm 打开它。在 New note 上 Confirm 打开空编辑器。

在一条 Note 上 Delete 会问 `Delete?`。Confirm 删除，Esc 取消。没有这张对话框的截图。

Notes 最多十六条。列表满了以后，New note 的 Footer 显示 `FULL`，不会再新建。

选中一条 Note 时的 Footer：`Ent open`，`Del delete`，`Esc back`。

选中 New note 时的 Footer：`Ent new`，`Esc back`。

## 编辑器

![Notes editor](../assets/luma-notes-edit.png)

第一行画得更大，那一行就是 Note title。其余是正文。

打字插入。方向键移动光标。Confirm 换行（`Ent line`）。Delete 删光标前一个字符（`Del bk`）。

Esc 离开编辑器并保存。空文档会被丢掉，包括你从没打过字的新 Note。

一份文档最多 1024 个字符。满了以后 Footer 显示 `FULL`，不再是平时那些 hint。保存失败时 Footer 显示 `SAVE FAIL`。
