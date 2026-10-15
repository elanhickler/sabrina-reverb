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
bool seIdentityOk(std::string &why);
uint64_t seGoldenRenderHashTapMod(float &peak, float delayTapMod);
float seDefaultValue(uint32_t id);
float seOldPresetTapMod(const char *saved511);

TEST_CASE("Sabrina identity is unchanged", "[identity]")
{
    std::string why;
    const bool ok = seIdentityOk(why);
    INFO(why);
    REQUIRE(ok);
}


TEST_CASE("Tap(-/+)Mod defaults to 1; explicit 1 matches golden, 0, 0.5 and 2 differ", "[tapmod]")
{
    REQUIRE(seDefaultValue(511) == 1.f);
    float pDef{0}, p1{0}, p0{0}, pHalf{0}, p2{0};
    const uint64_t def = seGoldenRenderHashTapMod(pDef, -1.f);
    const uint64_t one = seGoldenRenderHashTapMod(p1, 1.f);
    const uint64_t zero = seGoldenRenderHashTapMod(p0, 0.f);
    const uint64_t half = seGoldenRenderHashTapMod(pHalf, 0.5f);
    const uint64_t two = seGoldenRenderHashTapMod(p2, 2.f);
    REQUIRE(one == def);
    REQUIRE(p0 > 0.01f);
    REQUIRE(p2 > 0.01f);
    REQUIRE(zero != def);
    REQUIRE(half != def);
    REQUIRE(two != def);
    REQUIRE(half != zero);
    REQUIRE(two != half);
}

TEST_CASE("Old presets: missing Tap Mod loads 1, saved On loads 1, saved Off loads 0", "[tapmod]")
{
    REQUIRE(seOldPresetTapMod(nullptr) == 1.f);
    REQUIRE(seOldPresetTapMod("1") == 1.f);
    REQUIRE(seOldPresetTapMod("0") == 0.f);
}

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
