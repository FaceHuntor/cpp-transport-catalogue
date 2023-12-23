#include "json_builder.h"


namespace json {

Builder::Builder() {
    nodes_stack_.emplace_back(&root_);
}

Builder::AfterStartDict Builder::StartDict() {
    if(nodes_stack_.empty() || !nodes_stack_.back()->IsNull()) {
        throw std::logic_error("Incorrect place for StartDict");
    }
    nodes_stack_.back()->GetValue() = std::move(Dict());
    return *this;
}

Builder& Builder::EndDict() {
    if(nodes_stack_.empty() || !nodes_stack_.back()->IsMap()){
        throw std::logic_error("Incorrect place for EndDict");
    }
    PopValue();
    return *this;
}

Builder::AfterKey Builder::Key(std::string key) {
    if(nodes_stack_.empty() || !nodes_stack_.back()->IsMap()){
        throw std::logic_error("Incorrect place for adding Key");
    }
    auto& node = std::get<Dict>(nodes_stack_.back()->GetValue())[std::move(key)];
    nodes_stack_.emplace_back(&node);
    return *this;
}

Builder& Builder::Value(Node::Value val) {
    if(nodes_stack_.empty() || !nodes_stack_.back()->IsNull()) {
        throw std::logic_error("Incorrect place for Value");
    }
    nodes_stack_.back()->GetValue() = std::move(val);
    PopValue();
    return *this;
}

Builder::AfterStartArray Builder::StartArray() {
    if(nodes_stack_.empty() || !nodes_stack_.back()->IsNull()) {
        throw std::logic_error("Incorrect place for StartArray");
    }
    nodes_stack_.back()->GetValue() = std::move(Array());
    auto& back_array = std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back();
    nodes_stack_.emplace_back(&back_array);
    return *this;
}

Builder& Builder::EndArray() {
    if(nodes_stack_.size() < 2 || !(*std::next(nodes_stack_.rbegin()))->IsArray() || !nodes_stack_.back()->IsNull()){
        throw std::logic_error("Closing array outside the array");
    }
    nodes_stack_.pop_back();
    std::get<Array>(nodes_stack_.back()->GetValue()).pop_back();
    PopValue();

    return *this;
}

Node Builder::Build() {
    if(!nodes_stack_.empty()) {
        throw std::logic_error("Incomplete json for building");
    }
    return std::move(root_);
}

void Builder::PopValue() {
    nodes_stack_.pop_back();
    if(!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
        auto& back_array = std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back();
        nodes_stack_.emplace_back(&back_array);
    }
}

}