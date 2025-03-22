#pragma once

#include <ryuko/pch.hpp>

#include <ryuko/core.hpp>
#include <ryuko/emitter.hpp>
#include <ryuko/includer.hpp>
#include <ryuko/process.hpp>

namespace ryuko::compilation {

enum class ShaderStage {
  Vertex,
  Fragment,
};

struct ShaderCompilationResult final {
  Optional<std::vector<uint32_t>> vertexCode;
  Optional<std::vector<uint32_t>> fragmentCode;
};

struct Output : Emitter::Output {
  int version;
};

static Optional<std::vector<uint32_t>>
compile(const std::filesystem::path &inputFilePath, std::string_view source,
        const ShaderStage stage) {
  shaderc_shader_kind shaderStage;
  switch (stage) {
  case ShaderStage::Vertex:
    shaderStage = shaderc_vertex_shader;
    break;
  case ShaderStage::Fragment:
    shaderStage = shaderc_fragment_shader;
    break;
  default:
    error("shader compilation failed: unsupported stage {}",
          static_cast<int>(stage));
    return {};
  }

  const shaderc::Compiler compiler{};
  shaderc::CompileOptions options{};
  options.SetTargetEnvironment(shaderc_target_env_vulkan,
                               shaderc_env_version_vulkan_1_3);
  options.SetTargetSpirv(shaderc_spirv_version_1_6);
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  options.SetSourceLanguage(shaderc_source_language_glsl);
  options.SetIncluder(std::make_unique<CustomIncluder>());

  const auto preprocessedResult = compiler.PreprocessGlsl(
      source.begin(), shaderStage, inputFilePath.c_str(), options);
  if (preprocessedResult.GetCompilationStatus() !=
      shaderc_compilation_status_success) {
    error("shader preprocessing failed: {}",
          preprocessedResult.GetErrorMessage());
    return {};
  }

  const auto compilationResult = compiler.CompileGlslToSpv(
      preprocessedResult.begin(),
      preprocessedResult.end() - preprocessedResult.begin(), shaderStage,
      inputFilePath.c_str(), options);
  if (compilationResult.GetCompilationStatus() !=
      shaderc_compilation_status_success) {
    std::string preprocessedOutput{preprocessedResult.begin(),
                                   preprocessedResult.end()};
    debug("[gfx] preprocessed shader source:\n{}", preprocessedOutput);

    error("[gfx] shader compilation failed: {}",
          compilationResult.GetErrorMessage());
    return {};
  }

  return {std::vector(compilationResult.begin(), compilationResult.end())};
}

static ShaderCompilationResult compile(const std::filesystem::path &path,
                                       const Emitter::Output &code) {
  ShaderCompilationResult result{};

  if (code.vertex.empty()) {
    error("no vertex shader?");
  } else {
    result.vertexCode = compile(path, code.vertex, ShaderStage::Vertex);
  }

  if (code.fragment.has_value() && !code.fragment->empty()) {
    result.fragmentCode =
        compile(path, code.fragment.value(), ShaderStage::Fragment);
  }

  return result;
}

struct Sink {
  PipelineConfiguration config;
  std::vector<Varying> vertexInputs;
  std::vector<ShaderInput> inputs;
  std::vector<StorageBuffer> storageBuffers;
  std::vector<Uniform> uniforms;
  VkShaderModule fragment = VK_NULL_HANDLE;
  VkShaderModule vertex = VK_NULL_HANDLE;

public:
  virtual ~Sink() = default;

  [[nodiscard]] bool hasFragmentCode() const {
    return fragment != VK_NULL_HANDLE;
  }
  [[nodiscard]] bool hasVertexCode() const { return vertex != VK_NULL_HANDLE; }

  void load(const VkDevice device, const ShaderCompilationResult &result,
            const std::filesystem::path &path) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    if (result.vertexCode.has_value()) {
      createInfo.codeSize = result.vertexCode.value().size() * sizeof(uint32_t);
      createInfo.pCode = result.vertexCode.value().data();

      if (vkCreateShaderModule(device, &createInfo, nullptr, &vertex) !=
          VK_SUCCESS) {
        error("load failed for vertex shader {}", path.c_str());
        return;
      }
    } else {
      vertex = VK_NULL_HANDLE;
    }

    if (result.fragmentCode.has_value()) {
      createInfo.codeSize =
          result.fragmentCode.value().size() * sizeof(uint32_t);
      createInfo.pCode = result.fragmentCode.value().data();

      if (vkCreateShaderModule(device, &createInfo, nullptr, &fragment) !=
          VK_SUCCESS) {
        error("load failed for fragment shader {}", path.c_str());
        return;
      }
    } else {
      fragment = VK_NULL_HANDLE;
    }
  }

  void unload(const VkDevice device) {
    if (vertex != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device, vertex, nullptr);
    }

    if (fragment != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device, fragment, nullptr);
    }

    vertex = VK_NULL_HANDLE;
    fragment = VK_NULL_HANDLE;
  }

  virtual void write(const std::filesystem::path &basePath,
                     const Emitter::Output &code, VkDevice device) = 0;
};

struct DefaultSink final : Sink {
  std::filesystem::path path;

public:
  void write(const std::filesystem::path &path, const Emitter::Output &code,
             const VkDevice device) override {
    this->path = path;

    const auto result = compile(path, code);
    load(device, result, path);
  }
};

struct MemorySink final : Sink {
  std::filesystem::path path;
  std::string fragmentCode;
  std::string vertexCode;

public:
  void write(const std::filesystem::path &path, const Emitter::Output &code,
             const VkDevice device) override {
    this->path = path;
    this->vertexCode = code.vertex;

    if (code.fragment.has_value()) {
      this->fragmentCode = code.fragment.value();
    }

    const auto result = compile(path, code);
    load(device, result, path);
  }
};

[[maybe_unused]]
static Optional<Output> compile(const std::filesystem::path &path, Sink &sink,
                                const VkDevice device) {
  auto maybeProcessedOutput = process(path);
  if (!maybeProcessedOutput.has_value()) {
    return {};
  }

  auto &context = maybeProcessedOutput.value().context;
  const auto &emitterOutput = maybeProcessedOutput.value().output;

  sink.write(path, emitterOutput, device);

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

} // namespace ryuko::compilation
