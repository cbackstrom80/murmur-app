# Licensing

MURMUR is developed on the assumption that it **may become a commercial,
closed-source product.** Every dependency decision is made with that constraint in
front of it.

## This repository's own code

No LICENSE has been chosen yet for `pw8_core` / `pw8_plugin` / the tools/bindings
themselves -- see [LICENSE](../LICENSE) (currently "all rights reserved" as a
placeholder pending an explicit business decision). This is deliberate: choosing an
open-source license for the product's own code is a product/business decision, not
an engineering default, and shouldn't be made implicitly by scaffolding.

## Third-party dependencies

See [THIRD_PARTY_LICENSES.md](../THIRD_PARTY_LICENSES.md) for the human-readable
list and [third_party_dependencies.json](../third_party_dependencies.json) for the
machine-readable one. Summary:

| Dependency | License | Where it's used | Commercial-closed-source compatible? |
|---|---|---|---|
| nlohmann/json | MIT | `pw8_core` (patch JSON, internal only, not in public headers) | Yes |
| Catch2 | BSL-1.0 | Test suite only, never shipped | Yes |
| pybind11 | BSD-3-Clause | Python bindings (optional build) | Yes |
| JUCE | GPLv3 **or** commercial | Plugin scaffold (optional build, not built by default) | **Requires a paid JUCE commercial license before any closed-source plugin build/ship** |
| Google Benchmark | Apache-2.0 | Benchmarks (not yet implemented) | Yes |

**JUCE is the one dependency that needs an explicit business decision before
shipping.** Everything else in the current dependency graph is permissive
(MIT/BSD/BSL/Apache) and requires no special action for closed-source use beyond
carrying the attribution in `THIRD_PARTY_LICENSES.md`.

## Why no GPL in the core

The master spec's instruction to "prefer permissive dependencies where possible" and
"not pull GPL code into the core unless we have explicitly decided to accept the
consequences" was treated as a hard constraint for `pw8_core` specifically -- it has
zero GPL dependencies. The only place GPL enters the picture at all is JUCE's
GPLv3-or-commercial dual license for the *plugin* target, which is optional, off by
default, and untested in this pass (see PLUGIN_ARCHITECTURE.md). `pichenettes/eurorack`
concepts referenced in PRIOR_ART.md were deliberately restricted to the MIT-licensed
STM32F-target modules; the GPLv3 AVR-target modules were explicitly excluded from
consideration, and in any case no code was copied from either.

## Adding a new dependency

Before adding one, per the master spec, record: purpose, license, version, source.
Update both `THIRD_PARTY_LICENSES.md` and `third_party_dependencies.json`. Verify the
license is compatible with eventual closed-source commercial use before merging.
