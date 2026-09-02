#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required. Install it with Homebrew or apt." >&2
  exit 1
fi

source_files=()
while IFS= read -r source_file; do
  source_files+=("$source_file")
done < <(git ls-files '*.cc' '*.h' '*.cpp')
if [[ ${#source_files[@]} -gt 0 ]]; then
  clang-format -i "${source_files[@]}"
fi
