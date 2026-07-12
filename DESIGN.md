# DESIGN.md — bobbycar onboard tool

Single source of truth: `logger/internal/api/ui/theme.css` (design tokens,
shared components). All pages (dashboard `/`, live `/live/`, login) consume it.

## Theme

Dark by default — the original logger palette (`#14171c` bg, `#1c2027`
surface, `#2c313a` borders, `#e6e8eb` text, `#6ca4f5` accent, `#3fb950` ok,
`#d29922` warn, `#e05252` bad, `#e6b422` flag). A manual light mode for
bright outside testing days: ☀ toggle in the topbar, persisted in
`localStorage("theme")`, applied via `:root[data-theme="light"]` token
overrides. No `prefers-color-scheme` — the switch is situational, not
ambient. Toggling reloads the page so canvas charts re-init with the new
palette.

## Rules

- Plain over branded: no logo mark, no pill-shaped nav, no rounded-full
  chips beyond the original 12px badges
- Sections are bordered surface cards (the original look); tables inside
  with `#262b34` row hairlines
- Data values: `--mono` + `tabular-nums` (class `num`)
- Canvas charts can't read CSS vars from JS reliably — `chartTheme()` in
  each page script mirrors theme.css hex values per theme; keep in sync
- Boolean pills (live page): on = green, off = red (original semantics);
  `alert` class inverts for panic (red when on)
- Dot vocabulary in status chips: red = no data/bad (original), green ok,
  amber warn

## Mobile app shell

Sticky topbar (wordmark, logs|live nav, theme toggle, logout), status-chip
strip, fixed bottom tab bar ≤700px with safe-area insets, ≥44px touch
targets, installable (manifest.webmanifest + icon.svg, standalone).
