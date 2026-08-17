// Local compatibility addition for the MinGW GCC 7.3 toolchain shipped with Qt 5.12.
// SPDX-License-Identifier: MIT

#ifndef SYSCMDLINE_FILESYSTEM_COMPAT_P_H
#define SYSCMDLINE_FILESYSTEM_COMPAT_P_H

#include <string>

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#  include <experimental/filesystem>
namespace scl_fs = std::experimental::filesystem;
#else
#  include <filesystem>
namespace scl_fs = std::filesystem;
#endif

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
inline std::string scl_stream_path(const scl_fs::path &path) {
    return path.string();
}
#else
inline const scl_fs::path &scl_stream_path(const scl_fs::path &path) {
    return path;
}
#endif

#endif // SYSCMDLINE_FILESYSTEM_COMPAT_P_H
