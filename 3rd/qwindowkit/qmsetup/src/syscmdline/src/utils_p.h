#ifndef UTILS_H
#define UTILS_H

#include <cstring>
#include <string>
#include <vector>

namespace SysCmdLine::Utils {

    std::vector<std::string> split(const std::string &s, const std::string &delimiter);

    std::string join(const std::vector<std::string> &v, const std::string &delimiter);

    std::string formatText(const std::string &format, const std::vector<std::string> &args);

    std::vector<std::string> calcClosestTexts(const std::vector<std::string> &texts,
                                              const std::string &input, int threshold);

    std::string toUpper(std::string s);

    std::string toLower(std::string s);

    inline bool starts_with(const std::string_view &s, const std::string_view &prefix) {
#if __cplusplus >= 202002L
        return s.starts_with(prefix);
#else
        return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
#endif
    }

    template <class Container, class T>
    inline bool contains(const Container &container, const T &key) {
#if __cplusplus >= 202002L
        return container.contains(key);
#else
        return container.count(key) != 0;
#endif
    }

    template <class T>
    std::vector<T> concatVector(const std::vector<T> &v1, const std::vector<T> &v2) {
        std::vector<T> res;
        res.reserve(v1.size() + v2.size());
        for (const auto &item : v1)
            res.emplace_back(item);
        for (const auto &item : v2)
            res.emplace_back(item);
        return res;
    }

    template <class T>
    std::basic_string<T> trim(const std::basic_string<T> &str) {
        auto start = str.begin();
        while (start != str.end() && std::isspace(*start)) {
            start++;
        }

        auto end = str.end();
        do {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(*end));

        return {start, end + 1};
    }

}


#endif // UTILS_H
