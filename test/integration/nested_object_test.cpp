#include <catch2/catch.hpp>
#include <ycfp.h>

using namespace std;
using namespace ycfp;
using ex = ycfp::Existence;

TEST_CASE("A nested object is parsed correctly", "[nested]") {
    YAML::Node node = YAML::Load("{"
                                 "document: entry,"
                                 " security: 42,"
                                 " metadata: {name: Jack, age: 27, height: 23.7},"
                                 "address: {city: {population: 300000}}"
                                 "}");
    Object expectation(
            Node<string>("document", ex::Required),
            Node<int>("security", ex::Required),
            Object(
                    "metadata",
                    Node<string>("name", ex::Required),
                    Node<int>("age", ex::Required),
                    Node<double>("height", ex::Required)
            ),
            Object(
                    "address",
                    Object(
                            "city",
                            Node<int>("population", ex::Required)
                    )
            )
    );
    std::vector<ValidationError> log{0};
    auto result = parseExpected(node, expectation, &log);
    REQUIRE(log.empty());
    REQUIRE(result.get<int>({"address", "city", "population"}) == 300000);
}


