#include "json.h"

using namespace std;

namespace json {

using Number = std::variant<int, double>;

std::string LoadWord(std::istream& input) {
    std::string word;

    auto read_char = [&word, &input] {
        word += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    while (std::isalpha(input.peek())) {
        read_char();
    }

    return word;
}

bool LoadBool(std::istream& input) {
    std::string parsed_bool = LoadWord(input);
    if (parsed_bool == "true") {
        return true;
    }
    if (parsed_bool == "false") {
        return false;
    }
    throw json::ParsingError("Failed to read boolean from stream"s);
}

nullptr_t LoadNull(std::istream& input) {
    std::string parsed_bool = LoadWord(input);
    if (parsed_bool == "null") {
        return nullptr;
    }
    throw json::ParsingError("Failed to read boolean from stream"s);
}

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw json::ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw json::ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw json::ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw json::ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw json::ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw json::ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw json::ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadNode(istream& input);

Array LoadArray(istream& input) {
    Array result;
    char c;
    while (input >> c) {
        if (c == ']') {
            return result;
        }
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    throw json::ParsingError("Unexpected end of line"s);
}

Dict LoadDict(istream& input) {
    Dict result;
    char c;
    while (input >> c) {
        if (c == '}') {
            return result;
        }
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input);
        input >> c;
        result.insert({std::move(key), LoadNode(input)});
    }
    throw json::ParsingError("Unexpected end of line"s);
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else {
        input.putback(c);
        auto number = LoadNumber(input);
        if (holds_alternative<int>(number)) {
            return get<int>(number);
        } else {
            return get<double>(number);
        }
    }
}

bool Node::IsInt() const {
    return holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
    return holds_alternative<int>(*this) || holds_alternative<double>(*this);
}

bool Node::IsPureDouble() const {
    return holds_alternative<double>(*this);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(*this);
}

bool Node::IsString() const {
    return holds_alternative<string>(*this);
}

bool Node::IsNull() const {
    return holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(*this);
}

bool Node::IsMap() const {
    return holds_alternative<Dict>(*this);
}

const Array& Node::AsArray() const {
    try {
        return std::get<Array>(*this);
    }
    catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Node is not array");
    }
}

const Dict& Node::AsMap() const {
    try {
        return std::get<Dict>(*this);
    }
    catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Node is not dict");
    }
}

int Node::AsInt() const {
    try {
        return std::get<int>(*this);
    }
    catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Node is not int");
    }
}

double Node::AsDouble() const {
    if (holds_alternative<int>(*this)) {
        return std::get<int>(*this);
    } else if (holds_alternative<double>(*this)) {
        return std::get<double>(*this);
    }
    throw std::logic_error("Node is not double");
}

const string& Node::AsString() const {
    try {
        return std::get<string>(*this);
    }
    catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Node is not string");
    }
}

bool Node::AsBool() const {
    try {
        return std::get<bool>(*this);
    }
    catch (const std::bad_variant_access& ex) {
        throw std::logic_error("Node is not bool");
    }
}

Document::Document(Node root)
        : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

struct PrintContext {
    PrintContext(std::ostream& out)
            : out(out) {
    }

    PrintContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out), indent_step(indent_step), indent(indent) {
    }

    PrintContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

void PrintNode(const Node& node, PrintContext context);

template<typename Value>
void PrintValue(const Value& value, PrintContext context) {
    context.out << value;
}

void PrintValue(const bool& value, PrintContext context) {
    context.out << std::boolalpha << value;
}

void PrintValue(std::nullptr_t, PrintContext context) {
    context.out << "null";
}

void PrintValue(const std::string& value, PrintContext context) {
    context.out << "\"";
    for (const auto c: value) {
        switch (c) {
            case '\n':
                context.out << "\\n";
                break;
            case '\r':
                context.out << "\\r";
                break;
            case '\"':
                context.out << "\\\"";
                break;
            case '\t':
                context.out << "\\t";
                break;
            case '\\':
                context.out << "\\\\";
                break;

            default:
                context.out << c;
        }
    }
    context.out << "\"";
}

void PrintValue(const Array& value, PrintContext context) {
    context.out << "[" << endl;
    auto item_context = context.Indented();
    for (auto it = value.begin(); it != value.end(); ++it) {
        item_context.PrintIndent();
        PrintNode(*it, item_context);
        if (std::next(it) != value.end()) {
            context.out << ",";
        }
        context.out << endl;
    }
    context.PrintIndent();
    context.out << "]";
}

void PrintValue(const Dict& value, PrintContext context) {
    context.out << "{" << endl;
    auto item_context = context.Indented();
    for (auto it = value.begin(); it != value.end(); ++it) {
        item_context.PrintIndent();
        item_context.out << "\"" << it->first << "\": ";
        PrintNode(it->second, item_context);
        if (std::next(it) != value.end()) {
            item_context.out << ",";
        }
        item_context.out << endl;
    }
    context.PrintIndent();
    context.out << "}";
}

void PrintNode(const Node& node, PrintContext context) {
    std::visit(
            [&context](const auto& value) { PrintValue(value, context); },
            node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext(output, 4, 0));
}

}