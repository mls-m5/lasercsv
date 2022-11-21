#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace lasercsv {

class NoCopy {
public:
    NoCopy() = default;

    NoCopy(const NoCopy &) = delete;
    NoCopy(NoCopy &&) = delete;
    NoCopy &operator=(const NoCopy &) = delete;
    NoCopy &operator=(NoCopy &&) = delete;
};

class File : NoCopy {
public:
    File(std::filesystem::path path);

    std::string_view content() const {
        return _content;
    }

    std::filesystem::path path() const {
        return _path;
    }

private:
    std::string _content;
    std::filesystem::path _path;
};

class Table : NoCopy {
public:
    Table(std::filesystem::path path)
        : _file{path} {
        parseFile();
    }

    const auto &rows() const {
        return _rows;
    }

private:
    void parseFile() {
        auto row = std::vector<std::string_view>{};

        auto content = _file.content();

        auto current = std::string_view{};

        for (size_t i = 0; i < content.size(); ++i) {
            auto c = content.at(i);

            if (c == '\n') {
                _rows.push_back(std::move(row));
                row.clear();
                continue;
            }

            if (c == ',') {
                row.push_back(current);
                current = {};
                continue;
            }

            if (current.empty()) {
                current = {&content.at(i), 1};
            }
            else {
                current = {current.data(), current.size() + 1};
            }
        }
    }

    const File _file;
    std::vector<std::vector<std::string_view>> _rows;
};

inline File::File(std::filesystem::path path) {
    auto file = std::ifstream{path};

    if (!file) {
        throw std::runtime_error{"could not open file " + path.string()};
    }

    for (std::string line; std::getline(file, line);) {
        if (line.back() == '\r') {
            _content.pop_back();
        }
        line += "\n";
        _content += line;
    }

    _content.shrink_to_fit();
}

} // namespace lasercsv
