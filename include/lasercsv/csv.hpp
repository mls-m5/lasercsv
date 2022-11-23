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
    NoCopy(NoCopy &&) = default;
    NoCopy &operator=(const NoCopy &) = delete;
    NoCopy &operator=(NoCopy &&) = default;
};

class NoCopyNoMove {
public:
    NoCopyNoMove() = default;

    NoCopyNoMove(const NoCopyNoMove &) = delete;
    NoCopyNoMove(NoCopyNoMove &&) = delete;
    NoCopyNoMove &operator=(const NoCopyNoMove &) = delete;
    NoCopyNoMove &operator=(NoCopyNoMove &&) = delete;
};

// Contains all the original data from the file
class File : NoCopyNoMove {
public:
    File(std::filesystem::path path);
    File(std::filesystem::path path, std::string content)
        : _path{path}
        , _content{std::move(content)} {}

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

    operator std::string_view() const {
        return _content;
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

class Table;

class ColumnView {
    ColumnView(const Table &table, size_t col)
        : table{table}
        , col{col} {}

    const Table &table;
    size_t col = 0;
};

// Contains the original data
// If this class is destroyed all std::string_view's values will be invalid
class Table : NoCopyNoMove {
public:
    Table(std::filesystem::path path)
        : _file{path} {
        parseFile();
    }

    static Table fromString(std::string str,
                            std::filesystem::path path = "memory") {
        return Table{path, std::move(str)};
    }

    std::unique_ptr<Table> create(std::filesystem::path path) {
        return std::make_unique<Table>(path);
    }

    const auto &rows() const {
        return _rows;
    }

    bool empty() const {
        return _rows.empty();
    }

    size_t width() const {
        return _rows.front().size();
    }

    size_t height() const {
        return _rows.size();
    }

private:
    Table(std::filesystem::path path, std::string content)
        : _file(path, std::move(content)) {
        parseFile();
    };
    void parseFile();

    const File _file;
    std::vector<Row> _rows;
};

inline File::File(std::filesystem::path path)
    : _path{path} {
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
    int currentRowNum = 1;

    for (size_t i = 0; i < content.size(); ++i) {
        auto c = content.at(i);

        if (c == '\n') {
            if (!current.empty()) {
                row.push_back(current);
                current = {};
            }
            row.source({content.data() + rowBegin, i - rowBegin});
            rowSize = row.size();
            if (!row.empty()) {
                _rows.push_back(std::move(row));
            }
            row.clear();
            row.reserve(rowSize);
            rowBegin = i + 1;
            ++currentRowNum;
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
                    current = {};
                    i = j;

                    ++i;

                    if (content.at(i) != ',') {
                        throw std::invalid_argument{
                            "at " + _file.path().string() + ":" +
                            std::to_string(currentRowNum) +
                            ": expected ',' after " +
                            std::string{content.data() + rowBegin, 10}};
                    }

                    break;
                }
            }
            continue;
        }

        current = {&content.at(i), 1};
    }

    row.source(content.substr(rowBegin));
    if (!row.empty()) {
        _rows.push_back(std::move(row));
    }
}

} // namespace lasercsv
