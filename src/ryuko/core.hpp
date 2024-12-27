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
};

struct Context {
  std::vector<std::string> directives;
  std::vector<Function> functions;
  std::vector<Varying> varyings;
  std::vector<std::string> inlinedFragmentCode;
  int version;
};

} // namespace ryuko
