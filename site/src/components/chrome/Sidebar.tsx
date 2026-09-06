import { NavLink } from "react-router-dom";
import { NAV, navLabel } from "@/lib/nav";
import { docPath, type Locale } from "@/lib/paths";

type Props = {
  locale: Locale;
  version: string;
  currentSlug: string;
  hasUserGuide: boolean;
};

export function Sidebar({ locale, version, currentSlug, hasUserGuide }: Props) {
  if (!hasUserGuide) {
    return (
      <nav aria-label={locale === "zh" ? "目录" : "Directory"} className="text-sm">
        <p className="text-[color:var(--fg-muted)]">
          {locale === "zh" ? "此版本没有手册目录。" : "This version has no handbook directory."}
        </p>
      </nav>
    );
  }

  return (
    <nav aria-label={locale === "zh" ? "目录" : "Directory"} className="text-sm">
      <ul className="flex flex-col gap-1">
        {NAV.map((item) => (
          <li key={item.slug}>
            <NavLink
              to={docPath(version, locale, item.slug)}
              className={`block rounded-xl px-2 py-1.5 no-underline ${
                currentSlug === item.slug
                  ? "bg-[color:var(--color-fuji)]/20 text-[color:var(--fg)]"
                  : "text-[color:var(--fg)] hover:bg-[color:var(--bg-card)]/35"
              }`}
            >
              {navLabel(item, locale)}
            </NavLink>
            {item.children ? (
              <ul className="ml-3 mt-1 flex flex-col gap-1 border-l border-[color:var(--line)]/40 pl-2">
                {item.children.map((child) => (
                  <li key={child.slug}>
                    <NavLink
                      to={docPath(version, locale, child.slug)}
                      className={`block rounded-xl px-2 py-1 no-underline ${
                        currentSlug === child.slug
                          ? "bg-[color:var(--color-fuji)]/20 text-[color:var(--fg)]"
                          : "text-[color:var(--fg-muted)] hover:text-[color:var(--fg)]"
                      }`}
                    >
                      {navLabel(child, locale)}
                    </NavLink>
                  </li>
                ))}
              </ul>
            ) : null}
          </li>
        ))}
      </ul>
    </nav>
  );
}