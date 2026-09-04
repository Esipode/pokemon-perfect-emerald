## Manaphy — The Prince of the Sea

Manaphy fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

It keeps its own ability, **Hydration**. That does nothing at all for the first half of the fight —
and then it does a great deal.

Its Pokédex entry is the whole design:

> *"It starts its life with a wondrous power that permits it to bond with any kind of Pokémon."*

So it bonds with yours. Immediately, unasked, and it will not stop.

> **You cannot win this fight while it is bonded to you. There is exactly one answer and it is the
> cruel one: send your Pokémon away, break its heart, and hit it while it is falling apart.**

### THE BOND

One hidden number runs the whole battle. It climbs **by itself**, one step at the end of every turn
your Pokémon stays in. Nothing you do makes it climb faster and nothing you do slows it down.

```
   0          1            2           3          4           5
 severed  acquainted   attached    devoted   entwined     AS ONE
   |          |            |           |          |           |
 the       -             -         Heart Swap  the rain    nothing
 window                              unlocks    feeds it    reaches it

 guard 78 -- 84 -------- 88 ------- 90 ------- 92 -------- 94
```

At **94** you are dealing six percent of your damage. From 50% down, Manaphy heals more than that
back every turn. There is no move, no matchup and no status that gets around it — the arithmetic
simply does not work.

Nothing on screen ever shows you the number. What it shows you is **the bond**, and the bond
describes the number honestly, every step of the way:

| | Manaphy says |
| --- | --- |
| **5 — as one** | *"MANAPHY IS AS ONE WITH YOUR POKéMON. Nothing you throw at it seems to reach it any more."* |
| **4 — entwined** | *"Manaphy and your Pokémon are breathing in time."* |
| **3 — devoted** | *"Manaphy moves when your Pokémon moves. The water between them has gone still."* |
| **2 — attached** | *"Manaphy will not take its eyes off your Pokémon."* |
| **1 — acquainted** | *"Manaphy drifts a little closer to your Pokémon."* |
| **0 — severed** | *"Manaphy is thrashing at the water where your Pokémon was! It has stopped defending itself."* |

Every crossing is announced. Underneath them, from Bond 3 on, a short line keeps running every other
turn — *"Manaphy has not looked away from your Pokémon once."*

### THE ONLY LEVER: switch out

**Switching your Pokémon out severs the bond.** So does letting it faint — a replacement is a
switch-in either way.

Severing does three things at once:

- The guard falls from wherever it was to **78**, for the turn you switch and the two after it.
- **Every stat stage Manaphy has accumulated is wiped.** That is why the state below is survivable.
- Manaphy becomes **HEARTBROKEN**: **+2 Sp. Atk, +2 Speed, −1 Def, −1 Sp. Def.** It hits far harder,
  moves far faster, and stops defending itself.

When the window closes it finds your Pokémon again and the meter restarts at 1.

Switching *again* while it is already heartbroken is free — no second cost, and the window does not
reset.

### THE PRICE: your party is the network

Manaphy does not forget. **The Pokémon you send in inherits the broken bond**, and how hard that
lands depends on how deep the bond was when you cut it:

| Bond when you severed | What the newcomer pays |
| --- | --- |
| **0–1** | nothing — it barely knew you |
| **2–3** | **8%** of its max HP, −1 Sp. Def |
| **4–5** | **15%** of its max HP, −1 Sp. Def, −1 Speed |

So the loop is **bond → sever → echo → bond**. Each lap buys you damage and costs you a Pokémon's
worth of health. Severing *early and shallow* is nearly free but gives you fewer turns per lap;
severing at Bond 5 costs a sixth of a Pokémon before it acts. **The fight is asking how many laps
your bench can afford** — and it keeps the receipt for Phase 3.

### THE SEDUCTION: Heart Swap

From Bond 3, every second or third turn, Manaphy does this:

> *"Manaphy shared its strength with your Pokémon!"* — you heal **25%**, your status is cured, and you
> gain **+1 Attack** and **+1 Sp. Atk**
>
> *"Manaphy took on its burden!"* — Manaphy takes **8%** of its own health

**Both halves are completely real.** There is no catch printed anywhere and at Bond 3 it is a
straightforwardly good trade for you.

The catch is that the gift is a **bond event**: on the turn Manaphy helps you, the meter climbs by
**two** instead of one. Accepting help is the fastest possible route to a Manaphy nothing can hurt.

