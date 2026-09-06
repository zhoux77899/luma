import { useEffect, useState } from "react";
import { Navigate, Route, Routes } from "react-router-dom";
import { ThemeProvider } from "next-themes";
import { DocPage } from "@/app/DocPage";
import { HomePage } from "@/app/HomePage";
import { LocaleRedirect } from "@/app/LocaleRedirect";
import { loadVersions, type VersionInfo } from "@/lib/content";

export function App() {
  const [versions, setVersions] = useState<VersionInfo[] | null>(null);

  useEffect(() => {
    void loadVersions().then((file) => setVersions(file.versions));
  }, []);

  if (!versions) {
    return null;
  }

  return (
    <ThemeProvider attribute="data-theme" defaultTheme="system" enableSystem>
      <Routes>
        <Route path="/" element={<LocaleRedirect />} />
        <Route path="/zh" element={<HomePage locale="zh" versions={versions} />} />
        <Route path="/en" element={<HomePage locale="en" versions={versions} />} />
        <Route path="/:version/:locale/*" element={<DocPage versions={versions} />} />
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </ThemeProvider>
  );
}