# Sabrina Reverb, product #1. Identity, params and layout live in
# product.json (single source); this file only lists sources.
set(SE_PRODUCT_SOURCES src/ui/sabrina-face.cpp)
set(SE_PRODUCT_DSP_SOURCES dsp/sabrina_reverb.cpp)
set(SE_PRODUCT_TESTS tests/golden_render.cpp tests/golden_engine.cpp)
