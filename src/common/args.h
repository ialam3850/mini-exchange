#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace mx {

class Args {
 public:
  Args(int argc, char** argv) : prog_(argv[0]) {
    for (int i = 1; i < argc; ++i) items_.emplace_back(argv[i]);
  }

  std::string get(std::string_view key, std::string def) const {
    for (std::size_t i = 0; i + 1 < items_.size(); ++i)
      if (items_[i] == key) return items_[i + 1];
    return def;
  }
  long get_int(std::string_view key, long def) const {
    auto v = get(key, "");
    return v.empty() ? def : std::strtol(v.c_str(), nullptr, 10);
  }
  double get_double(std::string_view key, double def) const {
    auto v = get(key, "");
    return v.empty() ? def : std::strtod(v.c_str(), nullptr);
  }
  bool flag(std::string_view key) const {
    for (const auto& s : items_)
      if (s == key) return true;
    return false;
  }

  static std::vector<std::string> split(const std::string& s, char sep = ',') {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
      if (c == sep) {
        if (!cur.empty()) out.push_back(cur);
        cur.clear();
      } else {
        cur.push_back(c);
      }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
  }

 private:
  std::string prog_;
  std::vector<std::string> items_;
};

}
