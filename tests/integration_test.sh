#!/bin/bash
# Integration test runner for hyprbar
# Tests all example configs in screenshot mode

set -euo pipefail

BIN="./bin/hyprbar"
TMPDIR="/tmp/hyprbar-test-$$"
mkdir -p "$TMPDIR"

PASSED=0
FAILED=0

test_config() {
    local config=$1
    local name=$(basename "$config" .json)
    
    echo -n "Testing $name... "
    
    # Run with 8 second timeout (weather widget can take 5s)
    if timeout 8 "$BIN" --config "$config" --screenshot "$TMPDIR/$name.png" &>/dev/null; then
        # Check that screenshot was created
        if [ -f "$TMPDIR/$name.png" ]; then
            # Check that it's a valid PNG
            if file "$TMPDIR/$name.png" | grep -q "PNG image"; then
                echo "✓ PASS"
                ((PASSED++))
                return 0
            else
                echo "✗ FAIL (invalid PNG)"
                ((FAILED++))
                return 1
            fi
        else
            echo "✗ FAIL (no output)"
            ((FAILED++))
            return 1
        fi
    else
        local exit_code=$?
        echo "✗ FAIL (exit code: $exit_code)"
        ((FAILED++))
        return 1
    fi
}

echo "========================================="
echo "  Hyprbar Integration Tests"
echo "========================================="
echo

# Test all example configs (don't exit on failure)
set +e
for config in examples/config*.json; do
    test_config "$config"
done
set -e

echo
echo "========================================="
echo "Results: $PASSED passed, $FAILED failed"
echo "========================================="

# Cleanup
rm -rf "$TMPDIR"

exit $FAILED
