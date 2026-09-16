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

Small demo and test encounters live in `src/data/battle_encounters.encounter`; a legendary gets its
own file, `src/data/legendary_encounters/<pokemon>.encounter`. Every `.encounter` source is parsed
into the same `gEncounters[]`, so where a definition lives is purely an organisational choice; a new
file under `src/data/legendary_encounters/` is picked up automatically.

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

- `Encounter: Legendary_Barrier` names the encounter; `encounterproc` (the tool that compiles this
  file) emits its data as `gEncounters[ENCOUNTER_LEGENDARY_BARRIER]`. The `ENCOUNTER_*` id itself
  lives in `enum EncounterId` (`include/constants/battle_encounter.h`), which is hand-maintained —
  add the id there, before `ENCOUNTER_COUNT`, when you add the encounter.
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

Encounter scripts follow the same split: demo/test scripts sit in `data/battle_scripts_encounters.s`
and each legendary gets `data/legendary_encounters/<pokemon>.inc`, `.include`d from that file.

```asm
EncScript_LegendaryBarrier_PhaseTransition::
	encsetvar 0, 2   @ phase = 2
	trainerslidein BS_OPPONENT1
	printstring STRINGID_ENCLEGENDARYGATHERSSTRENGTH
	waitmessage B_WAIT_TIME_LONG
	trainerslideout BS_OPPONENT1
	encchangestat ENC_TARGET_BOSS, STAT_DEF, 3
	encchangestat ENC_TARGET_BOSS, STAT_SPDEF, 3
	encchangehp ENC_TARGET_BOSS, 20, ENC_AMOUNT_PERCENT
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

Note `encchangehp ..., 20, ENC_AMOUNT_PERCENT` rather than a raw HP number: with `Level: LevelCap`
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

For a static legendary standing on the map, two more pieces make the object behave correctly:

- Give the object event a real hide flag in the map's `map.json` (not `"0"`), and run
  `updatelegendaryvisibility SPECIES_X, FLAG_HIDE_X` from the map's `MAP_SCRIPT_ON_TRANSITION`.
  That command sets the flag while the player owns one of that species and clears it otherwise, so
  a caught legendary stays gone and a released one reappears. It also leaves `VAR_RESULT` set to
  whether the player owns one, for any follow-up branching the map needs.
- End the trigger script with `goto LegendaryEncounter_EventScript_FinishBattle`
  (`data/scripts/legendary_encounters.inc`), which removes the object only on `B_OUTCOME_CAUGHT`.
  Fainting or fleeing leaves it in place so the fight can be retried.

`checkspeciesowned SPECIES_X` is the same ownership test on its own, for encounters spawned by
`addobject` rather than by a hide flag. Both scan the party and every PC box, and treat alternate
forms as the same species.

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
| `Battler(<ref>).Protected` | `1` if the battler protected at any point this turn, else `0` | a whole-turn latch, not the live flag — see the [gotcha](#known-gotchas) |
| `Var(<name>)` | An author-defined variable | first reference declares it; scoped to the encounter |
| `Event.Battler` | The battler the event is *about* | compare against a `B_POSITION_*` id (below); see checkpoint table for validity |
| `Event.Target` | The other battler involved | " |
| `Event.Move` | The move involved | " |
| `Event.MoveType` | The type of the move involved | compare against a `TYPE_*` constant; `OnMoveEnd` only |
| `Event.MoveCategory` | The category of the move involved | compare against `DAMAGE_CATEGORY_PHYSICAL`, `DAMAGE_CATEGORY_SPECIAL` or `DAMAGE_CATEGORY_STATUS`; `OnMoveEnd` only |
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
| `Survive:` | `True`/`False` | The boss's HP can't be taken below 1 by anything that goes through the damage formula or a passive HP tick. For scripted last stands and to guarantee a catch window opens. Does **not** cover fixed-damage moves, Perish Song or Destiny Bond — see the gotcha below |
| `Ability:` | an `ABILITY_*` constant | The ability the boss fights with, replacing the one its ability slot would give it. Not restricted to the species' own abilities |
| `Moves:` | 1–4 `MOVE_*` constants, comma separated | Replaces the boss's moves, PP included. A slot the list doesn't reach keeps what it was built with |
| `AiFlags:` | one or more `AI_FLAG_*` constants, `\|` separated | The AI the opponent side runs, replacing whatever the battle type would derive |

`DamageReduction:`, `Immunities:`, `CapTypeEffectiveness:`, `FlatToxicDamage:`, `Survive:`,
`Ability:` and `Moves:` apply to the **boss** (the opponent's left slot). Any other battler — or any
change to these mid-battle — is a job for `encsetdamagereduction` / `encsetimmunity` /
`encsetcaptypeeffectiveness` / `encsetflattoxicdamage` / `encsetsurvive` in a script.

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

### `Ability:` — a boss ability, on or off the species' list

```text
    Ability: ABILITY_SNOW_WARNING
