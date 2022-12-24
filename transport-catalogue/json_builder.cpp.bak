#include "json_builder.h"

#include <iostream>

namespace json {
    BuilderBase::BuilderBase() {
        
    }
    
    void BuilderBase::ThrowIfReady() {
        if(context_stack_.size() == 0) {
            throw std::logic_error("Object already ready");
        }
    }
    
    BuilderBase& BuilderBase::Key(std::string key) {
        ThrowIfReady();
        
        if(context_stack_.back()->IsDict() == false) {
            throw std::logic_error("Call Key for not Dict");
        }
        context_stack_.push_back(&context_stack_.back()->AsDict()[key]);
        return *this;
    }
    BuilderBase& BuilderBase::Value(Node::Value value) {
        ThrowIfReady();
        
        if(context_stack_.back()->IsNull()) {
            *context_stack_.back() = std::move(value);
            context_stack_.pop_back();
            return *this;
        }
        if(context_stack_.back()->IsArray()) {
            //Node node;
            //node.SetValue(std::move(value));
            Array& arr = context_stack_.back()->AsArray();
            arr.push_back(Node(std::move(value)));
            return *this;
        }
        throw std::logic_error("Set value for invalid Node");
    }
    BuilderBase& BuilderBase::StartDict() {
        ThrowIfReady();
        if(context_stack_.back()->IsNull()) {
            *context_stack_.back() = Dict{};
            return *this;
        }
        if(context_stack_.back()->IsArray()) {
            context_stack_.back()->AsArray().push_back(Dict{});
            context_stack_.push_back(&(context_stack_.back()->AsArray().back()));
            return *this;
        }
        throw std::logic_error("Start dict for invalid Node");
    }
    BuilderBase& BuilderBase::StartArray() {
        ThrowIfReady();
        if(context_stack_.back()->IsNull()) {
            *context_stack_.back() = Array{};
            return *this;
        }
        if(context_stack_.back()->IsArray()) {
            context_stack_.back()->AsArray().push_back(Array{});
            context_stack_.push_back(&(context_stack_.back()->AsArray().back()));
            return *this;
        }
        throw std::logic_error("Start array for invalid Node");
    }
    BuilderBase& BuilderBase::EndDict() {
        ThrowIfReady();
        if(context_stack_.back()->IsDict()) {
            context_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("End dict for invalid Node");
    }
    BuilderBase& BuilderBase::EndArray() {
        ThrowIfReady();
        if(context_stack_.back()->IsArray()) {
            context_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("End array for invalid Node");
    }
    Node BuilderBase::Build() {
        if(context_stack_.size() != 0) {
            throw std::logic_error("Build for not ready builder");
        }
        return std::move(root_);
    }
    
    BuilderComplite::BuilderComplite(std::shared_ptr<BuilderBase> base): builder_{std::move(base)} {
    
    }
    Node BuilderComplite::Build() {
        return builder_->Build();
    }
    
    Builder::Builder(): builder_{std::make_shared<BuilderBase>()} {
        
    }
    BuilderComplite Builder::Value(Node::Value value) {
        builder_->Value(std::move(value));
        return BuilderComplite{builder_};
    }
    DictBuilder<BuilderComplite> Builder::StartDict() {
        builder_->StartDict();
        return DictBuilder<BuilderComplite>{builder_};
    }
    ArrayBuilder<BuilderComplite> Builder::StartArray() {
        builder_->StartArray();
        return ArrayBuilder<BuilderComplite>{builder_};
    }
}//namespace json 