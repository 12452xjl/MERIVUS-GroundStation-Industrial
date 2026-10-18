# Fleet OS intent and risk layer

`FleetIntentTask` is a transport independent task proposal. Set `intent` and
`parameters` (optionally including a `fleet` snapshot and capability
constraints), call `preparePreview()`, then `requestApproval()` to move the
task to `awaiting-approval`. `approve()` only records human approval; it does
not release a flight command, and it is rejected when the capability plan is
not feasible or the embedded risk assessment reports invalid input evidence.
`reject()` and `cancel()` close the proposal without dispatching
MAVLink.

Changing `intent` or `parameters` invalidates an existing preview or pending
approval, so approval cannot silently apply to stale task content.

`FleetRiskRadar.assess(task, riskSignals)` returns `{score, level, explanations,
inputValid, needsReview, requiresConfirmation}`. Task intent keywords and
caller supplied weighted signals are combined into a capped 0–100 score.
Scores of 30 or more require operator confirmation. Task parameters and risk
signals must use real QVariant booleans/maps and finite non-negative numeric
values; malformed types or out-of-range percentages make `inputValid=false`
and force a blocking score/review instead of being silently coerced by
`toBool()`/`toInt()`. The class only explains risk; it does not approve or
execute tasks.

Both classes are registered in QML module `Merivus 1.0` and are included by
`custom/custom.pri`. They intentionally do not depend on SwarmController,
GuidedActionsController, links, or transport implementations.

`FleetIntentTask.preparePreview()` embeds the radar result under
`riskAssessment`, so one preview carries candidate selection, reserves, risk
level, score, and explanations for the operator audit.

`FleetCapabilityMatcher.plan(task, fleet)` performs deterministic capability,
health, battery, link, optional speed, and optional endurance screening and
returns primary vehicles, reserves, roles, rejected-vehicle reasons and risk
signals. `requirePerformanceEvidence` can make missing speed/endurance data a
hard rejection. It is a proposal generator; the returned
`flightCommandReleased` flag is always false.
