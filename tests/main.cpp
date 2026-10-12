#include <cstring>

#include "test.h"

namespace test {

namespace {
int current_failures = 0;
}

std::vector<Case>& registry() {
  static std::vector<Case> cases;
  return cases;
}

void fail(const char* file, int line, const char* expr) {
  std::fprintf(stderr, "  %s:%d: CHECK(%s) failed\n", file, line, expr);
  ++current_failures;
}

}  // namespace test

int main(int argc, char** argv) {
  int run = 0, failed = 0;
  for (const auto& c : test::registry()) {
    bool selected = argc < 2;
    for (int i = 1; i < argc; ++i)
      if (std::strncmp(c.id, argv[i], std::strlen(argv[i])) == 0) selected = true;
    if (!selected) continue;

    test::current_failures = 0;
    c.fn();
    ++run;
    if (test::current_failures) {
      ++failed;
      std::fprintf(stderr, "FAIL %s %s\n", c.id, c.name);
    }
  }
  std::printf("%d/%d passed\n", run - failed, run);
  if (run == 0) {
    std::fprintf(stderr, "no tests matched\n");
    return 1;
  }
  return failed ? 1 : 0;
}
