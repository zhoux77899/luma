import { Check, ChevronDown } from "lucide-react";
import { useNavigate, useParams } from "react-router-dom";
import { Button } from "@/components/motion/button/base";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { storeLocale } from "@/lib/locale";
import { docPath, localeHome, type Locale } from "@/lib/paths";

type Props = {
  locale: Locale;
  version?: string;
};

const LOCALES: { id: Locale; label: string }[] = [
  { id: "zh", label: "简体中文" },
  { id: "en", label: "English" },
];

export function LocaleSelect({ locale, version }: Props) {
  const navigate = useNavigate();
  const params = useParams();
  const current = LOCALES.find((item) => item.id === locale) ?? LOCALES[0];
  const slug =
    params["*"] && params["*"].length > 0
      ? params["*"].replace(/\/$/, "")
      : "getting-started";

  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button type="button" variant="ghost" size="sm" className="rounded-xl px-2.5">
          <span>{current.label}</span>
          <ChevronDown size={14} />
        </Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent align="end" className="min-w-36 rounded-xl">
        {LOCALES.map((item) => (
          <DropdownMenuItem
            key={item.id}
            onSelect={() => {
              storeLocale(item.id);
              navigate(version ? docPath(version, item.id, slug) : localeHome(item.id));
            }}
          >
            <Check className={item.id === locale ? "opacity-100" : "opacity-0"} />
            <span>{item.label}</span>
          </DropdownMenuItem>
        ))}
      </DropdownMenuContent>
    </DropdownMenu>
  );
}
