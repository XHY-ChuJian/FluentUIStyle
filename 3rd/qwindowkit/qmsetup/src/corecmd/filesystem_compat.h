// Local compatibility addition for the MinGW GCC 7.3 toolchain shipped with Qt 5.12.
// SPDX-License-Identifier: MIT

#ifndef QMCORECMD_FILESYSTEM_COMPAT_H
#define QMCORECMD_FILESYSTEM_COMPAT_H

#include <string>

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#  include <experimental/filesystem>

namespace qm_filesystem {
    using namespace std::experimental::filesystem;

    // GCC 7 provides the filesystem TS but not C++17's relative(). The paths
    // used by qmcorecmd are absolute or canonical before reaching this helper,
    // so a component-wise relative path is sufficient and preserves the
    // behavior of the standard function for these call sites.
    inline path relative(const path &target, const path &base) {
        const path absoluteTarget = absolute(target);
        const path absoluteBase = absolute(base);

        if (absoluteTarget.root_name() != absoluteBase.root_name() ||
            absoluteTarget.root_directory() != absoluteBase.root_directory()) {
            return {};
        }

        auto targetPart = absoluteTarget.begin();
        auto basePart = absoluteBase.begin();
        while (targetPart != absoluteTarget.end() && basePart != absoluteBase.end() &&
               *targetPart == *basePart) {
            ++targetPart;
            ++basePart;
        }

        path result;
        for (; basePart != absoluteBase.end(); ++basePart) {
            if (*basePart != path(".")) {
                result /= "..";
            }
        }
        for (; targetPart != absoluteTarget.end(); ++targetPart) {
            result /= *targetPart;
        }
        return result.empty() ? path(".") : result;
    }
}
#else
#  include <filesystem>
namespace qm_filesystem = std::filesystem;
#endif

namespace fs = qm_filesystem;

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
inline std::string qm_stream_path(const fs::path &path) {
    return path.string();
}
#else
inline const fs::path &qm_stream_path(const fs::path &path) {
    return path;
}
#endif

#endif // QMCORECMD_FILESYSTEM_COMPAT_H
