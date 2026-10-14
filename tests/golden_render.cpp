/*
 * Golden render check. See golden_engine.cpp. Record a new golden only on
 * purpose: set SE_GOLDEN_RECORD=1 and run the tests once.
 */

#include "catch2/catch2.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>

uint64_t seGoldenRenderHash(float &peak);

TEST_CASE("Sabrina golden render is bit-identical", "[golden]")
{
    float peak{0};
    const uint64_t h = seGoldenRenderHash(peak);
    INFO("peak " << peak);
    REQUIRE(peak > 0.01f); // the render must not be silent
    const std::string path = std::string(SE_GOLDEN_DIR) + "/sabrina-reverb.fnv64";

    if (std::getenv("SE_GOLDEN_RECORD"))
    {
        std::ofstream(path) << std::hex << h << "\n";
        WARN("recorded golden " << std::hex << h);
        return;
    }
    std::ifstream in(path);
    REQUIRE(in.good());
    uint64_t want{0};
    in >> std::hex >> want;
    INFO("got " << std::hex << h << " want " << want);
    REQUIRE(h == want);
}
