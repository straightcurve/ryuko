#pragma once

#include <ryuko/core.hpp>
#include <ryuko/includer.hpp>

namespace ryuko {

struct Parser {
private:
  std::string input;
  size_t index;
  CustomIncluder includer;
  std::filesystem::path inputPath;

public:
  explicit Parser(std::string input,
                  const std::filesystem::path &inputPath = {})
      : input(std::move(input)), index(0), inputPath(inputPath) {}

public:
  [[nodiscard]] bool alphanumeric(const size_t i) const {
    return std::isalnum(input[i]) || input[i] == '_';
  }

  Optional<config::ConfigValue> consumePipelineConfigurationVariable() {
    const auto start = index;

    const auto prop = consumeIdentifier();
    consumeWhitespace();

    const auto it = config::variableTypeMap.find(prop);
    if (it == config::variableTypeMap.end()) {
      index = start;
      return {};
    }

    const auto value = consumeIdentifier();
    consumeWhitespace();

    if (!expect(';', __LINE__)) {
      index = start;
      return {};
    }

    switch (it->second) {
    case 0: { // color_blend
      const auto val = config::ColorBlend::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::ColorBlend{*val};
    }
    case 1: { // depth_test
      const auto val = config::DepthTest::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::DepthTest{*val};
    }
    case 2: { // depth_write
      const auto val = config::DepthWrite::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::DepthWrite{*val};
    }
    case 3: { // depth_op
      const auto val = config::DepthOp::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::DepthOp{*val};
    }
    case 4: { // polygon
      const auto val = config::Polygon::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::Polygon{*val};
    }
    case 5: { // cull
      const auto val = config::Cull::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::Cull{*val};
    }
    case 6: { // front_face
      const auto val = config::FrontFace::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::FrontFace{*val};
    }
    case 7: { // topology
      const auto val = config::Topology::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::Topology{*val};
    }
    case 8: { // multisampling
      const auto val = config::Multisampling::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::Multisampling{*val};
    }
    case 9: { // color_attachment_count
      const auto val = config::ColorAttachmentCount::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::ColorAttachmentCount{*val};
    }
    case 10: { // depth_attachment
      const auto val = config::DepthAttachment::parse(value);
      if (!val) {
        index = start;
        return {};
      }
      return config::DepthAttachment{*val};
    }
    default:
      index = start;
      return {};
    }
  }

  std::string consumeCharacter() { return input.substr(index++, 1); }

  Optional<std::string> consumeConst() {
    if (match("const")) {
      return consumeUntil("\n");
    }

    return {};
  }

  Optional<Function> consumeFunction() {
    const auto start = index;
    Function function{};

    function.returnType = consumeIdentifier();
    consumeWhitespace();

    function.name = consumeIdentifier();
    consumeWhitespace();

    if (peek() != '(') {
      index = start;
      return {};
    }

    consumeCharacter();
    consumeWhitespace();

    if (peek() == ')') {
      //  @note: function has no arguments
      consumeCharacter();
    } else {
      while (peek() != ')') {
        const auto argType = consumeIdentifier();
        consumeWhitespace();

        const auto argName = consumeIdentifier();
        consumeWhitespace();

        function.args.push_back(Function::Argument{argType, argName});

        if (peek() == ')') {
          break;
        }

        if (!expect(',', __LINE__)) {
          index = start;

          return {};
        }

        consumeWhitespace();
      }

      consumeCharacter();
    }

    consumeWhitespace();

    if (!expect('{', __LINE__)) {
      index = start;

      return {};
    }

    size_t scopes = 1;
    std::vector<std::string> args;

    while (scopes && !done()) {
      const auto c = consumeCharacter();
      if (c == "{") {
        scopes++;
      } else if (c == "}") {
        scopes--;
      }

      function.body += c;
    }

    if (scopes) {
      printError('}', __LINE__);

      index = start;

      return {};
    }

    return {function};
  }

  std::string consumeIdentifier() {
    const auto start = index;

    while (index < input.size() && alphanumeric(index)) {
      index++;
    }

    return input.substr(start, index - start);
  }

  Optional<std::string> consumeAttributeValue(const std::string &expected) {
    if (match(expected)) {
      consumeIdentifier();
      consumeWhitespace();

      if (!expect('=', __LINE__)) {
        return {};
      }

      consumeWhitespace();

      return consumeIdentifier();
    }

    return {};
  }

