#pragma once

#include <ryuko/emitter.hpp>
#include <ryuko/parser.hpp>
#include <ryuko/transpiler.hpp>

namespace ryuko {

struct Output : Emitter::Output {
  int version;
};

struct Sink {
  virtual ~Sink() = default;
  virtual void write(const std::string &vertexCode,
                     const std::string &fragmentCode,
                     const std::filesystem::path &basePath) = 0;
};

struct FileSink : Sink {
  void write(const std::string &vertexCode, const std::string &fragmentCode,
             const std::filesystem::path &path) override {
    auto basePath = path.parent_path();

    std::filesystem::path vertPath(basePath /
                                   fmt::format("{}.vert", path.stem().c_str()));
    if (std::ofstream vert(vertPath); vert.is_open()) {
      vert << vertexCode;
      vert.close();
    } else {
      error("failed to write vertex shader to file: {}", vertPath.string());
    }

    std::filesystem::path fragPath(basePath /
                                   fmt::format("{}.frag", path.stem().c_str()));
    if (std::ofstream frag(fragPath); frag.is_open()) {
      frag << fragmentCode;
      frag.close();
    } else {
      error("failed to write fragment shader to file: {}", fragPath.string());
    }
  }
};

struct MemorySink : Sink {
  std::filesystem::path path;
  std::string vertexCode;
  std::string fragmentCode;

  void write(const std::string &vertexCode, const std::string &fragmentCode,
             const std::filesystem::path &path) override {
    this->path = path;
    this->vertexCode = vertexCode;
    this->fragmentCode = fragmentCode;
  }
};

static Optional<Output> transpile(const std::filesystem::path &path,
                                  Sink &sink) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    error("failed to open file {}", path.c_str());

    return {};
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  Parser parser{buffer.str()};
  if (auto parseResult = parser.parse(); parseResult.has_value()) {
    Context &context = parseResult.value();

    Transpiler transpiler{context.functions, context.varyings};
    transpiler.setReturnValues();

    if (auto emitResult = Emitter::program(context); emitResult.has_value()) {
      sink.write(emitResult->vertex, emitResult->fragment, path);

      return {{path.parent_path() / fmt::format("{}.frag", path.stem().c_str()),
               path.parent_path() / fmt::format("{}.vert", path.stem().c_str()),
               context.version}};
    }

    error("failed to emit shader: {}", path.c_str());
  } else {
    error("failed to parse shader: {}", path.c_str());
  }

  return {};
}

} // namespace ryuko
