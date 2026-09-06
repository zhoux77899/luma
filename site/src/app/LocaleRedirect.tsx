import { useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { detectLocale } from "@/lib/locale";
import { localeHome } from "@/lib/paths";

export function LocaleRedirect() {
  const navigate = useNavigate();

  useEffect(() => {
    navigate(localeHome(detectLocale()), { replace: true });
  }, [navigate]);

  return null;
}