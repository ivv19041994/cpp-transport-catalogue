#include "json.h"

using namespace std;

namespace json {
    
Node::Node(Value value) : variant(std::move(value)) {}

bool Node::IsInt() const {
    return holds_alternative<int>(*this);
}
bool Node::IsDouble() const {
    return IsInt() || IsPureDouble();
}

bool Node::IsPureDouble() const {
    return holds_alternative<double>(*this);
}
bool Node::IsBool() const {
    return holds_alternative<bool>(*this);
}
bool Node::IsString() const {
    return holds_alternative<std::string>(*this);
}
bool Node::IsNull() const {
    return holds_alternative<std::nullptr_t>(*this);
}
bool Node::IsArray() const {
    return holds_alternative<Array>(*this);
}
bool Node::IsDict() const {
    return holds_alternative<Dict>(*this);
}
    
int Node::AsInt() const {
    if(IsInt()) {
        return std::get<int>(*this);
    }
    throw std::logic_error("Node is not int");
}
bool Node::AsBool() const {
    if(IsBool()) {
        return std::get<bool>(*this);
    }
    throw std::logic_error("Node is not bool");
}
double Node::AsDouble() const {//Возвращает значение типа double, если внутри хранится double либо int. В последнем случае возвращается приведённое в double значение.
    if(IsPureDouble()) {
        return std::get<double>(*this);
    }
    if(IsInt()) {
        return std::get<int>(*this);
    }
    throw std::logic_error("Node is not double");
}
const std::string& Node::AsString() const {
    if(IsString()) {
        return std::get<std::string>(*this);
    }
    throw std::logic_error("Node is not string");
}
const Array& Node::AsArray() const {
    if(IsArray()) {
        return std::get<Array>(*this);
    }
    throw std::logic_error("Node is not Array");
}
const Dict& Node::AsDict() const {
    if(IsDict()) {
        return std::get<Dict>(*this);
    }
    throw std::logic_error("Node is not map");
}
    
bool Node::operator==(const Node& other) const {
    return *this == other;
}
    
bool Node::operator!=(const Node& other) const {
    return *this != other;
}

bool Node::operator==(const std::string& str) const {
    return AsString() == str;
}
    
Node::Node(): Node{nullptr} {
}
Node::Node(size_t size): Node(static_cast<int>(size)) {

}

Dict& Node::AsDict() {
    using namespace std::literals;
    if (!IsDict()) {
        throw std::logic_error("Not a dict"s);
    }

    return std::get<Dict>(*this);
}

Array& Node::AsArray() {
    using namespace std::literals;
    if (!IsArray()) {
        throw std::logic_error("Not an array"s);
    }

    return std::get<Array>(*this);
}

namespace {

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;
    
    bool first = true;
    char c;
    for (; input >> c && c != ']';) {
        
        if(first) {
            if (c == ',') {
                throw ParsingError("Array separator invalid: "s + c);
            }
            first = false;
            input.putback(c);
        } else if (c != ',') {
            throw ParsingError("Array separator invalid: "s + c);
        }
        result.push_back(LoadNode(input));
    }
    
    if(c != ']') {
        throw ParsingError("Array ] not found"s);
    }

