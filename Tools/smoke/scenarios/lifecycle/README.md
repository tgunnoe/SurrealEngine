# Lifecycle runs

These runs drive stock maps of Unreal Gold 226b, Unreal Tournament 436 and Deus Ex 1112fm to show what happens to an item's effect when the item destroys itself while it is in its `Activated` state. `UActor::Destroy` now runs the current state's `EndState` once, after the actor is marked deleted and before `Destroyed`, so the effect the state started is undone while the owner is still set. The state is not left, because stock script such as Unreal Tournament's `Razor2` calls a state function after destroying its actor.

The original game's code was not read. That the original runs a state's `EndState` when an actor is destroyed in it is inferred from the stock scripts (Unreal Tournament's invisibility and Deus Ex's charged items undo their effect only there, and Unreal Tournament's `UDamage` guards its expiry function against the re-entry that only such an engine produces). A hand test of the original Unreal Gold game agrees: the Invisibility tint fades away a few seconds after the item runs out.

## Recipe

Every run is an unattended smoke run with the same flags, no `--user-dir` (the engine then writes nothing into the game folder):

```
SurrealEngine --autostart --exit-after=<seconds> --smoke-log=<log> --url=<map> \
    --exec='<items>' --trace-script=<entries> "<game folder>"
```

Tools/smoke/README.md describes the flags. What these runs add to them:

- `set <class> <property> <value>` changes a class default, so it shortens the charge of an item created after it with `summon` and cannot reach an item the map already placed. The Unreal Tournament recipes first enter `IAmTheOne`, the cheat that makes `summon` work there.
- The engine log (`SE-Log-LastRun.txt`, written when the engine exits) holds one `trace:` line per traced call, stamped with level time. The `--exec` seconds are wall seconds after the first `LOGIN`; the stamps a run is read by are level time.
- A run is read by finding the item (the object of the first `UsedUp` line in the `Activated` state), taking the expiry window from its first `Destroy()` line after that `UsedUp` to its next `Destroyed()` line, and counting the lines inside it: `EndState`, the reversing call, `Invisibility(false)`, `SetDefaultDisplayProperties` on the owner, `ChargedPickupEnd`. A traced line shows that a call began, not that its body ran, so an `EndState` line alone is never enough: each recipe also needs a line the body makes.

The four expiry recipes (`<game folder>` is the install of the game):

| Run | Map | `--exit-after` | Exec | Trace |
| :--- | :--- | ---: | :--- | :--- |
| Unreal Gold, Invisibility expires | `Vortex2` | 40 | `4:set UnrealI.Invisibility Charge 6;5:summon UnrealI.Invisibility;6:hold axis aForward 300 1.5;10:ActivateItem` | `Invisibility,PlayerPawn.ClientAdjustGlow,Pickup.UsedUp` |
| Unreal Gold, switched off by hand (control) | `Vortex2` | 45 | `4:set UnrealI.Invisibility Charge 30;5:summon UnrealI.Invisibility;6:hold axis aForward 300 1.5;10:ActivateItem;16:ActivateItem` | the same |
| Unreal Tournament, invisibility expires | `DM-Deck16][?NumBots=0` | 40 | `3:Fire;4:set BotPack.UT_Invisibility Charge 6;5:IAmTheOne;6:summon Botpack.UT_Invisibility;7:hold axis aForward 300 1.5;11:ActivateItem` | `UT_Invisibility,Pawn.SetDefaultDisplayProperties,Pickup.UsedUp` |
| Unreal Tournament, `UDamage` expires | `DM-Deck16][?NumBots=0` | 40 | `3:Fire;4:set BotPack.UDamage Charge 10;5:IAmTheOne;6:summon Botpack.UDamage;7:hold axis aForward 300 1.5;11:ActivateItem;16:call SetDefaultDisplayProperties` | `UDamage,Pawn.SetDefaultDisplayProperties,Weapon.SetDefaultDisplayProperties,Pickup.UsedUp` |

The Deus Ex recipe, in `00_TrainingFinal` with `--exit-after=110`, picks up the `AdaptiveArmor` that lies at (1025.1, 192.7, -98.1) by standing 43 units short of it, facing it, and frobbing:

```
4:place 1025.1 149.7 -98.1 90;6:ParseRightClick;8:call PutInHand name:AdaptiveArmor0;12:ParseLeftClick
```

with the trace `ChargedPickup.GiveTo,ChargedPickup.PostBeginPlay,ChargedPickup.Activate,ChargedPickup.BeginState,ChargedPickup.ChargedPickupBegin,ChargedPickup.Timer,ChargedPickup.UsedUp,ChargedPickup.ChargedPickupEnd,ChargedPickup.EndState,ChargedPickup.Destroy,ChargedPickup.Destroyed,ChargedPickup.SetOwner,DeusExPlayer.ParseRightClick,DeusExPlayer.HandleItemPickup,DeusExPlayer.PutInHand,DeusExPlayer.ParseLeftClick`. The charge is the class default, so the item expires near level time 24.5 in these runs. The item is judged only when it left the hand before it expired (a one-use charged item does; one still in hand runs `ChargedPickupEnd` from `UsedUp` itself and says nothing about this fix).

## Expiry runs

`Previous binary` is the build before the change, `Fixed binary` the build with it. The Unreal Tournament rows of the previous binary were not re-run: they are read from the logs of earlier traced runs of that same build (`DM-Deck16][`, the same recipes), which the engine source has not moved from.

| Run | Previous binary | Fixed binary |
| :--- | :--- | :--- |
| Unreal Gold Invisibility expires (item `Invisibility0`) | 1 activation `ClientAdjustGlow(-0.15, ...)`; at level time 13.500 `UsedUp`, `Destroy`, `Destroyed`, `SetOwner(None)`; 0 reversing `ClientAdjustGlow(0.15, ...)`, 0 `Invisibility(false)`, 0 `EndState` | the same activation; at 13.500 in one frame `Destroy`, `EndState`, `Invisibility(false)`, `ClientAdjustGlow(0.15, ...)`, `Destroyed`, `SetOwner(None)`, in that order, each once |
| Unreal Gold, switched off by hand | activation 1, reversing 1, `EndState` 1, `GotoState('DeActivated')` 1, `UsedUp` 0, `Destroy` 0, `Invisibility(false)` 1 | the same seven counts, unchanged |
| Unreal Tournament invisibility expires | `UsedUp` 1, `Destroy` 1, `Destroyed` 1; 0 `EndState`, 0 `SetDefaultDisplayProperties` on the owner | `UsedUp` 1, `Destroy` 1, `Destroyed` 1; 1 `EndState`, then `SetDefaultDisplayProperties` on the owner (`TMale20`), before `Destroyed` |
| Unreal Tournament `UDamage` expires | `UsedUp` 1, `Destroy` 1, `Destroyed` 1; 0 `EndState` | 1 `EndState` before `Destroyed`; `UsedUp` 2, `Destroy` 2, `Destroyed` 1 |
| Deus Ex `AdaptiveArmor` expires, out of hand | `UsedUp` 1, `Destroy` 1, `Destroyed` 1; 0 `ChargedPickupEnd`, 0 `EndState` (the item left the hand before it expired) | 1 `EndState` (two lines, since its `Super` call is traced too) with `ChargedPickupEnd(JCDentonMale0)` inside it, before `Destroyed` |

`UDamage`'s `EndState` body is one call of `UsedUp()`, and the engine does not enforce the `singular` modifier that `UsedUp` is declared with, so the body runs a second time from the `EndState`. The second `UsedUp` is the evidence that the `EndState` body ran. The second `Destroy` returns at once, because the actor is already marked deleted, which is why `Destroyed` stays at 1. The owner's weapon has its display reset twice at that stamp (once per `UsedUp`). That the `singular` modifier is not enforced is left as it is here.

Each fixed run ended with `script_errors=0 stub_hits=0 missing_natives=0 exceptions=0 script_warnings=0`, except the Deus Ex run, which has `stub_hits=1` as its previous-binary run and the committed Deus Ex rows do. The previous binary's runs have the same counters.

## Regression sample

Fifteen stock maps, 60 seconds each, on the fixed binary, as the committed sweeps ran them (`--url=<map>`, no other option, except that `AS-Frigate` and `AS-Overlord` ran as `?game=Botpack.Assault` and `CTF-Face` as `?game=Botpack.CTFGame`), with the trace `Actor.Destroy,Actor.EndState`. Each run is judged with the sweep runner's own `parse_log` and `classify` against the committed row of its map. All fifteen match in the five compared fields (verdict, script errors, exceptions, missing natives, stub hits); script warnings are not compared.

| Game | Map | Verdict | Script errors | Exceptions | Missing natives | Stub hits | Script warnings (committed, run) |
| :--- | :--- | :--- | ---: | ---: | ---: | ---: | :--- |
| Unreal Gold | Vortex2 | pass | 0 | 0 | 0 | 0 | 0, 0 |
| Unreal Gold | Dig | pass | 0 | 0 | 0 | 0 | 10, 18 |
| Unreal Gold | NyLeve | pass | 0 | 0 | 0 | 0 | 0, 9 |
| Unreal Gold | Chizra | pass | 0 | 0 | 0 | 0 | 17, 27 |
| Unreal Gold | Harobed | pass | 0 | 0 | 0 | 0 | 15, 6 |
| Unreal Tournament | AS-Frigate | pass | 0 | 0 | 0 | 0 | 0, 0 |
| Unreal Tournament | DM-Deck16][ | pass | 0 | 0 | 0 | 0 | 0, 0 |
| Unreal Tournament | AS-Overlord | pass | 0 | 0 | 0 | 0 | 0, 0 |
| Unreal Tournament | DM-Codex | pass | 0 | 0 | 0 | 0 | 0, 0 |
| Unreal Tournament | CTF-Face | pass | 0 | 0 | 0 | 0 | 0, 0 |
| Deus Ex | 00_Training | pass | 0 | 0 | 0 | 1 | 0, 0 |
| Deus Ex | 01_NYC_UNATCOIsland | pass | 0 | 0 | 0 | 1 | 0, 0 |
| Deus Ex | 02_NYC_BatteryPark | pass | 0 | 0 | 0 | 1 | 0, 0 |
| Deus Ex | 03_NYC_UNATCOHQ | pass | 0 | 0 | 0 | 1 | 0, 0 |
| Deus Ex | 04_NYC_Hotel | pass | 0 | 0 | 0 | 1 | 0, 0 |

The Unreal Gold warning counts vary from run to run and are not a regression signal. One logged Accessed None site has no row in the register under its scenario heading: `Slith.TakeDamage` (once, in the Chizra run). No other sample run and the botmatch below logged a new site.

What the sample exercised: the 15 runs made 2164 `Destroy` calls, and 0 of them raised an `EndState`. The sample never destroyed an actor that was in a state with an `EndState`, so it shows that the change does not disturb the ordinary destroys of these maps and says nothing more.

Botmatch guard: `DM-Deck16][?game=Botpack.DeathMatchPlus?NumBots=4?Difficulty=3`, `--exit-after=300`, `--exec=5:Fire`, trace `GameInfo.Killed,GameInfo.ScoreKill,Pawn.Died,Pawn.KilledBy`. The run logged 100 `GameInfo.Killed:` lines (an earlier measurement of the same recipe had 18; the guard is at least 9), the player was the victim of 1 of them, and the run ended with `script_errors=0 stub_hits=0 missing_natives=0 exceptions=0 script_warnings=6`.

## What the runs do not cover

- No pixel is read. The tint's removal is shown by the reversing `ClientAdjustGlow(0.15, ...)` call in the trace, not on screen.
- The predicted effects on Unreal's Dampener, SCUBA gear, Flashlight and Energy Amplifier, which have the same shape as Invisibility, are not run.
- Deus Ex was observed for one item, `AdaptiveArmor`, picked up and used in `00_TrainingFinal`; the other charged items are not run.
- The sample raised no `EndState` from a destroy (above); no run reached the trace cap of 20000 calls.
- An `EndState` that destroys its own actor from inside a `GotoState` would have the state's `EndState` raised a second time by the nested `Destroy`; no stock script does this and no run reaches it.
- The original's behaviour is inferred from the stock scripts and one hand test, not from its code.
