import type { ReactNode } from "react";
import { isValidElement } from "react";
import Markdown from "react-markdown";
import remarkGfm from "remark-gfm";
import { withBase } from "@/lib/paths";
import { slugify } from "@/lib/slug";

function textOf(node: ReactNode): string {
  if (typeof node === "string" || typeof node === "number") {
    return String(node);
  }
  if (Array.isArray(node)) {
    return node.map(textOf).join("");
  }
  if (isValidElement<{ children?: ReactNode }>(node)) {
    return textOf(node.props.children);
  }
  return "";
}

function heading(tag: "h1" | "h2" | "h3") {
  return function Heading({ children }: { children?: ReactNode }) {
    const Tag = tag;
    const id = slugify(textOf(children));
    return <Tag id={id}>{children}</Tag>;
  };
}

function resolveHref(href: string | undefined): string | undefined {
  if (!href) {
    return href;
  }
  if (/^(https?:|mailto:|#)/i.test(href)) {
    return href;
  }
  return withBase(href.replace(/^\//, ""));
}

export function MarkdownView({ markdown }: { markdown: string }) {
  return (
    <div
      data-doc-scroll
      className="typeset typeset-docs max-w-[72ch] lg:h-full lg:min-h-0 lg:flex-1 lg:overflow-y-auto lg:pt-20 lg:pb-6"
    >
      <Markdown
        remarkPlugins={[remarkGfm]}
        components={{
          h1: heading("h1"),
          h2: heading("h2"),
          h3: heading("h3"),
          a: ({ href, children }) => <a href={resolveHref(href)}>{children}</a>,
          img: ({ src, alt }) => <img src={resolveHref(src)} alt={alt ?? ""} />,
        }}
      >
        {markdown}
      </Markdown>
    </div>
  );
}