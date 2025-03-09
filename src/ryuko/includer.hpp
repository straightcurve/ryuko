#pragma once

#include <ryuko/pch.hpp>

namespace ryuko {

class CustomIncluder final : public shaderc::CompileOptions::IncluderInterface {
public:
  shaderc_include_result *
  GetInclude(const char *requestedSource, shaderc_include_type type,
             const char *requestingSource,
             [[maybe_unused]] size_t includeDepth) override {
    if (type != shaderc_include_type_relative) {
      error("only relative includes are supported right now\n  {} requested by "
            "{}",
            requestedSource, requestingSource);
      return nullptr;
    }

    std::filesystem::path path{requestingSource};
    std::ifstream file(path.parent_path() / requestedSource);
    if (!file.is_open()) {
      return make_error_result(fmt::format(
          "[shaderc][include] {} tried to include {}, but file not found.",
          path.c_str(), (path.parent_path() / requestedSource).c_str()));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    auto *content = new std::string(buffer.str());

    return new shaderc_include_result{requestedSource, strlen(requestedSource),
                                      content->c_str(), content->size(),
                                      content};
  }

  void ReleaseInclude(shaderc_include_result *includeResult) override {
    if (includeResult) {
      delete static_cast<std::string *>(includeResult->user_data);
      delete includeResult;
    }
  }

private:
  shaderc_include_result *make_error_result(const std::string &errorMessage) {
    const auto content = new std::string(errorMessage);
    return new shaderc_include_result{errorMessage.c_str(), errorMessage.size(),
                                      content->c_str(), content->size(),
                                      content};
  }
};

} // namespace ryuko