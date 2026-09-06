import { isLocale, type Locale } from "@/lib/paths";

const STORAGE_KEY = "luma-locale";

export function readStoredLocale(): Locale | null {
  try {
    const value = localStorage.getItem(STORAGE_KEY);
    return isLocale(value ?? "") ? value : null;
  } catch {
    return null;
  }
}

export function storeLocale(locale: Locale): void {
  document.documentElement.lang = locale === "zh" ? "zh-Hans" : "en";
  try {
    localStorage.setItem(STORAGE_KEY, locale);
  } catch {
    /* ignore quota / privacy mode */
  }
}

export function detectLocale(): Locale {
  const stored = readStoredLocale();
  if (stored) {
    return stored;
  }
  const language = navigator.language || "en";
  return language.toLowerCase().startsWith("zh") ? "zh" : "en";
}
