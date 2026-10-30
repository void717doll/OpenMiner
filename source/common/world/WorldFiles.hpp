#ifndef WORLDFILES_HPP_
#define WORLDFILES_HPP_

#include <string>
#include <filesystem.hpp>

namespace worldFiles {
namespace fs = ghc::filesystem;
inline fs::path directory(const std::string &name) { return fs::path("saves") / name; }
inline fs::path database(const std::string &name) { return directory(name) / "map.sqlite"; }
inline fs::path legacy(const std::string &name) { return fs::path("saves") / (name + ".dat"); }
inline bool exists(const std::string &name) { return fs::exists(database(name)) || fs::exists(legacy(name)); }
inline fs::path readable(const std::string &name) { return fs::exists(database(name)) ? database(name) : legacy(name); }
}

#endif
