import type { Locale } from "@/lib/paths";

export type NavItem = {
  slug: string;
  en: string;
  zh: string;
  children?: NavItem[];
};

export const NAV: NavItem[] = [
  { slug: "getting-started", en: "Getting started", zh: "快速开始" },
  { slug: "keys", en: "Keys", zh: "按键" },
  { slug: "launcher", en: "LAUNCHER", zh: "LAUNCHER" },
  {
    slug: "settings",
    en: "SETTINGS",
    zh: "SETTINGS",
    children: [
      { slug: "settings/display", en: "Display", zh: "Display" },
      { slug: "settings/sound", en: "Sound", zh: "Sound" },
      { slug: "settings/network", en: "Network", zh: "Network" },
      { slug: "settings/time", en: "Time", zh: "Time" },
      { slug: "settings/battery", en: "Battery", zh: "Battery" },
      { slug: "settings/system", en: "System", zh: "System" },
    ],
  },
  { slug: "notes", en: "NOTES", zh: "NOTES" },
  { slug: "dots", en: "DOTS", zh: "DOTS" },
];

export function navLabel(item: NavItem, locale: Locale): string {
  return locale === "zh" ? item.zh : item.en;
}
