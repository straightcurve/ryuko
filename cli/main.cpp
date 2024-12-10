#include <ryuko/ryuko.hpp>

int main(const int argc, char **argv) {
  if (argc < 2) {
    ryuko::error("usage: ryuko <input-file>");
    return 1;
  }

  if (auto result = ryuko::transpile(argv[1]); result.has_value()) {
    fmt::println("[ryuko] created {}", result.value().fragment);
    fmt::println("[ryuko] created {}", result.value().vertex);
  }
}