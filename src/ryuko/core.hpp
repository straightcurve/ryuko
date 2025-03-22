#pragma once

#include <ryuko/pch.hpp>

namespace ryuko {

template <typename T> using Optional = std::optional<T>;

static constexpr std::string EmptyString = {};
static constexpr auto VertFunctionName = "vert";
static constexpr auto FragFunctionName = "frag";

template <typename... T>
static void debug(fmt::format_string<T...> fmt, T &&...args) {
  constexpr auto style = fg(fmt::color::gray) | fmt::emphasis::bold;
  const auto formatted = fmt::format(style, fmt, std::forward<T>(args)...);

  fmt::println("{}", fmt::format(style, "{} {}", "[ryuko]", formatted));
}

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

namespace config {

struct ColorBlend {
  enum class Value { Additive, Alpha, Disabled };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "additive")
      return Value::Additive;
    if (str == "alpha")
      return Value::Alpha;
    if (str == "disabled")
      return Value::Disabled;
    return {};
  }
};

struct DepthTest {
  enum class Value { Enabled, Disabled };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "enabled")
      return Value::Enabled;
    if (str == "disabled")
      return Value::Disabled;
    return {};
  }
};

struct DepthWrite {
  enum class Value { Enabled, Disabled };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "enabled")
      return Value::Enabled;
    if (str == "disabled")
      return Value::Disabled;
    return {};
  }
};

struct DepthOp {
  enum class Value {
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    Always,
    Never
  };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "less")
      return Value::Less;
    if (str == "less_equal")
      return Value::LessEqual;
    if (str == "greater")
      return Value::Greater;
    if (str == "greater_equal")
      return Value::GreaterEqual;
    if (str == "equal")
      return Value::Equal;
    if (str == "not_equal")
      return Value::NotEqual;
    if (str == "always")
      return Value::Always;
    if (str == "never")
      return Value::Never;
    return {};
  }
};

struct Polygon {
  enum class Value { Fill, Line, Point };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "fill")
      return Value::Fill;
    if (str == "line")
      return Value::Line;
    if (str == "point")
      return Value::Point;
    return {};
  }
};

struct Cull {
  enum class Value { None, Front, Back, FrontAndBack };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "none")
      return Value::None;
    if (str == "front")
      return Value::Front;
    if (str == "back")
      return Value::Back;
    if (str == "front_and_back")
      return Value::FrontAndBack;
    return {};
  }
};

struct FrontFace {
  enum class Value { Clockwise, CounterClockwise };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "clockwise")
      return Value::Clockwise;
    if (str == "counter_clockwise")
      return Value::CounterClockwise;
    return {};
  }
};

struct Topology {
  enum class Value {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan
  };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "point_list")
      return Value::PointList;
    if (str == "line_list")
      return Value::LineList;
    if (str == "line_strip")
      return Value::LineStrip;
    if (str == "triangle_list")
      return Value::TriangleList;
    if (str == "triangle_strip")
      return Value::TriangleStrip;
    if (str == "triangle_fan")
      return Value::TriangleFan;
    return {};
  }
};

struct Multisampling {
  enum class Value { None, X2, X4, X8, X16, X32, X64 };
  Value value;

  static Optional<Value> parse(const std::string_view str) {
    if (str == "none")
      return Value::None;
    if (str == "2x")
      return Value::X2;
    if (str == "4x")
      return Value::X4;
    if (str == "8x")
      return Value::X8;
    if (str == "16x")
      return Value::X16;
    if (str == "32x")
      return Value::X32;
    if (str == "64x")
      return Value::X64;
    return {};
  }
};

struct ColorAttachmentCount {
  int count;

  static Optional<int> parse(const std::string_view str) {
    return atoi(str.begin() + 1); // NOLINT(*-err33-c)
  }
};

struct DepthAttachment {
  bool enabled;

  static Optional<bool> parse(const std::string_view str) {
    if (str == "enabled") {
      return true;
    }

    if (str == "disabled") {
      return false;
    }

    return false;
  }
};

using ConfigValue =
    std::variant<ColorBlend, DepthTest, DepthWrite, DepthOp, Polygon, Cull,
                 FrontFace, Topology, Multisampling, ColorAttachmentCount,
                 DepthAttachment>;

static const std::unordered_map<std::string_view, int> variableTypeMap = {
    {"color_blend", 0},      {"depth_test", 1},
    {"depth_write", 2},      {"depth_op", 3},
    {"polygon", 4},          {"cull", 5},
    {"front_face", 6},       {"topology", 7},
    {"multisampling", 8},    {"color_attachment_count", 9},
    {"depth_attachment", 10}};

} // namespace config

struct PipelineConfiguration {
  config::ColorBlend blend;
  config::DepthTest depthTest;
  config::DepthWrite depthWrite;
  config::DepthOp depthOp;
  config::Polygon polygon;
  config::Cull cull;
  config::FrontFace front_face;
  config::Topology topology;
  config::Multisampling multisampling;
  config::ColorAttachmentCount colorAttachmentCount;
  config::DepthAttachment depthAttachment;
};

struct Context {
  PushConstantsLayout pushConstantsLayout;
  PipelineConfiguration config;
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
