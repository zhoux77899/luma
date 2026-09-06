import { copyFileSync, existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const outDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../build/docs-site");
const index = path.join(outDir, "index.html");
if (!existsSync(index)) {
  throw new Error(`Missing ${index}`);
}
copyFileSync(index, path.join(outDir, "404.html"));
console.log("Wrote 404.html for GitHub Pages SPA fallback");
