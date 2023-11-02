#include "json_builder.h"

using namespace json;

Builder& Builder::Value(Node::Value value)
{
    if (nodes_stack_.empty() && root_.IsNull())
    {
        root_ = std::move(value);
        return *this;
    }
    else if (!nodes_stack_.empty())
    {
        if (nodes_stack_.back()->IsNull())
        {
            *nodes_stack_.back() = std::move(value);
            nodes_stack_.pop_back();
            return *this;
        }
        else if (nodes_stack_.back()->IsArray())
        {
            nodes_stack_.back()->AsArray().emplace_back(std::move(value));
            return *this;
        }
    }

    throw BuilderError("Invalid value add");
}

ObjectItemContext Builder::StartObject()
{
    if (nodes_stack_.empty() && root_.IsNull())
    {
        root_ = Node::Object{};
        nodes_stack_.push_back(&root_);
        return *this;
    }
    else if (!nodes_stack_.empty())
    {
        if (nodes_stack_.back()->IsNull())
        {
            *nodes_stack_.back() = Node::Object{};
            return *this;
        }
        else if (nodes_stack_.back()->IsArray())
        {
            nodes_stack_.push_back(&nodes_stack_.back()->AsArray().emplace_back(Node::Object{}));
            return *this;
        }
    }

    throw BuilderError("Invalid object start");
}

KeyItemContext Builder::Key(std::string key)
{
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsObject())
    {
        nodes_stack_.push_back(&nodes_stack_.back()->AsObject()[std::move(key)]);
        return *this;
    }

    throw BuilderError("Invalid object key add");
}

Builder& Builder::EndObject()
{
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsObject())
    {
        nodes_stack_.pop_back();
        return *this;
    }

    throw BuilderError("Invalid object end");
}

ArrayItemContext Builder::StartArray()
{
    if (nodes_stack_.empty() && root_.IsNull())
    {
        root_ = Node::Array{};
        nodes_stack_.push_back(&root_);
        return *this;
    }
    else if (!nodes_stack_.empty())
    {
        if (nodes_stack_.back()->IsNull())
        {
            *nodes_stack_.back() = Node::Array{};
            return *this;
        }
        else if (nodes_stack_.back()->IsArray())
        {
            nodes_stack_.push_back(&nodes_stack_.back()->AsArray().emplace_back(Node::Array{}));
            return *this;
        }
    }

    throw BuilderError("Invalid array start");
}

Builder& Builder::EndArray()
{
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray())
    {
        nodes_stack_.pop_back();
        return *this;
    }

    throw BuilderError("Invalid array end");
}

ArrayItemContext Builder::Merge(Node::Array right)
{
    if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray())
    {
        auto& left = root_.AsArray();
        left.insert(left.end(), make_move_iterator(right.begin()), make_move_iterator(right.end()));
        return *this;
    }

    throw BuilderError("Invalid array merge");
}

json::Node Builder::Build()
{
    if (nodes_stack_.empty() && !root_.IsNull())
    {
        return std::move(root_);
    }

    throw BuilderError("Error JSON build");
}

Builder& Builder::Clear()
{
    nodes_stack_.clear();
    root_ = Node{};
    return *this;
}

ItemContext::ItemContext(Builder& builder)
    : builder_(builder)
{}

ObjectItemContext ItemContext::StartObject()
{
    return builder_.StartObject();
}

ArrayItemContext ItemContext::StartArray()
{
    return builder_.StartArray();
}

KeyItemContext ObjectItemContext::Key(std::string key)
{
    return builder_.Key(std::move(key));
}

Builder& ObjectItemContext::EndObject()
{
    return builder_.EndObject();
}

ObjectItemContext KeyItemContext::Value(Node::Value value)
{
    return builder_.Value(std::move(value));
}

ArrayItemContext ArrayItemContext::Value(Node::Value value)
{
    return builder_.Value(std::move(value));
}

Builder& ArrayItemContext::EndArray()
{
    return builder_.EndArray();
}

ArrayItemContext ArrayItemContext::Merge(Node::Array right)
{
    return builder_.Merge(std::move(right));
}