    return Node(move(result));
}

Node LoadDigit(istream& input) {
    
    std::string digit;
    char temp;
    temp = input.peek();
    if(temp != '-' && !isdigit(temp)) {
        throw ParsingError("Digit begin invalid: "s + temp);
    }
    digit.push_back(input.get());
    if(temp == '-') {
        temp = input.peek();
        if(!isdigit(temp)) {
            throw ParsingError("First char is not digit: "s + temp);
        }
        digit.push_back(input.get());
    }
    
    while(isdigit(input.peek())) {
        input >> temp;
        digit.push_back(temp);
    }
    temp = input.peek();
    if(temp != '.' && temp != 'e' && temp != 'E') {
        return Node(stoi(digit));
    }
    if(temp == '.') {
        digit.push_back(input.get());

        temp = input.peek();
        if(!isdigit(temp)) {
            throw ParsingError("Not digit after dot: "s + temp);
        }
        digit.push_back(input.get());

        while(isdigit(input.peek())) {
            input >> temp;
            digit.push_back(temp);
        }
    }
    
    temp = input.peek();
    if(temp != 'e' && temp != 'E') {
        return Node(stod(digit));
    }
    
    digit.push_back(input.get());
    temp = input.peek();
    if(temp != '+' && temp != '-' && !isdigit(temp)) {
        throw ParsingError("Not +/- after e: "s + temp);
    }
    digit.push_back(input.get());
    if(!isdigit(temp)) {
        temp = input.peek();
        if(!isdigit(temp)) {
            throw ParsingError("Not digit after e+/-: "s + temp);
        }
        digit.push_back(input.get());
    }
    while(isdigit(input.peek())) {
        input >> temp;
        digit.push_back(temp);
    }
    return Node(stod(digit));
}

Node LoadString(istream& input) {    
    string line;
    char c;
    for(c = input.get(); input && c != '\"'; c = input.get()) { 
        if(c == '\\') {
            c = input.get();
            switch(c) {
                case 'n': line.push_back('\n'); break;
                case 'r': line.push_back('\r'); break;
                case 't': line.push_back('\t'); break;
                default: line.push_back(c);
            }
        } else {
            line.push_back(c);
        }
    }
    if(c != '\"') {
        throw ParsingError("String is not closed"s);
    }
    return Node(move(line));
}

Node LoadDict(istream& input) {
    Dict result;
    bool first = true;
    char c;
    for (; input >> c && c != '}';) {
        if(first) {
            if (c == ',') {
                throw ParsingError("Dict error format for first key \" != "s + c);
            }
            first = false;
        } else if (c != ',') {
            throw ParsingError("Dict error format , != "s + c);
        } else {
            input >> c;
        }
        
        if(c != '\"') {
            throw ParsingError("Dict error format \" != "s + c);
        }

        string key = LoadString(input).AsString();
        if(!(input >> c)) {
            throw ParsingError("Dict } not found"s);
        }
        if(c != ':') {
            throw ParsingError("Dict error format : != "s + c);
        }
        result.insert({move(key), LoadNode(input)});
    }
    if(c != '}') {
        throw ParsingError("Dict } not found"s);
    }
    return Node(move(result));
}
    
Node LoadNull(istream& input) {
    
    if(input.get() == 'u' && input.get() == 'l' && input.get() == 'l') {
        return Node(nullptr);
    }
    
    throw ParsingError("null invalid"s);
}
    
Node LoadBool(istream& input) {
    std::string line;
    int c = input.get();
    if(c == 't' && input.get() == 'r' && input.get() == 'u' && input.get() == 'e') {
        return Node(true);
    } else if(c == 'f' && input.get() == 'a' && input.get() == 'l' && input.get() == 's' && input.get() == 'e') {
        return Node(false);
    }
    throw ParsingError("Invalid bool");
}

Node LoadNode(istream& input) {
    char c;
    
    while(input >> c && (iscntrl(c) || isspace(c)));

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else {
        input.putback(c);
        return LoadDigit(input);
    }
}
    


}  // namespace

Document::Document(Node root)
    : root_(move(root)) {
}

bool Document::operator==(const Document& other) const {
    return root_ == other.root_;
}
    
bool Document::operator!=(const Document& other) const {
    return root_ != other.root_;
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}
    
struct NodePrinter {
  std::ostream& os;
    
    void operator()(int value) {
        os << value;
    }
    void operator()(double value) {
        os << value;
    }
    void operator()(const std::string& str) {
        os << '\"';
        for(char c: str) {
            switch(c) {
                case '\n': os << "\\n"; break;
                case '\r': os << "\\r"; break;
                case '\"': os << "\\\""; break;
                //case '\t': os << "\t"; break;
                case '\\': os << "\\\\"; break;
                default: os << c; break;
            }
        }
        os << '\"';
    }
    void operator()(bool b) {
        os << (b ? "true" : "false");
    }
    void operator()(const Array& array) {
        os << "[";
        bool second = false;
        for(auto& node: array) {
            if(second) {
                os << ",";
            } else {
                second = true;
            }
            node.Print(os);
        }
        
        os << "]";
    }
    void operator()(const Dict& dict) {
        os << "{ ";
        bool first = true;
        for(auto& [key, node]: dict) {
            if(first) {
                first = false;
            } else {
                os << ", ";
            }
            Node(key).Print(os);
            os << ": ";
            node.Print(os);
        }
        
        os << " }";
    }
    void operator()(std::nullptr_t) {
        os << "null";
    }
};
    
std::ostream& Node::Print(std::ostream& os) const {
    
    visit(NodePrinter{os}, *dynamic_cast<const json_variant*>(this));
    return os;
}

std::string Node::Print() const {
    std::stringstream ret;
    Print(ret);
    return ret.str();
}
    
void Print(const Document& doc, std::ostream& output) {
    doc.GetRoot().Print(output);
}