```

Applied in the same pre-battle window as `Level:` and `Moves:`, to the boss and nobody else. When
the ability is one the species actually has, all this does is pick that ability slot on the party
mon — the battler, the AI and the summary screen derive the rest as they always would.

An ability the species **doesn't** have works too, and is the interesting case for a boss. A party
Pokémon can only store an ability *slot*, so there is nowhere on it to put one; the engine instead
writes the ability onto the battler as it is built, exactly the way Skill Swap and Worry Seed do
mid-battle. Two things follow from that:

* It is applied before switch-in abilities activate, so an Intimidate or a Drought granted this way
  still fires on entry, and the AI's opening read of the boss sees the real ability.
* Outside the battle the mon is unchanged. A boss caught with an off-list ability keeps the ability
  its slot names, not the one it fought with.

The ability name passes through to the C compiler unchecked, like `Moves:` — a typo is a compiler
error, not an `encounterproc` error.

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
- **`encchangehp` in an encounter script.** The number in the script is the encounter's own; another
  encounter rule rescaling it would make phase scripts impossible to reason about.

The maximum is 99 (`ENC_MAX_DAMAGE_REDUCTION`). A battler nothing at all can damage isn't a fight —
if a specific move must not work, that's what the immunities are for.

### `DamageReduction:` — and what the boss *drains*

`DamageReduction:` cuts what reaches the boss. Nothing cuts what the boss **deals** — and a drain
move turns that undiminished damage straight into healing, which is how a reduction that reads as
"the fight lasts longer" quietly becomes "the fight cannot be won":

> At `DamageReduction: 90` the exchange runs ten to one against the player. Leech Seed alone takes
> 1/8 of their max HP every turn and hands **all** of it to a boss they can only chip one or two
> percent off. Nothing on screen says anything is wrong.

So healing the boss drains **out of another battler** is scaled by the encounter's `DamageReduction:`
automatically — it keeps the same fraction of what it drains that it lets through of what it takes.
This is not a property and there is nothing to opt into; it falls out of the number already authored.

| Scaled | Not scaled |
| --- | --- |
| Leech Seed's per-turn drain | **The damage** — the player still loses the full 1/8 a turn; only the boss's share of it shrinks |
| Absorb, Giga Drain, Drain Punch, Draining Kiss, Horn Leech, Leech Life, Dream Eater, … | Self-healing: Synthesis, Recover, Ingrain, Aqua Ring, Leftovers — a fraction of the boss's *own* max HP, already balanced against the fight's length |
| Strength Sap | A Liquid Ooze punish — that's damage, not healing, and `ApplyEncounterDamageReduction` has already scaled it once |
| The AI's estimate of what a drain is worth, so it doesn't spend turns on a heal it will barely keep | Any battler that isn't the boss |
| Grassy Terrain's end-turn heal on the boss — free every turn, so an unscaled 1/16 out-heals what gets through the guard | |

It reads the **authored property**, not the live per-battler value, and that distinction is the whole
point of the feature. The live number moves with a phase, a stance or a form, and the catch-window
guard pins it to `ENC_MAX_DAMAGE_REDUCTION` — balance would swing with all of that, and the catch
window would zero out drain healing outright. The property is the fight's fixed balance constant, so
this is too. A boss whose `Properties:` set no reduction is unaffected.

`ApplyEncounterDrainReduction` and `ApplyEncounterTerrainHealReduction` (`battle_encounter.c`) are
the implementation, alongside `ApplyEncounterDamageReduction` and `ApplyEncounterTypeAdaptation`.


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

### `Survive:` — a guaranteed last stand

`Survive: True` clamps every incoming hit so the boss's HP lands at exactly 1 rather than 0. The
clamp sits in `ApplyEncounterDamageReduction` (`battle_encounter.c`), the single choke point every
reduced damage source already passes through — move damage and every passive HP tick (weather, Toxic,
recoil, confusion self-hits). At 1 HP a further hit is clamped to 0 damage, the same value False
Swipe produces there, so nothing crashes and the health bar doesn't desync.

Use it for a scripted "refuses to fall" beat, a form change at death's door, or simply to guarantee
the player reaches an HP-threshold catch window instead of losing it to one oversized overkill hit.
A script clears it with `encsetsurvive <target>, FALSE` — typically in the weakened/catch-window
script, after which the automatic catch-window damage guard keeps the catch target alive.

**Gotcha — it does not cover everything.** Fixed-damage moves (Seismic Toss, Super Fang, Night
Shade), Perish Song and Destiny Bond bypass the damage formula entirely and are not clamped. Every
shipped legendary already shuts these out with `Immunities: Ohko, FixedDamage, HpSwap, SharedKo`, so
the hole is closed in practice — but an encounter that sets `Survive:` **without** the matching
`Immunities:` can still have its boss killed outright. The failure mode is graceful (the boss faints,
the battle ends as a normal win, the scripted sequence just doesn't play).

**Gotcha — the AI shares this path.** A boss the AI cannot KO through the reduced-damage path reads
to it as one it can never KO when it scores moves. This is the same deliberate trade `DamageReduction:`
already makes.

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
| `enccopyvar <dst>, <src>` | Copies author variable `<src>` into `<dst>` — for consuming a counter in a loop without losing it | wraps `copybyte` |
| `encchangehp <target>, <amount>[, <mode>]` | Heals (positive) or damages (negative) every battler `<target>` resolves to, with the normal animated health bar. A battler taken to 0 HP faints in place (and ends the battle if that empties a side), like status/weather chip damage | encounter-specific (`callnative`) |
| `encchangestat <target>, <stat>, <stages>` | Adds `<stages>` to `<stat>` for every battler `<target>` resolves to — silent, no message/animation | encounter-specific (`callnative`) |
| `encchangestatvalue <target>, <stat>, <amount>[, <mode>]` | Moves the **raw battle stat** (not the stage) of `<stat>` — silent, and not undone by Haze or switching out | encounter-specific (`callnative`) |
| `encsetdamagereduction <target>, <percent>` | Sets how much less damage `<target>` takes, `0`–`99`. Replaces the current value | encounter-specific (`callnative`) |
| `encsetimmunity <target>, <immunities>` | Replaces `<target>`'s `ENC_IMMUNE_*` mask (`0` clears it) | encounter-specific (`callnative`) |
| `encsetcaptypeeffectiveness <target>, <cap>` | `TRUE` clamps `<target>`'s incoming type effectiveness at 2x; `FALSE` removes the clamp | encounter-specific (`callnative`) |
| `encsetflattoxicdamage <target>, <flat>` | `TRUE` stops Toxic's counter from ramping `<target>`'s damage up each turn; `FALSE` restores the ramp | encounter-specific (`callnative`) |
| `encsetsurvive <target>, <survive>` | `TRUE` guards `<target>`'s HP against dropping below 1 through the damage formula or a passive tick; `FALSE` removes the guard | encounter-specific (`callnative`) |
| `encsetprotect <target>` | Gives `<target>` the same protection Protect itself grants, so Feint, never-miss moves and contact punishment resolve exactly as they do against a real Protect. `gProtectStructs` is cleared after end-of-turn effects, so this only has an effect at `OnTurnStart` — set there it covers the whole turn and expires on its own | encounter-specific (`callnative`) |
| `encsetcrit <target>, <state>` | `TRUE` sets the same Focus Energy volatile Focus Energy itself sets on every battler `<target>` resolves to (raising the critical-hit ratio); `FALSE` clears it again. The reversibility is the point — a meter that grants a crit boost has to be able to take it back. `Cmd_setfocusenergy` can't serve: it reads `gCurrentMove` to tell Focus Energy from Dragon Cheer and has no way to clear the bit. Silent; supply your own dialogue | encounter-specific (`callnative`) |
| `encsettrapped <target>, <trapped>` | `TRUE` sets the same escape-prevention volatile Mean Look sets on `<target>`, with the boss as the trapper, so switching and fleeing are both refused; `FALSE` clears it. Every existing rule applies unchanged — Ghost-types walk out for free under `B_GHOSTS_ESCAPE >= GEN_6`, Baton Pass passes it on, the AI's switch scoring reads it, and the boss fainting releases everything it held. Unlike `encsetprotect` it is **not** wiped at end of turn: it persists until a script clears it. The boss itself is always skipped. Silent; supply your own dialogue | encounter-specific (`callnative`) |
| `encsetembargo <target>, <turns>` | Shorts out the held item of every battler `<target>` resolves to for `<turns>` turns, by setting the same volatile Embargo itself sets — so every existing item check behaves unchanged, and the engine's own end-turn handler ticks the timer and prints the expiry line. `<turns>` of `0` clears it. The boss is always skipped, the same way `encsettrapped` skips it. Silent | encounter-specific (`callnative`) |
| `encdisablemove <target>, <turns>, <failLabel>` | Disables the move `<target>` (must resolve to exactly one battler) used last, for `<turns>` turns, writing the same volatiles Disable does — so the engine already ticks the timer, already drops the lock if the move leaves the moveset, and already prints its own "no longer disabled" line. The move's name is buffered into `{B_BUFF1}`. Jumps `<failLabel>` when there is nothing to take — no move used yet (turn 1), the move gone from the moveset or out of PP, something already disabled, or the target absent — all ordinary battle states, so they branch rather than assert. `Cmd_disablelastusedattack` can't serve here: it reads `gBattlerTarget`, which is stale outside a move, and derives its duration from `B_DISABLE_TURNS`. Silent | encounter-specific (`callnative`) |
| `encclearscreens <target>, <failLabel>` | Strips every screen, Safeguard, Mist, Tailwind and Lucky Chant (and their timers) from each side `<target>` resolves to, and jumps `<failLabel>` if there were none — so one command is both the test and the clear. Silent; supply your own dialogue. `trydefog` can't do this outside a move: it only ever clears the side opposite `gBattlerAttacker`, which is stale at `OnTurnStart`/`OnTurnEnd` | encounter-specific (`callnative`) |
| `encclearhazards <target>, <failLabel>` | Strips every entry hazard (Spikes, Toxic Spikes, Stealth Rock, Sticky Web, Steelsurge) from each side `<target>` resolves to, and jumps `<failLabel>` if there were none — so one command is both the test and the clear, exactly like `encclearscreens`. Unlike Defog's path it takes the side bare in one call and prints nothing, so the caller supplies one line for the whole scouring rather than one per hazard type. Nothing else could remove hazards from a checkpoint script | encounter-specific (`callnative`) |
| `encresetstats <target>, <failLabel>` | Resets every stat stage back to neutral on every battler `<target>` resolves to — the targeted form of Haze — and jumps `<failLabel>` when no battler in the set had a stage to strip, so one command is both the test and the clear, exactly like `encclearscreens`. `normalisebuffs` is the only other route to this and it Hazes every battler on the field including the boss's own setup, which a boss that wants to strip only the player is unable to use. Silent; supply your own dialogue | encounter-specific (`callnative`) |
| `encsetsidestatus <target>, <status>, <turns>` | Raises one side-wide status (an `ENC_SIDE_*` selector) on every side `<target>` resolves to; `<turns>` of `0` (the default) is permanent. The single-status counterpart to `encclearscreens`, and the only way to raise a Pledge status (Rainbow, Sea of Fire, Swamp) outside a Pledge combo. Silent | encounter-specific (`callnative`) |
| `encclearsidestatus <target>, <status>` | Drops one side-wide status, timer included, from every side `<target>` resolves to. The precise inverse of the above - the only way to remove a Pledge status, and how you strip one screen without the rest. Silent, and a no-op if it was not up | encounter-specific (`callnative`) |
| `encrevive <target>, <percent>` | Revives the first fainted **bench** party member on every side `<target>` resolves to, at `<percent>` of its max HP, and clears its status. A battler awaiting replacement is skipped rather than pulled back mid-faint. Silent, and a no-op when a side has nobody to revive, so it is safe to call unconditionally | encounter-specific (`callnative`) |
| `encsetballs <policy>` | `ENC_BALLS_DEFAULT` / `ENC_BALLS_BLOCKED` / `ENC_BALLS_ALLOWED` | encounter-specific (`callnative`) |
| `encsetcatchrate <rate>` | Replaces the catch rate for this battle; `ENC_CATCH_RATE_NONE` restores the species' own | encounter-specific (`callnative`) |
| `encsnapshothp <target>, <var>[, <mode>[, <failLabel>]]` | Records `<target>`'s current HP as a **percentage of its max HP** (0-100) into author variable `<var>`. `<target>` must resolve to exactly one battler. `<mode>` is `ENC_SNAP_SET` (default, overwrite), `ENC_SNAP_LOWEST` (write only if lower than what `<var>` holds), `ENC_SNAP_RECOVERY` (write `max(0, currentPct - <var>)` — recovery since the mark) or `ENC_SNAP_DAMAGE` (write `max(0, <var> - currentPct)` — damage taken since the mark). Jumps `<failLabel>` when nothing was written, so one command is both the test and the record | encounter-specific (`callnative`) |
| `encrewindhp <target>, <var>` | Moves `<target>`'s HP **to** the percentage held in `<var>`, healing or damaging as needed, with the same animated health bar `encchangehp` uses. The counterpart to `encsnapshothp` | encounter-specific (`callnative`) |
| `encowned <species>, <var>` | Writes how well the player knows `<species>` into `<var>`: `2` caught in the Pokédex, `1` only seen, `0` never met. Conditions read battle state only, so this is the only route from "what has the player done outside this battle" into an encounter script — a counterpart legendary, a fusion partner, a rival form. Two Pokédex bit tests, so it is cheap enough for any checkpoint; alternate forms share a dex slot and so count as the same species. Deliberately *not* `CheckPlayerOwnsSpecies`, which walks the party and every PC box decrypting each slot | encounter-specific (`callnative`) |
| `encstoreprediction <target>, <var>` | Writes the AI's predicted move **category** for `<target>` into `<var>`: `0` no prediction this turn, `1` physical, `2` special, `3` status. The real `AI_FLAG_PREDICT_MOVE` answer, so it only means anything at `OnTurnStart` and only when the encounter's `AiFlags:` include that flag | encounter-specific (`callnative`) |
| `enccomparestat <targetA>, <targetB>, <stat>, <var>` | Writes the result of comparing `<targetA>`'s live `<stat>` against `<targetB>`'s into `<var>`: `0` A is lower, `1` equal, `2` A is higher. Both targets must resolve to exactly one battler. `STAT_SPEED` reads the full turn-order speed (Tailwind, Choice Scarf, paralysis, stages); the other stats read the stat-with-stages value. The var-to-var comparison conditions cannot express | encounter-specific (`callnative`) |
| `encadapt <target>, <percent>, <slots>, <countVar>, <resultVar>` | **ANALYSIS.** Files the type of the move that just landed as a **type-keyed** damage resistance on `<target>`, at `<percent>` (capped at `ENC_MAX_ADAPT_PERCENT`), in a FIFO `<slots>` deep (capped at `ENC_MAX_ADAPTATIONS`). Re-filing a type already held raises its percent instead of taking a second slot; a full board pushes the **oldest** out; a `<slots>` narrower than the board drops the overflow first. `<countVar>` takes the resulting board size — the authoritative one, so a script mirroring it can't drift — and `<resultVar>` an `ENC_ADAPT_RESULT_*` outcome (`HARDENED` / `FILED` / `EVICTED`) so one command drives all three lines of dialogue. The new type is buffered into `{B_BUFF1}` and any evicted type into `{B_BUFF2}`. `OnMoveEnd` only, since it reads the event's move; a move that missed, was blocked or had no effect is ignored outright and leaves both vars untouched. `DamageReduction:` is flat and type-blind — this is the only per-type resistance in the engine | encounter-specific (`callnative`) |
| `encpurgeadapt <target>, <which>, <countVar>` | The inverse of `encadapt`: drops `ENC_ADAPT_OLDEST`, `ENC_ADAPT_NEWEST` or `ENC_ADAPT_ALL` from `<target>`'s board and writes the remaining count into `<countVar>`. The array is compacted on every removal, so slot 0 is always the oldest. The dropped type is buffered into `{B_BUFF1}`. Silent, and a no-op on an empty board, so it is safe to call unconditionally — test `<countVar>` afterwards to know whether anything fell | encounter-specific (`callnative`) |
| `encsetweather <weather>[, <turns>]` | Sets the battle weather to a `BATTLE_WEATHER_*` value. `<turns>` defaults to `0`, meaning permanent. Silent; clear it again with the existing `removeweather` | encounter-specific (`callnative`) |
| `encsetterrain <terrain>[, <turns>]` | Sets the field terrain to an `ENC_TERRAIN_*` selector. `<turns>` defaults to `0`, meaning permanent. The mirror of `encsetweather` one field over, and needed for the same reason: the stock `setterrain` opcode reads its terrain type off `gCurrentMove`, and a checkpoint script has no current move. Does nothing if that terrain is already up, or in a sky battle. Silent; clear it again with the existing `removeterrain` | encounter-specific (`callnative`) |
| `encsetfieldstatus <status>[, <turns>]` | Raises one field-wide status (an `ENC_FIELD_*` selector: `ENC_FIELD_TRICK_ROOM`, `ENC_FIELD_GRAVITY`, `ENC_FIELD_WONDER_ROOM`, `ENC_FIELD_MAGIC_ROOM`, `ENC_FIELD_FAIRY_LOCK`, `ENC_FIELD_MUD_SPORT`, `ENC_FIELD_WATER_SPORT`). `<turns>` defaults to `0`, meaning permanent. Field statuses are otherwise unreachable from a checkpoint script - the stock opcode for each one reads its effect off `gCurrentMove`, which a checkpoint script doesn't have. Terrain is deliberately excluded - `encsetterrain` owns it and runs `TryChangeBattleTerrain`, which does bookkeeping a raw flag write would skip. `encsetfieldstatus ENC_FIELD_GRAVITY` matches `Cmd_setgravity` exactly (flag plus timer, nothing else). Silent | encounter-specific (`callnative`) |
| `encclearfieldstatus <status>` | Drops one field-wide status, timer included. The precise inverse of `encsetfieldstatus`. Silent, and a no-op if it wasn't up | encounter-specific (`callnative`) |
| `encsethealblock <target>, <turns>` | Sets the same volatile Heal Block itself sets on every battler `<target>` resolves to, for `<turns>` turns; `0` clears it. `HandleEndTurnHealBlock` already ticks the timer and prints the engine's own expiry line. `healBlockTimer` is a bitfield sized by `B_HEAL_BLOCK_TIMER` - like `encsetembargo`'s `B_EMBARGO_TIMER` ceiling, a `<turns>` above it asserts rather than truncating silently. The boss is always skipped, the same way `encsettrapped`/`encsetembargo` skip it. Silent | encounter-specific (`callnative`) |
| `encmegaevolve <target>, <failLabel>` | Forces `<target>` (must resolve to exactly one battler) to Mega Evolve outside the normal gimmick-selection flow, with the stock Mega Evolution presentation | encounter-specific (`callnative`) |
| `encformchange <target>, <species>, <failLabel>[, <anim>]` | Changes `<target>` (must resolve to exactly one battler) into `<species>` outright, outside any form-change table, then plays `<anim>`. The general form of `encmegaevolve`: repeatable, reversible, and not limited to a form the battler holds a stone for. Keeps HP and the moveset; stats, types and ability come from the new species. Prints nothing — supply your own dialogue | encounter-specific (`callnative`) |
| `encsetmove <target>, <slot>, <move>` | Writes `<move>` into slot `<slot>` (`0`–`3`) of `<target>`'s **battle** mon with that move's full PP. The party Pokémon is never touched, so a boss caught afterwards keeps the moveset it was built with. `Moves:` is a battle-start property, so this is the only way a boss gains a move partway through a fight — the signature move a form unlocks when it transforms. Prints nothing | encounter-specific (`callnative`) |
| `enctransform <target>, <source>, <failLabel>` | Turns `<target>` (exactly one battler) into `<source>` (likewise), copying species, stats, stat stages, types, ability and moveset — exactly as the Transform move does. Copies **not** HP, level, item or status, and never writes the party Pokémon, so a boss transformed this way is still its own species if caught. Jumps `<failLabel>` if `<source>` is semi-invulnerable, already transformed, or behind Illusion. Prints nothing | encounter-specific (`callnative`) |
| `encuntransform <target>` | Reverts `<target>` from an `enctransform`/Transform copy to its own party species, rebuilding stats, types, ability and moves from the party Pokémon. Stat stages reset to neutral. Silent no-op if `<target>` isn't transformed, so it's safe to call unconditionally. Prints nothing | encounter-specific (`callnative`) |
| `encjumpifchance <percent>, <label>` | Branches to `<label>` with `<percent>` (`0`–`100`) probability, otherwise falls through. The roll is tagged `RNG_ENCOUNTER_SCRIPT` | encounter-specific (`callnative`) |
| `encswitchout <target>, <failLabel>` | Drags `<target>` (must resolve to exactly one battler, never the boss) out for a random healthy bench member, with the stock drag-out presentation - Roar addressed by encounter target instead of `gBattlerTarget`, which is stale outside a move. Jumps `<failLabel>` when there is nobody to send in (a player down to their last Pokémon, or the boss named as the target), so it branches rather than asserts. **`OnTurnEnd` only**: the replacement is marked as having already acted, so firing it any earlier would take a battler's turn away without saying so | encounter-specific (`callnative`) |
| `encsetrecharge <target>, <turns>` | Puts `<target>` on the engine's Hyper Beam recharge timer, so it loses an action and prints the stock "must recharge!" line when its move tries to resolve. The timer is decremented at the very end of the turn, *after* the `OnTurnEnd` checkpoint, so `<turns>` means different things depending on where you set it: `1` at `OnTurnStart` costs the battler **this** turn's action, `2` at `OnTurnEnd`/`OnMoveEnd` costs it **next** turn's. For an overexerted boss, a staggered beat, or any telegraphed free turn | encounter-specific (`callnative`) |
| `encaskyesno <stringId>, <yesLabel>` | Prints `<stringId>`, opens the standard YES/NO window, and jumps `<yesLabel>` on YES; falls through on NO (and on B, which the box reads as NO). The player-decision primitive: a bargain, a bet, a mercy, a branching narrative beat. The leading `printstring` is what makes it safe at `OnTurnStart` — it resets `gBattle_BG0_X/Y`, scrolling the still-open action menu away before the window is drawn. A decided battle (`gBattleOutcome != 0`) skips the prompt and takes the NO path, so scripted damage earlier in the same script can't leave the player answering a question about a fight that is already over. Single-player only — a link battle would desync on the input | a macro over `printstring`/`setbyte`/`yesnobox`/`jumpifbyte` |

### Fixed vs. percentage amounts

`encchangehp` and `encchangestatvalue` take an optional trailing `<mode>`:

| Mode | Reads `<amount>` as |
| --- | --- |
| `ENC_AMOUNT_FIXED` (the default when omitted) | Raw HP / raw stat points |
| `ENC_AMOUNT_PERCENT` | A percentage of that battler's own max HP / current value for that stat |
| `ENC_AMOUNT_TO_PERCENT` | A **destination**: the percentage of max HP to move the battler to. `encchangehp`'s modes are deltas; this one is an absolute target, and is what `encrewindhp` uses |

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

`encsetdamagereduction`, `encsetimmunity`, `encsetcaptypeeffectiveness`, `encsetflattoxicdamage`, `encsetsurvive`, `encsetballs` and `encsetcatchrate` all **replace** the
current value rather than adding to it. That's what lets a script raise a boss's guard for one phase
and drop it in the next without tracking what it added — `encsetdamagereduction ENC_TARGET_BOSS, 0`
always means "no reduction", whatever the `Properties:` block or an earlier phase set.

`encsetvar`/`encaddvar`/`encsubvar`/`encjumpifvar`/`enccopyvar` are thin wrappers: they're the same
`setbyte`/`addbyte`/`subbyte`/`jumpifbyte`/`copybyte` opcodes every other battle script uses, just pre-addressed
into the encounter's variable array so you write a var index instead of a raw address.

`<target>` on `encchangehp`/`encchangestat`/`encmegaevolve`/`encformchange`/`encsetmove`/`enctransform`/`encuntransform` is an `EncounterTarget`: `ENC_TARGET_BOSS`,
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
6. At most 8 scripts run per checkpoint (see [Limits](#limits)) — after that the dispatcher stops,
   even if more triggers are still eligible.
7. The "previous state" `OnEnter` compares against is always "as of the last checkpoint", not
   "as of the start of this checkpoint's re-evaluation loop" — a script mid-checkpoint changing HP
   doesn't retroactively change what an `OnEnter` trigger earlier in the same pass saw.

---

## Limits

| Limit | Value | What happens if you exceed it |
| --- | --- | --- |
| Scripts per checkpoint | 8 | Dispatch stops for that checkpoint; an `assertf` fires ("runaway trigger chain?") |
| Triggers per encounter | 32 | The whole encounter fails to load (`assertf`); no triggers dispatch |
| Author variables per encounter | 16 | `encounterproc` refuses to compile — "too many named variables" |
| Condition nesting depth (`All:`/`Any:`/`Not:`) | 4 | The offending subtree asserts and evaluates `FALSE`; `encounterproc` also flags it at compile time |
| Battle script call depth (`call`/`return`) | 8 | Shared with the rest of the engine, not encounter-specific — `assertf` on overflow |

All of these fail loud (an `assertf`) rather than silently misbehaving — see
[Debugging](#debugging).

**The script budget is a hang guard, not a resource limit.** Scripts are dispatched sequentially,
never nested, so however many run at one checkpoint the battle-script call stack stays one frame
deep. Its only job is to turn "a trigger with no self-disabling condition re-selects forever and
the game hangs" into a loud assert. It counts only checkpoints where a trigger was actually
eligible, so a checkpoint that legitimately runs all 8 is not tripped by the dispatcher's trailing
discovery pass. Eight message boxes at one checkpoint is already more than a player can read —
treat the limit as a design smoke alarm rather than something to budget against.

---

## Debugging

Every encounter assertion identifies **encounter, trigger/context, and reason**, e.g.:

```text
encounter 3: trigger 2: condition nesting deeper than 4
encounter 3: 8 scripts ran at checkpoint 2 - runaway trigger chain?
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
- **An encounter's opponent side never uses the automatic gimmick flow.** `CanActivateGimmick`
  (`battle_gimmick.c`) refuses every gimmick for a non-player battler while an encounter is active,
  so a boss can't Dynamax, Terastallize, Mega Evolve or fire a Z-Move off its own AI. Without that,
  a **wild** boss reads as opted in: the sentinels the AI checks for "this mon isn't meant to use a
  gimmick" (`BLOCK_AI_DYNAMAX`, `TYPE_MYSTERY` — see `ShouldTrainerBattlerUseGimmick`) are only
  written when a party is built from *trainer* data, so a wild legendary carries a real Dynamax
  level and a real Tera type. A Dynamax also doubles its HP mid-fight, which walks straight through
  an encounter's HP-threshold phases. Writing the sentinels onto the party mon instead isn't an
  option — the boss is a wild Pokémon the player can catch, and `MON_DATA_TERA_TYPE` persists on the
  caught mon. Scripted gimmicks are unaffected: `encmegaevolve` and `encformchange` never consult
  this, which is how a boss transforms on cue.
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
- **A boss with a drain move heals through its own guard, and the engine now scales that.** Healing
  the boss takes out of another battler (Leech Seed, the Absorb family, Strength Sap) is cut by the
  encounter's authored `DamageReduction:`, because nothing reduces what the boss *deals* and a drain
  would otherwise convert a full-strength hit into full-strength healing against a player whose own
  hits are cut by 90%. The drain's **damage** is untouched — only the boss's share of it. Self-heals
  are untouched too. See [`DamageReduction:` and what the boss drains](#damagereduction--and-what-the-boss-drains).
- **`DamageReduction:` doesn't stop Toxic from eventually overwhelming a boss.** The counter ramp is
  a multiplier on top of reduction, not something reduction caps — a long enough fight always reaches
  the turn where 90% off a maxed-out counter is still lethal. `FlatToxicDamage: True` is what actually
  bounds it.
- **`Immunities: FixedDamage` is not optional if you set `DamageReduction:`.** Super Fang and
  friends compute their damage from the target's HP, never from the damage formula — a 70%-reduced
  boss with no `FixedDamage` immunity still loses half its health to one Super Fang.
- **`Survive:` has the same blind spot as `DamageReduction:`.** It clamps everything that goes
  through the damage formula or a passive tick, but fixed-damage moves, Perish Song and Destiny Bond
  bypass that path and can still kill the boss outright. Pair `Survive:` with
  `Immunities: Ohko, FixedDamage, HpSwap, SharedKo` — the same set every legendary already sets — or
  accept that the scripted sequence won't play if the player finds the hole. The AI also reads a
  `Survive:` boss as one it can never KO, exactly like a heavily-reduced one.
- **`enctransform` copies the battler, not the Pokémon.** It writes species, stats, stat stages,
  types, ability and moveset onto the target's *battle* data — HP, level, item and status stay the
  target's own, and the party Pokémon is never touched. So a boss the player transforms into their
  own mon still catches as the boss's species, and its HP bar keeps ticking down from where it was.
  Pair it with `Survive: True` if the copied moveset could include a self-KO move (Explosion, Final
  Gambit). `encuntransform` puts it back and resets stat stages to neutral; it's a no-op on an
  untransformed battler, so call it unconditionally in a revert script.
- **`Battler(<ref>).Protected` is a whole-turn latch, not the live flag.** `TurnValuesCleanUp`
  clears `gProtectStructs[].protected` *before* end-of-turn effects run, so by the time `OnTurnEnd`
  dispatches the live flag is already gone — reading it there would always be false. The operand
  instead reads a bitmask set by `Cmd_setprotectlike` (every Protect-like move) and by
  `encsetprotect`, and cleared with the engine's own full protect reset, after the `OnTurnEnd`
  dispatch. So it answers "did this battler protect at any point this turn", which is the same
  statement as "is protected" — Protect is a whole-turn shield. It is what a delayed effect
  resolving at `OnTurnEnd` (a fused trap, a charged blast) has to test against.
- **A blank event-context cell is invalid, not zero.** Reading `Event.Move` at a checkpoint that
  doesn't populate it (see the [checkpoint table](#checkpoint-reference)) asserts rather than
  quietly returning 0 — that would otherwise be indistinguishable from a real "no move" case.
- **`Event.MoveType` is the move's base type.** It reads the type from move data, so a move
  re-typed at runtime — Normalize and the `-ate` abilities, Electrify, Tera — still matches its
  printed type here. Predictable to author against, but don't document it to players as "the type
  the attack actually hit as". `Event.MoveCategory` is the same deal one field over — it's the
  category on the move's data, not a runtime flip like Photon Geyser's or Tera Blast's.
- **An `OnTurnStart` script that animates must lead with `flushtextbox`.** The action/move selection
  menu is shown by *scrolling BG0* (`gBattle_BG0_Y`), and the only thing that scrolls it back is a
  `printstring` (`BtlController_HandlePrintString` resets `gBattle_BG0_X/Y`). `OnTurnStart` is
  dispatched before the turn's first action, so that scroll is still in place — an animation played
  before any dialogue renders on top of the still-open menu and corrupts it. Leading with dialogue
  is enough on its own; `flushtextbox` is the no-dialogue version, and it's how vanilla's own
  turn-start scripts (`BattleScript_QuickClawActivation`) open. `encformchange` already does this
  for you. Every other checkpoint runs after the turn's first message, so this is `OnTurnStart` only.
- **A percentage is the only portable way to remember a battler's HP.** An author variable is a
  `u8`, and the boss's max HP isn't known when the script is written (`Level: LevelCap`, New Game
  Plus offsets), so `encsnapshothp`/`encrewindhp` work in percent throughout. The percentage is
  floored at 1 for a living battler, and `encrewindhp` clamps at max HP - a variable holding more
  than 100 is the usual "nothing recorded yet" sentinel for `ENC_SNAP_LOWEST`, which only writes a
  value **lower** than what the variable already holds.
- **Conditions and `encjumpifvar` only ever compare a variable against a literal.** There is no
  var-to-var comparison opcode. When a design wants one, express it as a command that leaves the
  answer in a variable a literal test can read - `ENC_SNAP_LOWEST` is "keep the smaller of two
  values", `ENC_SNAP_RECOVERY` / `ENC_SNAP_DAMAGE` are "subtract one from the other" in either
  direction, and `enccomparestat` is "which of these two battlers has the bigger stat" - or as
  several triggers whose
  conditions each pin one literal case, with a catch-all behind them at a lower priority.
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
- **`encsethealblock`'s `<turns>` has an undocumented ceiling, like `encsetembargo`'s.**
  `healBlockTimer` is a bitfield sized by `B_HEAL_BLOCK_TIMER` (5) — a value above it truncates
  silently rather than sticking, so the command asserts instead. A mechanic that needs Heal Block to
  outlast that ceiling has to re-arm it periodically (e.g. every turn from `OnTurnEnd`) rather than
  set one long duration; the tick always runs before the re-arm, so the timer never actually reaches
  zero and the engine's own expiry line never fires.
- **A script that must run unconditionally once per turn needs a two-trigger scaffold.** The
  re-evaluation loop can't express "run every turn regardless of state" — a trigger with no
  self-disabling condition fires until the script cap trips. Use a guard var: an `OnTurnStart`
  trigger conditioned only on `Var(TurnGuard) == 0` that sets it to 1 (unconditional-once-per-turn),
  paired with an `OnTurnEnd` trigger conditioned only on `Var(TurnGuard) == 1` that clears it
  (self-disabling too). Two triggers buy an arbitrarily complex once-per-turn script — clearing
  other re-entry guards, ticking a passive counter, shedding a status.
- **`Event.OldValue`/`Event.NewValue` are stale after a move that missed or had no effect.** They
  are only written at the HP-commit point, so after a miss or an immunity they still hold the
  previous change's values — they can't be used to prove a hit actually landed. A trigger keyed on
  `Event.Battler` at `OnMoveEnd` still fires for a move that whiffed.

---

## `.encounter` authoring map

This is the complete vocabulary `encounterproc` accepts in any `.encounter` source. The processor
validates the keywords below, while an encounter name, script label, and condition value are passed
through to the C compiler.

Use `#` for a comment. A comment can occupy a whole line or follow a declaration/condition;
`encounterproc` ignores `#` and everything after it through the end of that line.

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
    Survive: <True|False>
    Ability: <an ABILITY_* constant>
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
| Name | Any C identifier: begins with `A-Z`, `a-z`, or `_`; subsequent characters may also be digits. For example, `Storm_Herald`, whose data is emitted as `gEncounters[ENCOUNTER_STORM_HERALD]` (the id is hand-added to `enum EncounterId`). |
| Contents | At most one `Properties:` block, then one or more `Trigger:` blocks; maximum 32 triggers. |

### `Properties:` fields

Every field is optional and may appear at most once. All but `Ability:`/`Moves:`/`AiFlags:` take a
closed vocabulary validated here; those three hold game constants, so — like a condition's
right-hand side — their names pass through to the C compiler and a typo surfaces as a compiler
error.

| Field | Every valid value |
| --- | --- |
| `Level:` | `LevelCap`, or a decimal integer `1` through `MAX_LEVEL` |
| `CatchRate:` | Decimal integer `0` through `255` |
| `Balls:` | `Default`, `Blocked`, or `Allowed` |
| `DamageReduction:` | Decimal integer `0` through `99` |
| `Immunities:` | Comma-separated list of `Ohko`, `FixedDamage`, `HpSwap`, `SharedKo`, `All`, or `None` |
| `CapTypeEffectiveness:` | `True` or `False` |
| `FlatToxicDamage:` | `True` or `False` |
| `Survive:` | `True` or `False` |
| `Ability:` | A single `ABILITY_*` constant. The name passes through to the C compiler unchecked; only its shape as an identifier is validated |
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
| `Battler(<ref>).Protected` | Same six refs | `0` or `1` |
| `Var(<name>)` | `<name>` is any C identifier | Encounter-local integer; first use declares it (maximum 16 per encounter). |
| `Event.Battler` | No argument | Battler id |
| `Event.Target` | No argument | Battler id |
| `Event.Move` | No argument | `MOVE_*` constant |
| `Event.MoveType` | No argument | `TYPE_*` constant |
| `Event.MoveCategory` | No argument | `DAMAGE_CATEGORY_PHYSICAL`, `DAMAGE_CATEGORY_SPECIAL`, or `DAMAGE_CATEGORY_STATUS` |
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
| `enccopyvar <dst>, <src>` | Two variables/indexes. |
| `enccomparestat <targetA>, <targetB>, <stat>, <var>` | Two targets that each resolve to exactly one battler; `STAT_ATK`, `STAT_DEF`, `STAT_SPATK`, `STAT_SPDEF` or `STAT_SPEED` (no battle stat exists behind `STAT_ACC`/`STAT_EVASION`); a variable/index the `0`/`1`/`2` result is written into. |
| `encadapt <target>, <percent>, <slots>, <countVar>, <resultVar>` | Target below; a reduction percent (`0`–`ENC_MAX_ADAPT_PERCENT`); a FIFO depth (`1`–`ENC_MAX_ADAPTATIONS`); a variable/index the resulting board size is written into; a variable/index the `ENC_ADAPT_RESULT_*` outcome is written into. `OnMoveEnd` only. |
| `encpurgeadapt <target>, <which>, <countVar>` | Target below; `ENC_ADAPT_OLDEST`, `ENC_ADAPT_NEWEST` or `ENC_ADAPT_ALL`; a variable/index the remaining board size is written into. |
| `encchangehp <target>, <amount>[, <mode>]` | Target below; signed 16-bit amount (positive heals, negative damages); optional `ENC_AMOUNT_FIXED` (default) or `ENC_AMOUNT_PERCENT`. |
| `encchangestat <target>, <stat>, <stages>` | Target below; `STAT_*` id; signed stage change. |
| `encchangestatvalue <target>, <stat>, <amount>[, <mode>]` | Target below; `STAT_ATK`, `STAT_DEF`, `STAT_SPATK`, `STAT_SPDEF` or `STAT_SPEED` (no battle stat exists behind `STAT_ACC`/`STAT_EVASION`); signed 16-bit amount; optional mode as above. |
| `encsetdamagereduction <target>, <percent>` | Target below; `0` through `ENC_MAX_DAMAGE_REDUCTION` (99). |
| `encsetimmunity <target>, <immunities>` | Target below; `0`, `ENC_IMMUNE_ALL`, or an OR of `ENC_IMMUNE_OHKO`, `ENC_IMMUNE_FIXED_DAMAGE`, `ENC_IMMUNE_HP_SWAP`, `ENC_IMMUNE_SHARED_KO`. |
| `encsetcaptypeeffectiveness <target>, <cap>` | Target below; `TRUE` or `FALSE`. |
| `encsetflattoxicdamage <target>, <flat>` | Target below; `TRUE` or `FALSE`. |
| `encsetsurvive <target>, <survive>` | Target below; `TRUE` or `FALSE`. |
| `encsetprotect <target>` | Target below. |
| `encsetcrit <target>, <state>` | Target below; `TRUE` or `FALSE`. |
| `encsettrapped <target>, <trapped>` | Target below; `TRUE` or `FALSE`. |
| `encclearscreens <target>, <failLabel>` | Target below, plus a script label taken when the side had nothing to clear. |
| `encclearhazards <target>, <failLabel>` | Target below, plus a script label taken when no side had a hazard on it. |
| `encresetstats <target>, <failLabel>` | Target below, plus a script label taken when no battler in the set had a stat stage to reset. |
| `encsetembargo <target>, <turns>` | Target below; a turn count, `0` to clear. |
| `encdisablemove <target>, <turns>, <failLabel>` | A target that resolves to exactly one battler; a turn count; a script label taken when there was no move to disable. |
| `encsetsidestatus <target>, <status>, <turns>` | Target below; an `ENC_SIDE_*` selector (`ENC_SIDE_REFLECT`, `ENC_SIDE_LIGHT_SCREEN`, `ENC_SIDE_AURORA_VEIL`, `ENC_SIDE_SAFEGUARD`, `ENC_SIDE_MIST`, `ENC_SIDE_TAILWIND`, `ENC_SIDE_LUCKY_CHANT`, `ENC_SIDE_RAINBOW`, `ENC_SIDE_SEA_OF_FIRE`, `ENC_SIDE_SWAMP`); a turn count, `0` (the default) for permanent. |
| `encclearsidestatus <target>, <status>` | Target below; the same `ENC_SIDE_*` selectors. |
| `encrevive <target>, <percent>` | Target below; `0` through `100`. |
| `encsetballs <policy>` | `ENC_BALLS_DEFAULT`, `ENC_BALLS_BLOCKED`, or `ENC_BALLS_ALLOWED`. |
| `encsetcatchrate <rate>` | `ENC_CATCH_RATE_NONE`, or `1` through `255`. |
| `encsetterrain <terrain>[, <turns>]` | `ENC_TERRAIN_ELECTRIC`, `ENC_TERRAIN_GRASSY`, `ENC_TERRAIN_MISTY` or `ENC_TERRAIN_PSYCHIC`; a turn count, `0` (the default) for permanent. |
| `encsetfieldstatus <status>[, <turns>]` | An `ENC_FIELD_*` selector (`ENC_FIELD_TRICK_ROOM`, `ENC_FIELD_GRAVITY`, `ENC_FIELD_WONDER_ROOM`, `ENC_FIELD_MAGIC_ROOM`, `ENC_FIELD_FAIRY_LOCK`, `ENC_FIELD_MUD_SPORT`, `ENC_FIELD_WATER_SPORT`); a turn count, `0` (the default) for permanent. |
| `encclearfieldstatus <status>` | The same `ENC_FIELD_*` selectors. |
| `encsethealblock <target>, <turns>` | Target below; a turn count, `0` to clear (asserts above `B_HEAL_BLOCK_TIMER`). |
| `encmegaevolve <target>, <failLabel>` | A target that resolves to exactly one battler, plus a script label. |
| `encformchange <target>, <species>, <failLabel>[, <anim>]` | A target that resolves to exactly one battler, a `SPECIES_*` constant, a script label, and an optional `B_ANIM_*` id (defaults to `B_ANIM_MEGA_EVOLUTION`). |
| `encsetmove <target>, <slot>, <move>` | A target that resolves to exactly one battler, a move slot `0`–`3`, and a `MOVE_*` constant. |
| `enctransform <target>, <source>, <failLabel>` | Two targets that each resolve to exactly one battler, plus a script label. |
| `encuntransform <target>` | A target that resolves to exactly one battler. |
| `encjumpifchance <percent>, <label>` | An integer `0`–`255` (asserts if above `100`) and a script label. |
| `encowned <species>, <var>` | A `SPECIES_*` constant and an author variable. |
| `encsetrecharge <target>, <turns>` | Target below; a turn count (`1` at `OnTurnStart` for one lost action, `2` elsewhere). |
| `encswitchout <target>, <failLabel>` | A target that resolves to exactly one battler (never the boss), plus a script label taken when there is nobody to send in. |
| `encaskyesno <stringId>, <yesLabel>` | A `STRINGID_*` constant and a script label taken on YES. |

Every valid `<target>` is `ENC_TARGET_BOSS`, `ENC_TARGET_SELF`, `ENC_TARGET_EVENT_TARGET`,
`ENC_TARGET_PLAYER_LEFT`, `ENC_TARGET_PLAYER_RIGHT`, `ENC_TARGET_OPPONENT_LEFT`,
`ENC_TARGET_OPPONENT_RIGHT`, `ENC_TARGET_ALL_FOES`, `ENC_TARGET_ALL_ALLIES`, or
`ENC_TARGET_ALL_BATTLERS`.
