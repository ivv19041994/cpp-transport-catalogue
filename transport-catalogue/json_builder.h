#pragma once

#include "json.h"

#include <vector>
#include <memory>

namespace json {


    class KeyBuilder;
    class DictBuilder;
    class ArrayBuilder;

    class Builder {
    public:
        Builder();
        KeyBuilder Key(std::string);
        Builder& Value(Node::Value);
        DictBuilder StartDict();
        ArrayBuilder StartArray();
        Builder& EndDict();
        Builder& EndArray();
        Node Build();
    private:
        std::unique_ptr<Node> root_ = std::make_unique<Node>();
        std::vector<Node*> context_stack_{ root_.get() };

        void ThrowIfReady();

        template<typename Xxxxxx>
        void StartXxxxx();
    };

    class KeyBuilder : public Builder {
    public:
        KeyBuilder(Builder&&);
        DictBuilder Value(Node::Value);
        //DictBuilder& StartDict();
        //ArrayBuilder& StartArray();
    private:
        using Builder::Key;
        using Builder::EndDict;
        using Builder::EndArray;
        using Builder::Build;
    };

    class DictBuilder : public Builder {
    public:
        DictBuilder(Builder&&);
        //Key 
         //EndDict
    private:
        using Builder::Value;
        using Builder::StartDict;
        using Builder::StartArray;
        using Builder::EndArray;
        using Builder::Build;
    };

    class ArrayBuilder :public Builder {
    public:
        ArrayBuilder(Builder&&);
        ArrayBuilder& Value(Node::Value);
        //StartDict
        //StartArray
        //EndArray
    private:

        using Builder::Key;
        using Builder::EndDict;
        using Builder::Build;
    };

    template<typename Xxxxxx>
    void Builder::StartXxxxx() {
        ThrowIfReady();
        if (context_stack_.back()->IsNull()) {
            *context_stack_.back() = Xxxxxx{};
            return;
        }
        if (context_stack_.back()->IsArray()) {
            context_stack_.back()->AsArray().push_back(Xxxxxx{});
            context_stack_.push_back(&(context_stack_.back()->AsArray().back()));
            return;
        }
        throw std::logic_error("Start* for invalid Node");
    }



}//namespace json