import type { PageRecord } from "@/lib/content";
import type { Locale } from "@/lib/paths";

type Props = {
  locale: Locale;
  headings: PageRecord["headings"];
};

export function Toc({ locale, headings }: Props) {
  if (headings.length === 0) {
    return null;
  }

  return (
    <nav aria-label={locale === "zh" ? "本页" : "On this page"} className="text-sm">
      <p className="mb-2 font-medium text-[color:var(--fg)]">
        {locale === "zh" ? "本页" : "On this page"}
      </p>
      <ul className="flex flex-col gap-1">
        {headings.map((heading) => (
          <li key={heading.id} className={heading.level === 3 ? "pl-3" : undefined}>
            <a href={`#${heading.id}`} className="text-[color:var(--fg-muted)] no-underline hover:text-[color:var(--fg)]">
              {heading.text}
            </a>
          </li>
        ))}
      </ul>
    </nav>
  );
}