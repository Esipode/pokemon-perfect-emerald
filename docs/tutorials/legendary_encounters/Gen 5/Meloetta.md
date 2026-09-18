## Meloetta — The Melody Pokémon

Meloetta fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Meloetta is singing a bar, and the bar has soft beats and a hard beat.**

Its meter does **not** make Meloetta harder to hurt. It changes **how much of the bar is the hard
beat**. The damage window is on a schedule, it is announced every single turn, and you control how
wide it is.

---

### THE BAR — four beats, one per turn, repeating

```
  RHYTHM/TEMPO 0-1     VERSE   VERSE   VERSE   CHORUS      three soft beats
  RHYTHM/TEMPO 2-3     VERSE   VERSE   CHORUS  CHORUS      two
  RHYTHM/TEMPO 4       VERSE   CHORUS  CHORUS  CHORUS      one
  RHYTHM/TEMPO 5       CHORUS  CHORUS  CHORUS  CHORUS      the song never comes back down
```

Every turn opens with a line telling you which beat is playing — "the verse plays softly" or "the
chorus swells". That line is the whole fight in one sentence: **hit it on the verse.**

| | VERSE | CHORUS |
| --- | --- | --- |
| Phase 0 (Overture) | 84 | 93 |
| Phase 1 (Grand Performance) | 86 | 95 |
| Phase 2 (The Final Note) | a flat 82 — the song has stopped | |
| Gathering a CRESCENDO | +3 on whichever beat is running | |
| STUMBLING out of a broken crescendo | **a flat 72 for one whole turn** | |

The numbers are never printed in-game. The beat callout is.

---

### THE TWO METERS — 0 to 5

Aria Forme carries **RHYTHM**, Pirouette Forme carries **TEMPO**. Only the active form's meter
moves; the other one is parked exactly where you left it and comes back when that form does. Every
change is announced, in **both** directions.

```
  +1  your active Pokémon lost 6% or more of its max HP this turn  (the action connected)
  +1  one of your Pokémon fainted this turn                        (the hall falls silent)
  -2  a RHYTHM BREAK — and the bar restarts at beat 1
  ->0 a broken CRESCENDO empties both meters
  ->2 a resolved CRESCENDO drops both meters to 2
```

**A RHYTHM BREAK is worth more than the two levels.** It also restarts the bar, so breaking the song
always buys you a full run of soft beats afterwards.

Three ways to break it, checked at the end of every turn:

| Route | Read as |
| --- | --- |
| **Meloetta is carrying a status condition** | The singer cannot hold the note |
| **You protected this turn** | The performance played to a closed door |
| **Your active Pokémon lost less than 6% of its max HP** | Meloetta's action did not land |

That third route is the wide one: a miss, an immunity, a switch into a wall, a turn Meloetta spent
on a status move, a sleeping or flinched Meloetta — all the same test.

Damage deliberately does **not** break the rhythm. The fight keeps its two jobs separate: you break
the song with **disruption**, and you spend the soft beats it buys on **damage**.

---

### WHAT THE METER BUYS

| Level | Aria (RHYTHM) | Pirouette (TEMPO) |
| --- | --- | --- |
| 0-1 | the callout only | the callout only |
| 2 | the bar tightens to two verse beats | same |
| **3** | the bar tightens again, and the **refrain chips** your side 5% on every chorus beat | same chip, plus a **raised critical-hit ratio** |
| **4** | one verse beat left; chip 7% | chip 7%, plus a **FOLLOW-UP** strike for 6% on any turn its action connected |
| **5** | the whole bar is chorus — and Aria **shifts into Pirouette** | the whole bar is chorus. With RHYTHM also at 5, a **CRESCENDO** begins |

Phase 1 adds +2 to both the chip and the follow-up. The chip and the follow-up come off **max HP**,
so Protect, a switch and a wall all fail to stop them.

---

### THE FORMS

