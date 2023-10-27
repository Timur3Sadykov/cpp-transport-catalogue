#include "json.h"
#include <cassert>
#include <sstream>

namespace json
{
namespace {
    using namespace std::literals;
    using Number = std::variant<int, double>;

    Number LoadNumber(std::istream& input) {
        std::string parsed_num;

        // Считывает в parsed_num очередной символ из input
        auto read_char = [&parsed_num, &input] {
            parsed_num += static_cast<char>(input.get());
            if (!input) {
                throw ParsingError("Failed to read number from stream"s);
            }
        };

        // Считывает одну или более цифр в parsed_num из input
        auto read_digits = [&input, read_char] {
            if (!std::isdigit(input.peek())) {
                throw ParsingError("A digit is expected"s);
            }
            while (std::isdigit(input.peek())) {
                read_char();
            }
        };

        if (input.peek() == '-') {
            read_char();
        }
        // Парсим целую часть числа
        if (input.peek() == '0') {
            read_char();
            // После 0 в JSON не могут идти другие цифры
        } else {
            read_digits();
        }

        bool is_int = true;
        // Парсим дробную часть числа
        if (input.peek() == '.') {
            read_char();
            read_digits();
            is_int = false;
        }

        // Парсим экспоненциальную часть числа
        if (int ch = input.peek(); ch == 'e' || ch == 'E') {
            read_char();
            if (ch = input.peek(); ch == '+' || ch == '-') {
                read_char();
            }
            read_digits();
            is_int = false;
        }

        try {
            if (is_int) {
                // Сначала пробуем преобразовать строку в int
                try {
                    return std::stoi(parsed_num);
                } catch (...) {
                    // В случае неудачи, например, при переполнении,
                    // код ниже попробует преобразовать строку в double
                }
            }
            return std::stod(parsed_num);
        } catch (...) {
            throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
        }
    }

    // Считывает содержимое строкового литерала JSON-документа
    // Функцию следует использовать после считывания открывающего символа ":
    std::string LoadString(std::istream& input) {
        auto it = std::istreambuf_iterator<char>(input);
        auto end = std::istreambuf_iterator<char>();
        std::string s;
        while (true) {
            if (it == end) {
                // Поток закончился до того, как встретили закрывающую кавычку?
                throw ParsingError("String parsing error"s);
            }
            const char ch = *it;
            if (ch == '"') {
                // Встретили закрывающую кавычку
                ++it;
                break;
            } else if (ch == '\\') {
                // Встретили начало escape-последовательности
                ++it;
                if (it == end) {
                    // Поток завершился сразу после символа обратной косой черты
                    throw ParsingError("String parsing error"s);
                }
                const char escaped_char = *(it);
                // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        // Встретили неизвестную escape-последовательность
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                }
            } else if (ch == '\n' || ch == '\r') {
                // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                throw ParsingError("Unexpected end of line"s);
            } else {
                // Просто считываем очередной символ и помещаем его в результирующую строку
                s.push_back(ch);
            }
            ++it;
        }

        return s;
    }

    Node LoadNode(std::istream& input);

    Node::Object LoadObject(std::istream& input) {
        Node::Object object;
        char c;

        while (input >> c && c != '}') {
            if (c != ',') {
                input.putback(c);
            }

            auto key = LoadNode(input);

            if (key.IsString()) {
                input >> c;

                while (c != ':') {
                    input >> c;
                }

                object.insert({std::move(key.As<std::string>()), LoadNode(input)});
            } else {
                throw ParsingError("Object parsing error"s);
            }
        }

        if (c != '}') {
            throw ParsingError("Object parsing error"s);
        }

        return object;
    }

    Node::Array LoadArray(std::istream& input) {
        Node::Array array;
        char c;

        while (input >> c && c != ']') {
            if (c != ',') {
                input.putback(c);
            }

            array.push_back(LoadNode(input));
        }

        if (c != ']') {
            throw ParsingError("Array parsing error"s);
        }

        return array;
    }

    Node LoadNode(std::istream& input) {
        char ch = ' ';

        while (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r') {
            input >> ch;
        }

        if (ch == 'n') {
            char u, l1, l2;
            input >> u >> l1 >> l2;

            if (u == 'u' && l1 == 'l' && l2 == 'l') {
                return {};
            }

            throw ParsingError("Parsing error"s);
        } else if (ch == 't') {
            char r, u, e;
            input >> r >> u >> e;

            if (r == 'r' && u == 'u' && e == 'e') {
                return true;
            }

            throw ParsingError("Parsing error"s);
        } else if (ch == 'f') {
            char a, l, s, e;
            input >> a >> l >> s >> e;

            if (a == 'a' && l == 'l' && s == 's' && e == 'e') {
                return false;
            }

            throw ParsingError("Parsing error"s);
        } else if (ch == '"') {
            return LoadString(input);
        } else if (ch == '{') {
            return LoadObject(input);
        } else if (ch == '[') {
            return LoadArray(input);
        } else {
            input.putback(ch);
            const auto number = LoadNumber(input);

            if (std::holds_alternative<int>(number)) {
                return std::get<int>(number);
            }

            return std::get<double>(number);
        }
    }

