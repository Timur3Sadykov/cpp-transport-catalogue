#pragma once
#include "json.h"
#include <vector>
#include <string>

namespace json
{
    class BuilderError : public std::logic_error
    {
    public:
        using logic_error::logic_error;
    };

    class ObjectItemContext;
    class KeyItemContext;
    class ArrayItemContext;

    class Builder
    {
    public:
        Builder& Value(Node::Value value);

        ObjectItemContext StartObject();
        KeyItemContext Key(std::string key);
        Builder& EndObject();

        ArrayItemContext StartArray();
        Builder& EndArray();
        ArrayItemContext Merge(Node::Array right);

        json::Node Build();
        Builder& Clear();

    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
    };

    class ItemContext
    {
    public:
        ItemContext(Builder& builder);

        ObjectItemContext StartObject();
        ArrayItemContext StartArray();

    protected:
        Builder& builder_;
    };

    class ObjectItemContext : private ItemContext
    {
    public:
        using ItemContext::ItemContext;

        KeyItemContext Key(std::string key);
        Builder& EndObject();
    };

    class KeyItemContext : private ItemContext
    {
    public:
        using ItemContext::ItemContext;
        using ItemContext::StartObject;
        using ItemContext::StartArray;

        ObjectItemContext Value(Node::Value value);
    };

    class ArrayItemContext : private ItemContext
    {
    public:
        using ItemContext::ItemContext;
        using ItemContext::StartObject;
        using ItemContext::StartArray;

        ArrayItemContext Value(Node::Value value);
        Builder& EndArray();
        ArrayItemContext Merge(Node::Array right);
    };
}