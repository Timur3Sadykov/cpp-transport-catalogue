#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>

namespace json {

    class JsonException : public std::exception {
        using exception::exception;
    };

    class ParsingError : public JsonException, public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class NodeOutOfRange : public JsonException, public std::out_of_range {
    public:
        using out_of_range::out_of_range;
    };

    class InvalidNodeType : public JsonException, public std::logic_error {
    public:
        using logic_error::logic_error;
    };

    class Node {
    public:
        using Object = std::map<std::string, Node>;
        using Array = std::vector<Node>;
        using Value = std::variant<std::nullptr_t, bool, int, double, std::string, Object, Array>;

        Node() = default;
        Node(std::nullptr_t);
        Node(bool value);
        Node(int value);
        Node(double value);
        Node(std::string value);
        Node(Object object);
        Node(Array array);
        Node(Value value);

        Value& GetValue() noexcept;
        const Value& GetValue() const noexcept;

        template <class ValueT>
        bool Is() const noexcept;

        bool IsNull() const noexcept;
        bool IsBool() const noexcept;
        bool IsInt() const noexcept;
        bool IsDouble() const noexcept;
        bool IsPureDouble() const noexcept;
        bool IsString() const noexcept;
        bool IsObject() const noexcept;
        bool IsArray() const noexcept;

        template <class ValueT>
        ValueT& As();

        template <class ValueT>
        const ValueT& As() const;

        bool AsBool() const;
        int AsInt() const;
        double AsDouble() const;
        const std::string& AsString() const;
        std::string& AsString();
        const Object& AsObject() const;
        Object& AsObject();
        const Array& AsArray() const;
        Array& AsArray();

        bool Contains(const std::string& key) const;

        const Node& At(const std::string& key) const;
        Node& At(const std::string& key);

    private:
        Value value_;
    };

    inline bool operator==(const Node& lhs, const Node& rhs) {
        return lhs.GetValue() == rhs.GetValue();
    }

    inline bool operator!=(const Node& lhs, const Node& rhs) {
        return !(lhs == rhs);
    }

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

    private:
        Node root_;
    };

    inline bool operator==(const Document& lhs, const Document& rhs) {
        return lhs.GetRoot() == rhs.GetRoot();
    }

    inline bool operator!=(const Document& lhs, const Document& rhs) {
        return !(lhs == rhs);
    }

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

    template <class ValueT>
    bool Node::Is() const noexcept {
        return std::holds_alternative<ValueT>(value_);
    }

    template <class ValueT>
    ValueT& Node::As() {
        using namespace std::literals;
        if (!Is<ValueT>()) {
            throw InvalidNodeType("Invalid value type"s);
        }

        return std::get<ValueT>(value_);
    }

    template <class ValueT>
    const ValueT& Node::As() const {
        using namespace std::literals;
        if (!Is<ValueT>()) {
            throw InvalidNodeType("Invalid value type"s);
        }

        return std::get<ValueT>(value_);
    }

}