Document LoadJSON(const std::string& s) {
    std::istringstream strm(s);
    return Load(strm);
}

std::string Print(const Node& node) {
    std::ostringstream out;
    Print(Document{ node }, out);
    return out.str();
}

namespace autotest {

[[maybe_unused]] void MustFailToLoad(const std::string& s) {
    using namespace std::literals;
    try {
        LoadJSON(s);
        std::cerr << "ParsingError exception is expected on '"sv << s << "'"sv << std::endl;
        assert(false);
    }
    catch (const ParsingError&) {
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

[[maybe_unused]] void TestNull() {
    using namespace std::literals;
    Node null_node;
    assert(null_node.IsNull());
    assert(!null_node.IsInt());
    assert(!null_node.IsDouble());
    assert(!null_node.IsPureDouble());
    assert(!null_node.IsString());
    assert(!null_node.IsArray());
    assert(!null_node.IsDict());

    Node null_node1{ nullptr };
    assert(null_node1.IsNull());

    assert(Print(null_node) == "null"s);
    assert(null_node == null_node1);
    assert(!(null_node != null_node1));

    const Node node = LoadJSON("null"s).GetRoot();
    assert(node.IsNull());
    assert(node == null_node);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON(" \t\r\n\n\r null \t\r\n\n\r "s).GetRoot() == null_node);
}

[[maybe_unused]] void TestNumbers() {
    using namespace std::literals;
    const Node int_node{ 42 };
    assert(int_node.IsInt());
    assert(int_node.AsInt() == 42);
    // целые числа являются подмножеством чисел с плавающей запятой
    assert(int_node.IsDouble());
    // Когда узел хранит int, можно получить соответствующее ему double-значение
    assert(int_node.AsDouble() == 42.0);
    assert(!int_node.IsPureDouble());
    assert(int_node == Node{ 42 });
    // int и double - разные типы, поэтому не равны, даже когда хранят
    assert(int_node != Node{ 42.0 });

    const Node dbl_node{ 123.45 };
    assert(dbl_node.IsDouble());
    assert(dbl_node.AsDouble() == 123.45);
    assert(dbl_node.IsPureDouble());  // Значение содержит число с плавающей запятой
    assert(!dbl_node.IsInt());

    assert(Print(int_node) == "42"s);
    assert(Print(dbl_node) == "123.45"s);
    assert(Print(Node{ -42 }) == "-42"s);
    assert(Print(Node{ -3.5 }) == "-3.5"s);

    assert(LoadJSON("42"s).GetRoot() == int_node);
    assert(LoadJSON("123.45"s).GetRoot() == dbl_node);
    assert(LoadJSON("0.25"s).GetRoot().AsDouble() == 0.25);
    assert(LoadJSON("3e5"s).GetRoot().AsDouble() == 3e5);
    assert(LoadJSON("1.2e-5"s).GetRoot().AsDouble() == 1.2e-5);
    assert(LoadJSON("1.2e+5"s).GetRoot().AsDouble() == 1.2e5);
    assert(LoadJSON("-123456"s).GetRoot().AsInt() == -123456);
    assert(LoadJSON("0").GetRoot() == Node{ 0 });
    assert(LoadJSON("0.0").GetRoot() == Node{ 0.0 });
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON(" \t\r\n\n\r 0.0 \t\r\n\n\r ").GetRoot() == Node{ 0.0 });
}

[[maybe_unused]] void TestStrings() {
    using namespace std::literals;
    Node str_node{ "Hello, \"everybody\""s };
    assert(str_node.IsString());
    assert(str_node.AsString() == "Hello, \"everybody\""s);

    assert(!str_node.IsInt());
    assert(!str_node.IsDouble());

    assert(Print(str_node) == "\"Hello, \\\"everybody\\\"\""s);
    assert(LoadJSON(Print(str_node)).GetRoot() == str_node);
    const std::string escape_chars
        = R"("\r\n\t\"\\")"s;  // При чтении строкового литерала последовательности \r,\n,\t,\\,\"
    // преобразовываться в соответствующие символы.
    // При выводе эти символы должны экранироваться, кроме \t.
    assert(Print(LoadJSON(escape_chars).GetRoot()) == "\"\\r\\n\t\\\"\\\\\""s);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON("\t\r\n\n\r \"Hello\" \t\r\n\n\r ").GetRoot() == Node{ "Hello"s });
}

