#ifndef YCFP_YCFP_H
#define YCFP_YCFP_H

#include <iostream>
#include <yaml-cpp/yaml.h>
#include <any>
#include <utility>

namespace ycfp {
    struct Object {
    };

    struct NodeType {
    };

    struct ValidationError {
        std::string message;
    };

    enum class Existence {
        Required,
        Optional
    };

    template<typename T = Object>
    class Node : NodeType {
    public:
        Node(std::string_view key, Existence existence) : key(key), existence(existence) {}

        std::optional<T> parse(const YAML::Node &node) const {
            if (!node) return {};
            return node.as<T>();
        }

        [[nodiscard]] std::vector<ValidationError> validate(const std::any &value) const {
            std::vector<ValidationError> errors;
            try {
                auto extractedValue = std::any_cast<std::optional<T>>(value);
                if (existence == Existence::Required && !extractedValue) {
                    errors.emplace_back(ValidationError{key + " is required!"});
                }
            } catch (const std::bad_any_cast &) {
                if (existence == Existence::Required) {
                    errors.emplace_back(ValidationError{key + " is required!"});
                }
            }
            return errors;
        }

        std::string key;
        static constexpr bool isMap = false;
    private:
        Existence existence;
    };


    class AccessError : public std::exception {
    public:
        explicit AccessError(std::string_view message) noexcept: message(message) {}

        ~AccessError() override = default;

        [[nodiscard]] const char *what() const noexcept override {
            return this->message.c_str();
        }

    private:
        std::string message;
    };

    class ParsingError : public std::exception {
    public:
        explicit ParsingError(std::string_view message) noexcept: message(message) {}

        ~ParsingError() override = default;

        [[nodiscard]] const char *what() const noexcept override {
            return this->message.c_str();
        }

    private:
        std::string message;
    };

    template<typename Res, typename Exp>
    class Result {
    public:
        explicit Result(const Res &res, const Exp &exp) : res(res), exp(exp) {}

        template<typename T>
        T get(std::initializer_list<std::string> keys) {
            if (keys.size() == 0)
                throw AccessError("At least one key must be supplied!");
            if constexpr (Exp::isMap) {
                if (res) {
                    try {
                        auto currentMap = res.value();
                        std::for_each(std::begin(keys), std::prev(std::end(keys)), [&currentMap](const auto &key) {
                            currentMap = std::any_cast<std::optional<std::map<std::string, std::any>>>(
                                    currentMap[key]).value();
                        });
                        return std::any_cast<std::optional<T>>(currentMap[*std::prev(std::end(keys))]).value();
                    } catch (const std::bad_any_cast&) {
                        throw AccessError("Accessing unparsed value!");
                    } catch (const std::bad_optional_access&) {
                        throw AccessError("Accessing unparsed value!");
                    }
                } else {
                    throw AccessError("Accessing unparsed value!");
                }
            } else {
                if (keys.size() == 1) {
                    if (res && *keys.begin() == exp.key) {
                        return res.value();
                    } else {
                        throw AccessError("Accessing unparsed value!");
                    }
                } else {
                    throw AccessError("Multiple keys for for single value expectation!");
                }
            }
        }

    private:
        Res res;
        Exp exp;
    };

    template<typename T, typename Log = std::vector<ValidationError>>
    auto parseExpected(const YAML::Node &node, const T &expectation, Log *log = nullptr) {
        auto value = expectation.parse(node);
        if (log) {
            const auto errors = expectation.validate(value);
            log->insert(std::end(*log), std::begin(errors), std::end(errors));
        }
        return Result(value, expectation);
    }

    template<>
    class Node<Object> : NodeType {
    public:
        template<typename T, typename... Ts>
        explicit Node(const T &first, const Ts &... expectations) {

            constexpr bool isRoot = !std::is_convertible<T, std::string>::value;
            if constexpr (!isRoot) {
                key = first;
            }

            parseCallback = [this, &first, expectations...](const YAML::Node &node) {
                std::map<std::string, std::any> result;
                if constexpr (isRoot) {
                    parseRecursively(result, node, first, expectations...);
                } else {
                    parseRecursively(result, node, expectations...);
                }
                return result;
            };

            validateCallback = [expectations..., &first](const std::any &value, std::vector<ValidationError>& errors) {
                if constexpr (isRoot) {
                    validateRecursively(errors, value, first, expectations...);
                } else {

                    validateRecursively(errors, value, expectations...);
                }
                return errors;
            };
        }

        [[nodiscard]] std::optional<std::map<std::string, std::any>> parse(const YAML::Node &node) const {
            if (!node) return {};
            return parseCallback(node);
        }

        [[nodiscard]] std::vector<ValidationError>
        validate(const std::any &value) const {
            auto castedValue = std::any_cast<std::optional<std::map<std::string, std::any>>>(value);
            std::vector<ValidationError> errors{0};
            if (!castedValue) {
                errors.emplace_back(ValidationError{this->key + " is required!"});
                return errors;
            }
            return validateCallback(value, errors);
        }

        static constexpr bool isMap = true;
        std::string key;
    private:
        std::function<std::map<std::string, std::any>(YAML::Node)> parseCallback;

        std::function<std::vector<ValidationError>(std::any value, std::vector<ValidationError>& errors)> validateCallback;

        static void assertExpectTypes() {}

        template<typename Node, typename... Rest>
        static void assertExpectTypes(const Node &expectation, const Rest &... rest) {
            static_assert(std::is_base_of<NodeType, Node>::value, "Not an NodeType");
            assertExpectTypes(rest...);
        }

        static void parseRecursively(std::map<std::string, std::any> &, const YAML::Node &) {}

        template<typename Node, typename... Rest>
        static void parseRecursively(std::map<std::string, std::any> &result, const YAML::Node &node,
                                     const Node &expectation, const Rest &... rest) {
            if (expectation.key.empty()) {
                throw ParsingError("Root must be a parent expectation");
            }
            result[expectation.key] = expectation.parse(node[expectation.key]);
            parseRecursively(result, node, rest...);
        }

        static void validateRecursively(std::vector<ValidationError> &, const std::any &) {}

        template<typename Node, typename... Rest>
        static void validateRecursively(
                std::vector<ValidationError> &errors,
                const std::any &value,
                const Node &expectation,
                const Rest &... rest
        ) {
            auto castedValue = std::any_cast<std::optional<std::map<std::string, std::any>>>(value).value();
            const auto &result = expectation.validate(castedValue[expectation.key]);
            errors.insert(std::begin(errors), std::begin(result), std::end(result));
            validateRecursively(errors, value, rest...);
        }
    };
}

#endif //YCFP_YCFP_H
