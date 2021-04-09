#ifndef YCFP_YCFP_H
#define YCFP_YCFP_H

#include <iostream>
#include <yaml-cpp/yaml.h>

namespace ycfp {
    struct Object {
    };

    struct ExpectationType {
    };

    struct ValidationError {
        std::string message;
    };

    struct ValidationWarning {

    };

    enum class Existence {
        Required,
        Optional
    };

    template<typename T = Object>
    class Expectation : ExpectationType {
    public:
        Expectation(std::string_view key, Existence existence): key(key), existence(existence) {}

        std::optional<T> parse(const YAML::Node &node) const {
            if (!node[key]) return {};
            return node[key].as<T>();
        }

        std::vector<ValidationError> validate(const std::optional<T>& value) const {
            std::vector<ValidationError> errors;
            if (existence == Existence::Required && !value) {
                errors.emplace_back(ValidationError{key + " is required!"});
            }
            return errors;
        }

    private:
        Existence existence;
        std::string key;
    };

    template<>
    class Expectation<Object> : ExpectationType {
    public:
        template<typename... Ts>
        explicit Expectation(const Ts &... expectations) {
            assertExpectTypes(expectations...);
        }

    private:
        static void assertExpectTypes() {}

        template<typename T, typename... Ts>
        static void assertExpectTypes(const T &expectation, const Ts &... rest) {
            static_assert(std::is_base_of<ExpectationType, T>::value, "Not an ExpectationType");
            assertExpectTypes(rest...);
        }

    };

    template<typename T, typename Log = std::vector<ValidationError>>
    auto parseExpected(const YAML::Node &parent, const T &expectation, Log* log = nullptr) {
        auto value = expectation.parse(parent);
        if (log) {
            const auto& errors = expectation.validate(value);
            log->insert(std::end(*log), std::begin(errors), std::end(errors));
        }
        return value;
    }
}

#endif //YCFP_YCFP_H
