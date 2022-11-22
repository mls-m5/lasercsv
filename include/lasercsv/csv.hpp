#pragma once

#include <charconv>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>
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

// Contains all the original data from the file
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

class Cell {
public:
    template <typename... Args>
    Cell(Args &&...args)
        : _content{std::forward<Args>(args)...} {}

    std::string_view str() const {
        return _content;
    }

    // Convert content to other type
    template <typename T>
    T as() const {
        if constexpr (std::is_same_v<std::string, T>) {
            return std::string{_content};
        }
        else {
            auto ret = T{};

            auto [ptr, ec] = std::from_chars(
                _content.data(), _content.data() + _content.size(), ret);

            if (ec == std::errc{}) {
            }
            else if (ec == std::errc::invalid_argument) {
                throw std::invalid_argument{"to<...>(...): \"" +
                                            std::string{_content} + "\""};
            }
            else if (ec == std::errc::result_out_of_range) {
                throw std::out_of_range{"to<...>(...): result is out of range"};
            }

            return ret;
        }
    }

    std::string_view content() const {
        return _content;
    }

    friend std::ostream &operator<<(std::ostream &stream, const Cell &cell) {
        stream << cell._content;
        return stream;
    }

private:
    std::string_view _content;
};

class Row : public std::vector<Cell> {
public:
    using vector::vector;

    // Access original text line from file
    void source(std::string_view content) {
        _source = content;
    }

    std::string_view source() const {
        return _source;
    }

private:
    std::string_view _source;
};

// Contains the original data
// If this class is destroyed all std::string_view's values will be invalid
class Table : NoCopy {
public:
    Table(std::filesystem::path path)
        : _file{path} {
        parseFile();
    }

    std::unique_ptr<Table> create(std::filesystem::path path) {
        return std::make_unique<Table>(path);
    }

    const auto &rows() const {
        return _rows;
    }

private:
    void parseFile();

    const File _file;
    std::vector<Row> _rows;
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

inline void Table::parseFile() {
    auto row = Row{};

    auto content = _file.content();

    auto current = std::string_view{};

    int rowSize = 0;
    int rowBegin = 0;

    for (size_t i = 0; i < content.size(); ++i) {
        auto c = content.at(i);

        if (c == '\n') {
            row.source({content.data() + rowBegin, i - rowBegin});
            rowSize = row.size();
            _rows.push_back(std::move(row));
            row.clear();
            row.reserve(rowSize);
            rowBegin = i + 1;
            continue;
        }

        if (c == ',') {
            row.push_back(current);
            current = {};
            continue;
        }

        if (!current.empty()) {
            current = {current.data(), current.size() + 1};
            continue;
        }

        if (c == '\"') {
            for (size_t j = i + 1; j < content.size(); ++j) {
                auto d = content.at(j);

                if (d == '\\' && content.at(j + 1) == '"') {
                    ++j;
                    continue;
                }

                if (d == '\"') {
                    row.emplace_back(content.data() + i + 1, j - i - 1);
                    i = j;
                    break;
                }
            }
            continue;
        }

        current = {&content.at(i), 1};
    }

    _rows.push_back(std::move(row));
}

} // namespace lasercsv
