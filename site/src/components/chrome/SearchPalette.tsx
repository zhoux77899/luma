import { Search } from "lucide-react";
import { useEffect, useMemo, useState } from "react";
import { useNavigate } from "react-router-dom";
import { Button } from "@/components/motion/button/base";
import { CommandPalette, type CommandItem } from "@/components/motion/command-palette";
import { loadSearch, type SearchDoc } from "@/lib/content";
import { docPath, type Locale } from "@/lib/paths";

type Props = {
  version: string;
  locale: Locale;
};

export function SearchPalette({ version, locale }: Props) {
  const navigate = useNavigate();
  const [open, setOpen] = useState(false);
  const [docs, setDocs] = useState<SearchDoc[]>([]);

  useEffect(() => {
    let cancelled = false;
    void loadSearch(version).then((items) => {
      if (!cancelled) {
        setDocs(items.filter((doc) => doc.locale === locale));
      }
    });
    return () => {
      cancelled = true;
    };
  }, [version, locale]);

  const items = useMemo<CommandItem[]>(
    () =>
      docs.map((doc) => ({
        id: doc.slug,
        label: doc.title,
        group: locale === "zh" ? "本版" : "This version",
        keywords: [doc.slug, doc.text],
        onSelect: () => navigate(docPath(version, locale, doc.slug)),
      })),
    [docs, locale, navigate, version],
  );

  return (
    <>
      <Button
        type="button"
        variant="outline"
        size="sm"
        onClick={() => setOpen(true)}
        className="rounded-xl px-2.5 text-[color:var(--fg-muted)] sm:px-3"
        aria-label={locale === "zh" ? "搜索" : "Search"}
      >
        <Search size={16} />
        <span className="hidden sm:inline">{locale === "zh" ? "搜索" : "Search"}</span>
        <kbd className="hidden text-xs md:inline">⌘K</kbd>
      </Button>
      <CommandPalette
        items={items}
        open={open}
        onOpenChange={setOpen}
        placeholder={locale === "zh" ? "搜索这一版…" : "Search this version…"}
        emptyMessage={locale === "zh" ? "没有匹配的页面。" : "No matching pages."}
      />
    </>
  );
}
