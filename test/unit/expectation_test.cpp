#include <catch2/catch.hpp>
#include <ycfp.h>

using namespace std;
using namespace ycfp;
using ex = ycfp::Existence;

TEST_CASE("A value expectation can be parsed to the value", "[expectation]") {
    YAML::Node node;
    node["name"] = "Jack";
    Expectation<string> expectation("name", ex::Required);
    REQUIRE(parseExpected(node, expectation) == "Jack");
}

TEST_CASE("Required value not parsed or parsed as null is an error", "[expectation]") {
    YAML::Node node;
    Expectation<string> expectation("name", ex::Required);
    vector<ValidationError> log;
    parseExpected(node, expectation, &log);
    REQUIRE(log.size() == 1);
    REQUIRE(log[0].message == "name is required!");
}