  Optional<StorageBuffer> consumeStorageBuffer() {
    /*
     *  layout (set = 2, binding = 0) buffer Hello {
     *    uint data[];
     *  };
     */

    const auto start = index;

    if (!match("layout")) {
      index = start;
      return {};
    }

    StorageBuffer buffer{};

    consumeIdentifier();
    consumeWhitespace();

    if (!expect('(', __LINE__)) {
      index = start;
      return {};
    }

    if (const auto maybeSet = consumeAttributeValue("set");
        maybeSet.has_value()) {
      buffer.set = atoi(maybeSet.value().c_str()); // NOLINT(*-err33-c)
      consumeWhitespace();
    } else {
      index = start;
      return {};
    }

    if (peek() != ',') {
      index = start;
      return {};
    }

    consumeCharacter();
    consumeWhitespace();

    if (const auto maybeBinding = consumeAttributeValue("binding");
        maybeBinding.has_value()) {
      buffer.binding = atoi(maybeBinding.value().c_str()); // NOLINT(*-err33-c)
      consumeWhitespace();
    } else {
      index = start;
      return {};
    }

    if (peek() != ')') {
      index = start;
      return {};
    }

    consumeCharacter();
    consumeWhitespace();

    if (match("readonly")) {
      consumeIdentifier();
      consumeWhitespace();

      buffer.readonly = true;
    }

    if (!match("buffer")) {
      index = start;
      return {};
    }

    consumeIdentifier();
    consumeWhitespace();

    buffer.name = consumeIdentifier();
    consumeWhitespace();

    const auto maybeStruct = consumeStruct();
    if (!maybeStruct.has_value()) {
      index = start;
      return {};
    }

    auto _struct = maybeStruct.value();
    _struct.name = buffer.name;

    buffer.description = _struct;

    return buffer;
  }

  Optional<BufferLayout> consumeBufferLayout() {
    /*
     *  layout (buffer_reference, std430) readonly buffer LightBuffer {
     *    Light lights[];
     *  };
     */

    const auto start = index;

    if (!match("layout")) {
      index = start;
      return {};
    }

    BufferLayout bufferLayout{};

    consumeIdentifier();
    consumeWhitespace();

    if (!expect('(', __LINE__)) {
      index = start;
      return {};
    }

    consumeWhitespace();

    if (!match("buffer_reference")) {
      index = start;
      return {};
    }

    consumeIdentifier();

    if (peek() == ',') {
      consumeCharacter();
      consumeWhitespace();

      if (!match("std")) {
        index = start;
        return {};
      }

      consumeCharacter();
      consumeCharacter();
      consumeCharacter();

      bufferLayout.standard =
          atoi(consumeIdentifier().c_str()); // NOLINT(*-err33-c)

      consumeWhitespace();
    }

    if (!expect(')', __LINE__)) {
      index = start;
      return {};
    }

    consumeWhitespace();

    if (match("readonly")) {
      consumeIdentifier();

      bufferLayout.readonly = true;
    }

    consumeWhitespace();

    if (!match("buffer")) {
      index = start;
      return {};
    }

    consumeIdentifier();
    consumeWhitespace();

    bufferLayout.name = consumeIdentifier();

    consumeWhitespace();

    if (!expect('{', __LINE__)) {
      index = start;
      return {};
    }

    consumeWhitespace();

    bufferLayout.typeName = consumeIdentifier();

    consumeWhitespace();

    consumeIdentifier();

    if (!expect('[', __LINE__) || !expect(']', __LINE__) ||
        !expect(';', __LINE__)) {
      index = start;
      return {};
    }

    consumeWhitespace();

    if (!expect('}', __LINE__)) {
      index = start;
      return {};
    }

    return bufferLayout;
  }

  Optional<Struct> consumeStruct() {
    const auto start = index;

    if (!expect('{', __LINE__)) {
      return {};
    }

    Struct _struct{};

    consumeWhitespace();

    while (peek() != '}') {
      Struct::Field field{};
      field.type = consumeIdentifier();
      consumeWhitespace();

      field.name = consumeIdentifier();
      consumeWhitespace();

      if (peek() == '[') {
        field.array = true;

        consumeCharacter();

        if (!expect(']', __LINE__)) {
          index = start;
          return {};
        }
      }

      _struct.fields.push_back(field);

      if (!expect(';', __LINE__)) {
        index = start;
        return {};
      }

      consumeWhitespace();
    }

    consumeCharacter();

    return _struct;
  }

