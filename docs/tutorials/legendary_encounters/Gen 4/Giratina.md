## Giratina — The Renegade Pokémon

Giratina fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

Its guard barely moves — 89%, then 91%, then 93% at the two phase transitions, and nowhere else.

> **Distortion does not buy Giratina tankiness. It buys rules.** The fight is not "can I dent this
> thing" — it is "do I understand what is happening right now, and can I plan one turn ahead of it?"

---

### DISTORTION and the wheel

```
 DISTORTION   0          1          2          3          4          5
            REALITY    REALITY   UNSTABLE  DISTORTION  REALITY   DISTORTION WORLD
            INTACT      BENDS     REALITY     WORLD    COLLAPSE      MASTERY

 rules live   0          1          1          2          2           3
 rotates in   4          4          3          3          2           2   turns
 guard       89 ------------------ 91 (Ph 1) ---------------- 93 (Ph 2)
                                                      ANCHOR window -> 84 for 3 turns
```

There is exactly **one clock**. When it runs out, the world turns over: every rule currently up is
cleared, the wheel advances and refills, Distortion rises one step, and any status standing on
Giratina is shed. One clock, several consequences, one banner — *"THE WORLD TURNS OVER."*

The level itself is announced every time it changes, **in both directions**:

| Level | Giratina says |
| --- | --- |
| **5** | *"DISTORTION WORLD MASTERY. Giratina has stopped bending the rules and started writing them."* |
| **4** | *"REALITY COLLAPSE. Two of this world's laws are wrong at once now."* |
| **3** | *"THE DISTORTION WORLD. You are not fighting in your world any more."* |
| **2** | *"UNSTABLE REALITY. The rules here have stopped staying where you put them."* |
| **1** | *"REALITY BENDS. Something about the ground is no longer agreeing with itself."* |
| **0** | *"The field is holding its shape."* |

Underneath the crossings, a short line telegraphs the next rotation one turn early — *"Something in
the air is about to give."*

### The five rules on the wheel

The wheel is fixed and cycles in the same order every time, so a player who has seen it once can plan
two rotations ahead: **SPEED → GRAVITY → DEFENCES → HEALING → POSSESSIONS →** back to SPEED. Every
rotation re-prints the name of every rule currently live, so you are never more than one rotation away
from being told exactly what is active.

| Rule | What it actually does |
| --- | --- |
| **SPEED INVERTED** | The slower Pokémon moves first (Trick Room). |
| **GRAVITY COLLAPSED** | Everything is grounded, accuracy rises, Fly/Bounce/Magnet Rise stop working — **and Origin Forme's own Levitate drops with everyone else's.** Ground moves land on Giratina for the first time in the fight. |
| **DEFENCES INVERTED** | Defense and Special Defense swap, for both sides (Wonder Room). |
| **POSSESSIONS INVERTED** | Held items stop working, both sides (Magic Room). |
| **HEALING INVERTED** | Your healing does not land (Heal Block) — items, Leftovers, draining moves, all of it. |

At Distortion 0 nothing is live yet — that is your baseline. From Distortion 1 one rule is up; from
3, two; at 5, three at once.

---

### STABILITY and ANCHOR REALITY

```
  STABILITY  0 --------------- 3

  +1   end of every turn your Pokémon is still standing
  -1   every rotation                     (the ground moves under you)
  ->0  you switch out, or your Pokémon faints
```

At **3**, you are offered the fight's release valve:

```
                        ANCHOR REALITY?
                      [  YES  ]   [  NO  ]

  YES ->  every rule currently up is cancelled, Distortion drops by 2 (never below the
          phase floor), Giratina loses its next action, and its guard falls 91/93 -> 55
          for THREE TURNS. Stability spends to 0 and the rotation clock restarts.

  NO  ->  Stability is kept. The prompt goes quiet for two turns.
```

**Holding at 3 is a real bet, not a free option** — the next rotation knocks you back to 2 regardless,
so declining is a choice to gamble one more turn hoping to dodge a nastier rule set, not a free stall.

**Switching costs everything.** It resets Stability to 0, and from Phase 1 onward it also feeds
Distortion +1 — the same price a faint pays. Giratina never traps you; it just makes leaving expensive
enough that you feel it.

---

### Status is a bet, not a lock

A status lands on Giratina and **works completely** — there is no immunity here. But it only lasts
until the next rotation (at most 4 turns), which sheds it automatically:

> *"The affliction slides off a Giratina that is no longer quite the one you gave it to."*

Time a status move for right after a rotation and you get full value from the lockout before the wheel
turns it over again. Toxic is held flat the whole fight, so stalling behind poison is safe to lean on.

---

### The phases

| | |
| --- | --- |
| **100–50% — Reality Intact / The Distortion World** | Altered Forme. Guard 89, capped at Distortion 2. Moves: Shadow Claw / Dragon Claw / Toxic / Hex. |
| **≤ 50% — ORIGIN FORME** | Guard rises to 91. **Origin Forme** — Levitate, and a 120/100/120/100 stat line. Distortion floors at 3 from here and two rules go up immediately. Shadow Force and Draco Meteor replace the first two slots. |
| **≤ 25% — REALITY BREAK** | Guard rises to 93. Distortion **pins at 5** — all three wheel slots stay filled from here on. Earthquake replaces Toxic, and pairs with GRAVITY COLLAPSED grounding Giratina itself. |
| **≤ 10% — THE RETURN** | Distortion counts back down 5→0, every rule clears, Giratina returns to Altered Forme, and the catch window opens. |

**The Distortion Gate** unlocks at Distortion 4 (Phase 1+): a tear opens beside Giratina and behaves
exactly like a real Protect — Feint, never-miss moves and contact punishment all resolve the way they
would against one.

**REALITY BREAK, Phase 2 only:** every four turns (telegraphed the turn before — *"The field will not
hold much longer."*), every rule on the wheel is re-rolled at once, your side takes a 10% max-HP chip,
and Giratina vanishes into the tear for that turn. This is the point ANCHOR REALITY stops being
optional — three turns at 55% guard is worth roughly eight times a turn at 93%, landing on a Giratina
that has already lost its action.

---

### Catching it

Poké Balls are **blocked for the entire fight** until Giratina is at 10% HP or below. At that point
every rule comes down, it returns to Altered Forme, its guard and immunities lift, and it tells you
plainly:

> *"Giratina has nothing left holding the world open, and it can barely hold itself. Now — now is the
> moment to catch it!"*

The catch rate scales with your **final Stability**:

| Final Stability | Catch rate | Line |
| --- | --- | --- |
| **3** | 200 | *"You are the only thing in this place still standing where you put yourself."* |
| **1–2** | 150 | *"It has been thrown around as much as you have."* |
| **0** | 100 | *"Neither of you is quite sure where the ground went."* |

---

### The short version

1. **Learn the wheel order** — SPEED, GRAVITY, DEFENCES, HEALING, POSSESSIONS — and plan two
   rotations ahead once you have seen it once.
2. **Climb Stability and use ANCHOR REALITY.** It is the fight's real damage window, not a bonus.
3. **Switching and fainting both cost you** — Stability to 0, and Distortion up a level from Phase 1.
   Treat a switch as a real decision, not a free reset.
4. **Time your status moves for right after a rotation** — you get the full lockout before the wheel
   sheds it.
5. **Watch for GRAVITY COLLAPSED in Phase 1+.** It is the one turn a Ground move actually lands on
   Giratina.
6. **In Phase 2, bank ANCHOR REALITY for right before a REALITY BREAK** — the guard window is worth
   far more when Giratina has also just lost its action.
