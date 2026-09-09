## Terrakion — The Unstoppable Force

Terrakion fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Terrakion's strength feeds on contact. The only thing that slows it down is a turn where it hits
> nothing.**

One number runs the whole fight, and it is announced in both directions.

---

### IMPACT

```
 IMPACT   0 ....... 1 ....... 2 ....... 3 ....... 4 ....... 5 ....... 6 ....... 7
                          SHOCKWAVES          BREAKER            (Rampage only)
                                                                       COLLAPSE

   +1  Terrakion landed a Rock or Fighting attack
   +1  Terrakion took a fifth of your Pokemon's health off in one turn
   +1  a strong attack landed on Terrakion (about an eighth of its health)
   +1  Terrakion's force went through a shield
   +2  Terrakion knocked one of your Pokemon out   (this one ignores the once-per-turn cap)

       at most one +1 per turn, from any combination of the first four

   -1  a turn passed and Terrakion's side took NO health off you at all
```

Terrakion attacks every turn and its whole moveset is Rock and Fighting, so IMPACT climbs almost on
its own. It is not something that happens *to* you — it is a meter you have to spend turns holding
**down**, by denying Terrakion a landed hit. A miss, a Protect, a sleep or paralysis turn, a switch
that eats the hit on something that resists it — all of them are the same currency. The turn
Terrakion's side takes nothing off you, IMPACT falls a level.

**IMPACT never changes the damage reduction.** The phase owns that number and the fight announces
every move of it separately. IMPACT buys Terrakion *reach*, not skin:

| Level | What it does |
| --- | --- |
| 0–1 | Nothing but the callout |
| **2+** | **SHOCKWAVES** — on a turn Terrakion connected, the force of the blow rolls across the whole field: 4% chip, plus one of *clears your hazards / breaks your screens / wipes terrain and field effects*, on a fixed wheel |
| **4+** | **BREAKER** — Protect stops working: the force goes through it for 12% of your max HP. On a 3-turn cooldown Terrakion also strips your stat stages and shatters your screens |
| **5** | Phase 0's ceiling. Peak force — the meter just sits there and hurts |
| **7** | Rampage's ceiling. **COLLAPSE** |

---

### The moveset

Slot 1 never changes. Slots 2–4 harden at each phase.

| Slot | Phase 0 | Rampage (50%) | Unstoppable (20%) |
| --- | --- | --- | --- |
| 1 | Sacred Sword | (same) | (same) |
| 2 | Rock Slide | Stone Edge | (same) |
| 3 | Smack Down | Close Combat | (same) |
| 4 | Swords Dance | Quick Attack | **Reversal** |

- **Sacred Sword** ignores your defensive stat changes — a Cosmic Power or Iron Defense wall is a
  losing plan.
- **Swords Dance** is a status move: a turn Terrakion spends on it lands nothing, so it *costs*
  Terrakion an IMPACT level. The AI winding up is your decay window.
- **Smack Down** grounds a Flying-type or Levitate answer.
- **Reversal** at Unstoppable's HP is a 150–200 BP Fighting move.

---

### Phase 1 — RAMPAGE (50%)

- Raw **Attack +20%** and **Speed +10%**, written into the stats — a Haze will not remove them
- The guard **drops** from 90 to 86. Terrakion has stopped defending itself
- The IMPACT ceiling rises from 5 to **7**, and **COLLAPSE** goes live

**This is where the fight inverts.** Everything Phase 0 taught you about holding IMPACT down is now
a weapon. Drive it to 7 on purpose:

> **COLLAPSE** — Terrakion overreaches, fires the **EARTH-SPLITTING CHARGE** for 30% of your max HP,
> then staggers: IMPACT resets to 0, both its defenses drop two stages, it loses its next action,
> and its guard drops another 20 points for two turns.

Two turns at a 66 guard against a Terrakion at −2/−2 that cannot move on the first of them. That is
the largest damage window in the fight, and you open it by doing the one thing Phase 0 spent ten
turns punishing.

> The charge can KO. If you push IMPACT to 7 on a nearly-dead team, you can lose there.

---

### Phase 2 — UNSTOPPABLE (20%)

- The guard climbs to **93**
- **Mist** on Terrakion's side — its Attack can no longer be lowered. **Defog strips it**, and
  Infiltrator ignores it
- IMPACT rises **+2 a turn**, so COLLAPSE comes around roughly every third turn
- SHOCKWAVES fire on **every** landed hit, not only on a turn IMPACT rose
- Slot 4 becomes **Reversal**

The last stretch is a rhythm of enormous hits and enormous openings. `Survive: True` guarantees you
reach 1 HP.

---

### The last stand, and the catch

At 1 HP Terrakion's strength **gives out**: one final Close Combat for 25% of your max HP, and only
then do the guard, the Mist, the immunities and Survive all come off and Poké Balls unlock at a
catch rate of 30. The engine's own catch-window guard keeps Terrakion alive while you throw.
**This is the only point in the fight where a ball works at all.**

---

### What does and does not work on it

| Plan | What happens |
| --- | --- |
| Sleep / paralysis / freeze | **They land and they work completely.** Terrakion has no status immunities. A Terrakion that can't act lands nothing — you are *spending* those turns on IMPACT control, which is the whole point |
| Toxic | Lands, but its damage is flat — it will not ramp to a lethal tick over a long fight |
| Protect / Quick Guard | Works below IMPACT 4 — that is the intended counterplay. At 4+ BREAKER punches through for 12%, and that chip denies the decay |
| Substitute / healing loops | The sub or the heal is not a hit, so Terrakion's own attack still lands and IMPACT still climbs |
| Double Team / evasion | Terrakion strips **two evasion stages a turn**; BREAKER wipes the rest at IMPACT 4+ |
| Cosmic Power / Iron Defense walling | **Sacred Sword ignores your defensive stat changes.** Fixed slot, never leaves |
| Lowering its Attack (Intimidate, Charm, Growl) | Untouched in Phases 0–1. **Mist** blocks it from Phase 2 — but Defog and Infiltrator still beat that |
| Bringing a Dark-type answer | **Justified.** A Dark move raises its Attack |
| A Steel-type coverage move | Incoming type effectiveness is capped at 2x — a 4x Steel hit lands as a 2x hit |
| Switching / fleeing | **Allowed, deliberately.** Terrakion never traps you. A switch costs a turn, and if Terrakion's attack still connects that turn, IMPACT still rises — but a switch onto a resist that eats the hit for nothing is a decay turn |
| PP stalling it out | Every phase transition refills its moves |

---

### The short version

1. **Phase 0: hold IMPACT down.** Every turn Terrakion connects on nothing, it falls a level. Miss
   it, Protect below 4, resist the hit, put it to sleep — all the same.
2. **Watch for BREAKER at 4.** Protect stops being safe; the fight tells you when.
3. **Phase 1: flip the plan.** Drive IMPACT to 7, survive the EARTH-SPLITTING CHARGE, then hit the
   stagger with everything — that is your biggest window.
4. **Phase 2 is a countdown.** IMPACT climbs two a turn; COLLAPSE comes fast. Ride the rhythm.
5. Save your ball. It does not work until Terrakion's strength gives out at 1 HP.
