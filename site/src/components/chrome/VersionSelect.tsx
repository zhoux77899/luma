import { Check, ChevronDown } from "lucide-react";
import { useNavigate, useParams } from "react-router-dom";
import { Button } from "@/components/motion/button/base";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import type { VersionInfo } from "@/lib/content";
import { docPath, type Locale } from "@/lib/paths";

type Props = {
  locale: Locale;
  version?: string;
  versions: VersionInfo[];
};

function versionLabel(item: VersionInfo): string {
  return item.prerelease ? `${item.label} pre` : item.label;
}

export function VersionSelect({ locale, version, versions }: Props) {
  const navigate = useNavigate();
  const params = useParams();
  const current = version ?? "latest";
  const selected = versions.find((item) => item.id === current) ?? versions[0];
  const slug =
    params["*"] && params["*"].length > 0
      ? params["*"].replace(/\/$/, "")
      : "getting-started";

  if (!selected) {
    return null;
  }

  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button type="button" variant="ghost" size="sm" className="rounded-xl px-2.5">
          <span>{versionLabel(selected)}</span>
          <ChevronDown size={14} />
        </Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent align="end" className="min-w-36 rounded-xl">
        {versions.map((item) => (
          <DropdownMenuItem
            key={item.id}
            onSelect={() => navigate(docPath(item.id, locale, slug))}
          >
            <Check className={item.id === current ? "opacity-100" : "opacity-0"} />
            <span>{versionLabel(item)}</span>
          </DropdownMenuItem>
        ))}
      </DropdownMenuContent>
    </DropdownMenu>
  );
}
