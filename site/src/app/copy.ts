import type { Locale } from "@/lib/paths";

export const copy = {
  zh: {
    title: "LUMA",
    offer: "LUMA 是跑在 M5Stack Cardputer ADV 上的固件。",
    read: "阅读当前指南",
    github: "GitHub",
    settings: "SETTINGS",
    settingsBody: "亮度、外观、Wi-Fi、时区、电量和 About。",
    notes: "NOTES",
    notesBody: "最多十六条笔记。第一行是标题。",
    dots: "DOTS",
    dotsBody: "60×30 的 Matrix，用 Confirm 点亮、Delete 熄灭。",
    missing: "找不到这一页。",
    stubLead: "这一版发行时还没有 User guide。",
    stubAction: "去看当前稿",
  },
  en: {
    title: "LUMA",
    offer: "LUMA is the firmware that runs on M5Stack Cardputer ADV.",
    read: "Read the current guide",
    github: "GitHub",
    settings: "SETTINGS",
    settingsBody: "Brightness, Theme, Wi-Fi, Time zone, Battery, and About.",
    notes: "NOTES",
    notesBody: "Up to sixteen Notes. The first line is the title.",
    dots: "DOTS",
    dotsBody: "A 60×30 Matrix. Confirm paints. Delete extinguishes.",
    missing: "This page is missing.",
    stubLead: "This Release shipped without a User guide.",
    stubAction: "Open the current draft",
  },
} as const satisfies Record<Locale, Record<string, string>>;
