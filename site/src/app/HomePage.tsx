import { motion, useReducedMotion } from "motion/react";
import { useEffect } from "react";
import { Link } from "react-router-dom";
import { copy } from "@/app/copy";
import { SiteHeader } from "@/components/chrome/SiteHeader";
import { Button, ButtonLink } from "@/components/motion/button/base";
import { TiltCard } from "@/components/motion/tilt-card";
import { EASE_OUT } from "@/lib/ease";
import type { VersionInfo } from "@/lib/content";
import { storeLocale } from "@/lib/locale";
import { docPath, withBase, type Locale } from "@/lib/paths";

type Props = {
  locale: Locale;
  versions: VersionInfo[];
};

export function HomePage({ locale, versions }: Props) {
  useEffect(() => {
    storeLocale(locale);
  }, [locale]);
  const text = copy[locale];
  const reduce = useReducedMotion();
  const shots = [
    {
      title: text.settings,
      body: text.settingsBody,
      src: withBase("assets/manual/latest/luma-settings-display.png"),
    },
    {
      title: text.notes,
      body: text.notesBody,
      src: withBase("assets/manual/latest/luma-notes-menu.png"),
    },
    {
      title: text.dots,
      body: text.dotsBody,
      src: withBase("assets/manual/latest/luma-dots-matrix-edit.png"),
    },
  ];

  return (
    <div className="min-h-dvh">
      <SiteHeader locale={locale} versions={versions} />
      <main className="relative min-h-dvh overflow-hidden">
        <div
          aria-hidden
          className="pointer-events-none absolute inset-0"
          style={{
            background:
              "radial-gradient(ellipse 70% 50% at 18% 28%, color-mix(in srgb, var(--color-fuji) 22%, transparent), transparent 70%), radial-gradient(ellipse 45% 40% at 88% 78%, color-mix(in srgb, var(--color-byakuroku) 16%, transparent), transparent 68%)",
          }}
        />
        <motion.div
          className="relative mx-auto flex min-h-dvh w-full max-w-6xl flex-col justify-center gap-12 px-4 pb-16 pt-24 lg:flex-row lg:items-center lg:gap-16"
          initial={reduce ? false : { opacity: 0.72, filter: "blur(10px)", y: 18 }}
          animate={{ opacity: 1, filter: "blur(0px)", y: 0 }}
          transition={{ duration: 0.7, ease: EASE_OUT }}
        >
          <div className="max-w-xl shrink-0">
            <img src={withBase("luma-logo.svg")} alt="" width={88} height={88} />
            <h1 className="mt-6 text-5xl font-medium tracking-[-0.03em] text-[color:var(--fg)] md:text-6xl">
              {text.title}
            </h1>
            <p className="mt-5 max-w-md text-lg text-[color:var(--fg-muted)]">{text.offer}</p>
            <ul className="mt-8 space-y-4">
              {shots.map((shot) => (
                <li key={shot.title}>
                  <p className="font-medium tracking-[-0.02em] text-[color:var(--fg)]">{shot.title}</p>
                  <p className="mt-1 text-sm text-[color:var(--fg-muted)]">{shot.body}</p>
                </li>
              ))}
            </ul>
            <div className="mt-10 flex flex-wrap gap-3">
              <Link to={docPath("latest", locale, "getting-started")} className="no-underline">
                <Button type="button">{text.read}</Button>
              </Link>
              <ButtonLink
                href="https://github.com/zhoux77899/luma"
                variant="outline"
                target="_blank"
                rel="noreferrer"
              >
                {text.github}
              </ButtonLink>
            </div>
          </div>
          <div className="relative w-full min-w-0 flex-1 lg:min-h-[28rem]">
            <div className="flex gap-3 overflow-x-auto pb-2 lg:block lg:overflow-visible lg:pb-0">
              {shots.map((shot, index) => (
                <TiltCard
                  key={shot.title}
                  className={`min-w-[13rem] shrink-0 bg-[color:var(--bg-card)]/20 shadow-[0_18px_40px_-24px_rgba(8,8,8,0.55)] lg:absolute lg:min-w-0 lg:w-[min(100%,22rem)] ${
                    index === 0
                      ? "lg:left-0 lg:top-0 lg:z-20"
                      : index === 1
                        ? "lg:right-0 lg:top-[4.5rem] lg:z-10"
                        : "lg:bottom-0 lg:left-[12%] lg:z-30"
                  }`}
                >
                  <figure className="p-2">
                    <img
                      src={shot.src}
                      alt=""
                      className="w-full rounded-xl bg-[color:var(--color-kuro)]"
                    />
                    <figcaption className="sr-only">{shot.title}</figcaption>
                  </figure>
                </TiltCard>
              ))}
            </div>
          </div>
        </motion.div>
      </main>
    </div>
  );
}
