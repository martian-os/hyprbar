# Quality Gates & Best Practices Review
## Hyprbar C++ Project Quality Analysis

_By a 20-year C++ veteran obsessed with quality_

---

## Executive Summary

**Current State:** Good foundation, but several critical quality gaps exist.

**Strengths:**
- ✅ Pre-commit hooks with formatting and tests
- ✅ Test coverage tracking (51% overall, 89%+ for core widgets)
- ✅ CI/CD with build and lint checks
- ✅ Modern C++17 codebase
- ✅ Clear separation of concerns

**Critical Gaps:**
- ❌ No static analysis (cppcheck, clang-tidy)
- ❌ No memory leak detection (valgrind, sanitizers)
- ❌ No code complexity metrics
- ❌ Missing const correctness in several places
- ❌ No performance benchmarks
- ❌ Insufficient error handling in critical paths

---

## Recommended Quality Gates

### 1. Static Analysis (CRITICAL)

**Add clang-tidy to CI:**

```yaml
# .github/workflows/ci.yml
  static-analysis:
    name: Static Analysis
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install clang-tidy
        run: sudo apt-get install -y clang-tidy
      - name: Run clang-tidy
        run: |
          clang-tidy src/**/*.cpp \
            --checks='*,-fuchsia-*,-google-*,-llvm-*,-modernize-use-trailing-return-type' \
            -- -std=c++17 -I include
```

**Add cppcheck:**

```yaml
      - name: Install cppcheck
        run: sudo apt-get install -y cppcheck
      - name: Run cppcheck
        run: |
          cppcheck --enable=all --inconclusive --std=c++17 \
            --suppress=missingIncludeSystem \
            --inline-suppr \
            --error-exitcode=1 \
            src/
```

**Makefile targets:**

```makefile
.PHONY: lint-tidy lint-cppcheck lint-all

lint-tidy:
	@echo "Running clang-tidy..."
	@clang-tidy src/**/*.cpp \
		--checks='*,-fuchsia-*,-google-*,-llvm-*,-modernize-use-trailing-return-type' \
		-- -std=c++17 -I include

lint-cppcheck:
	@echo "Running cppcheck..."
	@cppcheck --enable=all --inconclusive --std=c++17 \
		--suppress=missingIncludeSystem \
		--inline-suppr \
		src/

lint-all: format-check lint-tidy lint-cppcheck
	@echo "✅ All linting passed"
```

---

### 2. Memory Safety (CRITICAL)

**Add sanitizers to test builds:**

```makefile
# Makefile additions
CXXFLAGS_ASAN = -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g
CXXFLAGS_TSAN = -fsanitize=thread -g

.PHONY: test-asan test-tsan test-valgrind

test-asan: CXXFLAGS += $(CXXFLAGS_ASAN)
test-asan: LDFLAGS += $(CXXFLAGS_ASAN)
test-asan: clean $(TEST_TARGET)
	@echo "Running tests with AddressSanitizer..."
	@ASAN_OPTIONS=detect_leaks=1 $(TEST_TARGET)

test-tsan: CXXFLAGS += $(CXXFLAGS_TSAN)
test-tsan: LDFLAGS += $(CXXFLAGS_TSAN)
test-tsan: clean $(TEST_TARGET)
	@echo "Running tests with ThreadSanitizer..."
	@$(TEST_TARGET)

test-valgrind: $(TEST_TARGET)
	@echo "Running tests under Valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 \
		--suppressions=valgrind.supp \
		$(TEST_TARGET)
```

**CI integration:**

```yaml
  memory-safety:
    name: Memory Safety Tests
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies
        run: sudo apt-get install -y valgrind clang libwayland-dev libcairo2-dev libpango1.0-dev libdbus-1-dev libgdk-pixbuf2.0-dev librsvg2-dev
      - name: Test with AddressSanitizer
        run: make test-asan
      - name: Test with ThreadSanitizer
        run: make test-tsan
      - name: Test with Valgrind
        run: make test-valgrind
```

---

### 3. Code Coverage Enforcement

**Current:** 51% overall (good), but no enforcement.

**Add coverage gate:**

