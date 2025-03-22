#pragma once

#include <ryuko/core.hpp>
#include <ryuko/emitter.hpp>
#include <ryuko/parser.hpp>
#include <ryuko/transpiler.hpp>

namespace ryuko {

struct ProcessOutput final {
  Context context;
  Emitter::Output output;
};

[[maybe_unused]]
static Optional<ProcessOutput> process(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    error("failed to open file {}", path.c_str());

    return {};
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  Parser parser{buffer.str(), path};
  if (auto parseResult = parser.parse(); parseResult.has_value()) {
    Context &context = parseResult.value();

    Transpiler transpiler{context.functions, context.varyings};
    transpiler.setReturnValues();

    if (auto emitResult = Emitter::program(context); emitResult.has_value()) {
      return ProcessOutput{std::move(context), emitResult.value()};
    }

    error("failed to emit shader: {}", path.c_str());
  } else {
    error("failed to parse shader: {}", path.c_str());
  }

  return {};
}

} // namespace ryuko