    void PrintNode(const Node& node, std::ostream& out);

    // Шаблон для вывода double и int
    template<typename Value>
    void PrintValue(const Value& value, std::ostream& out) {
        out << value;
    }

    // Перегрузка функции PrintValue для вывода значений null
    void PrintValue(std::nullptr_t, std::ostream& out) {
        out << "null"sv;
    }

    void PrintValue(bool b, std::ostream& out) {
        out << std::boolalpha << b;
    }

    // Перегрузка функции PrintValue для вывода значений string
    void PrintValue(const std::string& str, std::ostream& out) {
        out << "\"";
        for (auto ch: str) {
            switch (ch) {
                case '\n':
                    out << "\\n";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                case '\"':
                    out << "\\\"";
                    break;
                case '\\':
                    out << "\\\\";
                    break;
                default:
                    out << ch;
                    break;
            }
        }
        out << "\"";
    }

    void PrintValue(const Node::Object& object, std::ostream& out) {
        out << "{";
        for (auto it = object.begin(), last_el_it = --object.end(); it != object.end(); ++it) {
            out << "\"" << it->first << "\":";
            PrintNode(it->second, out);

            if (it != last_el_it) {
                out << ",";
            }
        }
        out << "}";
    }

    void PrintValue(const Node::Array& array, std::ostream& out) {
        out << "[";
        for (auto it = array.begin(), last_el_it = --array.end(); it != array.end(); ++it) {
            PrintNode(*it, out);

            if (it != last_el_it) {
                out << ",";
            }
        }
        out << "]";
    }

    void PrintNode(const Node& node, std::ostream& out) {
        std::visit(
                [&out](const auto& value) { PrintValue(value, out); },
                node.GetValue());
    }
}

Node::Node(std::nullptr_t) {}

Node::Node(bool value)
    : value_(value) {}

Node::Node(int value)
    : value_(value) {}

Node::Node(double value)
    : value_(value) {}

Node::Node(std::string value)
    : value_(std::move(value)) {}

Node::Node(Object object)
    : value_(std::move(object)) {}

Node::Node(Array array)
    : value_(std::move(array)) {}

const Node::Value& Node::GetValue() const noexcept {
    return value_;
}

bool Node::IsNull() const noexcept {
    return Is<std::nullptr_t>();
}

bool Node::IsBool() const noexcept {
    return Is<bool>();
}

bool Node::IsInt() const noexcept {
    return Is<int>();
}

bool Node::IsDouble() const noexcept {
    return Is<int>() || Is<double>();
}

bool Node::IsPureDouble() const noexcept {
    return Is<double>();
}

bool Node::IsString() const noexcept {
    return Is<std::string>();
}

bool Node::IsObject() const noexcept {
    return Is<Object>();
}

bool Node::IsArray() const noexcept {
    return Is<Array>();
}

bool Node::AsBool() const {
    return As<bool>();
}

int Node::AsInt() const {
    return As<int>();
}

double Node::AsDouble() const {
    if (IsInt()) {
        return As<int>();
    }

    return As<double>();
}

const std::string& Node::AsString() const {
    return As<std::string>();
}

std::string& Node::AsString() {
    return As<std::string>();
}

const Node::Object& Node::AsObject() const {
    return As<Object>();
}

Node::Object& Node::AsObject() {
    return As<Object>();
}

const Node::Array& Node::AsArray() const {
    return As<Array>();
}

Node::Array& Node::AsArray() {
    return As<Array>();
}

bool Node::Contains(const std::string& key) const {
    return IsObject() && As<Object>().find(key) != As<Object>().end();
}

const Node& Node::At(const std::string& key) const {
    const auto& object = As<Object>();
    const auto it = object.find(key);

    if (it == object.end()) {
        throw NodeOutOfRange("The key does not exist"s);
    }

    return it->second;
}

Node& Node::At(const std::string& key) {
    auto& object = As<Object>();
    const auto it = object.find(key);

    if (it == object.end()) {
        throw NodeOutOfRange("The key does not exist"s);
    }

    return it->second;
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}
}