[[maybe_unused]] void TestBool() {
    using namespace std::literals;
    Node true_node{ true };
    assert(true_node.IsBool());
    assert(true_node.AsBool());

    Node false_node{ false };
    assert(false_node.IsBool());
    assert(!false_node.AsBool());

    assert(Print(true_node) == "true"s);
    assert(Print(false_node) == "false"s);

    assert(LoadJSON("true"s).GetRoot() == true_node);
    assert(LoadJSON("false"s).GetRoot() == false_node);
    assert(LoadJSON(" \t\r\n\n\r true \r\n"s).GetRoot() == true_node);
    assert(LoadJSON(" \t\r\n\n\r false \t\r\n\n\r "s).GetRoot() == false_node);
}

[[maybe_unused]] void TestArray() {
    using namespace std::literals;
    Node arr_node{ Array{Node(1), Node(1.23), Node("Hello"s)} };
    assert(arr_node.IsArray());
    const Array& arr = arr_node.AsArray();
    assert(arr.size() == 3);
    assert(arr.at(0).AsInt() == 1);

    assert(LoadJSON("[1,1.23,\"Hello\"]"s).GetRoot() == arr_node);
    assert(LoadJSON(Print(arr_node)).GetRoot() == arr_node);
    assert(LoadJSON(R"(  [ 1  ,  1.23,  "Hello"   ]   )"s).GetRoot() == arr_node);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON("[ 1 \r \n ,  \r\n\t 1.23, \n \n  \t\t  \"Hello\" \t \n  ] \n  "s).GetRoot()
        == arr_node);
}

[[maybe_unused]] void TestMap() {
    using namespace std::literals;
    Node dict_node{ Dict{{"key1"s, "value1"s}, {"key2"s, 42}} };
    assert(dict_node.IsDict());
    const Dict& dict = dict_node.AsDict();
    assert(dict.size() == 2);
    assert(dict.at("key1"s).AsString() == "value1"s);
    assert(dict.at("key2"s).AsInt() == 42);

    assert(LoadJSON("{ \"key1\": \"value1\", \"key2\": 42 }"s).GetRoot() == dict_node);
    assert(LoadJSON(Print(dict_node)).GetRoot() == dict_node);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(
        LoadJSON(
            "\t\r\n\n\r { \t\r\n\n\r \"key1\" \t\r\n\n\r: \t\r\n\n\r \"value1\" \t\r\n\n\r , \t\r\n\n\r \"key2\" \t\r\n\n\r : \t\r\n\n\r 42 \t\r\n\n\r } \t\r\n\n\r"s)
        .GetRoot()
        == dict_node);
}

[[maybe_unused]] void TestErrorHandling() {
    using namespace std::literals;
    MustFailToLoad("["s);
    MustFailToLoad("]"s);

    MustFailToLoad("{"s);
    MustFailToLoad("}"s);

    MustFailToLoad("\"hello"s);  // незакрытая кавычка

    MustFailToLoad("tru"s);
    MustFailToLoad("fals"s);
    MustFailToLoad("nul"s);

    Node dbl_node{ 3.5 };
    MustThrowLogicError([&dbl_node] {
        dbl_node.AsInt();
        });
    MustThrowLogicError([&dbl_node] {
        dbl_node.AsString();
        });
    MustThrowLogicError([&dbl_node] {
        dbl_node.AsArray();
        });

    Node array_node{ Array{} };
    MustThrowLogicError([&array_node] {
        array_node.AsDict();
        });
    MustThrowLogicError([&array_node] {
        array_node.AsDouble();
        });
    MustThrowLogicError([&array_node] {
        array_node.AsBool();
        });
}

[[maybe_unused]] void Benchmark() {
    using namespace std::literals;
    const auto start = std::chrono::steady_clock::now();
    Array arr;
    arr.reserve(1'000);
    for (int i = 0; i < 1'000; ++i) {
        arr.emplace_back(Dict{
            {"int"s, 42},
            {"double"s, 42.1},
            {"null"s, nullptr},
            {"string"s, "hello"s},
            {"array"s, Array{1, 2, 3}},
            {"bool"s, true},
            {"map"s, Dict{{"key"s, "value"s}}},
            });
    }
    std::stringstream strm;
    Print(Document{ arr }, strm);
    const auto doc = Load(strm);
    assert(doc.GetRoot() == arr);
    const auto duration = std::chrono::steady_clock::now() - start;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms"sv
        << std::endl;
}

int TestAll() {

    TestNull();
    TestNumbers();
    TestStrings();
    TestBool();
    TestArray();
    TestMap();
    TestErrorHandling();
    Benchmark();
    return 0;
}
} // namespace json::autotest
}  // namespace json