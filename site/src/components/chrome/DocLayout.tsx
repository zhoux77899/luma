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
    <>
      <SiteHeader locale={locale} version={version} versions={versions} />
      <div className="min-h-dvh lg:h-dvh lg:overflow-hidden">
        <div className="mx-auto grid min-h-dvh w-full max-w-6xl grid-cols-1 gap-8 px-4 pb-16 pt-20 lg:h-full lg:min-h-0 lg:grid-cols-[220px_minmax(0,1fr)_180px] lg:grid-rows-[minmax(0,1fr)] lg:pb-0 lg:pt-0">
          <aside className="lg:min-h-0 lg:overflow-y-auto lg:pt-20 lg:pb-6">
            <Sidebar
              locale={locale}
              version={version}
              currentSlug={slug}
              hasUserGuide={page.hasUserGuide}
            />
          </aside>
          <main className="min-w-0 lg:flex lg:h-full lg:min-h-0 lg:flex-col">{children}</main>
          <aside className="hidden lg:block lg:min-h-0 lg:overflow-y-auto lg:pt-20 lg:pb-6">
            <Toc locale={locale} headings={page.headings} />
          </aside>
        </div>
      </div>
    </>
  );
}