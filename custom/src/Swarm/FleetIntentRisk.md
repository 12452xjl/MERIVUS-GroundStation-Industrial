# Fleet OS intent and risk layer

`FleetIntentTask` is a transport independent task proposal. Set `intent` and
`parameters` (optionally including a `fleet` snapshot and capability
constraints), call `preparePreview()`, then `requestApproval()` to move the
task to `awaiting-approval`. `approve()` only records human approval; it does
not release a flight command. `reject()` and `cancel()` close the proposal
without dispatching MAVLink.

`FleetRiskRadar.assess(task, riskSignals)` returns `{score, level, explanations,
requiresConfirmation}`. Task intent keywords and caller supplied weighted
signals are combined into a capped 0–100 score. Scores of 30 or more require
operator confirmation. The class only explains risk; it does not approve or
execute tasks.

Both classes are registered in QML module `Merivus 1.0` and are included by
`custom/custom.pri`. They intentionally do not depend on SwarmController,
GuidedActionsController, links, or transport implementations.

`FleetCapabilityMatcher.plan(task, fleet)` performs deterministic capability,
health, battery and link screening and returns primary vehicles, reserves,
roles, rejected-vehicle reasons and risk signals. It is a proposal generator;
the returned `flightCommandReleased` flag is always false.
