# plugin/src/ui/

PLANNED (docs/ROADMAP.md Phase 17). Per the master spec, UI is deliberately last:
"prove engine quality" before building PLAY/DESIGN/LAB. Until then,
`PatchworkEightProcessor::createEditor()` returns `nullptr` / a generic parameter
editor. See `docs/PLUGIN_ARCHITECTURE.md` "Signature UI: Graph" for the eventual
algorithm-graph-centric design intent.