A player who never works this fight out ends up at full health, at Bond 5, unable to move the health
bar, being healed by the thing they are trying to beat. That is the intended way to lose.

### 50% — HEART OF THE OCEAN

*"Manaphy sinks to the floor of the water, and the whole sea leans in after it."*

**TIDAL BOND.** Manaphy raises **permanent rain**, and it does four things at once:

- The Bond climbs **two steps a turn** instead of one.
- At **Bond 4+**, Manaphy heals **3% of its max HP every turn** — *"Manaphy draws the rain into
  itself."*
- **Hydration** starts scrubbing sleep, paralysis, burn and poison off it at the end of every turn.
- Surf gets its rain boost.

At Bond 4+ that 3% is more than a 92%-guarded Manaphy loses to almost anything you have. **If you
watch the health bar go the wrong way, that is the fight telling you that you are doing it wrong.**

**The counterplay is the weather.** Change it — sun, sand, hail, snow, anything — and:

> *"The rain thins, and Manaphy falters."*

The heal stops, Hydration stops, the doubling stops, and Manaphy's Sp. Def takes a real **−1**. Three
turns later it calls the rain back down. So the answer is repeatable but never permanent: you are
buying three good turns with one turn of setup, over and over, for as long as you can afford it.

**Rain Dance is not counterplay.** Rain is what it wants.

### 20% — OCEAN'S HEART

*"Manaphy opens every bond it has ever made, all at once."*

Two things change.

**The guard stops listening to the Bond.** It is pinned at **90** whatever the meter says — the
network holds Manaphy up now, not the bond with you. Severing here buys you nothing on its own.

**Your damage comes back at you**, scaled by how many of your Pokémon Manaphy has bonded with over
the whole battle:

| You dealt it | few bonds | many bonds |
| --- | --- | --- |
| **15%+** | you take 10% | you take **16%** |
| **8%+** | you take 6% | you take 10% |
| **3%+** | you take 3% | you take 5% |
| under 3% | nothing | nothing |

*"The water carries the blow back to your Pokémon!"*

**This is the bill for the first two phases arriving at once.** A player who never switched barely
notices it. A player who ran four laps of the sever loop is handing back a sixth of their health
every time they land a real hit.

**THE COLLAPSE.** One more switch, once, and the network shatters:

> *"Every bond Manaphy made comes apart at once."*

**Guard 74. No rain. No sharing. No stat stages. Four turns.** At 20% health, that is the kill
window, and it is the only reason this phase ends.

It is also the question the whole fight has been building to: **do you still have a Pokémon to switch
to?** If you spent your bench on the loop, you do not, and you will have to grind Manaphy down at
guard 90 while it hands your own damage back to you.

### 10% — WEAKENED

Balls unlock, its status clears, the rain goes, and the guard is gone.

**The catch rate is read off the Bond at the moment the window opens**, and it is the last real
decision of the fight:

| Bond when it fell | Catch rate | |
| --- | --- | --- |
| **4–5** | **200** | *"It has not let go of you, even now."* |
| **2–3** | **140** | *"It is watching you, and deciding."* |
| **0–1** | **90** | *"It has been sent away too many times to come willingly."* |

Burst it down inside a heartbreak window and it falls at Bond 0 — **the easy kill, the hard catch**.
Let it re-bond first and finish it at Bond 4+ — much harder to pull off, and it comes to you almost
willingly. The window itself is identical on every path; what the Bond buys is how many balls it
takes.

### Its moves

`Surf` · `Ice Beam` · `Energy Ball` · `Aqua Ring`

All legal Manaphy moves, played by the smart trainer AI. **Ice Beam is there for the Grass-type you
were about to lead with** — Manaphy is pure Water, so without it both of its weaknesses would be
free. Aqua Ring is its safe turn. It has no stat-boosting move at all.

### If you take nothing else away

- **Switching out is not running away. It is the attack.** Everything else is setup for it.
- **Sever early.** A shallow bond costs your bench almost nothing; a Bond-5 bond costs 15% and two
  stat stages.
- **Heart Swap is a trap wearing a gift.** Every one you accept moves the meter two steps.
- **From 50%, break the weather.** It is the only thing that stops the heal, the doubling and
  Hydration — and it is the only way to make status stick.
- **Count your switches.** They are the fight's currency in Phase 2 and its bill in Phase 3.
- **Save one switch for Ocean's Heart.** The collapse is the only thing that ends that phase.
- How you finish it decides how hard it is to catch.
