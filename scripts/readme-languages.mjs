// Keeps the README language chooser in sync and checks the translated READMEs.
//
//   node scripts/readme-languages.mjs          rewrite the chooser in README.md and docs/i18n/README.*.md
//   node scripts/readme-languages.mjs --check  report problems (exit code 1 if there are any)
//
// To add a language: add a row to LANGUAGES, write docs/i18n/README.<code>.md between the two
// <!-- languages:... --> markers (leave them empty), and run this script without arguments.
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

// Same languages as Caption Finder (English first, the rest in its order).
export const LANGUAGES = [
  ["en", "English"],
  ["ar", "العربية"],
  ["bn", "বাংলা"],
  ["zh-Hans", "简体中文"],
  ["fil", "Filipino"],
  ["fr", "Français"],
  ["de", "Deutsch"],
  ["hi", "हिन्दी"],
  ["id", "Bahasa Indonesia"],
  ["it", "Italiano"],
  ["ja", "日本語"],
  ["ko", "한국어"],
  ["fa", "فارسی"],
  ["pt-BR", "Português (Brasil)"],
  ["ru", "Русский"],
  ["es", "Español"],
  ["te", "తెలుగు"],
  ["th", "ไทย"],
  ["tr", "Türkçe"],
  ["vi", "Tiếng Việt"],
];
const COLUMNS = 5;
const START = "<!-- languages:start -->";
const END = "<!-- languages:end -->";
// The word "Language" in many scripts, so every reader can recognise the menu.
const LABEL =
  "🌐 Language · Idioma · Langue · Sprache · Язык · 语言 · 言語 · 언어 · भाषा · لغة";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const englishPath = resolve(root, "README.md");
const pathFor = (code) => resolve(root, code === "en" ? "README.md" : `docs/i18n/README.${code}.md`);

function chooser(current) {
  const href = (code) =>
    current === "en"
      ? code === "en" ? "README.md" : `docs/i18n/README.${code}.md`
      : code === "en" ? "../../README.md" : `README.${code}.md`;
  const cells = LANGUAGES.map(([code, name]) =>
    code === current
      ? `    <td align="center"><b>${name}</b></td>`
      : `    <td align="center"><a href="${href(code)}">${name}</a></td>`,
  );
  const rows = [];
  for (let i = 0; i < cells.length; i += COLUMNS)
    rows.push(`  <tr>\n${cells.slice(i, i + COLUMNS).join("\n")}\n  </tr>`);
  return [
    START,
    '<div align="center">',
    "",
    "<details>",
    `<summary><b>${LABEL}</b></summary>`,
    "<br>",
    "",
    "<table>",
    ...rows,
    "</table>",
    "",
    "</details>",
    "",
    "</div>",
    END,
  ].join("\n");
}

const eolOf = (text) => (text.includes("\r\n") ? "\r\n" : "\n");
const normalise = (text) => text.replace(/\r\n/g, "\n");

function withChooser(text, current) {
  const eol = eolOf(text);
  const body = normalise(text);
  const block = chooser(current);
  const a = body.indexOf(START);
  const b = body.indexOf(END);
  let out;
  if (a >= 0 && b > a) out = body.slice(0, a) + block + body.slice(b + END.length);
  else out = `${block}\n\n${body}`; // README.md gets the chooser above its title the first time
  return out.replace(/\n/g, eol);
}

// The English text a translation has to cover: everything before "## Build" (build notes
// stay English-only) plus the License section, without the chooser.
function referenceText() {
  const text = normalise(readFileSync(englishPath, "utf8"));
  const noChooser = text.replace(new RegExp(`${START}[\\s\\S]*?${END}\\n*`), "");
  const dev = noChooser.indexOf("\n## Build");
  const credits = noChooser.indexOf("\n## License");
  if (dev < 0 || credits < 0) throw Error("README.md has no Build or License heading");
  return noChooser.slice(0, dev) + "\n" + noChooser.slice(credits);
}

const spans = (text) => new Set([...text.matchAll(/`([^`\n]+)`/g)].map((m) => m[1]));
const urls = (text) => new Set([...text.matchAll(/https?:\/\/[^\s)"<>]+/g)].map((m) => m[0]));
const count = (text, re) => (text.match(re) || []).length;
const shape = (text) => ({
  h2: count(text, /^## /gm),
  h3: count(text, /^### /gm),
  bullets: count(text, /^\s*[-*] /gm),
  steps: count(text, /^\s*\d+\. /gm),
  rows: count(text, /^\|.*\|\s*$/gm),
});

function check() {
  const problems = [];
  const ref = referenceText();
  const refSpans = spans(ref);
  const refUrls = urls(ref);
  const refShape = shape(ref);
  const english = normalise(readFileSync(englishPath, "utf8"));
  if (english !== normalise(withChooser(english, "en"))) problems.push("README.md: the language chooser is out of date");
  for (const [code] of LANGUAGES) {
    if (code === "en") continue;
    const file = pathFor(code);
    const label = `docs/i18n/README.${code}.md`;
    if (!existsSync(file)) {
      problems.push(`${label}: missing`);
      continue;
    }
    const raw = normalise(readFileSync(file, "utf8"));
    if (raw !== normalise(withChooser(raw, code))) problems.push(`${label}: the language chooser is out of date`);
    const body = raw.replace(new RegExp(`${START}[\\s\\S]*?${END}\\n*`), "").replace(/<\/?div[^>]*>\n?/g, "");
    for (const s of refSpans) if (!spans(body).has(s)) problems.push(`${label}: missing code text \`${s}\``);
    for (const s of spans(body)) if (!refSpans.has(s)) problems.push(`${label}: unexpected code text \`${s}\``);
    for (const u of refUrls) if (!urls(body).has(u)) problems.push(`${label}: missing link ${u}`);
    for (const u of urls(body)) if (!refUrls.has(u)) problems.push(`${label}: unexpected link ${u}`);
    const got = shape(body);
    const want = { ...refShape, h2: refShape.h2 + 1 }; // one extra heading: the Build note
    for (const k of Object.keys(want))
      if (got[k] !== want[k]) problems.push(`${label}: ${k} is ${got[k]}, expected ${want[k]}`);
    for (const m of body.matchAll(/\]\((?!https?:|#)([^)\s]+)\)|src="([^"]+)"/g)) {
      const target = (m[1] ?? m[2]).split("#")[0];
      if (target && !existsSync(resolve(dirname(file), target))) problems.push(`${label}: broken path ${target}`);
    }
  }
  return problems;
}

if (process.argv.includes("--check")) {
  const problems = check();
  if (problems.length) {
    console.log(problems.join("\n"));
    console.log(`\n${problems.length} problem(s).`);
    process.exit(1);
  }
  console.log(`README languages are in sync (${LANGUAGES.length} languages).`);
} else {
  let changed = 0;
  for (const [code] of LANGUAGES) {
    const file = pathFor(code);
    if (!existsSync(file)) continue;
    const before = readFileSync(file, "utf8");
    const after = withChooser(before, code);
    if (after !== before) {
      writeFileSync(file, after);
      changed++;
    }
  }
  console.log(`Updated the chooser in ${changed} file(s).`);
}
