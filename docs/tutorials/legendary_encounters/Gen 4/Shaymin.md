## Shaymin — The Gratitude Pokémon

Shaymin fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** for the whole fight, so the catch window at the end is
guaranteed no matter how the damage race goes.

It keeps its own ability. Land Forme has **Natural Cure**, which never comes up in a battle it does
not switch out of. Sky Forme has **Serene Grace** — and that one matters a great deal.

Every other legendary in the set asks how hard you can hit. Shaymin asks **how you fight**, and then
becomes the answer.

> **Shaymin takes more than four times as much damage in a garden you kept alive as it does in one
> you poisoned. The peaceful path is not the polite path. It is the short one.**

### THE GARDEN

One hidden number runs the whole battle. It starts in the middle and moves **at most one step per
turn**, so it is a slope, not a switch.

```
    BARREN   WITHERING   STRAINED  |  SPROUTING   VERDANT   FULL BLOOM
      0        1    2      3   4   |  5  6    7    8    9       10
      |        \____ ____/    |    |     \___ ___/  \__ __/      |
   the field     SKY FORME    |  the start  Grassy    + Safeguard |
    catches     +Spe +SpAtk   |             Terrain    for you    |
                              |
  it takes    6%      6%     8%    10%      14%       20%        26%
  of what             <-- your damage, after its guard -->
  you deal it
```

Nothing on screen ever shows you this number. What it does show you is **the clearing**, and the
clearing describes the number honestly, every step of the way:

| | The field says |
| --- | --- |
| **Full Bloom** | *"THE WHOLE CLEARING OPENS AT ONCE."* |
| **Verdant** | *"The clearing is thick and green now…"* |
| **Sprouting** | *"Shoots come up through the dirt where Shaymin is standing."* |
| **Strained** | *"The flowers nearest Shaymin have started to curl."* |
| **Withering** | *"The gracidea is going brown from the edges in."* |
| **Barren** | *"NOTHING IS LEFT GROWING HERE."* |

Every crossing is announced, and underneath those a short line keeps running every other turn —
*"The clearing is still growing around Shaymin"* / *"…still dying around Shaymin."* If you missed the
crossing, that line is still there waiting for you.

### What moves it

Each turn the credits and debits are counted, and the Garden takes **one step** toward whichever side
won. A tie holds it still.

| Tends the garden (+) | Harms it (−) |
| --- | --- |
| A **Grass-type move** — yours, or Shaymin's own | A **status standing on Shaymin** — *every turn it lasts* |
| Your Pokémon **recovered real HP** this turn | A **Fire-** or **Poison-type** move you used |
| Your Pokémon's **status was cured** | **Entry hazards** you set — Spikes, Toxic Spikes, Stealth Rock, Sticky Web |

Two things worth knowing:

- **Shaymin's own Grass moves only count while the Garden is 5 or higher.** A withered field no
  longer answers it. Below the middle, only *you* can bring it back.
- **"Recovered HP" means a real heal.** Grassy Terrain's own trickle and Leftovers-sized chip are
  below the line and do not count. Spend a turn or an item on it and it counts.

Type is read off the **move's printed type**, not what it hit as — a Normalize'd or Tera'd attack
still counts as whatever the move data says it is.

### The trap

Everything in the Harm column is something a good competitive player reaches for first. Toxic. A
burn. A Fire coverage move. Hazards on the way in. All of them work — Shaymin has **no status
immunity anywhere** — and all of them cost you a step of Garden, *every turn they keep running*.

A slept Shaymin at Barren is a worse deal than an awake one at Full Bloom. Working that out is the
fight.

### The two formes

Not an HP threshold. The Garden decides it, and you can change your mind.

| | **Land Forme** | **Sky Forme** |
| --- | --- | --- |
| Appears at | Garden **4 or higher** | Garden **2 or lower** |
| Typing | Grass | Grass / Flying |
| Second move | Earth Power | **Air Slash** |
| Ability | Natural Cure | **Serene Grace** — Air Slash flinches **60%** of the time |
| Build | 100 / 100 defences | 75 / 75 defences, far faster, +Sp. Atk |

The gap between 2 and 4 is deliberate: at Garden 3 it keeps whichever forme it arrived in, so it
cannot flap back and forth at the boundary.

Sky Forme is 4x weak to Ice on paper. **The encounter caps incoming effectiveness at 2x**, so that
does not become the shortcut it looks like.

### The three beats

**50% — GRATITUDE.** Shaymin clears its own status (a status lock has a shelf life here, not a
lifetime) and reads the Garden **once**. What it reads it then *commits to*, and from that point the
field tends or rots itself a little more every turn, on its own, for the rest of the battle.

| It reads | What happens |
| --- | --- |
| Garden **7+** | *"It is grateful."* It heals a little; **your** Pokémon is healed 30% and cured. From here the field tends itself every turn |
| Garden **4–6** | *"Shaymin is still deciding what this battle is."* It heals, and nothing is committed |
| Garden **3 or less** | *"Shaymin cries out."* It heals, is driven into the withering band and into **Sky Forme**, and sets your side of the field alight. From here the field rots itself every turn |

The bias is not a lock. Two Tend sources in a turn still beat it — a distressed garden **can** be
brought back by someone who works out how. It just takes real, sustained work.

**20% — SEED FLARE.** Shaymin gains its signature move (it replaces Synthesis) and fires a scripted
one every third turn. What that Flare *is* is read **live, at the moment it fires** — not decided
back at 20%. Three good turns can still change what the next one does to you.

| Garden at fire time | The Flare |
| --- | --- |
| **7+** | Takes 10% off you, heals Shaymin a little, and puts the grass back up |
| **4–6** | Takes 18% off you |
| **3 or less** | Takes **28%** off you, strips every screen, Safeguard, Mist and Tailwind from your side, and drops your Sp. Def |

A 28% scripted hit every third turn, on top of a Sky Forme that already moves first, is the real
pressure of the withered endgame.

**10% — WEAKENED.** Balls unlock. This window is **identical on every path** — how you fought never
decides *whether* you get to catch it. Shaymin returns to Land Forme, drops its guard, clears its
status and stops fighting the way it was.

What the Garden buys is how many balls it takes:

| Garden | Catch rate |
| --- | --- |
| **7+** | **180** — *it settles into the grass in front of you and waits* |
| **4–6** | **120** — *it is spent, and watching you* |
| **3 or less** | **70** — *it will not come down, and it will not stop watching you* |

### Its moves

`Energy Ball` · `Earth Power` (`Air Slash` in Sky Forme) · `Leech Seed` · `Synthesis` (replaced by
`Seed Flare` at 20%)

All of them legal Shaymin moves, played by the smart trainer AI. Note that **Energy Ball tends its
own garden** — on a turn where you do nothing hostile, the Garden climbs by itself. Peace is this
battle's default state. You have to work at the other one.

### If you take nothing else away

- The garden is not decoration. It is your damage output.
- Poison, burn, fire and hazards all *work*, and all of them make the fight longer.
- Grass moves, real heals and cleansing your own team are the levers, and they are cheap.
- You cannot lose the catch window. You can absolutely make it cost you a bag of balls.
