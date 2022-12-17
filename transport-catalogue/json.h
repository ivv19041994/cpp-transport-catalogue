#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <cassert>
#include <chrono>

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

using json_variant = std::variant<
    int,
    double,
    std::string,
    bool,
    Array,
    Dict,
    std::nullptr_t
>;

class Node: json_variant {
public:
    using Array = std::vector<Node>;
    using Dict = std::map<std::string, Node>; 

    using json_variant::variant;
    Node();
    Node(size_t size);
    
    bool IsInt() const;
    bool IsDouble() const;// Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const;// Возвращает true, если в Node хранится double.
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;
    
    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;//Возвращает значение типа double, если внутри хранится double либо int. В последнем случае возвращается приведённое в double значение.
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;
    
    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

    bool operator==(const std::string& str) const;

    
    std::ostream& Print(std::ostream& os) const;
    std::string Print() const;

private:
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;
    
    bool operator==(const Document& other) const;
    bool operator!=(const Document& other) const;
private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);
    
Document LoadJSON(const std::string& s);

std::string Print(const Node& node);

namespace autotest {

void MustFailToLoad(const std::string& s);

template <typename Fn>
[[maybe_unused]] void MustThrowLogicError(Fn fn) {
    using namespace std::literals;
    try {
        fn();
        std::cerr << "logic_error is expected"sv << std::endl;
        assert(false);
    }
    catch (const std::logic_error&) {
        // ok
    }
    catch (const std::exception& e) {
        std::cerr << "exception thrown: "sv << e.what() << std::endl;
        assert(false);
    }
    catch (...) {
        std::cerr << "Unexpected error"sv << std::endl;
        assert(false);
    }
}

void TestNull();
void TestNumbers();
void TestStrings();
void TestBool();
void TestArray();
void TestMap();
void TestErrorHandling();
void Benchmark();
int TestAll();
} // namespace json::autotest

}  // namespace json