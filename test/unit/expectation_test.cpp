#include <catch2/catch.hpp>
#include <ycfp.h>

using namespace std;
using namespace ycfp;
using ex = ycfp::Existence;

TEST_CASE("A value expectation can be parsed to the value", "[expectation]") {
    YAML::Node node;
    node["name"] = "Jack";
    Node<string> expectation("name", ex::Required);
    auto result = parseExpected(node["name"], expectation);
    REQUIRE(result.get<string>({"name"}) == "Jack");
}

TEST_CASE("Required value not parsed or parsed as null is an error", "[expectation]") {
    YAML::Node node;
    Node<string> expectation("name", ex::Required);
    vector<ValidationError> log{0};
    auto result = parseExpected(node["name"], expectation, &log);
    REQUIRE(log.size() == 1);
    REQUIRE(log[0].message == "name is required!");
}

TEST_CASE("Accessing expected value that was not successfully parsed is an error", "[expectation]") {
    YAML::Node node;
    Node<string> expectation("name", ex::Required);
    auto result = parseExpected(node["name"], expectation);
    REQUIRE_THROWS_AS(result.get<string>({"name"}), AccessError);
}


TEST_CASE("Accessing expected value with wrong key is an error", "[expectation]") {
    YAML::Node node;
    node["name"] = "Jack";
    Node<string> expectation("name", ex::Required);
    auto result = parseExpected(node["name"], expectation);
    REQUIRE_THROWS_AS(result.get<string>({"not_name"}), AccessError);
}

TEST_CASE("Object expectation parses all expected values and returns correct result", "[expectation]") {
    YAML::Node node;
    node["name"] = "Jack";
    node["age"] = (unsigned int) 33;
    Node<> expectation(
            Node<string>{"name", ex::Required},
            Node<unsigned int>{"age", ex::Required}
    );
    auto result = parseExpected(node, expectation);
    CHECK(result.get<string>({"name"}) == "Jack");
    REQUIRE(result.get<unsigned int>({"age"}) == 33);
}


TEST_CASE("Nested object expectations parse values can be retrieved", "[expectation]") {
    YAML::Node node;
    node["data"]["name"] = "Jack";
    node["data"]["age"] = (unsigned int) 33;
    Node<> expectation(
            "root",
            Node<>{
                    "data",
                    Node<string>{"name", ex::Required},
                    Node<unsigned int>{"age", ex::Required}
            }
    );
    auto result = parseExpected(node, expectation);
    CHECK(result.get<string>({"data", "name"}) == "Jack");
    REQUIRE(result.get<unsigned int>({"data", "age"}) == 33);
}

TEST_CASE("Accessing invalid nested object throws", "[expectation]") {
    YAML::Node node;
    auto result = parseExpected(node, Node<>{"node"});
    REQUIRE_THROWS_AS(result.get<string>({"person", "data", "name"}), AccessError);
}

TEST_CASE("Accessing valid but not parsed expectation logs and throws", "[expectation]") {
    YAML::Node node;
    std::vector<ValidationError> log{0};
    Node<> expectation{
            "root",
            Node<>{
                    "data",
                    Node<string>{"name", ex::Required}
            }
    };
    auto result = parseExpected(node, expectation, &log);
    REQUIRE(log.size() == 1);
    REQUIRE(log[0].message == "data is required!");
    REQUIRE_THROWS_AS(result.get<string>({"data", "name"}), AccessError);
}

TEST_CASE("Parsing expectation structure that has root as child is an error") {
    YAML::Node node;
    REQUIRE_THROWS_AS(parseExpected(node, Node<>{
            "root",
            Node<>{Node<std::string>{"name", ex::Required}}
    }), ParsingError);
}

