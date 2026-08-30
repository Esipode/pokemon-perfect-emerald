## Kyogre — The Endless Ocean

Kyogre fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. Sleep and freeze do not stick to it either.

None of that is the fight. Rayquaza was a fight over a **position**. Kyogre is a fight over a
**water level** — and unlike every mechanic before it, that level moves in **both directions**.

### The core loop

```
The TIDE runs from 0 to 5.  It rises on its own, it rises when KYOGRE
lands a Water move, and it rises when you burn it with FIRE.

High Tide does four things at once:
  - KYOGRE gets harder to damage
  - your whole party takes drowning damage every turn
  - at Tide 4 the field is SWAMPED and your Speed is quartered
  - at Tide 4 KYOGRE marks your active Pokémon with the UNDERTOW

You push the Tide back with ELECTRIC and GRASS damage, and by HOLDING
YOUR GROUND under the Undertow instead of switching away from it.

Only ONE tide change from a move can land per turn.  Kyogre is fast
enough to spend it first.  Losing the Speed race is losing the Tide
race, and Tide 4 quarters your Speed.  That is the death spiral.
```

### Reading the water

Kyogre never tells you a number. It tells you how high the water is, and it tells you **every time
that changes**, in either direction:

| The line | Where the water is | What it means |
| --- | --- | --- |
| *(no line)* | **Calm** | Kyogre's softest guard |
| *The water is creeping higher around your POKéMON.* | **Rising** | Same guard, but the clock is running |
| *HIGH TIDE. Kyogre is harder to reach through the swell.* | **High Tide** | Guard up |
| *FLOOD. The battlefield is going under!* | **Flood** | Guard up, and your whole side starts drowning every turn |
| *DELUGE. Everything is slowing down in the water!* | **Deluge** | Harder guard, heavier drowning, your Speed is quartered, and the Undertow arms |
| *OCEAN'S WRATH. Nothing can move against this current!* | **Ocean's Wrath** | The worst of everything |

Every single step is announced as it happens:

> *The tide rises!* → the water gained a step
> *The tide is driven back!* → you took one back

While the field is flooded, a line repeats every turn as your party takes damage:

> *The rising water drags at your POKéMON!*

### Moving the water

**It rises:**

- On its own, every third turn at first — and **every turn** once Kyogre reverts to its Primal form.
- Whenever Kyogre lands a **Water** move.
- Whenever a **Fire** move hits it, early on. *Kyogre answers the flame. The water surges up to meet
  it!* Once Primordial Sea is up, Fire moves simply fail, so this stops being relevant.
- Whenever it shakes off sleep or freeze.
- Whenever you switch a Pokémon out from under the Undertow.

**It falls:**

- Whenever a damaging **Electric** or **Grass** move hits Kyogre.
- Whenever a Pokémon survives all three turns of the Undertow.

**The catch — and this is the whole fight:** only **one** move-driven tide change lands per turn, in
either direction, and it goes to whoever moves first. If your Thunderbolt lands first, you spend it
on a step back. If Kyogre's Origin Pulse lands first, it spends it on a step forward and **your ebb
does not happen at all**. Meanwhile the natural rise costs Kyogre nothing.

Kyogre's Speed is 90. Outspeeding it is the difference between holding the line and drowning, which
is exactly why Deluge quarters your Speed. Falling behind makes falling further behind easier.

### The Undertow

From the Primal Reversion onward, any turn the water sits at Deluge or higher:

> *The current begins pulling your POKéMON beneath the waves!*

For three turns, that Pokémon takes extra damage on top of the drowning chip and has its **Special
Defense permanently eroded** — that loss does not come back with Haze or a switch.

> *The undertow drags it deeper!*

You have a real choice:

- **Hold your ground for all three turns** and the water gives way by one step:
  > *It held its ground against the current, and the water gave way!*
- **Switch out** and you waste every point of Sp. Def you already paid, *and* the water gains a step:
  > *It tore free of the undertow - and the sea rushed in behind it!*

Bailing is a two-step swing against you.

### Phase 2 — Primal Reversion (50%)

> *PRIMAL REVERSION*
> *Kyogre's ancient power awakens! The sea answers faster now!*

Three things change:

1. **Primordial Sea.** The rain cannot be removed, and **Fire moves fail outright**. Weather is no
   longer a lever for anyone.
2. **The natural rise goes from every third turn to every single turn.** Everything you were already
   doing still works. You just have to do it twice as fast — and the best a perfect ebb turn can now
   buy you is holding even.
3. Kyogre hits noticeably harder, and the Undertow arms.

### Phase 3 — The Great Deluge (25%)

> *The sea begins to rise beyond all control!*

The water now **floors at Deluge level 3**. You can still fight it back, just never to calm. And
Kyogre starts winding up:

> *Kyogre is gathering the whole ocean above the field!*
> *The wave is still building…*
> *The wave is about to break!*

Three turns later the wave resolves — **against the Tide, not against your HP**:

- **Water at Deluge (4) or higher:** *THE BATTLEFIELD IS SWALLOWED BY THE OCEAN!* Nearly half your
  active Pokémon's health is gone and the Tide locks to the very top. A healthy Pokémon lives this.
  One that has been drowning for three turns does not.
- **Water at 3 or lower:** *The raging sea subsides! Kyogre is spent from holding it up!* Kyogre
  loses its next action entirely and its guard collapses for **two full turns**:
  > *Kyogre is still reeling. Its guard is down!*

That exhaustion window is the only place in the last quarter of the fight where you meaningfully
damage it, and it is earned by holding the water at the floor for three straight turns. Then Kyogre
rests a turn and starts winding up again.

### Phase 4 — Weakened (10%)

> *The endless sea drains away, and Kyogre sinks back into its true shape.*
> *It's worn out - now is the moment to catch it!*

Primordial Sea drops, the swamp lifts, the Tide resets, and every mechanic in the fight goes quiet.
**This is the first and only point in the battle where a Poké Ball can be thrown.**

### What to bring

- **Electric or Grass attackers**, and fast ones. They are your only way to push the water back, and
  they only work if you move first.
- **Speed.** The entire fight is a Speed race, and Kyogre attacks it directly — the swamp quarters
  it, and Body Slam's paralysis threatens it.
- **Something that can eat three turns of the Undertow.** Bulk matters more than the Sp. Def stat
  itself, because the Sp. Def is what gets eaten.
- **Not just one answer.** Ice Beam is aimed squarely at a lone Grass ebber, and Thunder never misses
  under Kyogre's own rain, so a bulky Water wall is not the safe pick it looks like.
- Burn and paralysis both still land. They cost you a turn to set up, which is a real price in a
  fight this tight, but they work.
