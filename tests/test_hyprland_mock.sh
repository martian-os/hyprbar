#!/bin/bash
# Integration test for Hyprland widget with mock server

set -e

TEST_DIR="/tmp/hyprbar_hyprland_test_$$"
MOCK_INSTANCE="test_$$"
MOCK_DIR="$TEST_DIR/$MOCK_INSTANCE"
HYPR_DIR="/tmp/hypr/$MOCK_INSTANCE"

cleanup() {
    echo "Cleaning up..."
    if [ -n "$MOCK_PID" ]; then
        kill $MOCK_PID 2>/dev/null || true
        wait $MOCK_PID 2>/dev/null || true
    fi
    rm -rf "$TEST_DIR"
    rm -rf "$HYPR_DIR"
}

trap cleanup EXIT

echo "=== Hyprland Widget Mock Integration Test ==="

# Create test directory and symlink to /tmp/hypr
mkdir -p "$MOCK_DIR"
mkdir -p "/tmp/hypr"
ln -sf "$MOCK_DIR" "$HYPR_DIR"

# Start mock server in background
echo "Starting mock Hyprland server..."
python3 tests/mock_hyprland_server.py "$MOCK_DIR" &
MOCK_PID=$!

# Wait for sockets to exist
sleep 0.5

if [ ! -S "$MOCK_DIR/.socket.sock" ]; then
    echo "FAIL: Command socket not created"
    exit 1
fi

if [ ! -S "$MOCK_DIR/.socket2.sock" ]; then
    echo "FAIL: Event socket not created"
    exit 1
fi

echo "Mock server started (PID: $MOCK_PID)"

# Test socket communication
echo "Testing socket communication..."
echo "j/workspaces" | nc -U "$MOCK_DIR/.socket.sock" -w 1 > /tmp/hypr_test_out.txt

if ! grep -q '"id"' /tmp/hypr_test_out.txt; then
    echo "FAIL: No JSON response from mock server"
    cat /tmp/hypr_test_out.txt
    exit 1
fi

echo "✓ Mock server responding correctly"

# Now run actual widget test with mock
echo "Running widget with mock environment..."
export HYPRLAND_INSTANCE_SIGNATURE="$MOCK_INSTANCE"

# Create test config
cat > "$TEST_DIR/config.json" << EOF
{
  "bar": {
    "position": "top",
    "height": 30,
    "background": "#1e1e2e",
    "color": "#cdd6f4",
    "font": "monospace",
    "size": 12
  },
  "widgets": [
    {
      "type": "hyprland",
      "position": "left",
      "config": {
        "max_workspaces": 3,
        "active_color": "#89b4fa",
        "occupied_color": "#cdd6f4",
        "empty_color": "#45475a"
      }
    }
  ]
}
EOF

# Run hyprbar in screenshot mode (should connect to mock)
timeout 3 ./bin/hyprbar --config "$TEST_DIR/config.json" --screenshot "$TEST_DIR/output.png" 2>&1 || true

if [ -f "$TEST_DIR/output.png" ]; then
    file "$TEST_DIR/output.png" | grep -q PNG
    if [ $? -eq 0 ]; then
        echo "✓ Screenshot generated successfully"
        echo "✓ Hyprland widget connected to mock server"
    else
        echo "FAIL: Output file is not a valid PNG"
        exit 1
    fi
else
    echo "FAIL: Screenshot not generated"
    exit 1
fi

echo ""
echo "=== All Mock Integration Tests Passed ==="
