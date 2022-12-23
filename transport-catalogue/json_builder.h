#pragma once

#include "json.h"

#include <vector>
#include <memory>

namespace json {

class BuilderBase {
  public:
    BuilderBase();
    BuilderBase& Key(std::string);
    BuilderBase& Value(Node::Value);
    BuilderBase& StartDict();
    BuilderBase& StartArray();
    BuilderBase& EndDict();
    BuilderBase& EndArray();
    Node Build();
  private:
    Node root_;
    std::vector<Node*> context_stack_{&root_};
    
    void ThrowIfReady();
};
    
    
template <typename EndReturnClass>
class DictBuilder;
template <typename EndReturnClass>
class ArrayBuilder;
    
template <typename ReturnClass>
class DictValueBuilder {
    public:
    
    DictValueBuilder(std::unique_ptr<BuilderBase> base): builder_{std::move(base)} {
    
    }
    
    ReturnClass Value(Node::Value value) {
        builder_->Value(std::move(value));
        return ReturnClass{std::move(builder_)};
    }
    DictBuilder<ReturnClass> StartDict() {
        builder_->StartDict();
        return DictBuilder<ReturnClass>{std::move(builder_)};
    }
    ArrayBuilder<ReturnClass> StartArray() {
        builder_->StartArray();
        return ArrayBuilder<ReturnClass>{std::move(builder_)};
    }
    
    private:
    std::unique_ptr<BuilderBase> builder_;
};
    
template <typename EndReturnClass>
class DictBuilder {
    public:
    DictBuilder(std::unique_ptr<BuilderBase> base): builder_{std::move(base)} {
    
    }
    DictValueBuilder<DictBuilder<EndReturnClass>> Key(std::string key) {
        builder_->Key(std::move(key));
        return DictValueBuilder<DictBuilder<EndReturnClass>>{std::move(builder_)};
    }
    EndReturnClass EndDict() {
        builder_->EndDict();
        return EndReturnClass{std::move(builder_)};
    }
    private:
    std::unique_ptr<BuilderBase> builder_;
};

template <typename EndReturnClass>
class ArrayBuilder {
public:
    ArrayBuilder(std::unique_ptr<BuilderBase> base): builder_{std::move(base)} {
    
    }
    ArrayBuilder<EndReturnClass>& Value(Node::Value value) {
        builder_->Value(std::move(value));
        return *this;//ArrayBuilder<EndReturnClass>{std::move(builder_)};
    }
    DictBuilder<ArrayBuilder<EndReturnClass>> StartDict() {
        builder_->StartDict();
        return DictBuilder<ArrayBuilder<EndReturnClass>>{std::move(builder_)};
    }
    ArrayBuilder<ArrayBuilder<EndReturnClass>> StartArray() {
        builder_->StartArray();
        return ArrayBuilder<ArrayBuilder<EndReturnClass>>{std::move(builder_)};
    }
    EndReturnClass EndArray() {
        builder_->EndArray();
        return EndReturnClass{std::move(builder_)};
    }
    private:
    std::unique_ptr<BuilderBase> builder_;
};

class BuilderComplite {
    public:
    BuilderComplite(std::unique_ptr<BuilderBase> base);
    Node Build();
    private:
    std::unique_ptr<BuilderBase> builder_;
};

class Builder {
  public:
    Builder();
    BuilderComplite Value(Node::Value);
    DictBuilder<BuilderComplite> StartDict();
    ArrayBuilder<BuilderComplite> StartArray();
    private:
    std::unique_ptr<BuilderBase> builder_;
};
    
    
}//namespace json