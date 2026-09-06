import { useEffect, useState } from "react";
import { Link, useLocation } from "react-router-dom";
import { GitHubMark } from "@/components/chrome/GitHubMark";
import { LocaleSelect } from "@/components/chrome/LocaleSelect";
import { ThemeToggle } from "@/components/chrome/ThemeToggle";
import { VersionSelect } from "@/components/chrome/VersionSelect";
import { ButtonLink } from "@/components/motion/button/base";
import type { VersionInfo } from "@/lib/content";
import { cn } from "@/lib/utils";
import { localeHome, type Locale, withBase } from "@/lib/paths";

type Props = {
  locale: Locale;
  version?: string;
  versions: VersionInfo[];
};

function usePageScrolled(isHome: boolean) {
  const { pathname } = useLocation();
  const [scrolled, setScrolled] = useState(false);
  useEffect(() => {
    const read = () => {
      const article = document.querySelector("[data-doc-scroll]");
      const articleTop = article instanceof HTMLElement ? article.scrollTop : 0;
      setScrolled(window.scrollY > 0 || articleTop > 0);
    };
    read();
    window.addEventListener("scroll", read, { passive: true });
    document.addEventListener("scroll", read, { passive: true, capture: true });
    return () => {
      window.removeEventListener("scroll", read);
      document.removeEventListener("scroll", read, { capture: true });
    };
  }, [isHome, pathname]);
  return scrolled;
}

export function SiteHeader({ locale, version, versions }: Props) {
  const isHome = version === undefined;
  const scrolled = usePageScrolled(isHome);
  const frost = "bg-[color:var(--bg)]/20 backdrop-blur-xl";

  return (
    <header
      className={cn(
        "fixed inset-x-0 top-0 z-30 transition-[background-color,backdrop-filter,border-color]",
        isHome && !scrolled && "bg-transparent",
        isHome && scrolled && `border-b border-border ${frost}`,
        !isHome && frost,
        !isHome && scrolled && "border-b border-border",
      )}
    >
      <div className="mx-auto flex h-14 max-w-6xl items-center gap-3 px-4">
        <Link to={localeHome(locale)} className="flex items-center gap-2 text-[color:var(--fg)] no-underline">
          <img src={withBase("luma-logo.svg")} alt="" width={28} height={28} />
          <span className="hidden font-medium tracking-[-0.02em] sm:inline">LUMA</span>
        </Link>
        <div className="ml-auto flex items-center gap-1">
          <VersionSelect locale={locale} version={version} versions={versions} />
          <LocaleSelect locale={locale} version={version} />
          <ThemeToggle />
          <ButtonLink
            href="https://github.com/zhoux77899/luma"
            variant="ghost"
            size="icon"
            className="size-9 rounded-xl !text-[color:var(--fg)] hover:!text-[color:var(--fg)]"
            target="_blank"
            rel="noreferrer"
            aria-label={locale === "zh" ? "GitHub 仓库" : "GitHub repository"}
          >
            <GitHubMark className="size-4" />
          </ButtonLink>
        </div>
      </div>
    </header>
  );
}