```yaml
  coverage:
    name: Coverage Check
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies
        run: sudo apt-get install -y lcov gcov clang libwayland-dev libcairo2-dev libpango1.0-dev libdbus-1-dev libgdk-pixbuf2.0-dev librsvg2-dev
      - name: Run coverage
        run: make coverage
      - name: Check coverage thresholds
        run: |
          COVERAGE=$(lcov --summary coverage/coverage.info | grep lines | awk '{print $2}' | sed 's/%//')
          if (( $(echo "$COVERAGE < 50.0" | bc -l) )); then
            echo "❌ Coverage $COVERAGE% is below 50% threshold"
            exit 1
          fi
          echo "✅ Coverage $COVERAGE% meets threshold"
      - name: Upload to Codecov
        uses: codecov/codecov-action@v3
        with:
          files: ./coverage/coverage.info
```

**Per-component thresholds:**

```bash
# scripts/check-coverage.sh
#!/bin/bash
set -e

declare -A THRESHOLDS=(
  ["src/widgets/"]=85
  ["src/core/"]=70
  ["src/rendering/"]=65
  ["src/config/"]=80
)

for dir in "${!THRESHOLDS[@]}"; do
  threshold=${THRESHOLDS[$dir]}
  coverage=$(lcov --summary coverage/coverage.info --rc lcov_branch_coverage=1 | \
             grep "$(basename $dir)" | awk '{print $2}' | sed 's/%//')
  
  if (( $(echo "$coverage < $threshold" | bc -l) )); then
    echo "❌ $dir: $coverage% < $threshold%"
    exit 1
  fi
  echo "✅ $dir: $coverage% >= $threshold%"
done
```

---

### 4. Code Complexity Metrics

**Add complexity checks:**

```makefile
.PHONY: complexity

complexity:
	@echo "Checking code complexity (cyclomatic)..."
	@lizard -l cpp -w src/ -CCN 15 -L 200 -a 5
```

**Thresholds:**
- Cyclomatic Complexity: < 15 per function
- Lines per function: < 200
- Parameters per function: < 5

**CI enforcement:**

```yaml
      - name: Install lizard
        run: pip install lizard
      - name: Check complexity
        run: lizard -l cpp -w src/ -CCN 15 -L 200 -a 5 --fail_on_error
```

---

### 5. Documentation Quality

**Add documentation checks:**

```yaml
  documentation:
    name: Documentation Check
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install doxygen
        run: sudo apt-get install -y doxygen graphviz
      - name: Check doxygen warnings
        run: |
          doxygen Doxyfile 2>&1 | tee doxygen.log
          if grep -q 'warning:' doxygen.log; then
            echo "❌ Doxygen warnings found"
            exit 1
          fi
```

**Doxyfile configuration:**

```doxyfile
PROJECT_NAME           = "Hyprbar"
OUTPUT_DIRECTORY       = docs/api
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
WARN_IF_UNDOCUMENTED   = YES
WARN_AS_ERROR          = YES
EXTRACT_ALL            = NO
EXTRACT_PRIVATE        = NO
```

---

### 6. Dependency Management

**Lock dependencies with explicit versions:**

```bash
# deps/versions.txt
wayland-protocols=1.31
cairo=1.18.0
pango=1.50.0
dbus=1.14.0
gdk-pixbuf=2.42.0
librsvg=2.56.0
```

**Vulnerability scanning:**

```yaml
  security:
    name: Security Scan
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Run Trivy vulnerability scanner
        uses: aquasecurity/trivy-action@master
        with:
          scan-type: 'fs'
          scan-ref: '.'
          format: 'sarif'
          output: 'trivy-results.sarif'
      - name: Upload to GitHub Security
        uses: github/codeql-action/upload-sarif@v2
        with:
          sarif_file: 'trivy-results.sarif'
```

---

### 7. Performance Benchmarks

**Add benchmark suite:**

```cpp
// tests/benchmarks/render_benchmark.cpp
#include <benchmark/benchmark.h>
#include "hyprbar/rendering/renderer.h"

static void BM_RenderFullBar(benchmark::State& state) {
  Renderer renderer(1920, 30);
  for (auto _ : state) {
    renderer.render();
    benchmark::DoNotOptimize(renderer.get_surface());
  }
}
BENCHMARK(BM_RenderFullBar);

static void BM_WidgetUpdate(benchmark::State& state) {
  WidgetManager wm;
  // ... setup
  for (auto _ : state) {
    wm.update_widget("clock", "12:34:56");
  }
}
BENCHMARK(BM_WidgetUpdate);
```

