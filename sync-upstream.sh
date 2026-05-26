#!/usr/bin/env bash
# sync-upstream.sh — Sync changes from ppkantorski/Fizeau
# Applies upstream changes while protecting our customizations.
#
# Usage:
#   ./sync-upstream.sh           — sync and show what changed
#   ./sync-upstream.sh --dry-run — only show what would change, don't touch files

set -euo pipefail

UPSTREAM_REMOTE="upstream"
UPSTREAM_URL="https://github.com/ppkantorski/Fizeau.git"
UPSTREAM_BRANCH="master"
SYNC_FILE=".upstream-sync"
DRY_RUN=false

[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=true

# ─── Protected files ───────────────────────────────────────────────────────
# These are never overwritten from upstream (our custom code lives here).
PROTECTED=(
    # Overlay: complete rewrite with i18n + presets
    "overlay/src/main.cpp"
    "overlay/src/i18n.hpp"
    "overlay/src/presets.hpp"
    "overlay/Makefile"
    # Library: we use libryazhahand, not libultrahand
    "overlay/lib"
    ".gitmodules"
    # Build & CI
    "Makefile"
    ".github"
    # Docs
    "README.md"
)

# ─── Patches always re-applied after merge ─────────────────────────────────
# These fix issues that upstream doesn't have (imgui version, etc.)
apply_permanent_patches() {
    # imgui 1.89.4: SeparatorEx takes 1 arg, upstream uses 2
    if [[ -f application/src/gui.cpp ]]; then
        sed -i 's/SeparatorEx(ImGuiSeparatorFlags_Horizontal,[[:space:]]*[0-9.]*f)/SeparatorEx(ImGuiSeparatorFlags_Horizontal)/g' \
            application/src/gui.cpp
    fi
}

# ─── Helpers ───────────────────────────────────────────────────────────────
is_protected() {
    local f="$1"
    for p in "${PROTECTED[@]}"; do
        [[ "$f" == "$p" || "$f" == "$p/"* ]] && return 0
    done
    return 1
}

green()  { echo -e "\033[32m$*\033[0m"; }
yellow() { echo -e "\033[33m$*\033[0m"; }
red()    { echo -e "\033[31m$*\033[0m"; }
bold()   { echo -e "\033[1m$*\033[0m"; }

# ─── Setup remote ──────────────────────────────────────────────────────────
if ! git remote get-url "$UPSTREAM_REMOTE" &>/dev/null; then
    echo "→ Adding upstream remote: $UPSTREAM_URL"
    git remote add "$UPSTREAM_REMOTE" "$UPSTREAM_URL"
fi

echo "→ Fetching upstream ($UPSTREAM_URL)..."
git fetch "$UPSTREAM_REMOTE" --no-tags -q

UPSTREAM_HEAD=$(git rev-parse "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH")

# ─── Determine base commit ─────────────────────────────────────────────────
if [[ -f "$SYNC_FILE" ]]; then
    BASE=$(cat "$SYNC_FILE" | tr -d '[:space:]')
    # Validate the stored commit still exists
    if ! git cat-file -e "$BASE^{commit}" 2>/dev/null; then
        echo "Stored sync commit not found, falling back to merge-base."
        BASE=$(git merge-base HEAD "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH" 2>/dev/null \
            || git rev-list --max-parents=0 "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH")
    fi
    bold "Last synced upstream: ${BASE:0:12}"
else
    BASE=$(git merge-base HEAD "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH" 2>/dev/null \
        || git rev-list --max-parents=0 "$UPSTREAM_REMOTE/$UPSTREAM_BRANCH")
    bold "First run — base: ${BASE:0:12}"
fi

if [[ "$BASE" == "$UPSTREAM_HEAD" ]]; then
    green "✓ Already up to date with upstream."
    exit 0
fi

bold "\nNew upstream commits:"
git log --oneline "$BASE..$UPSTREAM_HEAD"

# Files added or modified upstream since base (deleted files ignored intentionally)
CHANGED=$(git diff --name-only --diff-filter=AM "$BASE" "$UPSTREAM_HEAD" || true)

if [[ -z "$CHANGED" ]]; then
    green "\n✓ No file changes to apply."
    echo "$UPSTREAM_HEAD" > "$SYNC_FILE"
    exit 0
fi

echo ""
bold "Applying changes..."
echo ""

APPLIED=0
SKIPPED=0
CONFLICTS=()

while IFS= read -r file; do
    [[ -z "$file" ]] && continue

    # ── Protected ──────────────────────────────────────────────────────────
    if is_protected "$file"; then
        yellow "  [skip protected]   $file"
        ((SKIPPED++)) || true
        continue
    fi

    # ── File missing upstream (shouldn't happen with --diff-filter=AM) ─────
    if ! git show "$UPSTREAM_HEAD:$file" &>/dev/null; then
        yellow "  [skip missing]     $file"
        continue
    fi

    if $DRY_RUN; then
        echo "  [would apply]      $file"
        continue
    fi

    # ── New file ───────────────────────────────────────────────────────────
    if [[ ! -f "$file" ]]; then
        mkdir -p "$(dirname "$file")"
        git show "$UPSTREAM_HEAD:$file" > "$file"
        green "  [add]              $file"
        ((APPLIED++)) || true
        continue
    fi

    # ── Modified file — 3-way merge ────────────────────────────────────────
    TMP=$(mktemp -d)
    TMPBASE="$TMP/base"
    TMPOURS="$TMP/ours"
    TMPTHEIRS="$TMP/theirs"

    git show "$BASE:$file" > "$TMPBASE" 2>/dev/null || touch "$TMPBASE"
    cp "$file" "$TMPOURS"
    git show "$UPSTREAM_HEAD:$file" > "$TMPTHEIRS"

    if git merge-file -q "$TMPOURS" "$TMPBASE" "$TMPTHEIRS" 2>/dev/null; then
        cp "$TMPOURS" "$file"
        green "  [merge ok]         $file"
        ((APPLIED++)) || true
    else
        # Copy merged result with conflict markers so user can resolve
        cp "$TMPOURS" "$file"
        red "  [conflict]         $file"
        CONFLICTS+=("$file")
    fi

    rm -rf "$TMP"

done <<< "$CHANGED"

if ! $DRY_RUN; then
    apply_permanent_patches
fi

# ─── Summary ───────────────────────────────────────────────────────────────
echo ""
echo "────────────────────────────────────────────"
bold "Applied: $APPLIED  |  Skipped: $SKIPPED  |  Conflicts: ${#CONFLICTS[@]}"

if [[ ${#CONFLICTS[@]} -gt 0 ]]; then
    echo ""
    red "Files with conflicts (resolve <<<<<<< markers, then commit):"
    for f in "${CONFLICTS[@]}"; do red "  $f"; done
    echo ""
    echo "After resolving conflicts run:"
    echo "  echo $UPSTREAM_HEAD > $SYNC_FILE"
    echo "  git add . && git commit -m 'chore: sync upstream'"
    exit 1
fi

if ! $DRY_RUN; then
    echo "$UPSTREAM_HEAD" > "$SYNC_FILE"
    green "✓ Sync point saved: ${UPSTREAM_HEAD:0:12}"
    echo ""
    echo "Review changes, then commit:"
    echo "  git diff"
    echo "  git add -p && git commit -m \"chore: sync upstream $(date +%Y-%m-%d)\""
fi
