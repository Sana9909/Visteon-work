#pragma once

// ============================================================================
// JsonParser.hpp — Self-Contained Lightweight C++17 JSON Parser
// ============================================================================
// Designed to parse JSON configuration and test databases without any external
// dependencies or third-party libraries (conforming to strict hackathon rules).
// ============================================================================

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cctype>
#include <utility>

/// @brief Represents a parsed JSON node.
class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

private:
    Type type_ = Type::Null;
    bool boolVal_ = false;
    double numVal_ = 0.0;
    std::string strVal_;
    std::vector<JsonValue> arrVal_;
    std::map<std::string, JsonValue> objVal_;

public:
    JsonValue() = default;
    explicit JsonValue(bool b) : type_(Type::Bool), boolVal_(b) {}
    explicit JsonValue(double n) : type_(Type::Number), numVal_(n) {}
    explicit JsonValue(std::string s) : type_(Type::String), strVal_(std::move(s)) {}
    explicit JsonValue(std::vector<JsonValue> a) : type_(Type::Array), arrVal_(std::move(a)) {}
    explicit JsonValue(std::map<std::string, JsonValue> o) : type_(Type::Object), objVal_(std::move(o)) {}

    Type getType() const { return type_; }
    bool asBool() const { return boolVal_; }
    double asNumber() const { return numVal_; }
    const std::string& asString() const { return strVal_; }
    const std::vector<JsonValue>& asArray() const { return arrVal_; }
    const std::map<std::string, JsonValue>& asObject() const { return objVal_; }

    bool hasKey(const std::string& key) const {
        return type_ == Type::Object && objVal_.find(key) != objVal_.end();
    }

    const JsonValue& operator[](const std::string& key) const {
        if (type_ != Type::Object) throw std::runtime_error("JsonValue: Not a JSON object");
        auto it = objVal_.find(key);
        if (it == objVal_.end()) throw std::runtime_error("JsonValue: Key not found: " + key);
        return it->second;
    }

    const JsonValue& operator[](size_t index) const {
        if (type_ != Type::Array) throw std::runtime_error("JsonValue: Not a JSON array");
        if (index >= arrVal_.size()) throw std::runtime_error("JsonValue: Index out of bounds");
        return arrVal_[index];
    }
};

/// @brief Recursive-descent parser to decode JSON strings.
class JsonParser {
private:
    static void skipWhitespace(const std::string& str, size_t& pos) {
        while (pos < str.length() && std::isspace(static_cast<unsigned char>(str[pos]))) {
            pos++;
        }
    }

    static std::string parseString(const std::string& str, size_t& pos) {
        pos++; // Skip opening '"'
        std::string res;
        while (pos < str.length() && str[pos] != '"') {
            if (str[pos] == '\\') {
                pos++; // Simple escape sequence jump
                if (pos >= str.length()) break;
            }
            res += str[pos++];
        }
        pos++; // Skip closing '"'
        return res;
    }

    static double parseNumber(const std::string& str, size_t& pos) {
        size_t start = pos;
        if (str[pos] == '-') pos++;
        while (pos < str.length() && 
               (std::isdigit(static_cast<unsigned char>(str[pos])) || 
                str[pos] == '.' || str[pos] == 'e' || str[pos] == 'E' || 
                str[pos] == '+' || str[pos] == '-')) {
            pos++;
        }
        return std::stod(str.substr(start, pos - start));
    }

    static JsonValue parseValue(const std::string& str, size_t& pos) {
        skipWhitespace(str, pos);
        if (pos >= str.length()) throw std::runtime_error("JsonParser: Unexpected EOF");

        if (str[pos] == '"') {
            return JsonValue(parseString(str, pos));
        } else if (str[pos] == '{') {
            return parseObject(str, pos);
        } else if (str[pos] == '[') {
            return parseArray(str, pos);
        } else if (str[pos] == 't' || str[pos] == 'f') {
            bool val = (str[pos] == 't');
            pos += (val ? 4 : 5); // Skip "true" or "false"
            return JsonValue(val);
        } else if (str[pos] == 'n') {
            pos += 4; // Skip "null"
            return JsonValue();
        } else {
            return JsonValue(parseNumber(str, pos));
        }
    }

    static JsonValue parseArray(const std::string& str, size_t& pos) {
        pos++; // Skip '['
        std::vector<JsonValue> arr;
        skipWhitespace(str, pos);
        if (str[pos] == ']') {
            pos++;
            return JsonValue(arr);
        }
        while (true) {
            arr.push_back(parseValue(str, pos));
            skipWhitespace(str, pos);
            if (str[pos] == ']') {
                pos++;
                break;
            } else if (str[pos] == ',') {
                pos++;
            } else {
                throw std::runtime_error("JsonParser: Expected ',' or ']' inside array");
            }
        }
        return JsonValue(arr);
    }

    static JsonValue parseObject(const std::string& str, size_t& pos) {
        pos++; // Skip '{'
        std::map<std::string, JsonValue> obj;
        skipWhitespace(str, pos);
        if (str[pos] == '}') {
            pos++;
            return JsonValue(obj);
        }
        while (true) {
            skipWhitespace(str, pos);
            if (str[pos] != '"') throw std::runtime_error("JsonParser: Expected string key inside object");
            std::string key = parseString(str, pos);
            skipWhitespace(str, pos);
            if (str[pos] != ':') throw std::runtime_error("JsonParser: Expected ':' after object key");
            pos++; // Skip ':'
            obj[key] = parseValue(str, pos);
            skipWhitespace(str, pos);
            if (str[pos] == '}') {
                pos++;
                break;
            } else if (str[pos] == ',') {
                pos++;
            } else {
                throw std::runtime_error("JsonParser: Expected ',' or '}' inside object");
            }
        }
        return JsonValue(obj);
    }

public:
    static JsonValue parse(const std::string& str) {
        size_t pos = 0;
        return parseValue(str, pos);
    }

    static JsonValue parseFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("JsonParser: Failed to open JSON file: " + filePath);
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return parse(ss.str());
    }
};
