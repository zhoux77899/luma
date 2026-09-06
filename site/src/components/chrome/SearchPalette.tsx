import { Search } from "lucide-react";
import { useEffect, useMemo, useState } from "react";
import { useNavigate } from "react-router-dom";
import { Button } from "@/components/motion/button/base";
import {
  CommandDialog,
  CommandEmpty,
  CommandGroup,
  CommandInput,
  CommandItem,
  CommandList,
} from "@/components/ui/command";
import { Kbd, KbdGroup } from "@/components/ui/kbd";
import { loadSearch, type SearchDoc } from "@/lib/content";
import { docPath, type Locale } from "@/lib/paths";

type Props = {
  version: string;
  locale: Locale;
};

function useModKeyLabel() {
  const [label, setLabel] = useState("Ctrl");
  useEffect(() => {
    const mac = /Mac|iPhone|iPad/.test(navigator.platform) || navigator.userAgent.includes("Mac");
    setLabel(mac ? "⌘" : "Ctrl");
  }, []);
  return label;
}

export function SearchPalette({ version, locale }: Props) {
  const navigate = useNavigate();
  const [open, setOpen] = useState(false);
  const [docs, setDocs] = useState<SearchDoc[]>([]);
  const modKey = useModKeyLabel();

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

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if (event.key.toLowerCase() === "k" && (event.metaKey || event.ctrlKey)) {
        event.preventDefault();
        setOpen((current) => !current);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, []);

  const items = useMemo(
    () =>
      docs.map((doc) => ({
        id: doc.slug,
        label: doc.title,
        onSelect: () => {
          navigate(docPath(version, locale, doc.slug));
          setOpen(false);
        },
      })),
    [docs, locale, navigate, version],
  );

  return (
    <>
      <Button
        type="button"
        variant="outline"
        size="sm"
        pressScale={1}
        whileHover={{ scale: 1 }}
        onClick={() => setOpen(true)}
        className="mb-3 w-full justify-start rounded-xl px-2.5 text-[color:var(--fg-muted)]"
        aria-label={locale === "zh" ? "搜索" : "Search"}
      >
        <Search className="size-4" />
        <span>{locale === "zh" ? "搜索" : "Search"}</span>
        <KbdGroup className="ml-auto hidden md:inline-flex">
          <Kbd>{modKey}</Kbd>
          <Kbd>K</Kbd>
        </KbdGroup>
      </Button>
      <CommandDialog
        open={open}
        onOpenChange={setOpen}
        title={locale === "zh" ? "搜索" : "Search"}
        description={locale === "zh" ? "搜索这一版的用户指南。" : "Search this version of the User guide."}
        showCloseButton={false}
      >
        <CommandInput placeholder={locale === "zh" ? "搜索这一版…" : "Search this version…"} />
        <CommandList>
          <CommandEmpty>{locale === "zh" ? "没有匹配的页面。" : "No matching pages."}</CommandEmpty>
          <CommandGroup heading={locale === "zh" ? "本版" : "This version"}>
            {items.map((item) => (
              <CommandItem key={item.id} value={`${item.label} ${item.id}`} onSelect={item.onSelect}>
                {item.label}
              </CommandItem>
            ))}
          </CommandGroup>
        </CommandList>
      </CommandDialog>
    </>
  );
}
