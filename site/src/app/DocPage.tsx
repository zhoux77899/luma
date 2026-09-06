import { useEffect, useState } from "react";
import { Link, Navigate, useParams } from "react-router-dom";
import { copy } from "@/app/copy";
import { DocLayout } from "@/components/chrome/DocLayout";
import { MarkdownView } from "@/components/chrome/MarkdownView";
import { loadPage, type PageRecord, type VersionInfo } from "@/lib/content";
import { storeLocale } from "@/lib/locale";
import { docPath, isLocale, type Locale } from "@/lib/paths";

type Props = {
  versions: VersionInfo[];
};

export function DocPage({ versions }: Props) {
  const { version = "", locale = "", "*": rest } = useParams();
  const slug = rest && rest.length > 0 ? rest.replace(/\/$/, "") : "getting-started";

  if (!isLocale(locale) || !versions.some((item) => item.id === version)) {
    return <Navigate to="/" replace />;
  }

  if (version === "main") {
    return <Navigate to={docPath("latest", locale, slug)} replace />;
  }

  return <LoadedDoc version={version} locale={locale} slug={slug} versions={versions} />;
}

function LoadedDoc({
  version,
  locale,
  slug,
  versions,
}: {
  version: string;
  locale: Locale;
  slug: string;
  versions: VersionInfo[];
}) {
  const [page, setPage] = useState<PageRecord | null | undefined>(undefined);

  useEffect(() => {
    storeLocale(locale);
  }, [locale]);

  useEffect(() => {
    let cancelled = false;
    setPage(undefined);
    void loadPage(version, locale, slug).then((record) => {
      if (!cancelled) {
        setPage(record);
      }
    });
    return () => {
      cancelled = true;
    };
  }, [version, locale, slug]);

  if (page === undefined) {
    return null;
  }

  const text = copy[locale];
  if (!page) {
    return (
      <div className="mx-auto max-w-3xl px-4 pb-20 pt-24">
        <p>{text.missing}</p>
        <Link to={docPath(version, locale, "getting-started")}>{text.read}</Link>
      </div>
    );
  }

  const meta = versions.find((item) => item.id === version);

  return (
    <DocLayout locale={locale} version={version} versions={versions} slug={slug} page={page}>
      {!page.hasUserGuide || meta?.hasUserGuide === false ? (
        <p className="mb-6 text-[color:var(--fg-muted)]">{text.stubLead}</p>
      ) : null}
      <MarkdownView key={slug} markdown={page.markdown} />
      {!page.hasUserGuide ? (
        <p className="mt-8">
          <Link to={docPath("latest", locale, "getting-started")}>{text.stubAction}</Link>
        </p>
      ) : null}
    </DocLayout>
  );
}