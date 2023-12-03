#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

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

class Node {
public:
    using Value = std::variant<std::nullptr_t, int, double, bool, std::string,  Array, Dict>;

    Node() = default;
    Node(std::nullptr_t null);
    Node(int value);
    Node(double value);
    Node(bool value);
    Node(Dict map);
    Node(std::string value);
    Node(const char* value);
    Node(Array array);

    Node(const Node& other) = default;
    Node(Node&& other) = default;
    ~Node() = default;

    Node& operator=(const Node& other) = default;
    Node& operator=(Node&& other) = default;
    bool operator==(const Node& other) const {
        return data_ == other.data_;
    }
    bool operator!=(const Node& other) const {
        return !(*this == other);
    }

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    const Value& GetValue() const {return data_;};

private:
     Value data_;
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const {
        return root_ == other.root_;
    }
    bool operator!=(const Document& other) const {
        return !(*this == other);
    }

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json