  Optional<Uniform> consumeUniform() {
    /*
     layout (set = 1, binding = 0) uniform sampler2D textures[];
     */

    /*
     *  layout (set = 2, binding = 0) uniform Global {
          vec4 time;
          CircleBuffer cb;
          uint circleCount;
        } global;
     */

    const auto start = index;

    Uniform uniform{};

    if (!match("layout")) {
      index = start;
      return {};
    }

    consumeIdentifier();

    {
      consumeWhitespace();

      if (!expect('(', __LINE__)) {
        index = start;
        return {};
      }

      consumeWhitespace();

      {
        if (match("push_constant")) {
          consumeIdentifier();
          consumeWhitespace();

          uniform.value.kind = UniformValue::Kind_PushConstants;
        } else if (match("set")) {
          consumeIdentifier();
          consumeWhitespace();

          if (!expect('=', __LINE__)) {
            index = start;
            return {};
          }

          consumeWhitespace();

          uniform.set = atoi(consumeIdentifier().c_str()); // NOLINT(*-err33-c)

          consumeWhitespace();
          if (!expect(',', __LINE__)) {
            index = start;

            return {};
          }

          consumeWhitespace();

          {
            if (!match("binding")) {
              index = start;
              return {};
            }

            consumeIdentifier();
            consumeWhitespace();

            if (!expect('=', __LINE__)) {
              index = start;
              return {};
            }

            consumeWhitespace();

            uniform.binding =
                atoi(consumeIdentifier().c_str()); // NOLINT(*-err33-c)

            consumeWhitespace();
          }
        } else {
          index = start;
          return {};
        }
      }

      if (!expect(')', __LINE__)) {
        index = start;
        return {};
      }
    }

    consumeWhitespace();

    if (!match("uniform")) {
      index = start;
      return {};
    }

    consumeIdentifier();
    consumeWhitespace();

    std::string typeName = consumeIdentifier();
    consumeWhitespace();

    if (peek() != '{') {
      uniform.accessor = consumeIdentifier();

      if (expect('[', __LINE__)) {
        //  it's an array

        if (peek() != ']') {
          uniform.value.arrayLength =
              atoi(consumeIdentifier().c_str()); // NOLINT(*-err33-c)
        }

        if (!expect(']', __LINE__)) {
          index = start;
          return {};
        }
      }

      //  @todo: parse type correctly
      if (typeName == "sampler2D") {
        uniform.value.kind = UniformValue::Kind_Sampler2D;
      } else if (uniform.value.kind == UniformValue::Kind_Unknown) {
        uniform.value.kind = UniformValue::Kind_Vec4;
      }

      return uniform;
    }

    {
      if (uniform.value.kind == UniformValue::Kind_Unknown) {
        uniform.value.kind = UniformValue::Kind_Struct;
      }

      if (!expect('{', __LINE__)) {
        index = start;
        return {};
      }

      consumeWhitespace();

      while (peek() != '}') {
        Struct::Field field{};
        field.type = consumeIdentifier();
        consumeWhitespace();

        field.name = consumeIdentifier();
        consumeWhitespace();

        uniform.value.struct_.fields.push_back(field);

        if (!expect(';', __LINE__)) {
          index = start;
          return {};
        }

        consumeWhitespace();
      }

      consumeCharacter();
    }

    consumeWhitespace();

    uniform.accessor = consumeIdentifier();

    consumeWhitespace();

    if (!expect(';', __LINE__)) {
      index = start;
      return {};
    }

    consumeWhitespace();

    return uniform;
  }

  Optional<PushConstantsLayout> consumePushConstantsLayout() {
    const auto start = index;

    PushConstantsLayout pushConstantsLayout{};

    if (!match("layout")) {
      index = start;
      return {};
    }

    consumeIdentifier();
    consumeWhitespace();

    if (!expect('(', __LINE__)) {
      index = start;
      return {};
    }

    consumeWhitespace();

    if (match("push_constant")) {
      consumeIdentifier();
      consumeWhitespace();
    } else {
      index = start;
      return {};
    }

    if (!expect(')', __LINE__)) {
      index = start;
      return {};
    }

    consumeWhitespace();

    if (!match("uniform")) {
      index = start;
      return {};
    }

    consumeIdentifier();
    consumeWhitespace();

    //  uniform type name, useless
    consumeIdentifier();
    consumeWhitespace();

    const auto _struct = consumeStruct();
    if (!_struct.has_value()) {
      index = start;
      return {};
    }

    pushConstantsLayout.fields = _struct.value().fields;

    consumeWhitespace();
    pushConstantsLayout.name = consumeIdentifier();
    consumeWhitespace();

    if (!expect(';', __LINE__)) {
      index = start;
      return {};
    }

    return pushConstantsLayout;
  }