**CI regression detection:**

```yaml
  benchmarks:
    name: Performance Benchmarks
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install google-benchmark
        run: sudo apt-get install -y libbenchmark-dev
      - name: Build benchmarks
        run: make benchmarks
      - name: Run benchmarks
        run: ./bin/benchmarks --benchmark_format=json > bench-results.json
      - name: Compare with baseline
        run: python scripts/compare-benchmarks.py bench-results.json baseline-bench.json
```

---

### 8. API Stability Checks

**Track ABI compatibility:**

```yaml
  abi-check:
    name: ABI Compatibility
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
      - name: Install abi-compliance-checker
        run: sudo apt-get install -y abi-compliance-checker
      - name: Check ABI
        run: |
          git checkout HEAD~1
          make && mv bin/libhyprbar.so bin/libhyprbar-old.so
          git checkout -
          make
          abi-compliance-checker -lib hyprbar \
            -old bin/libhyprbar-old.so \
            -new bin/libhyprbar.so
```

---

### 9. Code Quality Metrics Dashboard

**Recommended tools:**
- **SonarQube/SonarCloud** - Code quality tracking
- **Codecov** - Coverage visualization
- **Coverity** - Static analysis
- **Codacy** - Automated code reviews

**GitHub README badges:**

```markdown
![CI Status](https://github.com/martian-os/hyprbar/actions/workflows/ci.yml/badge.svg)
![Coverage](https://codecov.io/gh/martian-os/hyprbar/branch/main/graph/badge.svg)
![Code Quality](https://api.codacy.com/project/badge/Grade/...)
![License](https://img.shields.io/github/license/martian-os/hyprbar)
```

---

## Code Issues Found (Manual Review)

### Critical Issues

**1. Missing null checks in D-Bus code:**

```cpp
// src/widgets/tray_widget.cpp:442
DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
if (dbus_error_is_set(&err) || !conn) {  // Good!
  // ...
}

// But later at line 456:
DBusMessage* msg = dbus_message_new_method_call(...);
// No null check! If this fails, we crash.
if (!msg) {  // ADD THIS
  dbus_connection_unref(conn);
  return;
}
```

**Fix:** Add null checks after all D-Bus allocations.

**2. Potential use-after-free in event loop:**

```cpp
// src/core/event_loop.cpp
void EventLoop::remove_fd(int fd) {
  fd_handlers_.erase(fd);
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}
```

If the handler is holding a reference to itself and calls `remove_fd()` from its callback, we have UAF.

**Fix:** Defer removals:

```cpp
std::vector<int> pending_removals_;

void remove_fd(int fd) {
  pending_removals_.push_back(fd);
}

void run() {
  while (running_) {
    // ... handle events
    
    // Process removals after event handling
    for (int fd : pending_removals_) {
      fd_handlers_.erase(fd);
      epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    }
    pending_removals_.clear();
  }
}
```

**3. Thread safety in TrayWidget:**

```cpp
// fetch_tray_icons() spawns thread that modifies icons_
// render() reads icons_
// Both use mutex (good), but update_icons() doesn't!

void TrayWidget::update_icons() {
  // MISSING: std::lock_guard<std::mutex> lock(icons_mutex_);
  icons_.clear();
  fetch_tray_icons();
}
```

**Fix:** Add mutex lock.

**4. RAII violations:**

```cpp
// src/widgets/tray_widget.cpp:184
GError* error = nullptr;
GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(..., &error);
if (pixbuf) {
  cairo_surface_t* surface = pixbuf_to_cairo_surface(pixbuf);
  g_object_unref(pixbuf);
  return surface;  // Good
}
if (error) {
  g_error_free(error);  // Good
}
// But what if another code path is added that returns early?
```

**Fix:** Use RAII wrappers:

```cpp
struct GErrorDeleter { void operator()(GError* e) { g_error_free(e); } };
using GErrorPtr = std::unique_ptr<GError, GErrorDeleter>;

struct GObjectUnref { void operator()(gpointer p) { g_object_unref(p); } };
template<typename T>
using GObjectPtr = std::unique_ptr<T, GObjectUnref>;

// Usage:
GErrorPtr error;
GObjectPtr<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file_at_size(..., error.get()));
```

