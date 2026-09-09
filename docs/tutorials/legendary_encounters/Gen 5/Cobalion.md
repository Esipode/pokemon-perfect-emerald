## Cobalion — The Iron Will

Cobalion fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Cobalion's stance answers your last turn. Its Discipline answers your last ten.**

Two numbers, and the fight announces both. Neither is a place you can live.

---

### The stance

Cobalion picks a stance at the top of every turn, based on what you did on the previous one.

| Stance | It picks this when | Slots 3 and 4 become | Its stats | Guard |
| --- | --- | --- | --- | --- |
| **GUARD** — *"it sets itself against the blow it has just seen"* | You attacked it last turn | Metal Burst, Safeguard | Def +1, Sp. Def +1 | **92 / 94 / 90** |
| **ASSAULT** — *"you gave it room"* | You set up, switched, or used a status move last turn | Close Combat, **Quick Attack** | Atk +2, Speed +1, Def −1 | **80 / 82 / 80** |
| **COMMAND** — *"it stops fighting and starts watching you"* | Discipline is 4+, or it is carrying a status it wants gone | Taunt, Aura Sphere | — | **86 / 88 / 84** |

(Guard values are Phase 0 / Phase 1 / Phase 2. Sacred Sword and Iron Head never leave slots 1 and 2.)

**GUARD is the wall. ASSAULT is your damage window — and its damage window.** The only way to open
the guard is to stop attacking, and the turn you stop attacking is the turn Cobalion comes forward
at +2 Attack with a priority move in hand. That trade is the fight.

COMMAND is the turn Cobalion spends reading instead of hitting. It also **sheds its own status** when
it enters, then goes on a 3-turn cooldown before it can do that again.

---

### DISCIPLINE

```
 DISCIPLINE   0 ....... 1 ....... 2 ....... 3 ....... 4 ....... 5
              unread                     COUNTERS  COMMAND   THE READ

  +1  you did the same KIND of thing again (physical / special / status / switching)
  -1  you did something different
```

Discipline **never changes the guard.** It punishes instead:

| Level | What it does |
| --- | --- |
| 0–2 | Nothing but the callout. It is still reading you |
| **3+** | **COUNTERSTRIKE** — repeat a damage category into GUARD and it comes back for **12% / 15% / 18%** of your Pokémon's max HP |
| **4+** | Cobalion drops into COMMAND on its next turn |
| **5** | **THE READ** — it disables the move you keep leaning on for 3 turns, and from Phase 1 it also drags your Pokémon out. Discipline then falls back to 3 |

The category is the one printed on the move, so Photon Geyser and Tera Blast count as whatever their
move data says. **Switching counts too**: answering every stance change with a switch is a pattern
like any other.

Doing *nothing* is also a pattern. Three turns in a row where you commit no action against Cobalion
(Protect, Substitute, recovering, stalling) costs **10%** of your max HP and a Discipline level. A
status move breaks the count — it feeds the ordinary Discipline path instead.

---

### Phase 1 — LEADER OF THE SWORDS (50%)

Attack **+25%** and Speed **+15%**, written into the raw stats — a Haze will not take them off.
Discipline floors at 1. And **TACTICAL COMMAND** begins.

Every turn from here, Cobalion declares the approach it expects:

- *"Cobalion sets its stance against a head-on blow."* — it is expecting **physical**
- *"Cobalion braces for something sent from a distance."* — it is expecting **special**
- *"Cobalion watches your Pokémon's footing, not its attack."* — it is expecting a **status move**

That read is the battle AI's real prediction, not a script. Then:

| | |
| --- | --- |
| **It guessed right** | It counters you for **14%** (16% in Phase 2) of your max HP, and gains a Discipline level |
| **It guessed wrong** | Its guard falls **18 points** for the rest of the turn and it loses a Defense stage. **This is the biggest damage window in the fight** |

> **The declaration you read this turn is the read you beat *next* turn.** The stance is chosen after
> turn order is already locked, so the line prints after you have committed. Read it, remember it,
> and pick against it on the following turn.

Some turns the AI has no prediction and says nothing. That is a quiet turn, not a trick.

---

### Phase 2 — IRON RESOLVE (25%)

- Discipline floors at **2** — it never forgets what it learned
- Counterstrike hits for **18%**, the read for **16%**
- ASSAULT's guard falls to **74** — the softest Cobalion ever gets
- **The stance flips every single turn.** Any turn the ledger would have left it alone, it swaps
  GUARD ↔ ASSAULT anyway. You cannot settle into a line of play

---

### The last stand, and the catch

At 1 HP Cobalion **stands firm**, spends everything on one final assault for **35%** of your
Pokémon's max HP, and only then can it be beaten — or caught.

> That final assault can KO. If you arrive at 1 HP on a nearly-dead team, you can still lose there.

Once it lands, the fight goes completely quiet: no stances, no Discipline, no counters. Poké Balls
unlock at a catch rate of 30, and the engine's own catch-window guard keeps Cobalion alive while you
throw. **This is the only point in the fight where a ball works at all.**

---

### What does and does not work on it

| Plan | What happens |
| --- | --- |
| Sleep / paralysis / burn | **They land and they work completely.** Cobalion has no status immunities. It sheds one when COMMAND comes around — up to three turns later. Time yours right after it sheds |
| Safeguard (GUARD slot 4) | Five turns where nothing new lands on it. **Defog strips it** like any other Safeguard |
| Toxic | Steel-type; it refuses poison outright |
| Double Team / evasion | Cobalion strips **two evasion stages a turn**, and Aura Sphere in COMMAND cannot miss |
| Cosmic Power / Iron Defense walling | **Sacred Sword ignores your defensive stat changes.** It is in a fixed slot and never leaves |
| PP stalling it out | Every stance change refills slots 3 and 4; every phase transition refills the rest |
| Bringing a Dark-type answer | **Justified.** A Dark move raises its Attack |
| Switching out of a punish | **Allowed, deliberately.** Cobalion never traps you — switching is one of the ways to break GUARD. It just counts toward Discipline if you keep doing it |

---

### The short version

1. **Alternate.** Physical, then something else, then physical. Repeating anything feeds Discipline.
2. **Alternating is also what flips the stance**, which is how you reach ASSAULT's soft guard.
3. From 50%, **read the declaration and beat it next turn.** Outguessing it is worth more than any
   single attack.
4. **Never stall.** Three idle turns costs you health and a Discipline level.
5. Save your ball. It does not work until Cobalion has made its last stand.
