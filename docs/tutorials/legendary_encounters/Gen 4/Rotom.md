## Rotom — The Possessive Pokémon

Rotom fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** for the whole fight, so the catch window at the end is
guaranteed no matter how the damage race goes.

It keeps **Levitate**. Ground moves do nothing. Levitate does *not* absorb Electric, which matters
more than it sounds — see below.

Every other legendary in the set fights *you*. Rotom fights **the machine you are fighting it in**.

> **Rotom is nearly unkillable at rest, and briefly wide open when it blows out. The whole fight is
> learning to make it blow out on your schedule instead of its own.**

### Two numbers

| | |
| --- | --- |
| **FORM** | Which appliance it is currently wearing. Five of them, each a different fight. |
| **OVERCLOCK** | 0 → 5. How hard its systems are running. **Everything it does raises it.** |

```
 OVERCLOCK   0 ......... 1 ......... 2 ......... 3 ......... 4 ......... 5
                                                 ^ IT HARDENS         ^ SYSTEM
                                                                       OVERLOAD
  Its guard   88          88          88          93          93       -> 55
                                                                        for 3 turns,
                                                                        out of form,
                                                                        and it loses a turn
```

Guard 93 means your attacks are doing roughly **half** what they did at 88. Guard 55 means they are
doing **six times** what they did at 93. That swing *is* the fight.

Every rung on the meter is announced with a line of its own, and while the wall is up you are told so
every turn — *"Rotom is holding itself together, and holding you off with it."*

### What raises OVERCLOCK

| Source | Amount |
| --- | --- |
| Every possession (it takes a new appliance) | +1 |
| Every hijack (it seizes one of your systems) | +1 |
| Being in **Heat** form | +1 more, every turn |
| Below 50% / below 25% | +1 / +2 more, every turn |
| **You hit it with an Electric-type move** | **+1**, once per turn |

**Nothing lowers it.** Only the overload itself resets it to 0.

### Your lever: Electric moves

Rotom is Electric-typed in every one of its forms, so an Electric attack is the **worst damage you
can pick** against it — 0.5x, and 0.25x against Mow.

That is exactly the point.

> **Spend a turn dealing almost nothing to crank the meter, and cash it for three turns at guard 55
> where your real attacks land.**

Feed it too eagerly and you have spent the fight doing chip damage. Never feed it and you sit at
guard 93 waiting on its own clock. (It reads the move's *printed* type, so a Normalize'd or
Tera-typed move does not count.)

### The five possessions

It takes a new appliance every **4 turns**, then every **2**, then **every turn**. It never repeats
the one it is already wearing, and which one comes next is not fully predictable.

| Form | Signature | What it does |
| --- | --- | --- |
| **Heat** | Overheat | Harsh sunlight, Sp. Atk +1 — **and +1 extra OVERCLOCK every turn.** The form you want to see |
| **Wash** | Hydro Pump | Rain, **scours every screen off the field — both sides** — and heals itself 6% a turn |
| **Frost** | Blizzard | Snow (so Blizzard never misses), your Speed −1, its Def and Sp. Def +1 |
| **Fan** | Air Slash | Tailwind, Speed +2, evasion +1 — **and it strikes for 8% before you act** |
| **Mow** | Leaf Storm | Grassy Terrain (which heals **you** too), Def +1 |

Each possession rebuilds its stat stages from scratch, so nothing it stacked in the last form carries
into the next one — and nothing *you* stripped off it does either.

#### The reboot

> **Every possession clears Rotom's status.**

This is *not* a status immunity. Sleep, paralysis, burn and Toxic all land on Rotom, fully, and buy
you real turns. They just get flushed with the old appliance.

So a status has a shelf life of **4 turns**, then **2**, then **1** — and a sleep landed right *after*
a possession is worth three times one landed just before it.

### The three hijacks

Between possessions it seizes one of your systems. All three rotate, so you see all three before any
repeats.

| Hijack | What it takes | Your out |
| --- | --- | --- |
| **CONTROL** | The move you used last stops answering, for 4 turns | Use something else — or switch, which clears it |
| **POWER** | Your held item is shorted out for 5 turns | Wait it out |
| **FIELD** | A Sea of Fire or a Swamp on your side for 4 turns, plus a Light Screen for itself | Its next **Wash** form scours the field bare — its own screen included |

**Every hijack is +1 OVERCLOCK.** Rotom cannot take anything from you without cooking itself, and
that is what turns the endgame's chaos into your win condition.

Nothing traps you in this fight. Switching out is real counter-play, and it clears a hijacked move.

### SYSTEM OVERLOAD

At OVERCLOCK 5 it blows out.

```
  It is thrown out of the appliance:  back to base Rotom, its signature move gone
  The field goes with it:             weather, terrain, its Tailwind, all cleared
  Its stats crater:                   Sp. Atk -2, Def -2, Sp. Def -2
  Its guard falls:                    93  ->  55
  It loses a turn outright, then two more turns with the guard down
```

*"Rotom is still trying to bring itself back up."* runs every turn the window is open.

Base Rotom is genuinely weak — its last move slot falls back to **Astonish**, 30 power off a 65
Attack stat. Losing the possession is losing the fight's damage. **This is the window you are meant
to win in.** Then it finds a socket, slides back in, and the loop starts over from 0.

### Phase 1 — IDLE MACHINE (100% – 50%)

Guard 88 (93 once it hardens). A possession every 4 turns, one hijack per cycle. OVERCLOCK rises only
from its own actions — and from whatever you feed it.

A player who never touches the Electric lever sees roughly one overload every three possessions.

Its base kit is **Thunderbolt, Shadow Ball, Confuse Ray** plus whatever the current form loaded into
its fourth slot. Confuse Ray next to Blizzard and Overheat is a real threat at the level cap.

### Phase 2 — ROGUE PROGRAM (50% and below)

- Your screens come down on the way in.
- Possession every **2** turns; a hijack on every turn one doesn't fire.
- **+1 OVERCLOCK passively, every turn** — it is coming apart on its own now.
- Guard 88 → 90.

### Phase 3 — TOTAL SYSTEM TAKEOVER (25% and below)

- Possession **every turn**. Hijack **every turn**.
- **+2 OVERCLOCK passively, every turn.** With the possession and the hijack on top, the meter maxes
  within about two turns, every time.
- Guard 92.

#### CRITICAL SYSTEM FAILURE

The same overload, with the numbers this band needs: **guard 40**, and **four** turns of window
instead of three.

That means the 25%-to-10% band is mostly *window*. You do not have to out-damage the takeover — you
have to **survive two turns of it**, and then it hands you the fight.

### Catching it

Poké Balls are **blocked** for the entire fight.

At **10% HP in the final phase**, Rotom stops being able to hold a shape. Everything the fight built
comes off — the field, the hijacks, its stat stages — and the balls unlock:

> *"Rotom flickers, and cannot hold a shape any more. Now — now is the moment to catch it!"*

From that point it cannot be knocked out, so take as many throws as you need.

### The short version

- Its guard is the whole fight. 88 → 93 → **55**.
- **Electric moves are your button.** They deal nothing and buy everything.
- Status works, but only until the next appliance.
- Heat form is your friend. Mow form heals you. Fan form hits first. Wash form cleans its own screen
  off the field.
- Below 25% it kills itself faster than you can. Just live through the wall.
