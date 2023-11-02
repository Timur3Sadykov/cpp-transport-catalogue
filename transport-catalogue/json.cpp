#include "json.h"
#include <cassert>
#include <sstream>

namespace json
{
    namespace {
        using namespace std::literals;

        Node LoadNode(std::istream& input);
        Node LoadString(std::istream& input);

        std::string LoadLiteral(std::istream& input) {
            std::string s;
            while (std::isalpha(input.peek())) {
                s.push_back(static_cast<char>(input.get()));
            }
            return s;
        }

        Node LoadArray(std::istream& input) {
            std::vector<Node> result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            if (!input) {
                throw ParsingError("Array parsing error"s);
            }
            return Node(std::move(result));
        }

        Node LoadObject(std::istream& input) {
            Node::Object object;

            for (char c; input >> c && c != '}';) {
                if (c == '"') {
                    std::string key = LoadString(input).AsString();
                    if (input >> c && c == ':') {
                        if (object.find(key) != object.end()) {
                            throw ParsingError("Duplicate key '"s + key + "' have been found");
                        }
                        object.emplace(std::move(key), LoadNode(input));
                    } else {
                        throw ParsingError(": is expected but '"s + c + "' has been found"s);
                    }
                } else if (c != ',') {
                    throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
                }
            }
            if (!input) {
                throw ParsingError("Dictionary parsing error"s);
            }
            return Node(std::move(object));
        }

        Node LoadString(std::istream& input) {
            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    ++it;
                    break;
                } else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
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
                            throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                } else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                } else {
                    s.push_back(ch);
                }
                ++it;
            }

            return Node(std::move(s));
        }

        Node LoadBool(std::istream& input) {
            const auto s = LoadLiteral(input);
            if (s == "true"sv) {
                return Node{true};
            } else if (s == "false"sv) {
                return Node{false};
            } else {
                throw ParsingError("Failed to parse '"s + s + "' as bool"s);
            }
        }

        Node LoadNull(std::istream& input) {
            if (auto literal = LoadLiteral(input); literal == "null"sv) {
                return Node{nullptr};
            } else {
                throw ParsingError("Failed to parse '"s + literal + "' as null"s);
            }
        }

        Node LoadNumber(std::istream& input) {
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
                        // В случае неудачи, например, при переполнении
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return std::stod(parsed_num);
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadNode(std::istream& input) {
            char c;
            if (!(input >> c)) {
                throw ParsingError("Unexpected EOF"s);
            }
            switch (c) {
                case '[':
                    return LoadArray(input);
                case '{':
                    return LoadObject(input);
                case '"':
                    return LoadString(input);
                case 't':
                    // Атрибут [[fallthrough]] (провалиться) ничего не делает, и является
                    // подсказкой компилятору и человеку, что здесь программист явно задумывал
                    // разрешить переход к инструкции следующей ветки case, а не случайно забыл
                    // написать break, return или throw.
                    // В данном случае, встретив t или f, переходим к попытке парсинга
                    // литералов true либо false
                    [[fallthrough]];
                case 'f':
                    input.putback(c);
                    return LoadBool(input);
                case 'n':
                    input.putback(c);
                    return LoadNull(input);
                default:
                    input.putback(c);
                    return LoadNumber(input);
            }
        }

        struct PrintContext {
            std::ostream& out;
            int indent_step = 4;
            int indent = 0;

            void PrintIndent() const {
                for (int i = 0; i < indent; ++i) {
                    out.put(' ');
                }
            }

            PrintContext Indented() const {
                return {out, indent_step, indent_step + indent};
            }
        };

        void PrintNode(const Node& value, const PrintContext& ctx);

        template <typename Value>
        void PrintValue(const Value& value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintString(const std::string& value, std::ostream& out) {
            out.put('"');
            for (const char c : value) {
                switch (c) {
                    case '\r':
                        out << "\\r"sv;
                        break;
                    case '\n':
                        out << "\\n"sv;
                        break;
                    case '"':
                        // Символы " и \ выводятся как \" или \\, соответственно
                        [[fallthrough]];
                    case '\\':
                        out.put('\\');
                        [[fallthrough]];
                    default:
                        out.put(c);
                        break;
                }
            }
            out.put('"');
        }

        template <>
        void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
            PrintString(value, ctx.out);
        }

        template <>
        void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
            ctx.out << "null"sv;
        }

        // В специализаци шаблона PrintValue для типа bool параметр value передаётся
        // по константной ссылке, как и в основном шаблоне.
        // В качестве альтернативы можно использовать перегрузку:
        // void PrintValue(bool value, const PrintContext& ctx);
        template <>
        void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
            ctx.out << (value ? "true"sv : "false"sv);
        }

        template <>
        void PrintValue<Node::Array>(const Node::Array& nodes, const PrintContext& ctx) {
            std::ostream& out = ctx.out;
            out << "[\n"sv;
            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const Node& node : nodes) {
                if (first) {
                    first = false;
                } else {
                    out << ",\n"sv;
                }
                inner_ctx.PrintIndent();
                PrintNode(node, inner_ctx);
            }
            out.put('\n');
            ctx.PrintIndent();
            out.put(']');
        }

        template <>
        void PrintValue<Node::Object>(const Node::Object& nodes, const PrintContext& ctx) {
            std::ostream& out = ctx.out;
            out << "{\n"sv;
            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const auto& [key, node] : nodes) {
                if (first) {
                    first = false;
                } else {
                    out << ",\n"sv;
                }
                inner_ctx.PrintIndent();
                PrintString(key, ctx.out);
                out << ": "sv;
                PrintNode(node, inner_ctx);
            }
            out.put('\n');
            ctx.PrintIndent();
            out.put('}');
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            std::visit(
                    [&ctx](const auto& value) {
                        PrintValue(value, ctx);
                    },
                    node.GetValue());
        }

    }  // namespace

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

Node::Node(Value value)
    : value_(std::move(value)) {}

Node::Value& Node::GetValue() noexcept {
    return value_;
}

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
    : root_(std::move(root))
{}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{output});
}
}