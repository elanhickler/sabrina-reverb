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
uint64_t seGoldenRenderHashTapMod(float &peak, int delayTapMod);
float seDefaultValue(uint32_t id);
float seOldPresetTapMod();

TEST_CASE("Sabrina identity is unchanged", "[identity]")
{
    std::string why;
    const bool ok = seIdentityOk(why);
    INFO(why);
    REQUIRE(ok);
}


TEST_CASE("Tap Mod defaults to On; explicit On matches golden, Off differs", "[tapmod]")
{
    REQUIRE(seDefaultValue(511) == 1.f);
    float pOn{0}, pOff{0}, pDef{0};
    const uint64_t def = seGoldenRenderHashTapMod(pDef, -1);
    const uint64_t on = seGoldenRenderHashTapMod(pOn, 1);
    const uint64_t off = seGoldenRenderHashTapMod(pOff, 0);
    REQUIRE(on == def);
    REQUIRE(pOff > 0.01f);
    REQUIRE(off != def);
}

TEST_CASE("Old presets without Tap Mod load as On", "[tapmod]")
{
    REQUIRE(seOldPresetTapMod() == 1.f);
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
