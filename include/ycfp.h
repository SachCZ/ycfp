#ifndef YCFP_YCFP_H
#define YCFP_YCFP_H

#include <iostream>
#include <yaml-cpp/yaml.h>
#include <any>
#include <utility>

namespace ycfp {
    template<class F, class...Args>
    F doForEach(F f, Args &&...args) {
        (f(std::forward<Args>(args)), ...);
        return f;
    }

    struct ValidationError {
        std::string message;
    };

    enum class Existence {
        Required,
        Optional
    };

    template<typename T>
    class Node {
    public:
        explicit Node(Existence existence): existence(existence) {}
        Node(std::string_view key, Existence existence) : key(key), existence(existence) {}

        std::optional<T> parse(const YAML::Node &node) const {
            if (!node) return {};
            return node.as<T>();
        }

        [[nodiscard]] std::vector<ValidationError> validate(const std::any &value) const {
            std::vector<ValidationError> errors;
            auto extractedValue = std::any_cast<std::optional<T>>(value);
            if (existence == Existence::Required && !extractedValue) {
                errors.emplace_back(ValidationError{key + " is required!"});
            }
            return errors;
        }

        std::string key{};
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

    template<typename Exp>
    class Sequence {
    public:
        explicit Sequence(const Exp& expectation): expectation(expectation) {}
        [[nodiscard]] std::optional<std::vector<std::any>> parse(const YAML::Node &node) const {
            if (!node) return {};
            std::vector<std::any> result;
            for (auto&& subNode : node){
                result.emplace_back(expectation.parse(subNode));
            }
            return result;
        }

        [[nodiscard]] std::vector<ValidationError> validate(const std::any &value) const {

        }
    private:
        Exp expectation;
    };

    template<typename... Ts>
    class Object {
    public:
        explicit Object(std::string_view key, const Ts &&... expectations) : key(key), expectations(expectations...) {}
        explicit Object(const Ts &&... expectations) : expectations(expectations...) {}

        [[nodiscard]] std::optional<std::map<std::string, std::any>> parse(const YAML::Node &node) const {
            if (!node)
                return {};
            std::map<std::string, std::any> result;
            std::apply([&node, &result](const Ts &... args) {
                doForEach([&](const auto &expectation) {
                    if (expectation.key.empty()) {
                        throw ParsingError("Non root object must have key");
                    }
                    result[expectation.key] = expectation.parse(node[expectation.key]);
                }, args...);
            }, expectations);
            return result;
        }

        [[nodiscard]] std::vector<ValidationError> validate(const std::any &value) const {
            auto castedValue = std::any_cast<std::optional<std::map<std::string, std::any>>>(value);
            std::vector<ValidationError> errors{0};
            if (!castedValue) {
                errors.emplace_back(ValidationError{this->key + " is required!"});
                return errors;
            }
            std::apply([&errors, castedValue](const Ts &... args) {
                doForEach([&](const auto &expectation) {
                    const auto &result = expectation.validate(castedValue.value().at(expectation.key));
                    errors.insert(std::begin(errors), std::begin(result), std::end(result));
                }, args...);
            }, expectations);
            return errors;
        }

        std::string key;
    private:
        std::tuple<Ts...> expectations;
    };

    template<typename Res, typename Exp>
    class Result {
    public:
        explicit Result(const Res &res, const Exp &exp) : res(res), exp(exp) {}

        template<typename T>
        T get(std::initializer_list<std::string> keys) {
            if (keys.size() == 0) {
                throw AccessError("At least one key must be supplied!");
            }
            try {
                if constexpr (std::is_convertible<Exp, Node<T>>::value) {
                    if (exp.key != *keys.begin()) {
                        throw AccessError("Given key does not match the node key!");
                    } else {
                        return res.value();
                    }
                } else {
                    auto currentMap = res.value();
                    std::for_each(std::begin(keys), std::prev(std::end(keys)), [&currentMap](const auto &key) {
                        currentMap = std::any_cast<std::optional<std::map<std::string, std::any>>>(
                                currentMap[key]).value();
                    });
                    return std::any_cast<std::optional<T>>(currentMap[*std::prev(std::end(keys))]).value();
                }
            } catch (const std::bad_any_cast &) {
                throw AccessError("Accessing value by invalid type!");
            } catch (const std::bad_optional_access &) {
                throw AccessError("Accessing unparsed value!");
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


}

#endif //YCFP_YCFP_H
