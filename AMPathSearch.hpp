#pragma once
#include "AMPath.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <functional>
#include <regex>
#include <string>
#include <vector>

constexpr char *exc1 = "am1sp2exg6";
constexpr char *exc2 = "amtmpexc5";
constexpr char *exc3 = "atmasdapxch";
constexpr char *exc4 = "amtmasdapaexch";
constexpr char *exc5 = "amtessixhxch";
constexpr char *exc6 = "amtasdmpexsch";

namespace AMPathSearch
{
    constexpr char *AMERROR = "ERROR";
    constexpr char *AMWARNING = "WARNING";
    namespace fs = std::filesystem;
    std::string regex_escape(const std::string &input)
    {
        std::string escaped;
        escaped.reserve(input.size() * 2); // 预分配内存优化性能

        for (char c : input)
        {
            // 匹配需要转义的正则元字符
            switch (c)
            {
            case '\\':
            case '^':
            case '$':
            case '.':
            case '|':
            case '?':
            case '*':
            case '+':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
                escaped += '\\'; // 添加转义符
                break;
            default:
                break;
            }
            escaped += c; // 追加当前字符
        }

        return escaped;
    }

    using CB = std::function<void(std::string, std::exception)>;

    enum class PathFormat
    {
        Absolute = 1,
        Relative = 2,
        User = 3
    };

    enum class SearchType
    {
        All = 0,
        File = 1,
        Directory = 2
    };

    std::tuple<std::vector<std::string>, std::string, bool> preprocess(std::string path, bool use_regex)
    {
        auto path_parts = AMPath::split(path);
        bool is2star = false;
        bool is_recursive = false;
        for (auto it = path_parts.begin(); it != path_parts.end(); ++it)
        {
            if (*it == "**")
            {
                is_recursive = true;
                if (is2star)
                {
                    path_parts.erase(it);
                }
                else
                {
                    is2star = true;
                }
            }
            else
            {
                is2star = false;
            }
        }

        std::vector<std::string> certain_parts;
        bool is_match;
        for (auto it = path_parts.begin(); it != path_parts.end(); ++it)
        {
            auto &part = *it;
            if (use_regex)
            {
                is_match = std::regex_match(part, std::regex("[\\*\\?<>\\+]"));
            }
            else
            {
                is_match = part.find('*') == std::string::npos;
            }
            if (!is_match)
            {
                certain_parts.push_back(part);
                path_parts.erase(it);
            }
            else
            {
                break;
            }
        }
        return std::make_tuple(path_parts, AMPath::join(certain_parts), is_recursive);
    }

    bool _match(std::string name, std::string pattern, bool use_regex)
    {
        if (!use_regex)
        {
            pattern = pattern.replace(pattern.find('*'), 1, std::string(exc1));
            pattern = regex_escape(pattern);
            pattern = pattern.replace(pattern.find(exc1), std::string(exc1).size(), ".*");
            pattern = '^' + pattern + '$';
            return std::regex_match(name, std::regex(pattern));
        }
        else
        {
            pattern = pattern.replace(pattern.find('*'), 1, std::string(exc1));
            pattern = pattern.replace(pattern.find('?'), 1, std::string(exc2));
            pattern = pattern.replace(pattern.find('<'), 1, std::string(exc3));
            pattern = pattern.replace(pattern.find('>'), 1, std::string(exc4));
            pattern = pattern.replace(pattern.find('+'), 1, std::string(exc5));
            pattern = regex_escape(pattern);
            pattern = pattern.replace(pattern.find(exc1), std::string(exc1).size(), ".*");
            pattern = pattern.replace(pattern.find(exc2), std::string(exc2).size(), "?");
            pattern = pattern.replace(pattern.find(exc3), std::string(exc3).size(), "[");
            pattern = pattern.replace(pattern.find(exc4), std::string(exc4).size(), "]");
            pattern = pattern.replace(pattern.find(exc5), std::string(exc5).size(), "+");
            pattern = '^' + pattern + '$';
            return std::regex_match(name, std::regex(pattern));
        }
    }

