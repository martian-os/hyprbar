#!/bin/bash
# Test: Ensure root directory only contains README.md and AGENTS.md
# All other documentation should be in /docs directory

set -e

cd "$(dirname "$0")/.."

echo "Checking for stray markdown files in root directory..."

# Find all .md files in root (maxdepth 1)
STRAY_FILES=$(find . -maxdepth 1 -name "*.md" -type f ! -name "README.md" ! -name "AGENTS.md")

if [ -n "$STRAY_FILES" ]; then
    echo "❌ Found markdown files in root directory that should be in /docs:"
    echo "$STRAY_FILES"
    echo ""
    echo "Please move these files to the /docs directory:"
    echo "  mkdir -p docs"
    for file in $STRAY_FILES; do
        basename_file=$(basename "$file")
        echo "  mv $basename_file docs/$basename_file"
    done
    echo ""
    echo "Root directory should only contain:"
    echo "  - README.md (project overview)"
    echo "  - AGENTS.md (AI agent instructions)"
    echo ""
    echo "All other documentation belongs in /docs/"
    exit 1
fi

echo "✅ Root directory structure is clean"
exit 0