  Optional<std::string> consumeDirective() {
    if (peek() != '#') {
      return {};
    }

    consumeCharacter();
    return consumeUntil("\n");
  }

  std::string consumeUntil(const std::string &expected) {
    const auto start = index;

    while (index < input.size()) {
      size_t e = 0;
      while (e < expected.size() && expected[e] == input[index + e]) {
        e++;
      }

      if (e >= expected.size()) {
        return input.substr(start, index - start);
      }

      index++;
    }

    return input.substr(start, index - start);
  }

  Optional<Varying> consumeVarying() {
    if (!match("varying")) {
      return {};
    }

    consumeIdentifier();
    consumeWhitespace();

    Varying varying{};

    varying.precision = consumeIdentifier();
    consumeWhitespace();

    varying.type = consumeIdentifier();
    consumeWhitespace();

    if (peek() == ';') {
      //  @note: we don't have precision
      varying.name = varying.type;
      varying.type = varying.precision;
      varying.precision = EmptyString;

      return {varying};
    }

    varying.name = consumeIdentifier();
    consumeWhitespace();

    if (!expect(';', __LINE__)) {
      return {};
    }

    return {varying};
  }

  Optional<int> consumeVersion() {
    const auto start = index;

    int version = 0;

    if (!match("#version")) {
      return {};
    }

    consumeCharacter();
    consumeIdentifier();
    consumeWhitespace();

    version = atoi(consumeIdentifier().c_str()); // NOLINT(*-err33-c)
    if (version == 0) {
      index = start;

      return {};
    }

    consumeWhitespace();

    return {version};
  }

  std::string consumeWhile(const std::function<bool()> &predicate) {
    const auto start = index;

    while (index < input.size() && predicate()) {
      index++;
    }

    return input.substr(start, index - start);
  }

  void consumeWhitespace() {
    while (whitespace(index)) {
      index++;
    }
  }

  [[nodiscard]] bool done() const { return index >= input.size(); }

  [[nodiscard]] bool expect(const char expected, size_t line) {
    if (input[index] == expected) {
      index++;

      return true;
    }

    printError(expected, line);

    return false;
  }

  [[nodiscard]] bool match(const std::string &expected) const {
    const auto start = index;

    size_t e = 0;
    while (start + e < input.size() && e < expected.size()) {
      if (expected[e] == input[start + e]) {
        e++;
      } else {
        return false;
      }

      if (e >= expected.size()) {
        return true;
      }
    }

    return false;
  }

  void parseInclude(const std::string &file, Context &parentContext) {
    const auto includedSource =
        includer.GetInclude(file.c_str(), shaderc_include_type_relative,
                            (char *)inputPath.c_str(), 3);

    Parser parser{includedSource->content, file};
    if (auto parseResult = parser.parse(); parseResult.has_value()) {
      Context &context = parseResult.value();

      for (auto &bufferLayout : context.bufferLayouts) {
        parentContext.bufferLayouts.push_back(bufferLayout);
      }

      for (auto &directive : context.directives) {
        parentContext.directives.push_back(directive);
      }

      for (auto &function : context.functions) {
        parentContext.functions.push_back(function);
      }

      for (auto &input : context.inputs) {
        parentContext.inputs.push_back(input);
      }

      for (auto &buffer : context.storageBuffers) {
        parentContext.storageBuffers.push_back(buffer);
      }

      for (auto &uniform : context.uniforms) {
        parentContext.uniforms.push_back(uniform);
      }

      for (auto &varying : context.varyings) {
        parentContext.varyings.push_back(varying);
      }
    }

    includer.ReleaseInclude(includedSource);
  }

