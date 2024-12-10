#pragma once

#include <ryuko/core.hpp>

namespace ryuko {

struct Emitter {
  struct Output {
    std::string fragment;
    std::string vertex;
  };

  struct State {
    std::unordered_set<std::string> emittedFunctions{};
    std::unordered_set<std::string> emittedFunctionSignatures{};
    std::string output{};
    Context &context;
    Function *main;
    uint32_t varyingInputIndex;
    uint32_t varyingOutputIndex;

  public:
    explicit State(Context &context, const std::string &mainFn)
        : context(context), main(nullptr), varyingInputIndex(0),
          varyingOutputIndex(0) {
      for (auto &f : context.functions) {
        if (f.name == mainFn) {
          main = &f;
          break;
        }
      }
    }
  };

public:
  Emitter() = delete;
  Emitter(const Emitter &) = delete;
  Emitter(Emitter &&) = delete;
  ~Emitter() = delete;

public:
  static void function(const Function &function, State &state) {
    std::string functionName = function.name;
    std::string functionReturnType = function.returnType;

    if (function.name == VertFunctionName) {
      functionName = "main";
      functionReturnType = "void";
    }

    if (function.name == FragFunctionName) {
      functionName = "main";
      functionReturnType = "void";
    }

    state.output += fmt::format("{} {}(", functionReturnType, functionName);

    for (auto a = 0; a < function.args.size(); a++) {
      const auto &[type, name] = function.args[a];
      state.output += fmt::format("{} {}{}", type, name,
                                  a + 1 < function.args.size() ? ", " : "");
    }

    state.output += fmt::format(") {}{}\n", "{", function.body);
  }

  static void functionWithCallees(const Function &_function, State &state) {
    if (state.emittedFunctions.contains(_function.name)) {
      return;
    }

    function(_function, state);
    newLine(state);
    state.emittedFunctions.insert(_function.name);

    for (const auto &fn : state.context.functions) {
      if (fn.name == _function.name) {
        continue;
      }

      if (_function.body.find(fn.name) != std::string::npos) {
        functionWithCallees(fn, state);
      }
    }
  }

  static void functionSignature(const Function &function, State &state) {
    if (function.name == VertFunctionName ||
        function.name == FragFunctionName) {
      return;
    }

    state.output += fmt::format("{} {}(", function.returnType, function.name);

    for (auto a = 0; a < function.args.size(); a++) {
      const auto &[type, name] = function.args[a];
      state.output += fmt::format("{} {}{}", type, name,
                                  a + 1 < function.args.size() ? ", " : "");
    }

    state.output += ");\n";
  }

  static void functionSignatureWithCallees(const Function &function,
                                           State &state) {
    if (state.emittedFunctionSignatures.contains(function.name)) {
      return;
    }

    if (function.name != VertFunctionName &&
        function.name != FragFunctionName) {
      functionSignature(function, state);
      state.emittedFunctionSignatures.insert(function.name);
    }

    for (const auto &fn : state.context.functions) {
      if (fn.name == function.name) {
        continue;
      }

      if (function.body.find(fn.name) != std::string::npos) {
        functionSignatureWithCallees(fn, state);
      }
    }
  }

  static void newLine(State &state) { state.output += '\n'; }

  static Optional<Output> program(Context &context) {
    State fragment{context, FragFunctionName};
    State vertex{context, VertFunctionName};

    version(fragment);
    newLine(fragment);

    version(vertex);
    newLine(vertex);

    if (!vertex.main) {
      error("no vertex main function");
      return {};
    }

    if (!fragment.main) {
      error("no fragment main function");
      return {};
    }

    std::vector<std::string> extensions{
        "precision mediump int;",
        "precision highp float;",
        "#extension GL_EXT_buffer_reference: require",
        // "#extension GL_EXT_debug_printf: require",
        "const float PI = 3.14159265359;",
    };

    for (const auto &extension : extensions) {
      vertex.output += extension;
      newLine(vertex);

      fragment.output += extension;
      newLine(fragment);
    }

    for (const auto &directive : context.directives) {
      vertex.output += fmt::format("#{}", directive);
      newLine(vertex);

      fragment.output += fmt::format("#{}", directive);
      newLine(fragment);
    }

    for (auto &varying : fragment.context.varyings) {
      if (const auto &assignment = fmt::format(" {} =", varying.name);
          vertex.main->body.find(assignment) != std::string::npos) {
        varyingOutput(varying, vertex);
        varyingInput(varying, fragment);
      } else if (fragment.main->body.find(assignment) != std::string::npos) {
        varyingOutput(varying, fragment);
      } else {
        varyingInput(varying, vertex);
      }
    }

    if (vertex.varyingInputIndex + vertex.varyingOutputIndex) {
      newLine(vertex);
    }

    if (fragment.varyingInputIndex + fragment.varyingOutputIndex) {
      newLine(fragment);
    }

    functionSignatureWithCallees(*vertex.main, vertex);
    functionSignatureWithCallees(*fragment.main, fragment);

    if (!vertex.emittedFunctionSignatures.empty()) {
      newLine(vertex);
    }

    if (!fragment.emittedFunctionSignatures.empty()) {
      newLine(fragment);
    }

    functionWithCallees(*vertex.main, vertex);
    functionWithCallees(*fragment.main, fragment);

    return {Output{fragment.output, vertex.output}};
  }

  static void varyingInput(const Varying &varying, State &state) {
    state.output +=
        fmt::format("layout (location = {}) in", state.varyingInputIndex);

    if (!varying.precision.empty()) {
      state.output += fmt::format(" {}", varying.precision);
    }

    state.output += fmt::format(" {} {};\n", varying.type, varying.name);

    state.varyingInputIndex++;
  }

  static void varyingOutput(const Varying &varying, State &state) {
    state.output +=
        fmt::format("layout (location = {}) out", state.varyingOutputIndex);

    if (!varying.precision.empty()) {
      state.output += fmt::format(" {}", varying.precision);
    }

    state.output += fmt::format(" {} {};\n", varying.type, varying.name);

    state.varyingOutputIndex++;
  }

  static void version(State &state) {
    state.output += fmt::format("#version {}\n", state.context.version);
  }
};

} // namespace ryuko