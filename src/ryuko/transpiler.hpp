#pragma once

#include <ryuko/core.hpp>

namespace ryuko {

struct Transpiler {
  std::vector<Function> &functions;
  std::vector<Varying> &varyings;

public:
  explicit Transpiler(std::vector<Function> &functions,
                      std::vector<Varying> &varyings)
      : functions(functions), varyings(varyings) {}

public:
  void setReturnValues() const {
    Function *vert = findFunction(VertFunctionName);
    if (!vert) {
      error("no vertex main function");
      return;
    }

    if (auto expression = extractReturnExpression(*vert);
        expression.has_value()) {
      if (const auto index = vert->body.find("return");
          index != std::string::npos) {
        vert->body = vert->body.substr(0, index);
        vert->body +=
            fmt::format("gl_Position = {};\n{}", expression.value(), "}");
      }
    } else {
      error("vert() must return a vec4");
    }

    if (Function *frag = findFunction(FragFunctionName)) {
      if (auto expression = extractReturnExpression(*frag);
          expression.has_value()) {
        if (const auto index = frag->body.find("return");
            index != std::string::npos) {
          const Varying outColor{"ryuko_outColor", "vec4", "highp"};
          frag->body = frag->body.substr(0, index);
          frag->body += fmt::format("{} = {};\n{}", outColor.name,
                                    expression.value(), "}");

          varyings.push_back(outColor);
        }
      } else {
        error("frag() must return a vec4");
      }
    }
  }

private:
  [[nodiscard]]
  static Optional<std::string>
  extractReturnExpression(const Function &function) {
    const auto &body = function.body;
    if (const auto index = body.find("return"); index != std::string::npos) {
      const auto start = index + 6;
      if (const auto end = body.find_first_of(';', start);
          end != std::string::npos) {
        Parser p{std::string{body.begin() + start, body.begin() + end}};
        p.consumeWhitespace();
        return p.consumeUntil(";");
      }
    }

    return {};
  };

  [[nodiscard]] Function *findFunction(const std::string &name) const {
    const auto it = std::ranges::find_if(
        functions, [&](const auto &f) { return f.name == name; });
    return (it != std::ranges::end(functions)) ? &(*it) : nullptr;
  }
};

} // namespace ryuko