| Trigger | Direction |
| --- | --- |
| RHYTHM reaches 5 | Aria → **Pirouette**, and TEMPO picks up where it was parked |
| TEMPO is broken to 0 | Pirouette → **Aria**, and RHYTHM picks up where it was parked |
| Phase 1, every second beat | it turns over on its own, whichever way it is facing |
| Phase 2 | forced back to Aria, permanently |

Pirouette is not scripted stat inflation — it is the **species**. Attack 128 and Speed 128 against
Aria's 77 and 90, Normal/Fighting instead of Normal/Psychic, and a completely different moveset.

| Slot | Aria | Pirouette |
| --- | --- | --- |
| 0 | Psychic | Close Combat |
| 1 | Hyper Voice | Acrobatics |
| 2 | Echoed Voice | Quick Attack |
| 3 | Shadow Ball | Brick Break |

Its ability is **Serene Grace** in both forms, so Psychic's Sp. Def drop lands twice as often as you
would expect.

---

### THE CRESCENDO

When **RHYTHM 5 and TEMPO 5** are both standing, Meloetta gathers the whole song. This is
telegraphed at the **start** of a turn, before you pick a move, and it wipes your stat stages on the
spot. Its guard goes up 3 while it gathers.

You have that whole turn to answer it, and the answer is the same as always — put a status on it,
Protect, or make sure its action does not connect.

| | |
| --- | --- |
| **Interrupted** | Both meters to **0**, Meloetta **loses its next action**, and its guard collapses to **72 for that entire turn**. The biggest window in the fight. |
| **Not interrupted** | **RELIC SONG** for 28% of your side's max HP (34% in Phase 1), **+4% for every finale it has already performed**, up to 46%. Both meters drop to 2 and the song starts over. |

---

### THE PHASES

| HP | Phase | What changes |
| --- | --- | --- |
| 100% | **OVERTURE** | 84 / 93 |
| **50%** | **GRAND PERFORMANCE** | 86 / 95. +12% raw Attack *and* Sp. Atk, so both halves of the cycle got stronger. The chip and the follow-up gain +2. Relic Song moves to 34%. The forms now cycle on their own every second beat — **and every shift sheds Meloetta's status.** |
| **15%** | **THE FINAL NOTE** | The fight pauses. Meloetta restores **8%** of its max HP and then the song **stops** — both meters die, the bar stops counting, the form locks to Aria, and the guard settles at a flat **82** for the quiet last stretch. |
| **10%** | **SPENT** | Every restriction comes off and **you may finally throw a ball.** |

---

### ANTI-ABUSE

| The line you might take | What the fight does |
| --- | --- |
| Status-lock it and stall | Status is **rewarded** — it breaks the rhythm every turn it holds. But from Phase 1 every form shift sheds it, and the forms shift every second beat, so it has to be re-applied around the cycle |
| Sit behind Protect forever | Protect breaks the rhythm, which is the intended counterplay — but a protected turn deals no damage of its own, and the chorus chip and Relic Song both come off max HP |
| Set up stat stages | Every crescendo opens by wiping them |
| Toxic and outlast | Toxic damage is flat, not ramping |
| A 4x matchup | Incoming type effectiveness is capped at 2x — and the form change moves the matchup under you mid-fight |
| Fixed damage / OHKO / Pain Split / Destiny Bond | All immune |
| Switch to a wall to deny the meter | This **works**, and is meant to — it is the third break route. It costs you the turn, and the chorus chip still lands |

---

### THE SHORT VERSION

1. Read the beat line at the top of every turn. Attack on **VERSE**, not on **CHORUS**.
2. Keep the meter down so most of the bar stays verse. Status, Protect, or deny it a real hit.
3. A break is worth two levels **and** a fresh bar. Take it whenever the alternative is a chorus beat.
4. If it ever reaches 5 and 5, answer the crescendo telegraph — the interrupt hands you a turn at
   **72**, the softest Meloetta is ever going to be.
