import { withBase, type Locale } from "@/lib/paths";

export type VersionInfo = {
  id: string;
  label: string;
  ref: string;
  hasUserGuide: boolean;
  prerelease: boolean;
};

export type VersionsFile = {
  latest: string;
  versions: VersionInfo[];
};

export type PageRecord = {
  slug: string;
  title: string;
  markdown: string;
  headings: { level: number; text: string; id: string }[];
  hasUserGuide: boolean;
};

export type Manifest = Record<Locale, { slug: string; title: string }[]>;

export type SearchDoc = {
  slug: string;
  title: string;
  text: string;
  locale: Locale;
  version: string;
};

export async function loadVersions(): Promise<VersionsFile> {
  const response = await fetch(withBase("content/versions.json"));
  if (!response.ok) {
    throw new Error("Missing versions.json — run npm run assemble");
  }
  return response.json() as Promise<VersionsFile>;
}

export async function loadManifest(version: string): Promise<Manifest> {
  const response = await fetch(withBase(`content/pages/${version}/manifest.json`));
  if (!response.ok) {
    throw new Error(`Missing manifest for ${version}`);
  }
  return response.json() as Promise<Manifest>;
}

export async function loadPage(
  version: string,
  locale: Locale,
  slug: string,
): Promise<PageRecord | null> {
  const file = slug.replaceAll("/", "__");
  const response = await fetch(withBase(`content/pages/${version}/${locale}/${file}.json`));
  if (!response.ok) {
    return null;
  }
  return response.json() as Promise<PageRecord>;
}

export async function loadSearch(version: string): Promise<SearchDoc[]> {
  const response = await fetch(withBase(`content/search/${version}.json`));
  if (!response.ok) {
    return [];
  }
  return response.json() as Promise<SearchDoc[]>;
}

export function isKnownVersion(versions: VersionInfo[], id: string): boolean {
  return versions.some((version) => version.id === id);
}
