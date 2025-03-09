#pragma once

#include <ryuko/emitter.hpp>
#include <ryuko/parser.hpp>
#include <ryuko/transpiler.hpp>

namespace ryuko {

struct Output : Emitter::Output {
  int version;
};

struct Sink {
  std::vector<Varying> vertexInputs;
  std::vector<Varying> varyings;
  std::vector<Varying> fragmentOutputs;
  std::vector<BufferLayout> bufferLayouts;
  std::vector<ShaderInput> inputs;
  std::vector<StorageBuffer> storageBuffers;
  std::vector<Uniform> uniforms;

public:
  virtual ~Sink() = default;

  virtual bool hasFragmentCode() const = 0;
  virtual bool hasVertexCode() const = 0;
  virtual void write(const std::string &vertexCode,
                     const std::string &fragmentCode,
                     const std::filesystem::path &basePath) = 0;
};

struct FileSink final : Sink {
  bool hasFragmentCode() const override {
    assert(false);
    return false;
  }

  bool hasVertexCode() const override {
    assert(false);
    return false;
  }

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

struct MemorySink final : Sink {
  std::filesystem::path path;
  std::string vertexCode;
  std::string fragmentCode;

public:
  bool hasFragmentCode() const override { return !fragmentCode.empty(); }
  bool hasVertexCode() const override { return !vertexCode.empty(); }

  void write(const std::string &vertexCode, const std::string &fragmentCode,
             const std::filesystem::path &path) override {
    this->path = path;
    this->vertexCode = vertexCode;
    this->fragmentCode = fragmentCode;
  }
};

[[maybe_unused]]
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

  Parser parser{buffer.str(), path};
  if (auto parseResult = parser.parse(); parseResult.has_value()) {
    Context &context = parseResult.value();

    Transpiler transpiler{context.functions, context.varyings};
    transpiler.setReturnValues();

    if (auto emitResult = Emitter::program(context); emitResult.has_value()) {
      if (emitResult->fragment.has_value()) {
        sink.write(emitResult->vertex, emitResult->fragment.value(), path);
      } else {
        sink.write(emitResult->vertex, {}, path);
      }

      sink.storageBuffers = context.storageBuffers;
      sink.bufferLayouts = context.bufferLayouts;
      sink.inputs = context.inputs;
      sink.uniforms = context.uniforms;

      //  @temp(v2f): generate vertex input struct
      {
        std::vector<Varying> vertexInputs;
        std::vector<Varying> varyings;
        std::vector<Varying> fragmentOutputs;

        for (auto &varying : context.varyings) {
          if (varying.vertexInput) {
            vertexInputs.push_back(varying);
          } else if (varying.fragmentOutput) {
            fragmentOutputs.push_back(varying);
          } else {
            varyings.push_back(varying);
          }
        }

        sink.vertexInputs = vertexInputs;
        sink.varyings = varyings;
        sink.fragmentOutputs = fragmentOutputs;
      }

      Output o{};
      o.version = context.version;
      o.fragment =
          path.parent_path() / fmt::format("{}.frag", path.stem().c_str());
      o.vertex =
          path.parent_path() / fmt::format("{}.vert", path.stem().c_str());

      return o;
    }

    error("failed to emit shader: {}", path.c_str());
  } else {
    error("failed to parse shader: {}", path.c_str());
  }

  return {};
}

} // namespace ryuko
