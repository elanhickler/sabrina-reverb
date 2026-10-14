/*
 * Shared product parameter descriptor types. Each product's table is
 * generated from products/<slug>/product.json into product_generated.h.
 */

#ifndef SOUNDEMOTE_SHELL_PRODUCT_PARAM_DESC_H
#define SOUNDEMOTE_SHELL_PRODUCT_PARAM_DESC_H

#include <cstddef>
#include <cstdint>

namespace se::product
{
enum class ParamType
{
    Percent, // float 0..1 shown as percent
    Unit,    // float min..max, dimensionless, 3 decimals
    Integer, // stepped int min..max
    Choice   // stepped int, labelled choices
};

struct ChoiceDesc
{
    int value;
    const char *label;
};

struct ParamDesc
{
    uint32_t id; // host id, frozen by params.lock
    const char *key;
    const char *name;
    const char *group;
    ParamType type;
    double min, max, def;
    float defFloat; // same default as a float literal
    const char *unit;
    const ChoiceDesc *choices;
    int numChoices;
};
} // namespace se::product

#endif
