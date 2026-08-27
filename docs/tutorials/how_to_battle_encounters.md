# How to Build Battle Encounters

Battle encounter scripting lets a specific battle (usually a legendary or a scripted boss trainer)
react to what happens during the fight — HP thresholds, turn counts, moves, faints, switches — by
running a battle script, without writing any new C. It sits entirely on top of the normal battle
engine: an ordinary battle that doesn't opt in behaves exactly as it always has.

## Contents

1. [What this is, and when not to use it](#what-this-is-and-when-not-to-use-it)
2. [A complete worked example](#a-complete-worked-example)
3. [Checkpoint reference](#checkpoint-reference)
4. [Condition reference](#condition-reference)
5. [Trigger flags](#trigger-flags)
6. [Property reference](#property-reference)
7. [Command reference](#command-reference)
8. [Determinism rules](#determinism-rules)
9. [Limits](#limits)
10. [Debugging](#debugging)

---

## What this is, and when not to use it

An **encounter** is a named bundle of **triggers**, plus an optional bundle of **properties**. Each
trigger says: *at this checkpoint, if these conditions hold, run this battle script.* The system
watches for eligible triggers at seven fixed points in the battle flow (the checkpoints — see
below), runs the highest-priority one that's eligible, and lets the engine continue. Properties are
the other half: they configure the battle itself once at battle start (the boss's level, whether
Poké Balls work, how tanky it is) instead of reacting to what happens in it.

Use it for:

- A boss that changes phase, grows a barrier, or heals once it crosses an HP threshold.
- A trainer who Mega Evolves their Pokémon on a scripted turn instead of through the normal
  gimmick-selection flow.
- A Pokémon that reacts to a specific move, or taunts when your Pokémon faints.
- Any battle where "when X happens, if Y is true, do Z" describes the design better than a move,
  ability, or item would.

**Don't use it for an ordinary trainer or wild battle.** If nothing in the fight needs to react to
battle state beyond what moves/abilities/items already do, an encounter adds an unused
`ENCOUNTER_*` id and trigger table for no benefit. The feature is opt-in per battle
(`setbattleencounter`, below) specifically so normal battles pay nothing for it — don't spend that
opt-in on a battle that's actually just a battle.

The whole system can also be compiled out entirely with `B_ENCOUNTER_SCRIPTING FALSE` in
`include/config/battle.h`, which is the config flag every encounter ultimately depends on.

---

## A complete worked example

This is a real encounter, taken from `src/data/battle_encounters.encounter` — copy it as a
starting point. It's a legendary that fights at the player's level cap, shrugs off most damage and
can't be caught, then — the first time it drops to 50% HP or below — shows its trainer sprite,
delivers a line, heals a fifth of its health back and raises its own Defense and Sp. Defense. Once
it's nearly beaten, its guard drops and the player can finally throw a ball at it.

### 1. The `.encounter` definition

```text
Encounter: Legendary_Barrier

Properties:
    Level: LevelCap
    CatchRate: 15
    Balls: Blocked
    DamageReduction: 70
    Immunities: Ohko, FixedDamage, HpSwap, SharedKo

Trigger: OnMoveEnd
Priority: 10
Flags: Once, OnEnter
Script: EncScript_LegendaryBarrier_PhaseTransition
Conditions:
    Battler(Boss).HpPercent <= 50
    Var(Phase) == 0

Trigger: OnTurnEnd
Priority: 10
Flags: Once, OnEnter
Script: EncScript_LegendaryBarrier_Weakened
Conditions:
    Battler(Boss).HpPercent <= 10
    Var(Phase) == 2
```

Read the first trigger as: *at the end of any move, if the boss is at 50% HP or below and its
`Phase` variable is still 0, run `EncScript_LegendaryBarrier_PhaseTransition` — and only the first
time that becomes true.*

- `Encounter: Legendary_Barrier` names the encounter. `encounterproc` (the tool that compiles this
  file) turns the name into `ENCOUNTER_LEGENDARY_BARRIER` for you — you don't declare the id
  yourself anywhere else.
- `Properties:` is optional, appears at most once, and comes before the first `Trigger:`. Every
  field can be omitted; an encounter with no `Properties:` block changes nothing about the battle.
  See the [property reference](#property-reference).
- `Trigger: OnMoveEnd` is the checkpoint (see the [reference](#checkpoint-reference) below).
- `Priority: 10` — required on every trigger. Lower runs first if more than one trigger is eligible
  at the same checkpoint.
- `Flags: Once, OnEnter` — `Once` means it never fires again after it runs; `OnEnter` means it only
  fires on the moment the conditions become true, not every checkpoint they happen to still be true
  (see [Trigger flags](#trigger-flags) — this combination is almost always what a phase transition
  wants).
- `Script:` is a label in a `.s` battle script file — an ordinary battle script label, nothing
  encounter-specific about the name.
- `Conditions:` is an indented block, one condition per line, implicitly ANDed together.
  `Var(Phase)` is author state: referencing an undeclared name here is how you declare it —
  `encounterproc` assigns it an index and emits `#define ENC_VAR_LEGENDARY_BARRIER_PHASE 0` for
  the script to use.

### 2. The battle script

```asm
EncScript_LegendaryBarrier_PhaseTransition::
	encsetvar 0, 2   @ phase = 2
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCLEGENDARYGATHERSSTRENGTH
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 3
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 3
	enchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
	playanimation BS_OPPONENT1, B_ANIM_SIMPLE_HEAL
	printstring STRINGID_ENCMYSTERIOUSBARRIERSURROUNDS
	waitmessage B_WAIT_TIME_LONG
	return

EncScript_LegendaryBarrier_Weakened::
	encsetvar 0, 3   @ phase = 3
	encsetdamagereduction ENC_TARGET_BOSS, 0
	encsetimmunity ENC_TARGET_BOSS, 0
	encchangestatvalue ENC_TARGET_BOSS, STAT_DEF, -25, ENC_AMOUNT_PERCENT
	encsetballs ENC_BALLS_ALLOWED
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCLEGENDARYWEAKENED
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	return
```

Everything except the `enc*` commands is a normal battle-script opcode
(`asm/macros/battle_script.inc`) — `trainerslidein`/`trainerslideout` for the presentation
interrupt, `printstring`/`waitmessage` for dialogue, `playanimation` for the visual (see the
[command reference](#command-reference) for the `enc*` set).

Note `enchangehp ..., 20, ENC_AMOUNT_PERCENT` rather than a raw HP number: with `Level: LevelCap`
this boss's max HP depends on the player's save, so a fixed figure can't be balanced against it.
The second script is the mirror image of the `Properties:` block — the same three settings the
battle started with, turned back off at the point the fight is meant to become winnable.

`encsetvar 0, 2` writes the var this trigger's own condition reads (`ENC_VAR_LEGENDARY_BARRIER_PHASE`,
i.e. index 0) so `Once`/`OnEnter` aren't the only thing stopping a re-fire — the condition itself
goes false too, which matters if you ever add a second trigger that also reads `Phase`.

**The script ends with `return`, never `end`/`end2`/`end3`.** The engine already wraps dispatch in
whatever shim the call site needs (see [Debugging](#debugging)); ending the script yourself with an
`end`-family opcode pops stack frames the engine still needs and corrupts the callback stack.

### 3. Hooking it up from the overworld

The overworld side is one command, placed before the battle starts:

```asm
	setbattleencounter ENCOUNTER_LEGENDARY_BARRIER
	trainerbattle_single TRAINER_SOME_BOSS, TrainerBossIntroText, TrainerBossDefeatText
```

`setbattleencounter` (a normal overworld script command, `asm/macros/event.inc`) just remembers the
id; the next battle that starts consumes it. It works identically ahead of `dowildbattle` for a
wild encounter — the system doesn't care whether the opponent is a trainer or wild.

---

## Checkpoint reference

A checkpoint is a fixed point in the battle flow where the engine asks "does an encounter trigger
want to run here?" Checkpoints describe **when**; they carry no information about what happened —
that's the event context, described per checkpoint below.

| Checkpoint | Fires | Event context populated |
| --- | --- | --- |
| `OnBattleStart` | Once, after all battlers have switched in, abilities/hazards resolved | none |
| `OnTurnStart` | Once per turn, before the first action runs (after turn order is locked in) | none |
| `OnMoveEnd` | After a move fully resolves — once per hit for multi-hit moves, once per target for spread moves | `Event.Battler`, `Event.Target`, `Event.Move`, `Event.Cause`, `Event.OldValue`/`Event.NewValue` |
| `OnFaint` | When a battler faints, after EXP/absent-flag bookkeeping is settled | `Event.Battler`, `Event.Cause` |
| `OnSwitchIn` | When a battler enters, after its own entry abilities/hazards resolve | `Event.Battler` |
| `OnTurnEnd` | Once per turn, after end-of-turn effects (weather, status, Leftovers, …) resolve | `Event.Battler`, `Event.Cause`, `Event.OldValue`/`Event.NewValue` |
| `OnBattleEnd` | Once, before teardown, after the outcome (win/loss/flee) is already decided | none |

Reading a blank "event context" cell as a condition — e.g. `Event.Move` at `OnBattleStart` — is
invalid and asserts; there's nothing there to read yet. Only reference `Event.*` in a trigger whose
checkpoint actually populates the field you want (Battler/Cause columns above), and `Event.OldValue`/
`Event.NewValue` only where "values" is listed.

There's no `OnDamage` checkpoint. HP changes are recorded at the moment they happen but dispatched
at the next safe checkpoint (`OnMoveEnd` or `OnTurnEnd`) — an HP-change animation can't be
interrupted mid-flight, so the encounter reacts once it's finished rather than while it's playing.

`OnBattleEnd` is presentation-only: the outcome is already fixed, so a state-changing command there
asserts.

---

## Condition reference

A condition is `<operand> <comparison> <value>`, one per line inside a `Conditions:` block, ANDed
together unless grouped (below). Comparisons are `==`, `!=`, `<`, `<=`, `>`, `>=`.

| Operand | Reads | Notes |
| --- | --- | --- |
| `Battler(<ref>).HP` | Current HP | |
| `Battler(<ref>).HpPercent` | Current HP as 0–100 | |
| `Battler(<ref>).MaxHp` | Max HP | |
| `Battler(<ref>).Species` | Species id | compare against a `SPECIES_*` constant |
| `Battler(<ref>).Ability` | Current ability | compare against an `ABILITY_*` constant |
| `Battler(<ref>).Status` | `status1` bitfield | compare against a `STATUS1_*` constant |
| `Battler(<ref>).Type` | Primary type | compare against a `TYPE_*` constant |
| `Battler(<ref>).Stat(<STAT>)` | Stat *stage* (not the stat value) | `<STAT>` is a `STAT_*` constant, e.g. `Stat(STAT_DEF)` |
| `Var(<name>)` | An author-defined variable | first reference declares it; scoped to the encounter |
| `Event.Battler` | The battler the event is *about* | compare against a `B_POSITION_*` id (below); see checkpoint table for validity |
| `Event.Target` | The other battler involved | " |
| `Event.Move` | The move involved | " |
| `Event.MoveType` | The type of the move involved | compare against a `TYPE_*` constant; `OnMoveEnd` only |
| `Event.Cause` | Why the event happened (`ENC_CAUSE_*`) | " |
| `Event.OldValue` / `Event.NewValue` | Before/after values (e.g. HP before/after a change) | " |
| `Weather` | Current battle weather | |
| `Terrain` | Current field terrain | |
| `Turn` | The turn counter | |
| `BattlerCount` | Number of battlers this battle (2 or 4) | doubles-aware conditions |

### Battler references

`<ref>` inside `Battler(...)` is one of: `Boss`, `Self`, `PlayerLeft`, `PlayerRight`,
`OpponentLeft`, `OpponentRight`.

- `Boss` is always the opponent's left slot — the encounter's subject.
- `Self` is whichever battler raised the current event (only meaningful where `Event.Battler` is
  valid).
- The four positional refs resolve through the same battle-position lookup commands use for
  targeting; see the [`_RIGHT` gotcha](#debugging) for what happens in a singles battle.

`Event.Battler` / `Event.Target` are not `Battler(...)` refs — they're raw battler ids, so compare
them against `B_POSITION_PLAYER_LEFT` (0), `B_POSITION_OPPONENT_LEFT` (1),
`B_POSITION_PLAYER_RIGHT` (2), `B_POSITION_OPPONENT_RIGHT` (3). In a legendary (singles) fight the
player is `B_POSITION_PLAYER_LEFT` and the boss is `B_POSITION_OPPONENT_LEFT`.

At `OnMoveEnd`, `Event.Battler` is the battler that **got hit** and `Event.Target` is the one that
**used the move** — so "the boss used an Ice move" is `Event.Target == B_POSITION_OPPONENT_LEFT`,
and "a move hit the boss" is `Event.Battler == B_POSITION_OPPONENT_LEFT`. At `OnSwitchIn` / `OnFaint`,
`Event.Battler` is the battler that switched in / fainted and `Event.Target` is unused.

### Compound logic: `All:` / `Any:` / `Not:`

A flat list of conditions is an implicit `All:`. For anything else, nest an indented block:

```text
Conditions:
    Battler(Boss).HpPercent <= 50

    Any:
        Weather == B_WEATHER_RAIN_NORMAL
        Weather == B_WEATHER_SUN_NORMAL
```

This means: boss at 50% HP or below, **and** (raining **or** sunny). `Not:` takes exactly one
child condition and inverts it. Nesting is bounded to 4 levels deep — see [Limits](#limits).

---

## Trigger flags

`Once` and `OnEnter` are independent and combine meaningfully:

| Flags | Behavior |
| --- | --- |
| *(neither)* | Fires at every checkpoint where the conditions hold — a *level*. |
| `Once` | Fires the first time the conditions hold, then never again. |
| `OnEnter` | Fires each time the conditions become true after having been false — an *edge*. |
| `Once, OnEnter` | Fires only on the first crossing. **The phase-transition default.** |

The distinction matters because a condition like `HpPercent <= 50` *stays* true once crossed. Plain
`Once` would also fire the very first time the checkpoint is checked if the boss simply *starts*
below the threshold (e.g. after a status-damage tick), which is usually not what a one-shot phase
transition wants. `OnEnter` requires an actual false→true transition, which is almost always the
correct semantics for "the moment it crossed" — pair it with `Once` unless you deliberately want the
transition to be able to re-fire (e.g. a barrier that can be knocked down and regrown).

---

## Property reference

`Properties:` is an optional indented block, at most one per encounter, placed before the first
`Trigger:`. It answers "what kind of battle is this?", where a trigger answers "what happens when
X?". Every field is optional and omitting one changes nothing.

| Property | Value | Does |
| --- | --- | --- |
| `Level:` | `LevelCap`, or a level from 1 to `MAX_LEVEL` | Rebuilds every opponent Pokémon at that level — experience, stats and HP all restated, not healed. `LevelCap` reads `GetProgressionLevelCap()` (`src/caps.c`), so the fight tracks story progression and New Game Plus offsets |
| `CatchRate:` | `0`–`255` | Replaces the species' own catch rate for this battle. Higher is easier |
| `Balls:` | `Default`, `Blocked`, `Allowed` | Whether the player may throw a Poké Ball |
| `DamageReduction:` | `0`–`99` | The boss takes this much less damage, as a percentage, from every source |
| `Immunities:` | comma list of `Ohko`, `FixedDamage`, `HpSwap`, `SharedKo`, plus `All`/`None` | Move classes the boss ignores |
| `CapTypeEffectiveness:` | `True`/`False` | Clamps the boss's incoming type-effectiveness multiplier at 2x — a double weakness stacked onto another double weakness can't spike to 4x |
| `FlatToxicDamage:` | `True`/`False` | Toxic deals the same flat 1/16 max HP against the boss every turn instead of its counter ramping that up turn over turn |
| `Moves:` | 1–4 `MOVE_*` constants, comma separated | Replaces the boss's moves, PP included. A slot the list doesn't reach keeps what it was built with |
| `AiFlags:` | one or more `AI_FLAG_*` constants, `\|` separated | The AI the opponent side runs, replacing whatever the battle type would derive |

`DamageReduction:`, `Immunities:`, `CapTypeEffectiveness:`, `FlatToxicDamage:` and `Moves:` apply to
the **boss** (the opponent's left slot). Any other battler — or any change to these mid-battle — is
a job for `encsetdamagereduction` / `encsetimmunity` / `encsetcaptypeeffectiveness` /
`encsetflattoxicdamage` in a script.

### `Level:` and when it applies

The level override runs once, before the battle's Pokémon are built from the party, which is the
only window where changing a level is a clean operation. It applies to **both opponent parties**
(`B_TRAINER_OPPONENT_A` and `B_TRAINER_OPPONENT_B`), never to the player or their partner, and it
skips empty slots and eggs. There is no per-slot form — an encounter that needs one opponent at a
different level than the rest should set the levels in its trainer data instead.

### `Moves:` — a curated boss moveset

`Level:` restates stats, not moves, so without this a boss keeps whatever its learnset gave it at
the level the overworld script's `setwildbattle` named — which is rarely the fight you designed.
`Moves:` names the moveset outright:

```text
    Moves: MOVE_FREEZE_DRY, MOVE_BLIZZARD, MOVE_HURRICANE, MOVE_ANCIENT_POWER
```

It applies to the boss and nobody else — a move list is inherently per-mon, so there's no sensible
"apply to every opponent" reading of it. It runs in the same pre-battle window as `Level:`. Each
move's PP is set to that move's own maximum; PP Ups are not applied. A list shorter than four leaves
the remaining slots as they were built, so `Moves: MOVE_BLIZZARD` replaces only the first slot.

The move names pass through to the C compiler unchecked, so a typo is a compiler error, not an
`encounterproc` error. Nothing verifies the species can legally learn them, either — that's a design
decision, not a build-time one.

### `AiFlags:` — giving a wild boss an AI

A wild battle has **no AI scoring at all** by default: the opponent picks a random legal move.
`AiFlags:` is what makes a scripted wild legendary play properly:

```text
    AiFlags: AI_FLAG_SMART_TRAINER
```

Multiple flags combine with `|`. The value replaces whatever the battle type would otherwise derive
rather than adding to it, matching how the rest of `Properties:` works, and it applies to both
opponent slots. In a doubles encounter `AI_FLAG_DOUBLE_BATTLE` is added automatically, the same way
the normal trainer path adds it.

This is per-encounter state in ROM, unlike `B_VAR_WILD_AI_FLAGS`, which is a global save variable
that would have to be set and cleared around every battle.

### `Balls:` — blocking, and the "now catch it" pattern

`Allowed` only lifts a block **this encounter** imposed. It never makes a battle catchable that
wouldn't be otherwise: a trainer battle, a Ghost without a Silph Scope, Nuzlocke's used-up zone and
the other existing rules all still apply and are checked first.

The pattern this exists for is a legendary that can't be caught until it's beaten down: start with
`Balls: Blocked`, then have the final phase's script run `encsetballs ENC_BALLS_ALLOWED` (usually
alongside `encsetcatchrate`, as in the worked example above). A blocked throw behaves exactly like
the existing Nuzlocke and Mono-Type blocks: the ball animation plays, a message prints, and the turn
ends.

**The catch-window damage guard is automatic.** Whenever `ballPolicy` is `ENC_BALLS_ALLOWED`, the
engine forces the boss's damage reduction to the maximum (`ENC_MAX_DAMAGE_REDUCTION`) so a stray hit
can't KO the catch target — you don't script this, and it can't be turned off per-encounter. The
guard lifts the moment the boss recovers any HP (restoring whatever reduction the encounter itself
had set) and re-arms at the next checkpoint where the boss is catchable and hasn't just healed. A
script that sets the boss's reduction while the guard holds is changing the value the guard will
restore, not the live one. See `UpdateEncounterCatchGuard` in `src/battle_encounter.c`.

### `DamageReduction:` — what counts as "damage"

A reduction of 70 means the battler takes 70% less from anything that goes through the damage
formula or a passive HP tick: moves, weather, burn/poison, entry hazards, Leftovers-style chip,
recoil, and ability/item damage. Damage is floored at 1, so a reduced hit is never silently turned
into nothing.

Three things deliberately **ignore** it:

- **Fixed-damage moves** (Super Fang, Seismic Toss, Dragon Rage, Endeavor, …). They never touch the
  damage formula, so they'd otherwise cut straight past a boss's reduction — which is exactly why
  `Immunities: FixedDamage` exists.
- **Perish Song and Destiny Bond.** These are lethal effects, not damage; reducing them to
  survivable would be incoherent. `Immunities: SharedKo` is how you refuse them.
- **`enchangehp` in an encounter script.** The number in the script is the encounter's own; another
  encounter rule rescaling it would make phase scripts impossible to reason about.

The maximum is 99 (`ENC_MAX_DAMAGE_REDUCTION`). A battler nothing at all can damage isn't a fight —
if a specific move must not work, that's what the immunities are for.

There is no on-screen indicator for any of this — a reduced hit just shows a smaller number. If a
script raises, lowers, or gates the reduction as a *mechanic* (a regenerating barrier, a phase that
hardens the boss, a stance that punishes a move type), the player has to be told through dialogue,
or the fight just reads as "my attacks stopped working" with no cause. Point at the cause without
spelling out the counter, and don't fire the line just once — a single message is easily missed.
Stash the last-announced reduction in a script var and re-print the line whenever it changes, plus
keep a short recurring line running while the mechanic is active.

### `Immunities:` — the four classes

| Name | Covers | Player sees |
| --- | --- | --- |
| `Ohko` | `EFFECT_OHKO`: Sheer Cold, Fissure, Guillotine, Horn Drill | "It doesn't affect …" |
| `FixedDamage` | Every effect whose damage bypasses the damage formula: Super Fang, Night Shade, Seismic Toss, Dragon Rage, Sonic Boom, Psywave, Endeavor, Final Gambit, Counter/Mirror Coat/Metal Burst, Bide | "It doesn't affect …" |
| `HpSwap` | Pain Split | "But it failed!" |
| `SharedKo` | Destiny Bond, Perish Song | Destiny Bond doesn't take the boss down with it; a Perish Song count already on the boss is cancelled |

`All` sets every bit; `None` sets none (the same as omitting the field). They're separate classes on
purpose — giving every legendary the same immunity list makes every legendary fight the same fight.

### `CapTypeEffectiveness:` — softening double weaknesses

`DamageReduction:` is a flat percentage on top of whatever the type matchup already multiplied
damage by, so a quadruple weakness (two 2x type matchups stacking to 4x, e.g. Rock into an Ice/Flying
boss) can still land a disproportionate hit even at 90% reduction — the pre-reduction number was
already four times larger than a neutral hit.

`CapTypeEffectiveness: True` clamps the multiplier itself to 2x before reduction ever applies. It
affects `CalcTypeEffectivenessMultiplier`, so the AI's move scoring and the "It's super effective!"
messaging see the same capped value the damage calc uses — nothing reads as stronger than it hits.
A double weakness alone is untouched; only a matchup that would exceed 2x gets clamped down to it.
This never blocks a move outright — that's what `Immunities:` is for.

### `FlatToxicDamage:` — Toxic without the ramp

Toxic's counter (`STATUS1_TOXIC_COUNTER`) grows by one every turn it stays on a battler, up to 16,
and each end-of-turn tick multiplies the base 1/16-max-HP poison damage by that counter — meaning by
turn 16 a single tick is a full max-HP hit. That ramp is balanced around how long an ordinary battle
runs. An encounter can run for many more turns than that (a multi-phase boss fight, a stalled-out
`OnTurnEnd` loop), so left alone, `Toxic`ing the boss once and outlasting it eventually deals lethal
damage regardless of `DamageReduction:` — the percentage is applied every turn to a number that keeps
growing on its own.

`FlatToxicDamage: True` keeps the counter advancing (so nothing else that reads it changes behavior)
but skips the multiply: the boss takes the same flat 1/16 max HP every turn Toxic is active, exactly
like ordinary Poison, for as long as the fight lasts. It only affects the multiply in
`HandleEndTurnPoison` (`battle_end_turn.c`) — Toxic still applies and still stacks with everything
else `DamageReduction:` already scales down.

---

## Command reference

Anything not listed here is an ordinary battle-script opcode
(`asm/macros/battle_script.inc`/`data/battle_scripts_*.s`) — dialogue, animations, trainer
sprite handling, and the rest of the existing engine command set all work unchanged inside an
encounter script. There is no separate scripting language.

The commands below exist specifically for encounter scripts:

| Command | Does | Pre-existing opcode? |
| --- | --- | --- |
| `encsetvar <var>, <value>` | Sets author variable `<var>` | wraps `setbyte` |
| `encaddvar <var>, <value>` | Adds to author variable `<var>` | wraps `addbyte` |
| `encsubvar <var>, <value>` | Subtracts from author variable `<var>` | wraps `subbyte` |
| `encjumpifvar <cmp>, <var>, <value>, <label>` | Branches on author variable `<var>` | wraps `jumpifbyte` |
| `enchangehp <target>, <amount>[, <mode>]` | Heals (positive) or damages (negative) every battler `<target>` resolves to, with the normal animated health bar | encounter-specific (`callnative`) |
| `encchangestat <target>, <stat>, <stages>` | Adds `<stages>` to `<stat>` for every battler `<target>` resolves to — silent, no message/animation | encounter-specific (`callnative`) |
| `encchangestatvalue <target>, <stat>, <amount>[, <mode>]` | Moves the **raw battle stat** (not the stage) of `<stat>` — silent, and not undone by Haze or switching out | encounter-specific (`callnative`) |
| `encsetdamagereduction <target>, <percent>` | Sets how much less damage `<target>` takes, `0`–`99`. Replaces the current value | encounter-specific (`callnative`) |
| `encsetimmunity <target>, <immunities>` | Replaces `<target>`'s `ENC_IMMUNE_*` mask (`0` clears it) | encounter-specific (`callnative`) |
| `encsetcaptypeeffectiveness <target>, <cap>` | `TRUE` clamps `<target>`'s incoming type effectiveness at 2x; `FALSE` removes the clamp | encounter-specific (`callnative`) |
| `encsetflattoxicdamage <target>, <flat>` | `TRUE` stops Toxic's counter from ramping `<target>`'s damage up each turn; `FALSE` restores the ramp | encounter-specific (`callnative`) |
| `encsetballs <policy>` | `ENC_BALLS_DEFAULT` / `ENC_BALLS_BLOCKED` / `ENC_BALLS_ALLOWED` | encounter-specific (`callnative`) |
| `encsetcatchrate <rate>` | Replaces the catch rate for this battle; `ENC_CATCH_RATE_NONE` restores the species' own | encounter-specific (`callnative`) |
| `encsetweather <weather>[, <turns>]` | Sets the battle weather to a `BATTLE_WEATHER_*` value. `<turns>` defaults to `0`, meaning permanent. Silent; clear it again with the existing `removeweather` | encounter-specific (`callnative`) |
| `encmegaevolve <target>, <failLabel>` | Forces `<target>` (must resolve to exactly one battler) to Mega Evolve outside the normal gimmick-selection flow, with the stock Mega Evolution presentation | encounter-specific (`callnative`) |

### Fixed vs. percentage amounts

`enchangehp` and `encchangestatvalue` take an optional trailing `<mode>`:

| Mode | Reads `<amount>` as |
| --- | --- |
| `ENC_AMOUNT_FIXED` (the default when omitted) | Raw HP / raw stat points |
| `ENC_AMOUNT_PERCENT` | A percentage of that battler's own max HP / current value for that stat |

Prefer `ENC_AMOUNT_PERCENT` in anything using `Level: LevelCap` or facing New Game Plus offsets: the
boss's max HP and stats aren't known when the script is written, so a fixed number can't be balanced
against them. A percentage that would round down to zero is floored at 1 HP, so a "heal 5%" never
silently does nothing on a small Pokémon.

`encchangestat` (stages) and `encchangestatvalue` (raw stats) are different tools. Stages are the
standard, visible currency: clamped to ±6, cleared by Haze, undone by switching out. A raw stat
change moves the number the boss was built with — permanent for the rest of the battle, invisible to
the player, and unbounded past what a stage could reach. Reach for stages first; use the raw form
when a phase transition is meant to make the boss *fundamentally* different, or when scaling a stat
by a percentage is the only portable way to express it.

### Setting vs. accumulating

`encsetdamagereduction`, `encsetimmunity`, `encsetcaptypeeffectiveness`, `encsetflattoxicdamage`, `encsetballs` and `encsetcatchrate` all **replace** the
current value rather than adding to it. That's what lets a script raise a boss's guard for one phase
and drop it in the next without tracking what it added — `encsetdamagereduction ENC_TARGET_BOSS, 0`
always means "no reduction", whatever the `Properties:` block or an earlier phase set.

`encsetvar`/`encaddvar`/`encsubvar`/`encjumpifvar` are thin wrappers: they're the same
`setbyte`/`addbyte`/`subbyte`/`jumpifbyte` opcodes every other battle script uses, just pre-addressed
into the encounter's variable array so you write a var index instead of a raw address.

`<target>` on `enchangehp`/`encchangestat`/`encmegaevolve` is an `EncounterTarget`: `ENC_TARGET_BOSS`,
`ENC_TARGET_SELF`, `ENC_TARGET_EVENT_TARGET`, `ENC_TARGET_PLAYER_LEFT`/`_RIGHT`,
`ENC_TARGET_OPPONENT_LEFT`/`_RIGHT`, `ENC_TARGET_ALL_FOES`, `ENC_TARGET_ALL_ALLIES`,
`ENC_TARGET_ALL_BATTLERS`. The group targets (`ALL_FOES`/`ALL_ALLIES`/`ALL_BATTLERS`) are what make a
command doubles-safe without writing two versions of a script — `ALL_FOES` hits both opposing
battlers in a double battle and just the one in a single battle, automatically.

A command that mutates a battler asserts if a **single-slot** target (`BOSS`, `SELF`,
`EVENT_TARGET`, `PLAYER_LEFT`, …) resolves to a fainted or absent battler — naming a gone battler
outright is an authoring mistake. A **group** target (`ALL_FOES`/`ALL_ALLIES`/`ALL_BATTLERS`) instead
skips any fainted/absent member silently: at `OnMoveEnd`/`OnFaint` the event battler is often already
down, and "everyone still standing" is the sensible reading of the set.

---

## Determinism rules

When more than one trigger is eligible, the order they run in is fixed by these rules — reason
about a multi-trigger checkpoint with these, not by reading the dispatcher's source:

1. At a checkpoint, the eligible trigger with the lowest `Priority` runs first.
2. Ties break by trigger order within the `.encounter` file — earliest declared, first run.
3. After a script finishes, every trigger at that checkpoint is re-evaluated against the new state.
4. A trigger that lost on priority (but is still eligible) isn't discarded — it can win a later pass.
5. A `Once` trigger is marked fired the moment it's *selected*, so it can never run twice even
   within the same checkpoint's re-evaluation passes.
6. At most 4 scripts run per checkpoint (see [Limits](#limits)) — after that the dispatcher stops,
   even if more triggers are still eligible.
7. The "previous state" `OnEnter` compares against is always "as of the last checkpoint", not
   "as of the start of this checkpoint's re-evaluation loop" — a script mid-checkpoint changing HP
   doesn't retroactively change what an `OnEnter` trigger earlier in the same pass saw.

---

## Limits

| Limit | Value | What happens if you exceed it |
| --- | --- | --- |
| Scripts per checkpoint | 4 | Dispatch stops for that checkpoint; an `assertf` fires ("runaway trigger chain?") |
| Triggers per encounter | 32 | The whole encounter fails to load (`assertf`); no triggers dispatch |
| Author variables per encounter | 16 | `encounterproc` refuses to compile — "too many named variables" |
| Condition nesting depth (`All:`/`Any:`/`Not:`) | 4 | The offending subtree asserts and evaluates `FALSE`; `encounterproc` also flags it at compile time |
| Battle script call depth (`call`/`return`) | 8 | Shared with the rest of the engine, not encounter-specific — `assertf` on overflow |

All of these fail loud (an `assertf`) rather than silently misbehaving — see
[Debugging](#debugging).

---

## Debugging

Every encounter assertion identifies **encounter, trigger/context, and reason**, e.g.:

```text
encounter 3: trigger 2: condition nesting deeper than 4
encounter 3: 4 scripts ran at checkpoint 2 - runaway trigger chain?
```

In a dev build this shows a resumable crash screen and then still runs the assertion's recovery
block; a release build runs the recovery silently; a test build marks the test `INVALID`. This is
the same `assertf(cond) { recovery }` pattern used throughout the battle engine, not something
encounter-specific.

To read an assertion back to a `.encounter` line: match the encounter id in the message against
`enum EncounterId`, then count triggers in that encounter's block in the `.encounter` file in
declaration order — trigger index 2 is the third `Trigger:` block under that `Encounter:`. (Dev
builds also compile a per-trigger `{file, line}` table alongside each encounter's trigger array,
generated by `encounterproc` for future tooling to consume — it isn't wired into assertion output
yet, so the manual count is currently the reliable path.)

### Known gotchas

- **Script terminators.** An encounter script always ends with `return`, never `end`/`end2`/`end3`.
  The engine supplies whichever shim the call site needs; ending the script yourself pops stack
  frames the engine still needs and corrupts the callback stack.
- **`TURN_START` doesn't re-sort turn order.** Turn order is locked in before `OnTurnStart` fires.
  A script that changes Speed or forces a switch at this checkpoint changes state for *later*
  turns, not the one in progress.
- **`_RIGHT` refs in a singles battle.** `PlayerRight`/`OpponentRight` (as a condition battler ref
  or an `EncounterTarget`) have no battler to resolve to in a singles battle. A condition using one
  asserts and evaluates false; a command target using one resolves to an empty set (contributes no
  battler, rather than reading a stale/inactive battler's data) — silently doing nothing rather than
  acting on the wrong Pokémon. Guard doubles-only conditions/targets with a `BattlerCount == 4`
  check if the encounter can also run as a single battle.
- **`Level:` restates the Pokémon, it doesn't level it up.** Experience is reset to the exact
  threshold for that level and HP is refilled, so a `Level:` override on a battle the player can
  re-enter always produces the same opponent. It runs before the battle's Pokémon are built, which
  is also why nothing in a script can change a level — by then the level is baked into stats that
  have already been derived from it.
- **The AI sees the damage reduction.** It shares the damage-calculation path, so a boss the player
  can barely dent reads as one when the AI picks a move. That's intentional; don't be surprised when
  a heavily-reduced boss stops being predictable about which attack it leads with.
- **The player does *not* see the damage reduction.** There's no UI for it — a blunted hit is just a
  small number. Any script that moves or gates the reduction as a mechanic (regenerating barrier,
  hardening phase, move-type stance) needs dialogue saying *something* is wrong and roughly what's
  causing it. See [`DamageReduction:`](#damagereduction-what-counts-as-damage).
- **`CapTypeEffectiveness:` doesn't replace `DamageReduction:`, it complements it.** Reduction is a
  flat percentage applied after the type multiplier, so a quadruple weakness still hits several times
  harder than a neutral move even at 90% reduction — it was that much larger before reduction ever
  touched it. Set both on any boss with a real 4x matchup in its typing.
- **`DamageReduction:` doesn't stop Toxic from eventually overwhelming a boss.** The counter ramp is
  a multiplier on top of reduction, not something reduction caps — a long enough fight always reaches
  the turn where 90% off a maxed-out counter is still lethal. `FlatToxicDamage: True` is what actually
  bounds it.
- **`Immunities: FixedDamage` is not optional if you set `DamageReduction:`.** Super Fang and
  friends compute their damage from the target's HP, never from the damage formula — a 70%-reduced
  boss with no `FixedDamage` immunity still loses half its health to one Super Fang.
- **A blank event-context cell is invalid, not zero.** Reading `Event.Move` at a checkpoint that
  doesn't populate it (see the [checkpoint table](#checkpoint-reference)) asserts rather than
  quietly returning 0 — that would otherwise be indistinguishable from a real "no move" case.
- **`Event.MoveType` is the move's base type.** It reads the type from move data, so a move
  re-typed at runtime — Normalize and the `-ate` abilities, Electrify, Tera — still matches its
  printed type here. Predictable to author against, but don't document it to players as "the type
  the attack actually hit as".
- **Split a countdown across two checkpoints.** Every trigger is re-evaluated after each script
  runs (determinism rule 3), so a timer that both ticks and resolves at the same checkpoint chases
  itself through all its states in one dispatch and fires instantly. Decrement it at `OnTurnStart`
  and resolve it at `OnTurnEnd` (or any other two distinct checkpoints) so a real turn passes in
  between.
- **An event-reacting trigger must disable itself.** The re-evaluation loop keeps re-selecting any
  trigger whose conditions still hold, and an `Event.*` condition (`Event.MoveType == TYPE_ICE`,
  `Event.Battler == …`) stays true for the whole dispatch — a script can't change the event. So a
  plain trigger that reacts to a move/switch/faint fires again and again until the per-checkpoint
  script cap trips. `Once` is wrong (it means once per *battle*). Instead gate it on a variable it
  sets — `Var(Handled) == 0`, script sets `Handled` to 1 — and clear that variable at `OnTurnEnd`
  (or wherever the next occurrence should be allowed). `OnEnter` does **not** help here: its edge
  check compares against an HP snapshot, which an event condition doesn't move.

---

## `.encounter` authoring map

This is the complete vocabulary `encounterproc` accepts in
`src/data/battle_encounters.encounter`. The processor validates the keywords below, while an
encounter name, script label, and condition value are passed through to the C compiler.

Use `#` for a comment. A comment can occupy a whole line or follow a declaration/condition; the
build strips `#` and everything after it through the end of that line before preprocessing.

```text
Encounter: <EncounterName>

Properties:                 # optional; at most one, before the first Trigger:
    Level: <level or LevelCap>
    CatchRate: <0-255>
    Balls: <Default|Blocked|Allowed>
    DamageReduction: <0-99>
    Immunities: <immunity list>
    CapTypeEffectiveness: <True|False>
    FlatToxicDamage: <True|False>
    Moves: <1-4 MOVE_* constants, comma separated>
    AiFlags: <one or more AI_FLAG_* constants, '|' separated>

Trigger: <Checkpoint>
Priority: <0-255>
Flags: <optional flags>
Script: <BattleScriptLabel>
Conditions:                 # optional; requires at least one indented item
    <operand> <comparison> <value>
```

### `Encounter:`

| Part | Every valid form |
| --- | --- |
| Name | Any C identifier: begins with `A-Z`, `a-z`, or `_`; subsequent characters may also be digits. For example, `Storm_Herald`, which generates `ENCOUNTER_STORM_HERALD`. |
| Contents | At most one `Properties:` block, then one or more `Trigger:` blocks; maximum 32 triggers. |

### `Properties:` fields

Every field is optional and may appear at most once. All but `Moves:`/`AiFlags:` take a closed
vocabulary validated here; those two hold game constants, so — like a condition's right-hand side —
their names pass through to the C compiler and a typo surfaces as a compiler error.

| Field | Every valid value |
| --- | --- |
| `Level:` | `LevelCap`, or a decimal integer `1` through `MAX_LEVEL` |
| `CatchRate:` | Decimal integer `0` through `255` |
| `Balls:` | `Default`, `Blocked`, or `Allowed` |
| `DamageReduction:` | Decimal integer `0` through `99` |
| `Immunities:` | Comma-separated list of `Ohko`, `FixedDamage`, `HpSwap`, `SharedKo`, `All`, or `None` |
| `CapTypeEffectiveness:` | `True` or `False` |
| `FlatToxicDamage:` | `True` or `False` |
| `Moves:` | Comma-separated list of one to four `MOVE_*` constants. The names pass through to the C compiler unchecked; only the count is validated |
| `AiFlags:` | One or more `AI_FLAG_*` constants separated by `\|`. Also passed through unchecked |

### `Trigger:` fields

| Field | Every valid value |
| --- | --- |
| `Trigger:` | `OnBattleStart`, `OnTurnStart`, `OnMoveEnd`, `OnFaint`, `OnSwitchIn`, `OnTurnEnd`, `OnBattleEnd` |
| `Priority:` | Decimal integer `0` through `255` (required). Lower runs first; ties use file order. |
| `Flags:` | Omit it, `Once`, `OnEnter`, or `Once, OnEnter`. No other flags are valid. |
| `Script:` | Any C-identifier battle-script label (required), e.g. `EncScript_StormHerald_Surge`. It must exist and end with `return`. |
| `Conditions:` | Omit for unconditional; otherwise one or more indented items, nested at most four levels. |

### `Conditions:`

A leaf is exactly `<operand> <comparison> <value>`. The only comparisons are `==`, `!=`, `<`,
`<=`, `>`, and `>=`. Siblings are an implicit `All:`. The only explicit group forms are `All:`
(one or more children), `Any:` (one or more children), and `Not:` (exactly one child).

| Operand syntax | Every valid ref/field | Expected value |
| --- | --- | --- |
| `Battler(<ref>).HP` | `<ref>` is `Boss`, `Self`, `PlayerLeft`, `PlayerRight`, `OpponentLeft`, or `OpponentRight` | Current HP integer |
| `Battler(<ref>).HpPercent` | Same six refs | `0` to `100` |
| `Battler(<ref>).MaxHp` | Same six refs | Maximum HP integer |
| `Battler(<ref>).Species` | Same six refs | `SPECIES_*` constant |
| `Battler(<ref>).Ability` | Same six refs | `ABILITY_*` constant |
| `Battler(<ref>).Status` | Same six refs | `STATUS1_*` constant or bitfield expression |
| `Battler(<ref>).Type` | Same six refs | `TYPE_*` constant |
| `Battler(<ref>).Stat(<stat>)` | Same six refs; `<stat>` is `STAT_ATK`, `STAT_DEF`, `STAT_SPEED`, `STAT_SPATK`, `STAT_SPDEF`, `STAT_ACC`, or `STAT_EVASION` | Stat-stage integer |
| `Var(<name>)` | `<name>` is any C identifier | Encounter-local integer; first use declares it (maximum 16 per encounter). |
| `Event.Battler` | No argument | Battler id |
| `Event.Target` | No argument | Battler id |
| `Event.Move` | No argument | `MOVE_*` constant |
| `Event.MoveType` | No argument | `TYPE_*` constant |
| `Event.Cause` | No argument | `ENC_CAUSE_NONE`, `ENC_CAUSE_MOVE_DAMAGE`, `ENC_CAUSE_RECOIL`, `ENC_CAUSE_DRAIN`, `ENC_CAUSE_END_TURN`, `ENC_CAUSE_ITEM`, `ENC_CAUSE_ABILITY`, or `ENC_CAUSE_ENCOUNTER_SCRIPT` |
| `Event.OldValue`, `Event.NewValue` | No argument | Integer |
| `Weather` | No argument | `B_WEATHER_*` constant |
| `Terrain` | No argument | `STATUS_FIELD_*_TERRAIN` constant |
| `Turn` | No argument | Turn-number integer |
| `BattlerCount` | No argument | Usually `2` (singles) or `4` (doubles) |

Event validity is fixed: `Event.Battler` and `Event.Cause` work at `OnMoveEnd`, `OnFaint`,
`OnSwitchIn`, and `OnTurnEnd`; `Event.Target` and `Event.Move` only at `OnMoveEnd`; and
`Event.OldValue`/`Event.NewValue` at `OnMoveEnd` and `OnTurnEnd`.

The `<value>` text is copied into generated C, so it is not a closed encounter-language list: it
can be any valid C integer expression. Use the matching constant family in the table; undefined
constants are compiler errors.

### Encounter script methods

`Script:` uses all normal battle-script commands plus these encounter-specific methods:

| Method | Valid arguments |
| --- | --- |
| `encsetvar <var>, <value>` | Generated `ENC_VAR_<ENCOUNTER>_<NAME>` (or numeric index) and a byte value. |
| `encaddvar <var>, <value>` | Same as `encsetvar`. |
| `encsubvar <var>, <value>` | Same as `encsetvar`. |
| `encjumpifvar <comparison>, <var>, <value>, <label>` | Normal battle-script byte comparison, variable/index, byte value, and script label. |
| `enchangehp <target>, <amount>[, <mode>]` | Target below; signed 16-bit amount (positive heals, negative damages); optional `ENC_AMOUNT_FIXED` (default) or `ENC_AMOUNT_PERCENT`. |
| `encchangestat <target>, <stat>, <stages>` | Target below; `STAT_*` id; signed stage change. |
| `encchangestatvalue <target>, <stat>, <amount>[, <mode>]` | Target below; `STAT_ATK`, `STAT_DEF`, `STAT_SPATK`, `STAT_SPDEF` or `STAT_SPEED` (no battle stat exists behind `STAT_ACC`/`STAT_EVASION`); signed 16-bit amount; optional mode as above. |
| `encsetdamagereduction <target>, <percent>` | Target below; `0` through `ENC_MAX_DAMAGE_REDUCTION` (99). |
| `encsetimmunity <target>, <immunities>` | Target below; `0`, `ENC_IMMUNE_ALL`, or an OR of `ENC_IMMUNE_OHKO`, `ENC_IMMUNE_FIXED_DAMAGE`, `ENC_IMMUNE_HP_SWAP`, `ENC_IMMUNE_SHARED_KO`. |
| `encsetcaptypeeffectiveness <target>, <cap>` | Target below; `TRUE` or `FALSE`. |
| `encsetflattoxicdamage <target>, <flat>` | Target below; `TRUE` or `FALSE`. |
| `encsetballs <policy>` | `ENC_BALLS_DEFAULT`, `ENC_BALLS_BLOCKED`, or `ENC_BALLS_ALLOWED`. |
| `encsetcatchrate <rate>` | `ENC_CATCH_RATE_NONE`, or `1` through `255`. |
| `encmegaevolve <target>, <failLabel>` | A target that resolves to exactly one battler, plus a script label. |

Every valid `<target>` is `ENC_TARGET_BOSS`, `ENC_TARGET_SELF`, `ENC_TARGET_EVENT_TARGET`,
`ENC_TARGET_PLAYER_LEFT`, `ENC_TARGET_PLAYER_RIGHT`, `ENC_TARGET_OPPONENT_LEFT`,
`ENC_TARGET_OPPONENT_RIGHT`, `ENC_TARGET_ALL_FOES`, `ENC_TARGET_ALL_ALLIES`, or
`ENC_TARGET_ALL_BATTLERS`.
