## Regirock — The Ancient Fortress

Regirock fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage.

Groudon is a ratchet you pry back down. Regice takes your turns away. Regirock is a **wall you have
to pick the right tool for** — and the wrong tool makes it thicker.

### The core loop

```
FORTIFICATION runs 0 to 5.  Every layer is a different, VISIBLE
defensive property, not just a bigger number:

  1  Stone Skin         a little harder to damage
  2  Reinforced Core    critical hits stop landing
  3  Stone Shell        physical damage is halved
  4  Ancient Fortress   stat drops are refused
  5  Mountain Form      and it braces, on a telegraph

It goes UP when you hit it with something it does not fear -
any PHYSICAL move that is not one of its weaknesses - when it
lands a ROCK move or uses CURSE, and every turn you fail to
hurt it.

It comes DOWN for one reason: a damaging move of a type ROCK
is weak to.  WATER, GRASS, GROUND, FIGHTING, STEEL.

Only ONE move-driven change lands per turn, in either
direction.  Whoever gets there first spends it.
```

**The break lever does not care how hard you hit.** A 4-power Bubble strips a layer exactly as well
as a Hydro Pump. That is the safety valve: however thick the wall gets, and however little damage
you are doing through it, your ability to take it apart never degrades.

The corollary is the trap. Leading with your biggest neutral physical attack is *helping it*. The
mountain packs tighter every time it is struck by something it does not fear.

### Reading the wall

Regirock never tells you a number. It tells you every single time the Fortification moves:

> *Stone drags itself onto Regirock's body!* → a layer went up
> *The rock around Regirock crumbles away!* → a layer came off

And it names the tier as you cross into it, **in either direction** — so you see the same lines
again on the way back up if you let it rebuild:

| The line | Fortification | What it means |
| --- | --- | --- |
| *(no line)* | **0** | Bare stone. Its softest guard — and it has nothing to throw |
| *Regirock's skin has set like stone.* | **1** | Guard climbing |
| *…core is packed too tight to find a weak point.* | **2** | **Your critical hits stop landing** |
| *A shell of rock closes over Regirock.* | **3** | **Physical damage is halved** |
| *Regirock is a fortress now.* | **4** | **Stat drops are refused** |
| *Regirock has become a MOUNTAIN.* | **5** | It braces every third turn |

While the wall is at 3 or higher, a line repeats every turn so you cannot miss that something is
wrong:

> *Regirock's plating grinds and settles.*

### The quiet turn

At the top of every turn, Regirock checks how much you took off it over the previous one. Less than
**2% of its max HP** — including none at all — and it gains a layer:

> *The dust settles, and Regirock's stone knits together.*

This is the clock in this fight, and it is not on a timer: **passivity is what builds the wall**.
The bar is low on purpose — almost any real attack clears it. What it punishes is a turn where you
did nothing to Regirock at all: setting up, stalling, healing, switching, missing. A turn it spent
braced doesn't count against you.

### The reconstruction

Every few turns Regirock **spends its action** rebuilding itself, announced at the top of the turn:

> *Regirock stops attacking and begins reconstructing its body!*

That turn it does not attack — it is a free turn, handed to you. What you do with it decides what it
costs:

| What you do | What happens |
| --- | --- |
| Take **3%+** of its max HP off that turn | *The battering shattered the reconstruction!* — it **loses** a layer |
| Anything less | *Regirock is whole again!* — it gains a layer and heals 3% |

That is a two-layer swing on one turn. The bar is low enough that any genuine attack clears it, so
the decision isn't whether you can burst — it's whether you **spend** the free turn on Regirock at
all, or take it for your own setup and hand back a layer for the privilege.

The cadence is every 4th turn to start, every 3rd from the halfway mark.

### Phase 2 — Mountain's Wrath (≤ 50%)

> *Regirock's body is breaking apart. It stops defending, and starts throwing the pieces!*

It stops hiding behind the fortress and starts **hurling it at you**. Its Attack goes up once,
permanently, and **Rockfall** begins on a cadence — every 3rd turn here, every 2nd in the last
phase:

> *Regirock tears off a slab and hurls it!*

| Fortification when it fires | Damage to your whole party |
| --- | --- |
| **0** | **refused** |
| 1–2 | 4% |
| 3–4 | 8% |
| 5 | 12% |

Rockfall **spends a layer** to fire, and it can only fire while it *has* one:

> *Regirock reaches for a slab to throw — there's nothing left!*

**This is the payoff for the whole break-the-wall game.** A stripped fortress has nothing to throw.
From here, keeping the Fortification at 0 is offence as well as defence — and because the break
lever works at any damage level, a Water- or Grass-type with a weak move and decent Speed can hold
the wall at zero all by itself.

Every Rockfall also permanently shaves 3% off Regirock's raw Defense. That one is silent and it
never reverses, so the more it throws, the easier it is to hurt — and letting it throw from a thick
wall is the fastest way to bleed that Defense down, if you can afford the hits.

### Phase 3 — Collapse (≤ 25%)

> *Regirock can barely hold itself together. The whole cavern starts coming down.*

The mountain starts falling apart on its own. Every turn:

> *Another piece of Regirock breaks away!*

A layer sheds by itself and the debris does 8% to your whole party — while the reconstruction is
still running on its 3-turn cadence, pulling the other way. Rockfall now fires every other turn. It
is the most dangerous stretch of the fight by a distance.

And a **fixed 5-turn clock** is running. When it lands, so does everything else:

> *MOUNTAIN'S COLLAPSE! Regirock brings everything down at once!*

Your screens, Safeguard, Mist, Tailwind and Lucky Chant are all stripped, your party takes 30%, and
Regirock drops itself to **8% HP**. That is an absolute destination, not damage — **the catch window
always opens**, no matter how the damage race went. Surviving the collapse is the only thing you
have to get right.

### Building for it

Regirock is **80 / 100 / 200 / 50 / 100 / 50**, pure Rock. That 200 base Defense, plus Reflect from
Fortification 3, is why the physical route grinds — and its Special Defense is half that.

The damage reduction itself climbs only slightly as the wall thickens. **The layers hurt you through
their properties, not through a rising number** — losing your crits, then half your physical damage,
then your stat drops, then a turn in three. Stripping a layer is worth far more than the raw
reduction it removes.

**Bring one of the five breaker types.** Water, Grass, Ground, Fighting or Steel. You need it for
the wall regardless of what it does for damage, and the two special ones (Water, Grass) are hitting
the weaker side of a Pokémon whose Defense is its whole identity.

**Don't lead with a big neutral physical hit.** Normal, Flying, Psychic, Dark, Fire, Ice, Electric,
Bug, Poison, Ghost, Dragon, Fairy — every physical attack of those types is a free layer for it.
Special attacks of those types are safe; they simply do nothing to the wall either way.

**Its moveset punishes the obvious answers:**

| Move | Why it's there |
| --- | --- |
| **Stone Edge** | Rock STAB — and it **builds a layer** when it lands, so you can watch the rule work from the other side |
| **Earthquake** | For the Steel and Fire answers people bring to a Rock boss. "Just wall it" is not a plan |
| **Hammer Arm** | Fighting coverage into the Steel, Ice and Normal walls that Rock + Ground can't break |
| **Curse** | +Atk/+Def/−Spe — and **every cast feeds the Fortification** |

**Sleep does not work, and it costs you.** Regirock throws it off and thickens for the trouble:

> *Regirock doesn't sleep. The stone only settles harder.*

Everything Regirock does — Rockfall, Curse, the reconstruction, its whole offence — is an *action*,
so a permanently slept Regirock would be a statue. Paralysis and burn are left alone, and burn's
chip damage is one of the few things that quietly beats the quiet-turn clock.

**Lower its stats early or not at all.** It does **not** have Clear Body here — Screech, Icy Wind
and Intimidate all land normally. But Fortification 4 raises Mist and refuses stat drops outright,
so that lever is only available while you're keeping the wall down. Rockfall's Defense loss is a
raw-stat change and ignores Mist entirely.

**Critical hits stop at Fortification 2.** If you were building a fight plan around Focus Energy or
a high-crit move, it has a two-layer shelf life.

### Catching it

You cannot throw a Poké Ball for almost the entire fight. The window opens only in the final phase,
at **10% HP or less**, and Regirock tells you when:

> *The rubble settles, and Regirock lies exposed.*
> *It's worn out — now is the moment to catch it!*

Everything releases at once: the Fortification goes to 0 and takes Reflect, Lucky Chant and Mist
with it, every clock stops, the damage reduction and the type-effectiveness cap come off, and Poké
Balls are unlocked. From that point the game keeps the catch target alive for you — you cannot
accidentally KO it out of the window.