    void search(std::vector<std::string> &results, fs::path root, std::string name, std::vector<std::string> remains, SearchType type, bool use_regex, bool silence, CB callback)
    {
        if (!remains.empty())
        {
            if (!fs::is_directory(root))
            {
                return;
            }
            try
            {
                for (auto &entry : fs::directory_iterator(root))
                {
                    if (name == "**")
                    {
                        if (_match(entry.path().filename().string(), remains.front(), use_regex))
                        {
                            if (fs::is_directory(entry.path()))
                            {
                                auto new_name = remains.front();
                                auto new_remains = remains;
                                new_remains.erase(new_remains.begin());
                                search(results, entry.path(), new_name, new_remains, type, use_regex, silence, callback);
                            }
                            else if (type != SearchType::Directory)
                            {
                                results.push_back(entry.path().string());
                            }
                        }
                        else if (fs::is_directory(entry.path()))
                        {
                            search(results, entry.path(), name, remains, type, use_regex, silence, callback);
                        }
                        continue;
                    }
                    else if (_match(entry.path().filename().string(), name, use_regex) && fs::is_directory(entry.path()))
                    {
                        std::string new_name = remains.front();
                        auto new_remains = remains;
                        new_remains.erase(new_remains.begin());
                        search(results, entry.path(), new_name, new_remains, type, use_regex, silence, callback);
                    }
                }
            }
            catch (const std::exception e)
            {
                if (!silence && callback)
                {
                    callback(AMERROR, e);
                }
            }
        }
        else
        {
            if (!fs::is_directory(root))
            {
                if (type == SearchType::Directory && _match(root.filename().string(), name, use_regex))
                {
                    results.push_back(root.string());
                }
                return;
            }

            if (name == "**")
            {
                for (auto &entry : fs::directory_iterator(root))
                {
                    bool is_dir = fs::is_directory(entry.path());
                    if (!is_dir)
                    {
                        if (type != SearchType::Directory)
                        {
                            results.push_back(entry.path().string());
                        }
                        continue;
                    }
                    if (fs::is_empty(entry.path()))
                    {
                        if (type != SearchType::File)
                        {
                            results.push_back(entry.path().string());
                        }
                        continue;
                    }
                    search(results, entry.path(), "**", {}, type, use_regex, silence, callback);
                }
                return;
            }

            bool is_dir2;
            for (auto &entry : fs::directory_iterator(root))
            {
                if (!_match(entry.path().filename().string(), name, use_regex))
                {
                    continue;
                }
                is_dir2 = fs::is_directory(entry.path());
                if (is_dir2 && type != SearchType::File)
                {
                    results.push_back(entry.path().string());
                }
                else if (!is_dir2 && type != SearchType::Directory)
                {
                    results.push_back(entry.path().string());
                }
            }
        }
    }

    std::variant<std::vector<std::string>, std::pair<std::string, std::string>> find(std::string path, SearchType type = SearchType::All, bool use_regex = false, bool silence = false, CB callback = nullptr)
    {
        path = AMPath::realpath(Strip(path), false, '\\');
        if (fs::exists(path) && !use_regex)
        {
            return std::vector<std::string>{path};
        }
        auto pre_result = preprocess(path, use_regex);
        auto root_path = std::get<1>(pre_result);
        auto match_parts = std::get<0>(pre_result);
        bool is_recursive = std::get<2>(pre_result);
        std::vector<std::string> results;

        if (root_path.empty())
        {
            return std::make_pair(AMERROR, fmt::format("Can't parse the root of the path: {}", path));
        }

        if (!fs::exists(root_path))
        {
            return {};
        }

        if (match_parts.empty())
        {
            return std::vector<std::string>{root_path};
        }
        std::string name_first = match_parts.front();
        match_parts.erase(match_parts.begin());

        search(results, fs::path(root_path), name_first, match_parts, type, use_regex, silence, callback);
        return results;
    }

}
