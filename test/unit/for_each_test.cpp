#include <catch2/catch.hpp>
#include <ycfp.h>

using namespace ycfp;


TEST_CASE("For each", "[foreach]") {
    int count = 0;
    doForEach([&](auto &&par) {
        count++;
    }, "3", 1, 1.0);
    REQUIRE(count == 3);
}

TEST_CASE("For each constructs just once", "[foreach]") {
    struct Sentry {
        Sentry() {
            count++;
        }
        int count = 0;
    };

    Sentry sentry;
    doForEach([](auto &&par) {
    }, sentry);
    REQUIRE(sentry.count == 1);
}
