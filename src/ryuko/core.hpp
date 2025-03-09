#pragma once

#include <ryuko/pch.hpp>

namespace ryuko {

template <typename T> using Optional = std::optional<T>;

static constexpr std::string EmptyString = {};
static constexpr auto VertFunctionName = "vert";
static constexpr auto FragFunctionName = "frag";

template <typename... T>
static void error(fmt::format_string<T...> fmt, T &&...args) {
  constexpr auto style = fg(fmt::color::red) | fmt::emphasis::bold;
  const auto formatted = fmt::format(style, fmt, std::forward<T>(args)...);

  fmt::println("{}", fmt::format(style, "{} {}", "[ryuko]", formatted));
}

struct Function {
  struct Argument {
    std::string type;
    std::string name;
    bool array = false;
  };

public:
  std::string returnType;
  std::string name;
  std::string body;
  std::vector<Argument> args;
};

struct Varying {
  std::string name;
  std::string type;
  std::string precision;

  //  @temp(v2f): mark inputs/outputs
  bool vertexInput = false;
  bool fragmentInput = false;

  bool vertexOutput = false;
  bool fragmentOutput = false;
};

struct Struct {
  using Field = Function::Argument;

public:
  std::string name;
  std::vector<Field> fields;
};

struct UniformValue {
  constexpr static uint32_t Kind_Unknown = 0;
  constexpr static uint32_t Kind_Struct = 1;
  constexpr static uint32_t Kind_PushConstants = 2;
  constexpr static uint32_t Kind_Vec4 = 5;
  constexpr static uint32_t Kind_Sampler2D = 6;

public:
  Struct struct_;
  uint32_t kind = Kind_Unknown;
  uint32_t arrayLength;

  //  @todo: use a bit of kind to store this instead
  bool array = false;
};

struct Uniform {
  using Field = Function::Argument;

public:
  UniformValue value;
  std::string accessor;
  uint32_t set;
  uint32_t binding;
};

struct PushConstantsLayout : Struct {};

struct BufferLayout {
  std::string typeName;
  std::string name;

  //  @fixme: standard has limited number of values
  //  @fixme: change standard's type to bitfield
  //  @fixme: readonly should be part of standard
  uint32_t standard;
  bool readonly;
};

struct StorageBuffer {
  Struct description;
  std::string name;
  uint32_t set;
  uint32_t binding;
  bool readonly;
};

struct ShaderInput {
  static constexpr uint32_t Kind_Uniform = 0;
  static constexpr uint32_t Kind_StorageBuffer = 1;

public:
  Uniform uniform;
  StorageBuffer storageBuffer;
  uint32_t kind;

public:
  ShaderInput(const uint32_t kind, const Uniform &uniform)
      : uniform(uniform), kind(kind) {}

  ShaderInput(const uint32_t kind, const StorageBuffer &buffer)
      : storageBuffer(buffer), kind(kind) {}

  ~ShaderInput() {}

  ShaderInput(const ShaderInput &other) {
    this->kind = other.kind;

    if (this->kind == Kind_Uniform) {
      this->uniform = other.uniform;
    } else if (this->kind == Kind_StorageBuffer) {
      this->storageBuffer = other.storageBuffer;
    }
  }

  ShaderInput &operator=(const ShaderInput &other) {
    this->kind = other.kind;

    if (this->kind == Kind_Uniform) {
      this->uniform = other.uniform;
    } else if (this->kind == Kind_StorageBuffer) {
      this->storageBuffer = other.storageBuffer;
    }

    return *this;
  }
};

struct Context {
  PushConstantsLayout pushConstantsLayout;
  std::vector<std::string> directives;
  std::vector<Function> functions;
  std::vector<Varying> varyings;
  std::vector<std::string> inlinedFragmentCode;
  std::vector<Uniform> uniforms;
  std::vector<BufferLayout> bufferLayouts;
  std::vector<StorageBuffer> storageBuffers;
  std::vector<ShaderInput> inputs;
  int version;
};

} // namespace ryuko