### High Priority Issues

**5. Const correctness:**

```cpp
// Many getters are not const:
class Widget {
  int get_width() { return width_; }  // Should be const!
  int get_height() { return height_; }  // Should be const!
};
```

**Fix:** Add const to all read-only methods.

**6. Missing override keywords:**

```cpp
// Some virtual overrides lack 'override' keyword
class TrayWidget : public Widget {
  void render(cairo_t* cr);  // Missing override
  void update();              // Missing override
};
```

**Fix:** Add `override` everywhere.

**7. Raw pointers in public API:**

```cpp
Widget* WidgetManager::get_widget(const std::string& id) {
  return widgets_[id].get();  // Returning raw pointer from unique_ptr
}
```

**Risk:** Caller might delete it or store dangling pointer.

**Fix:** Return reference or shared_ptr depending on ownership semantics.

**8. Exception safety:**

No exception specifications anywhere. Code assumes no exceptions, but:
- `std::vector` can throw `bad_alloc`
- `std::string` can throw
- Config parsing can throw

**Fix:** Either:
- Add `noexcept` where appropriate
- Or handle exceptions properly
- Document exception guarantees

---

## Recommended Makefile Targets

```makefile
# Quality targets
.PHONY: quality quality-fast quality-full

quality-fast: format-check lint-tidy
	@echo "✅ Fast quality checks passed"

quality-full: format-check lint-all test-asan coverage complexity
	@echo "✅ Full quality gate passed"

# Pre-commit should run quality-fast
pre-commit: quality-fast build test-fast
	@echo "✅ Pre-commit checks passed"
```

---

## Recommended Pre-commit Hook Updates

```bash
#!/bin/bash
# .git/hooks/pre-commit

echo "🔍 Pre-commit checks..."

# 1. Format check
echo "📝 Checking formatting..."
make format-check || exit 1

# 2. Static analysis (clang-tidy on changed files only)
echo "🔍 Running static analysis..."
CHANGED_FILES=$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(cpp|h)$')
if [ -n "$CHANGED_FILES" ]; then
  clang-tidy $CHANGED_FILES -- -std=c++17 -I include || exit 1
fi

# 3. Build
echo "🔨 Building..."
make clean && make || exit 1

# 4. Fast tests
echo "🧪 Running tests..."
make test-fast || exit 1

# 5. Coverage check (only if tests changed)
if git diff --cached --name-only | grep -q "tests/"; then
  echo "📊 Checking coverage..."
  make coverage-quick || exit 1
fi

echo "✅ Pre-commit checks passed!"
```

---

## Priority Implementation Order

### Week 1 (CRITICAL - Fix CI)
1. ✅ Fix CI build dependencies
2. ✅ Fix formatting issues
3. Add clang-tidy to CI
4. Fix null check issues in D-Bus code

### Week 2 (HIGH - Memory Safety)
5. Add AddressSanitizer tests
6. Add ThreadSanitizer tests
7. Fix thread safety issues
8. Add RAII wrappers for C APIs

### Week 3 (MEDIUM - Code Quality)
9. Add const correctness
10. Add override keywords
11. Add cppcheck
12. Add complexity checks

### Week 4 (NICE TO HAVE)
13. Add benchmarks
14. Add ABI compatibility checks
15. Add documentation quality checks
16. Set up SonarCloud/Codecov

---

## Summary Recommendations

**Must Have (Block merges):**
- ✅ Formatting (clang-format) - Already done
- ✅ Build success - Already done
- ✅ Unit tests pass - Already done
- 🔴 Clang-tidy static analysis - **ADD NOW**
- 🔴 AddressSanitizer - **ADD NOW**
- 🔴 Coverage >= 50% - **ENFORCE NOW**

**Should Have (Warn but don't block):**
- 🟡 ThreadSanitizer
- 🟡 Cppcheck
- 🟡 Complexity < 15
- 🟡 Documentation coverage

**Nice to Have (Track trends):**
- ⚪ Benchmarks
- ⚪ ABI compatibility
- ⚪ Security scans

**Technical Debt to Pay:**
- Fix null checks in D-Bus code
- Add const correctness
- Add override keywords
- RAII wrappers for C APIs
- Exception safety guarantees

---

_Quality is not an act, it is a habit. — Aristotle_
