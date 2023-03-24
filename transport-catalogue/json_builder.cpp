#include "json_builder.h"

#include <iostream>

namespace json {

Builder::Builder() {

}

void Builder::ThrowIfReady() {
    if (context_stack_.size() == 0) {
        throw std::logic_error("Object already ready");
    }
}

KeyBuilder Builder::Key(std::string key) {
    ThrowIfReady();

    if (context_stack_.back()->IsDict() == false) {
        throw std::logic_error("Call Key for not Dict");
    }
    context_stack_.push_back(&context_stack_.back()->AsDict()[key]);
    return KeyBuilder(std::move(*this));
}

Builder& Builder::Value(Node::Value value) {
    ThrowIfReady();

    if (context_stack_.back()->IsNull()) {
        *context_stack_.back() = std::move(value);
        context_stack_.pop_back();
        return *this;
    }
    if (context_stack_.back()->IsArray()) {
        Array& arr = context_stack_.back()->AsArray();
        arr.push_back(Node(std::move(value)));
        return *this;
    }
    throw std::logic_error("Set value for invalid Node");
}

DictBuilder Builder::StartDict() {

    StartXxxxx<Dict>();
    return DictBuilder{ std::move(*this) };
}

ArrayBuilder Builder::StartArray() {
    StartXxxxx<Array>();
    return ArrayBuilder{ std::move(*this) };
}

Builder& Builder::EndDict() {
    ThrowIfReady();
    if (context_stack_.back()->IsDict()) {
        context_stack_.pop_back();
        return *this;
    }
    throw std::logic_error("End dict for invalid Node");
}

Builder& Builder::EndArray() {
    ThrowIfReady();
    if (context_stack_.back()->IsArray()) {
        context_stack_.pop_back();
        return *this;
    }
    throw std::logic_error("End array for invalid Node");
}

Node Builder::Build() {
    if (context_stack_.size() != 0) {
        throw std::logic_error("Build for not ready builder");
    }
    return std::move(*root_);
}

DictBuilder KeyBuilder::Value(Node::Value value) {
    Builder::Value(std::move(value));
    return DictBuilder(std::move(*this));
}

ArrayBuilder& ArrayBuilder::Value(Node::Value value) {
    Builder::Value(std::move(value));
    return *this;
}

KeyBuilder::KeyBuilder(Builder&& builder) : Builder{ std::move(builder) } {

}

DictBuilder::DictBuilder(Builder&& builder) : Builder{ std::move(builder) } {

}

ArrayBuilder::ArrayBuilder(Builder&& builder) : Builder{ std::move(builder) } {

}

}//namespace json 