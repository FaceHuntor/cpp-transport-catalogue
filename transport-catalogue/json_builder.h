#include "json.h"
#include <iostream>

namespace json {

class Builder {
    class AfterKeyValue{
    public:
        AfterKeyValue(Builder& builder) : builder_(builder) {}

        auto Key(std::string key) {
            return builder_.Key(std::move(key));
        }
        auto& EndDict() {
            return builder_.EndDict();
        }

    private:
        Builder& builder_;
    };

    class AfterKey{
    public:
        AfterKey(Builder& builder) : builder_(builder) {}

        AfterKeyValue Value(Node::Value val) {
            return builder_.Value(std::move(val));
        }
        auto StartDict() {
            return builder_.StartDict();
        }
        auto StartArray() {
            return builder_.StartArray();
        }

    private:
        Builder& builder_;
    };

    class AfterStartDict{
    public:
        AfterStartDict(Builder& builder) : builder_(builder) {}

        auto Key(std::string key) {
            return builder_.Key(std::move(key));
        }
        auto& EndDict() {
            return builder_.EndDict();
        }

    private:
        Builder& builder_;
    };

    class AfterStartArrayValue{
    public:
        AfterStartArrayValue(Builder& builder) : builder_(builder) {}

        AfterStartArrayValue Value(Node::Value val) {
            return builder_.Value(std::move(val));
        }

        auto StartDict() {
            return builder_.StartDict();
        }
        auto StartArray() {
            return builder_.StartArray();
        }

        auto& EndArray() {
            return builder_.EndArray();
        }

    private:
        Builder& builder_;
    };

    class AfterStartArray{
    public:
        AfterStartArray(Builder& builder) : builder_(builder) {}

        AfterStartArrayValue Value(Node::Value val) {
            return builder_.Value(std::move(val));
        }

        auto StartDict() {
            return builder_.StartDict();
        }
        auto StartArray() {
            return builder_.StartArray();
        }

        auto& EndArray() {
            return builder_.EndArray();
        }

    private:
        Builder& builder_;
    };

public:
    Builder();

    AfterStartDict StartDict();

    Builder& EndDict();

    AfterKey Key(std::string key);

    Builder& Value(Node::Value val) ;

    AfterStartArray StartArray();

    Builder& EndArray();

    Node Build();

private:
    void PopValue();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;
};




}