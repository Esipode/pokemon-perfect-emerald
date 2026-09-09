## Virizion — The Graceful Blade

Virizion fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Momentum is Virizion's armour. It only flows while its rhythm holds — break the rhythm and the
> armour goes with it.**

One number runs the whole fight, and it is announced in both directions.

---

### MOMENTUM

```
 MOMENTUM  0 ....... 1 ....... 2 ....... 3 ....... 4 ....... 5 ....... 6 ....... 7
 guard     78       81        84        87        90        93        95        97
                                        ^                   ^
                                   BLADE DANCE          UNTOUCHABLE
                                   +2 evasion, crit,    scripted dodges,
                                   +1 Speed, 2nd slash  +1 Speed

   +1  Virizion's side took health off you this turn
   +1  ...and nothing took health off Virizion (a flawless pass)
   +1  Virizion knocked one of your Pokemon out
   +1  every turn, no matter what, once SACRED BLADE begins

   ->0 a turn Virizion's side took no health off you at all
   ->0 a super-effective type landed on it
   ->0 it is carrying a status at the end of the turn
   ->0 its Speed stage is below neutral at the end of the turn
```

**Unlike most legendaries, the phase does not own Virizion's guard — the meter does.** At Momentum 0
it takes 78% less damage, the softest any shipped legendary gets. At Momentum 5 it takes 93% less
and is dodging, critting and outrunning you. You cannot out-damage Virizion while it is flowing, so
the fight is about *windows*: break the rhythm, then unload into the turns that follow.

A break drops the meter to **0** and nothing else — but that alone is 15–19 guard points, plus
evasion, the crit boost and the Speed stages all stripped in one beat.

| Level | What it does |
| --- | --- |
| 0–2 | Nothing but the guard and the callout |
| **3+** | **BLADE DANCE** — on a turn the meter rose, Virizion's blow carries into a second scripted slash (8/10/12% by phase). Also +2 evasion, +1 Speed, and a permanently raised critical-hit ratio |
| **5+** | **UNTOUCHABLE** (Phase 1+) — a telegraphed scripted dodge on a 2-turn cooldown, plus another +1 Speed |
| **5** | Phase 0's ceiling |
| **7** | Untouchable's ceiling, and where the SACRED BLADE charge fires |

The break is worth aiming for, not stumbling into. A super-effective hit, a status, a Speed drop, or
simply a turn where Virizion's side connects on nothing — all four reset the meter to 0.

---

### The moveset

Slot 1 never changes. Slot 4 hardens at 50%.

| Slot | Phase 0 | Untouchable (50%) | Sacred Blade (15%) |
| --- | --- | --- | --- |
| 1 | Sacred Sword | (same) | (same) |
| 2 | Leaf Blade | (same) | (same) |
| 3 | Quick Attack | (same) | (same) |
| 4 | Magical Leaf | **Close Combat** | (same) |

- **Sacred Sword** ignores your defensive stat changes *and* your evasion — a Double Team stack or a
  Cosmic Power wall is a losing plan.
- **Leaf Blade** has a high critical-hit ratio of its own; at Momentum 3+ that stacks with the
  meter's crit boost.
- **Quick Attack** is priority. You have to actually stop the hit, not merely outspeed it.
- **Magical Leaf** never misses, and it is the *weakest* slot — while you are still learning the
  meter, Virizion has a guaranteed way to keep its flow.
- **Close Combat** replaces Magical Leaf at 50%: bigger, and it lowers Virizion's own defenses.

---

### Phase 1 — UNTOUCHABLE (50%)

- Raw **Speed +10%**, written into the stats — a Haze will not remove it
- The Momentum ceiling rises from 5 to **7**
- Scripted dodges go live at Momentum 5+, on a 2-turn cooldown. They are announced *before* your
  move, so you can spend the turn on setup, a heal or a switch instead
- Blade Dance's slash hits harder
- Virizion now **sheds what slows it**: a status is cleared at the top of every turn, and a lowered
  Speed stage climbs back one stage a turn. Status and Speed control keep working, but now cost a
  move every turn instead of once

---

### Phase 2 — SACRED BLADE (15%)

> **Virizion gathers itself.** The meter now rises **+1 every turn**, whether or not Virizion landed
> anything — so denying it a hit no longer stops the climb. Only a real disruptor does: a
> super-effective type, a status, or a Speed drop.

At Momentum 7 it fires **SACRED SWORD** for **32% of your side**, then falls back to 3 to charge
again — roughly every fourth turn. Break the charge before it completes and Virizion is left
**EXHAUSTED**: meter 0, −1 Def/Sp. Def, and it loses its next action. That is strictly better than
eating the charge, which is what makes the race worth running.

> The charge can KO. If you let it complete on a nearly-dead team, you can lose there.

`Survive: True` carries the fight to 1 HP.

---

### The last stand, and the catch

At 1 HP Virizion's blade **finally falls**: one last strike for 20% of your side, and only then do
the guard, the immunities, the evasion, the crit boost and Survive all come off, and Poké Balls
unlock at a catch rate of 30. The engine's own catch-window guard keeps Virizion alive while you
throw. **This is the only point in the fight where a ball works at all.**

---

### What does and does not work on it

| Plan | What happens |
| --- | --- |
| Sleep / paralysis / freeze | **They land and they work completely.** In Phase 0 a status holds the meter at 0 for as long as it lasts — a genuinely strong opening. From 50% Virizion sheds it every turn, so it costs you a move each turn instead of once |
| Toxic | Lands, its damage is flat, and — being a status — it holds the meter down in Phase 0 and is shed from Phase 1 |
| Lowering its Speed (Icy Wind, Sticky Web, paralysis) | Below-neutral Speed breaks the meter every turn it lasts. From Phase 1 Virizion recovers one stage a turn |
| Protect / Substitute / healing loops | A turn Virizion's side takes nothing off you breaks the meter — until SACRED BLADE, where the charge climbs regardless. The fight tells you when that changes |
| Double Team / evasion | **Sacred Sword ignores evasion.** Fixed slot, never leaves |
| Cosmic Power / Iron Defense walling | The same move ignores defensive stat changes too |
| Bringing a Dark-type answer | **Justified.** A Dark move raises its Attack |
| A Flying-type coverage move | Incoming type effectiveness is capped at 2x — a 4x Flying hit lands as a 2x hit. It still counts as super-effective and breaks the meter |
| Switching / fleeing | **Allowed, deliberately.** Virizion never traps you. A switch is a turn your health still drops to Quick Attack, so the meter keeps climbing |
| PP stalling it out | Every phase transition refills its moves |

---

### The short version

1. **You cannot out-damage Virizion while it is flowing.** Break the meter, then hit it in the
   turns after.
2. **Any of four things breaks it:** a super-effective hit, a status, a Speed drop, or a turn where
   it connects on nothing (Protect, a miss, a resist). A break drops the meter to 0 and strips
   everything the meter gave it.
3. **Phase 1 sheds status and Speed drops** — they still work, they just cost a move every turn now.
   Watch for the announced dodge at Momentum 5+.
4. **Phase 2 is a countdown.** The meter climbs on its own; only a real disruptor stops it. Break
   the charge for the EXHAUSTED window, or eat SACRED SWORD for 32%.
5. Save your ball. It does not work until Virizion's blade falls at 1 HP.
