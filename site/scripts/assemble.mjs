import { execFileSync } from "node:child_process";
import {
  copyFileSync,
  existsSync,
  mkdirSync,
  readdirSync,
  readFileSync,
  rmSync,
  statSync,
  writeFileSync,
} from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const siteRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const repoRoot = path.resolve(siteRoot, "..");
const contentRoot = path.join(siteRoot, "public", "content");
const assetsRoot = path.join(siteRoot, "public", "assets", "manual");
const sourceManual = path.join(repoRoot, "docs", "user-manual");

const LOCALES = ["en", "zh"];

function git(args, options = {}) {
  return execFileSync("git", args, {
    cwd: repoRoot,
    encoding: options.encoding ?? "utf8",
    stdio: ["ignore", "pipe", "pipe"],
    maxBuffer: 16 * 1024 * 1024,
  });
}

function gitBuffer(args) {
  return execFileSync("git", args, {
    cwd: repoRoot,
    encoding: "buffer",
    stdio: ["ignore", "pipe", "pipe"],
    maxBuffer: 16 * 1024 * 1024,
  });
}

function listReleases() {
  try {
    const raw = execFileSync(
      "gh",
      ["release", "list", "--limit", "100", "--json", "tagName,isDraft,isPrerelease"],
      { cwd: repoRoot, encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] },
    );
    return JSON.parse(raw)
      .filter((release) => !release.isDraft)
      .map((release) => ({
        id: release.tagName,
        label: release.tagName,
        prerelease: Boolean(release.isPrerelease),
      }));
  } catch {
    const tags = git(["tag", "-l", "v*"])
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter(Boolean);
    return tags.map((tag) => ({
      id: tag,
      label: tag,
      prerelease: tag.includes("-"),
    }));
  }
}

function compareVersions(left, right) {
  const parse = (id) => {
    const match = /^v(\d+)\.(\d+)\.(\d+)/.exec(id);
    if (!match) {
      return [0, 0, 0, id];
    }
    return [Number(match[1]), Number(match[2]), Number(match[3]), id];
  };
  const a = parse(left);
  const b = parse(right);
  for (let i = 0; i < 3; i += 1) {
    if (a[i] !== b[i]) {
      return b[i] - a[i];
    }
  }
  return String(b[3]).localeCompare(String(a[3]));
}

function tagHasUserGuide(ref) {
  try {
    git(["cat-file", "-e", `${ref}:docs/user-manual/zh/README.md`]);
    return true;
  } catch {
    return false;
  }
}

function listGitFiles(ref, prefix) {
  const listed = git(["ls-tree", "-r", "--name-only", ref, "--", prefix]);
  return listed
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter(Boolean);
}

function walkFiles(rootDir) {
  const files = [];
  const visit = (dir) => {
    for (const entry of readdirSync(dir)) {
      const full = path.join(dir, entry);
      if (statSync(full).isDirectory()) {
        visit(full);
      } else {
        files.push(path.relative(rootDir, full).split(path.sep).join("/"));
      }
    }
  };
  visit(rootDir);
  return files;
}

function firstHeading(markdown) {
  const match = /^#\s+(.+)$/m.exec(markdown);
  return match ? match[1].trim() : "";
}