  Optional<Context> parse() {
    Context context{};

    context.config.blend.value = config::ColorBlend::Value::Disabled;
    context.config.depthTest.value = config::DepthTest::Value::Enabled;
    context.config.depthWrite.value = config::DepthWrite::Value::Enabled;
    context.config.depthOp.value = config::DepthOp::Value::Less;
    context.config.polygon.value = config::Polygon::Value::Fill;
    context.config.cull.value = config::Cull::Value::Back;
    context.config.front_face.value =
        config::FrontFace::Value::CounterClockwise;
    context.config.topology.value = config::Topology::Value::TriangleList;
    context.config.multisampling.value = config::Multisampling::Value::None;
    context.config.colorAttachmentCount.count = 1;
    context.config.depthAttachment.enabled = true;

    while (!done()) {
      consumeWhitespace();

      if (auto result = consumeVersion(); result.has_value()) {
        context.version = result.value_or(450);
        continue;
      }

      if (auto result = consumePipelineConfigurationVariable();
          result.has_value()) {

        std::visit(
            [&context](const auto &value) {
              using T = std::decay_t<decltype(value)>;

              if constexpr (std::is_same_v<T, config::ColorBlend>) {
                context.config.blend = value;
              } else if constexpr (std::is_same_v<T, config::DepthTest>) {
                context.config.depthTest = value;
              } else if constexpr (std::is_same_v<T, config::DepthWrite>) {
                context.config.depthWrite = value;
              } else if constexpr (std::is_same_v<T, config::DepthOp>) {
                context.config.depthOp = value;
              } else if constexpr (std::is_same_v<T, config::Polygon>) {
                context.config.polygon = value;
              } else if constexpr (std::is_same_v<T, config::Cull>) {
                context.config.cull = value;
              } else if constexpr (std::is_same_v<T, config::FrontFace>) {
                context.config.front_face = value;
              } else if constexpr (std::is_same_v<T, config::Topology>) {
                context.config.topology = value;
              } else if constexpr (std::is_same_v<T, config::Multisampling>) {
                context.config.multisampling = value;
              } else if constexpr (std::is_same_v<
                                       T, config::ColorAttachmentCount>) {
                context.config.colorAttachmentCount = value;
              } else if constexpr (std::is_same_v<T, config::DepthAttachment>) {
                context.config.depthAttachment = value;
              }
            },
            result.value());
        continue;
      }

      if (auto result = consumeConst(); result.has_value()) {
        continue;
      }

      if (auto result = consumeStorageBuffer(); result.has_value()) {
        context.storageBuffers.push_back(result.value());
        context.inputs.emplace_back(ShaderInput::Kind_StorageBuffer,
                                    result.value());
        continue;
      }

      if (auto result = consumeBufferLayout(); result.has_value()) {
        context.bufferLayouts.push_back(result.value());
        continue;
      }

      if (auto result = consumePushConstantsLayout(); result.has_value()) {
        context.pushConstantsLayout = result.value();
        continue;
      }

      if (auto result = consumeUniform(); result.has_value()) {
        context.uniforms.push_back(result.value());
        context.inputs.emplace_back(ShaderInput::Kind_Uniform, result.value());
        continue;
      }

      if (auto result = consumeDirective(); result.has_value()) {
        if (const auto &directive = result.value();
            directive.starts_with("include")) {
          auto file = directive.substr(9, directive.size() - 1 - 9);
          parseInclude(file, context);
          context.directives.push_back(directive);
        } else if (directive == "dawn_inline_frag") {
          auto inlinedCode = consumeUntil("#dawn_inline_frag");
          context.inlinedFragmentCode.push_back(inlinedCode);
          consumeDirective();
        } else if (directive != "dawn_inline_frag") {
          context.directives.push_back(directive);
        }

        continue;
      }

      if (match("//")) {
        consumeUntil("\n");
        consumeWhitespace();
        continue;
      }

      if (auto result = consumeVarying(); result.has_value()) {
        context.varyings.push_back(result.value());
        continue;
      }

      if (auto result = consumeFunction(); result.has_value()) {
        context.functions.push_back(result.value());
        continue;
      }

      consumeUntil("\n");
      consumeWhitespace();
    }

    return {context};
  }

  void printError(const char expected, size_t line) {
    fmt::println("[{}] {}\n at line {} (in the cpp file)", index,
                 input.substr(0, index), line);

    error("expected character '{}' at index {}, got '{}'", expected, index,
          input[index]);
  }

  [[nodiscard]] char peek() const { return input[index]; }

  [[nodiscard]] bool whitespace(const size_t i) const {
    return input[i] == '\t' || input[i] == '\n' || std::isspace(input[i]);
  }
};

} // namespace ryuko
