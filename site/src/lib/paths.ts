export function withBase(path: string): string {
  if (/^(https?:|mailto:|#)/i.test(path)) {
    return path;
  }
  const base = import.meta.env.BASE_URL;
  const trimmed = path.replace(/^\//, "");
  return `${base}${trimmed}`;
}

export function localeHome(locale: Locale): string {
  return `/${locale}`;
}

export function docPath(version: string, locale: Locale, slug = "getting-started"): string {
  return `/${version}/${locale}/${slug}`;
}

export type Locale = "en" | "zh";

export function isLocale(value: string | undefined): value is Locale {
  return value === "en" || value === "zh";
}