function headings(markdown) {
  const items = [];
  for (const line of markdown.split(/\r?\n/)) {
    const match = /^(#{2,3})\s+(.+)$/.exec(line);
    if (!match) {
      continue;
    }
    const text = match[2].trim();
    items.push({
      level: match[1].length,
      text,
      id: slugify(text),
    });
  }
  return items;
}

function slugify(text) {
  return text
    .toLowerCase()
    .replace(/[`*_]/g, "")
    .replace(/[^\p{L}\p{N}]+/gu, "-")
    .replace(/^-+|-+$/g, "");
}

function searchText(markdown) {
  return markdown
    .replace(/```[\s\S]*?```/g, " ")
    .replace(/!\[[^\]]*\]\([^)]+\)/g, " ")
    .replace(/\[[^\]]*\]\([^)]+\)/g, (link) => {
      const label = /^\[([^\]]*)\]/.exec(link);
      return label ? label[1] : " ";
    })
    .replace(/[#>*`_~\-]/g, " ")
    .replace(/\s+/g, " ")
    .trim();
}

function rewriteMarkdown(markdown, { version, locale, slug, basePath }) {
  return markdown.replace(/(!?)\[[^\]]*\]\(([^)]+)\)/g, (full, bang, href) => {
    const trimmed = href.trim();
    if (/^(https?:|mailto:|#)/i.test(trimmed)) {
      return full;
    }

    const label = full.slice(0, full.lastIndexOf("("));
    if (/\.(png|jpe?g|gif|svg|webp)$/i.test(trimmed)) {
      const name = trimmed.split("/").pop();
      return `${label}(${basePath}assets/manual/${version}/${name})`;
    }

    if (!trimmed.includes(".md")) {
      return full;
    }

    let nextLocale = locale;
    if (/(^|\/)en\//.test(trimmed) || trimmed.includes("../en/")) {
      nextLocale = "en";
    } else if (/(^|\/)zh\//.test(trimmed) || trimmed.includes("../zh/")) {
      nextLocale = "zh";
    }

    const fromDir = slug.includes("/") ? slug.slice(0, slug.lastIndexOf("/")) : "";
    const joined = path.posix.normalize([fromDir, trimmed].filter(Boolean).join("/"));
    let nextSlug = joined
      .replace(/^(\.\.\/)+/, "")
      .replace(/^\/+/, "")
      .replace(/^en\//, "")
      .replace(/^zh\//, "")
      .replace(/\.md$/i, "")
      .replace(/(^|\/)README$/i, "");
    if (!nextSlug || nextSlug === "." || nextSlug === "README") {
      nextSlug = "getting-started";
    }
    return `${label}(${basePath}${version}/${nextLocale}/${nextSlug})`;
  });
}

function writePage({ version, locale, slug, markdown, hasUserGuide }) {
  const dir = path.join(contentRoot, "pages", version, locale);
  mkdirSync(dir, { recursive: true });
  const title = firstHeading(markdown) || slug;
  const body = rewriteMarkdown(markdown, {
    version,
    locale,
    slug,
    basePath: "/",
  });
  writeFileSync(
    path.join(dir, `${slug.replaceAll("/", "__")}.json`),
    `${JSON.stringify(
      {
        slug,
        title,
        markdown: body,
        headings: headings(body),
        hasUserGuide,
      },
      null,
      2,
    )}\n`,
  );
  return { slug, title, text: searchText(body), locale, version };
}

function copyManualAssets(version, files, readFile) {
  const dest = path.join(assetsRoot, version);
  mkdirSync(dest, { recursive: true });
  for (const file of files) {
    if (!file.startsWith("docs/user-manual/assets/") && !file.startsWith("assets/")) {
      continue;
    }
    const name = file.split("/").pop();
    if (!name || name === ".gitkeep") {
      continue;
    }
    writeFileSync(path.join(dest, name), readFile(file));
  }
}

function collectWorkingTreeManual() {
  const files = walkFiles(sourceManual).map((rel) => `docs/user-manual/${rel}`);
  return {
    files,
    readFile: (file) => {
      const abs = path.join(repoRoot, file);
      return readFileSync(abs);
    },
    readText: (file) => readFileSync(path.join(repoRoot, file), "utf8"),
  };
}

function collectGitManual(ref) {
  const files = listGitFiles(ref, "docs/user-manual");
  return {
    files,
    readFile: (file) => gitBuffer(["show", `${ref}:${file}`]),
    readText: (file) => git(["show", `${ref}:${file}`]),
  };
}

function stubMarkdown(locale, version) {
  if (locale === "zh") {
    return `# 该版本无用户指南

\`${version}\` 的发行 tag 里没有 User guide。阅读 [当前稿 latest](/latest/zh/getting-started)。
`;
  }
  return `# This release has no User guide

The \`${version}\` tag does not contain a User guide. Read the [current latest draft](/latest/en/getting-started).
`;
}

function publishManual(version, source, hasUserGuide) {
  const searchDocs = [];
  const manifest = { en: [], zh: [] };

  if (!hasUserGuide) {
    for (const locale of LOCALES) {
      const page = writePage({
        version,
        locale,
        slug: "getting-started",
        markdown: stubMarkdown(locale, version),
        hasUserGuide: false,
      });
      searchDocs.push(page);
      manifest[locale].push({ slug: page.slug, title: page.title });
    }
    writeManifest(version, manifest);
    writeSearch(version, searchDocs);
    return;
  }

  copyManualAssets(
    version,
    source.files.filter((file) => file.includes("/assets/")),
    source.readFile,
  );

  for (const locale of LOCALES) {
    const prefix = `docs/user-manual/${locale}/`;
    for (const file of source.files) {
      if (!file.startsWith(prefix) || !file.endsWith(".md")) {
        continue;
      }
      const rel = file.slice(prefix.length);
      if (rel === "README.md" || rel.endsWith("/README.md")) {
        continue;
      }
      const slug = rel.replace(/\.md$/i, "");
      const page = writePage({
        version,
        locale,
        slug,
        markdown: source.readText(file),
        hasUserGuide: true,
      });
      searchDocs.push(page);
      manifest[locale].push({ slug: page.slug, title: page.title });
    }
    manifest[locale].sort((a, b) => a.slug.localeCompare(b.slug));
  }

  writeManifest(version, manifest);
  writeSearch(version, searchDocs);
}

function writeManifest(version, manifest) {
  const dir = path.join(contentRoot, "pages", version);
  mkdirSync(dir, { recursive: true });
  writeFileSync(path.join(dir, "manifest.json"), `${JSON.stringify(manifest, null, 2)}\n`);
}

function writeSearch(version, docs) {
  const dir = path.join(contentRoot, "search");
  mkdirSync(dir, { recursive: true });
  writeFileSync(path.join(dir, `${version}.json`), `${JSON.stringify(docs, null, 2)}\n`);
}

function resetOutput() {
  rmSync(contentRoot, { recursive: true, force: true });
  rmSync(assetsRoot, { recursive: true, force: true });
  mkdirSync(contentRoot, { recursive: true });
  mkdirSync(assetsRoot, { recursive: true });
}

function main() {
  if (!existsSync(sourceManual)) {
    throw new Error("docs/user-manual is missing on this checkout");
  }

  resetOutput();

  const releases = listReleases().sort((a, b) => compareVersions(a.id, b.id));
  const versions = [
    {
      id: "latest",
      label: "latest",
      ref: "main",
      hasUserGuide: true,
      prerelease: false,
    },
  ];

  publishManual("latest", collectWorkingTreeManual(), true);

  for (const release of releases) {
    const hasUserGuide = tagHasUserGuide(release.id);
    versions.push({
      id: release.id,
      label: release.label,
      ref: release.id,
      hasUserGuide,
      prerelease: release.prerelease,
    });
    publishManual(
      release.id,
      hasUserGuide ? collectGitManual(release.id) : { files: [], readFile() {}, readText() { return ""; } },
      hasUserGuide,
    );
  }

  const versionsFile = `${JSON.stringify({ latest: "latest", versions }, null, 2)}\n`;
  writeFileSync(path.join(contentRoot, "versions.json"), versionsFile);
  writeFileSync(path.join(siteRoot, "public", "versions.json"), versionsFile);

  const logoSrc = path.join(repoRoot, "assets", "luma-logo", "luma-logo.svg");
  const logoDest = path.join(siteRoot, "public", "luma-logo.svg");
  mkdirSync(path.dirname(logoDest), { recursive: true });
  copyFileSync(logoSrc, logoDest);

  console.log(`Assembled ${versions.length} versions into public/content`);
}

main();
