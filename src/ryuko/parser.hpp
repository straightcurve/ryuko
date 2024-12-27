#pragma once

#include <ryuko/core.hpp>

namespace ryuko {

struct Parser {
private:
  std::string input;
  size_t index;

public:
  explicit Parser(std::string input) : input(std::move(input)), index(0) {}

public:
  [[nodiscard]] bool alphanumeric(const size_t i) const {
    return std::isalnum(input[i]) || input[i] == '_';
  }

  std::string consumeCharacter() { return input.substr(index++, 1); }

  Optional<Function> consumeFunction() {
    const auto start = index;
    Function function{};

    function.returnType = consumeIdentifier();
    consumeWhitespace();

    function.name = consumeIdentifier();
    consumeWhitespace();

    if (!match("(")) {
      index = start;

      return {};
    }

    if (!expect('(')) {
      return {};
    }

    consumeWhitespace();

    if (match(")")) {
      //  @note: function has no arguments
      if (!expect(')')) {
        return {};
      }
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

        if (!expect(',')) {
          index = start;

          return {};
        }

        consumeWhitespace();
      }

      if (!expect(')')) {
        index = start;

        return {};
      }
    }

    consumeWhitespace();

    if (!match("{")) {
      index = start;

      return {};
    }

    if (!expect('{')) {
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
      printError('}');

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
      auto e = 0;
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

    if (!expect(';')) {
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

  [[nodiscard]] bool expect(const char expected) {
    if (input[index] == expected) {
      index++;

      return true;
    }

    printError(expected);

    return false;
  }

  [[nodiscard]] bool match(const std::string &expected) const {
    const auto start = index;

    auto e = 0;
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

  Optional<Context> parse() {
    Context context{};

    while (!done()) {
      consumeWhitespace();

      if (auto result = consumeVersion(); result.has_value()) {
        context.version = result.value_or(450);
        continue;
      }

      if (auto result = consumeDirective(); result.has_value()) {
        if (result.value() != "dawn_inline_frag") {
          context.directives.push_back(result.value());
          continue;
        }

        auto inlinedCode = consumeUntil("#dawn_inline_frag");
        context.inlinedFragmentCode.push_back(inlinedCode);

        consumeDirective();
        continue;
      }

      if (match("//")) {
        consumeUntil("\n");
        consumeWhitespace();
        continue;
      }

      if (auto result = consumeFunction(); result.has_value()) {
        context.functions.push_back(result.value());
        continue;
      }

      if (auto result = consumeVarying(); result.has_value()) {
        context.varyings.push_back(result.value());
        continue;
      }

      consumeUntil("\n");
      consumeWhitespace();
    }

    return {context};
  }

  void printError(const char expected) {
    fmt::println("[{}] {}", index, input.substr(0, index));

    error("expected character '{}' at index {}, got '{}'", expected, index,
          input[index]);
  }

  [[nodiscard]] char peek() const { return input[index]; }

  [[nodiscard]] bool whitespace(const size_t i) const {
    return input[i] == '\t' || input[i] == '\n' || std::isspace(input[i]);
  }
};

} // namespace ryuko
