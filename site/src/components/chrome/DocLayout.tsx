import type { ReactNode } from "react";
import { Sidebar } from "@/components/chrome/Sidebar";
import { SiteHeader } from "@/components/chrome/SiteHeader";
import { Toc } from "@/components/chrome/Toc";
import type { PageRecord, VersionInfo } from "@/lib/content";
import type { Locale } from "@/lib/paths";

type Props = {
  locale: Locale;
  version: string;
  versions: VersionInfo[];
  slug: string;
  page: PageRecord;
  children: ReactNode;
};

export function DocLayout({ locale, version, versions, slug, page, children }: Props) {
  return (
    <div className="min-h-dvh">
      <SiteHeader locale={locale} version={version} versions={versions} />
      <div className="mx-auto grid max-w-6xl gap-8 px-4 pb-16 pt-20 lg:grid-cols-[220px_minmax(0,1fr)_180px]">
        <aside className="lg:sticky lg:top-20 lg:self-start">
          <Sidebar
            locale={locale}
            version={version}
            currentSlug={slug}
            hasUserGuide={page.hasUserGuide}
          />
        </aside>
        <main>{children}</main>
        <aside className="hidden lg:sticky lg:top-20 lg:block lg:self-start">
          <Toc locale={locale} headings={page.headings} />
        </aside>
      </div>
    </div>
  );
}