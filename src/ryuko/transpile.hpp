#pragma once

#include <ryuko/core.hpp>
#include <ryuko/emitter.hpp>
#include <ryuko/process.hpp>

namespace ryuko::transpilation {

struct Output : Emitter::Output {
  int version;
};

struct Sink {
  PipelineConfiguration config;
  std::vector<Varying> vertexInputs;
  std::vector<ShaderInput> inputs;
  std::vector<StorageBuffer> storageBuffers;
  std::vector<Uniform> uniforms;
  Optional<std::string> fragment;
  Optional<std::string> vertex;

public:
  virtual ~Sink() = default;

  [[nodiscard]] bool hasFragmentCode() const { return fragment.has_value(); }
  [[nodiscard]] bool hasVertexCode() const { return vertex.has_value(); }

  virtual void write(const std::filesystem::path &basePath,
                     const Emitter::Output &code) = 0;
};

struct DefaultSink final : Sink {
  std::filesystem::path path;

public:
  void write(const std::filesystem::path &path,
             const Emitter::Output &code) override {
    this->path = path;
    this->fragment = code.fragment;
    this->vertex = code.vertex;
  }
};

struct FileSink final : Sink {
  void write(const std::filesystem::path &path,
             const Emitter::Output &code) override {
    auto basePath = path.parent_path();

    std::filesystem::path vertPath(
        basePath / fmt::format("{}_vertex.glsl", path.stem().c_str()));
    if (std::ofstream vert(vertPath); vert.is_open()) {
      vert << code.vertex;
      vert.close();
    } else {
      error("failed to write vertex shader to file: {}", vertPath.string());
    }

    if (code.fragment.has_value()) {
      std::filesystem::path fragPath(
          basePath / fmt::format("{}_fragment.glsl", path.stem().c_str()));
      if (std::ofstream frag(fragPath); frag.is_open()) {
        frag << code.fragment.value();
        frag.close();
      } else {
        error("failed to write fragment shader to file: {}", fragPath.string());
      }
    }
  }
};

struct MemorySink final : Sink {
  std::filesystem::path path;
  std::string fragmentCode;
  std::string vertexCode;

public:
  void write(const std::filesystem::path &path,
             const Emitter::Output &code) override {
    this->path = path;
    this->fragmentCode = code.fragment.has_value() ? code.fragment.value() : "";
    this->vertexCode = code.vertex;
  }
};

[[maybe_unused]]
static Optional<Output> transpile(const std::filesystem::path &path,
                                  Sink &sink) {
  auto maybeProcessedOutput = process(path);
  if (!maybeProcessedOutput.has_value()) {
    return {};
  }

  auto &context = maybeProcessedOutput.value().context;
  const auto &emitterOutput = maybeProcessedOutput.value().output;

  sink.write(path, emitterOutput);

  sink.storageBuffers = context.storageBuffers;
  sink.inputs = context.inputs;
  sink.uniforms = context.uniforms;
  sink.config = context.config;

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
  }

  Output o{};
  o.version = context.version;

  //  @fixme: what is this used for?
  o.fragment = path.parent_path() / fmt::format("{}.frag", path.stem().c_str());
  o.vertex = path.parent_path() / fmt::format("{}.vert", path.stem().c_str());

  return o;
}

} // namespace ryuko::transpilation