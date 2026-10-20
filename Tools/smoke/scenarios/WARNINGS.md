# Accessed None register

The engine logs `Accessed None in <Class>.<Function>` when a script reads a
member of a None object, and the smoke harness keeps each one as a
`SCRIPT_WARNING` line. Each sweep's `SUMMARY.md` lists its distinct sites in a
"Warning sites" table. This register has one row for each row of those tables,
with the verdict of a reviewer who read the site against the stock scripts, and
below the tables the grounds for each site.

## Policy

An `Accessed None` warning is counted and reported by the sweep and never holds
a map's clean verdict (`warnings_fail_clean` is off in every scenario). Every
site is read once by a reviewer and its verdict entered here, so that a new
site is seen and read rather than absorbed into a total.

A site that a sweep table lists and this register lacks is unread: nothing is
known yet about whether the original engine would warn the same way.
The check command of each scenario, `python3 -I -B
Tools/smoke/triage/__main__.py Tools/smoke/scenarios/<name> check` fails for
every site that the sweep's `errors.json` records and that has no row under the
`###` heading of the same name as the scenario, so a new site fails the check
until it is read. It compares the site alone: the lines and maps columns are the
counts at the time the verdict was entered, are not compared, and are refreshed
when the register is next touched. A warning of another form than `Accessed None
in <site>` (`Accessed None when calling ...`) needs a row whose site cell is its
whole text. A map whose `errors.json` entry has no `warning_sites` contributes
nothing, so an `unknown` row in a sweep table needs a reader. A verdict is
entered by adding the row and its section.

## Verdicts

- `faithful`: the stock script reaches the same None, from its own data (a
  property or argument the stock script sets or leaves None), in the situation
  the sweep creates, so the original engine warns the same way. This does not
  say that the original raises the event that starts the call in the same
  instance; the Limits list names the events the engine raises that were not
  read against the original.
- `engine-side`: the engine supplied, or failed to supply, a value, an event or
  a step of script execution that the original would not, or would.
- `undecided`: the stock script and the engine's logs do not decide it; the
  site's section says what would. Two kinds recur: the None exists only if the
  engine does something whose original behaviour is the question
  (`HorseFly.meander`: running state code after `Destroy()`); and the None is a
  property (the replication info) of the pawn the engine itself hands to an
  event (`SeePlayer`), so whom the original hands it to decides it.

Where no traced run observed the statement, the function decides the row. A
function that offers one None operand (`BabyCow.PickDestination`,
`UTConsole.AddMessage`, `TriggerLight.Trigger`, `ShakeTrigger.Trigger`,
`LAM.Explode`) has the operand named by the stock text. A function with several
candidates (`UnrealCTFScoreboard.DrawTrailer`) is `faithful` only when the
stock text and the situation single out one None and the section says it is
inferred (the rated-game branch). Where the candidates depend on bot state
(three of the four `Roaming` classes), the row is `undecided`.

A verdict rests on the stock script text of the shipped packages and, for the
sites a run could show, on the engine log with its callstack and the
`--trace-script` lines. In the logs quoted below the leading number is the
engine's level time in seconds, and a callstack line `at
<Class>[.<State>].<Function> line <N>: <statement>` names a statement by its
disassembled text (a native frame has none). `N` is, in the stock script text,
the line of the first statement of a function with a body (the last line of the
declaration of a native one) or the first label of a state, so it is the line
of the statement named only when that is the first. In a quoted log line the
token after the number (`Pawn.Died:`, `UnrealShare.HorseFly:`, `<Surreal
Engine>:`) is the logger's source field, not the site.

## Sites by sweep

Rows follow each sweep's "Warning sites" table: the site is
`<RuntimeClass>.<Function or State>` as logged, lines are the lines logged over
all maps, maps is the number of maps. The per-map counts are in each site's
section.

### unreal

| Site | Lines | Maps | Verdict | Grounds |
| :--- | ---: | ---: | :--- | :--- |
| HorseFly.meander | 166 | 14 | undecided | On Bluff a fly lost its owner when the carcass swarm that spawned it was destroyed (inferred from the timing and the script); the stock `meander` state then calls `Destroy()` on the fly and the next two statements dereference `Owner`, and the engine runs them. Whether the original keeps running state code after `Destroy()` is not known. |
| SinglePlayer.Killed | 17 | 8 | faithful | The stock `SinglePlayer.Killed` subtracts 2 from `killer.PlayerReplicationInfo.Score` when a Nali dies, with no check, and the killer of a Nali can be a monster, which has no replication info, or None. |
| UPakShieldEffect.Tick | 6 | 2 | undecided | `UPakShieldEffect` is a class of `UPak.u` (Return to Na Pali), whose script is unread on this host: no dump of that package exists here. Its export table lists a `Tick` of its own, so the stock `ShieldBeltEffect.Tick`, which tests `Owner` for None, is not the body that ran. A dump of `UPak.u` would decide it. |
| SinglePlayer.ScoreKill | 5 | 2 | faithful | The `ScoreKill` that `SinglePlayer` inherits from `GameInfo` subtracts 1 from `Other.PlayerReplicationInfo.Score` when the killer is None or the victim itself, and a monster victim has no replication info. |
| BabyCow.PickDestination | 2 | 2 | faithful | The stock Ruins map places a `BabyCow` with no `mom`, and the stock `Grazing` state's `PickDestination` has no `return` after its `Mom == None` branch, so it falls through to `mom.Destination`. |
| Slith.TakeDamage | 1 | 1 | faithful | The callstack names `ScriptedPawn`'s `Patroling` state body, one of seven stock bodies that read `Enemy` unguarded; a Slith in `Patroling` has no enemy, and the call is a BiterFish's stock `Pawn.BaseChange` stomp of the Slith. The engine raises `BaseChange` from its own physics, the one part not read against the original. |
| UPakTransitionInfo.ScoreKill | 1 | 1 | undecided | `UPakTransitionInfo` is a class of `UPak.u` (Return to Na Pali), whose script is unread on this host. Its export table lists no `ScoreKill`, so the body is the stock `GameInfo.ScoreKill`, which reads two things that can be None (the victim and its replication info); the sweep keeps no callstack to say which statement or who died. A plaintext engine log of an Inter2 run would decide it. |

### ut

| Site | Lines | Maps | Verdict | Grounds |
| :--- | ---: | ---: | :--- | :--- |
| UnrealCTFScoreboard.DrawTrailer | 3234 | 1 | faithful | The stock training game sets the rated-game flag, and the scoreboard's `DrawTrailer` then reads `RatedGameLadderObj.SkillText`, a ladder object that a direct map launch never assigns. Residual: the None operand is inferred by elimination, and the original's startup is unread. |
| UTConsole.AddMessage | 8 | 1 | faithful | The stock tutorial calls `AddMessage` on a console whose `MessageWindow` exists only after the menu has created the UWindow root, which a direct launch never does. Residual: the original's startup is unread. |
| MinigunCannon.SeePlayer | 4 | 1 | undecided | `MinigunCannon` inherits `TeamCannon`'s `Idle` `SeePlayer`, which reads `Pawn(SeenPlayer).PlayerReplicationInfo.Team` unguarded; the engine raises `SeePlayer` for each live pawn in sight, and in a traced AS-HiSpeed run of `TeamCannon` the seen pawn was another cannon. Whom the original raises it for is not known; no run traced it on CTF-High. |
| TeamCannon.SeePlayer | 4 | 3 | undecided | In the one traced warning (an AS-HiSpeed run with bots; none of these four lines was traced) the seen pawn was another `TeamCannon`, which has no replication info, so the stock `Idle` `SeePlayer` reads None; the engine raises `SeePlayer` for each live pawn in sight, and whom the original raises it for is not known. |

### ut-bots

| Site | Lines | Maps | Verdict | Grounds |
| :--- | ---: | ---: | :--- | :--- |
| UnrealCTFScoreboard.DrawTrailer | 3190 | 1 | faithful | As in the `ut` table: the same stock training game and scoreboard, with the same residual. |
| AssaultRandomizer.SpecialCost | 20 | 1 | undecided | The callstack names the one statement of the stock `CostEnabled` `SpecialCost`, called from the engine's path search for a bot. In the traced run only one of four bots warned (996 lines against 18 in the sweep of 73df9f26 and 20 in the sweep of 8d225ef3), so by differential the None is that bot's own `PlayerReplicationInfo`; why it has none is not known. |
| TMale1Bot.Roaming | 18 | 2 | undecided | The same `Bot` `Roaming` body; no traced run logged this class's statement. |
| TMale2Bot.Roaming | 8 | 2 | faithful | The callstack names `TurnTo(Location + AmbushSpot.lookDir)` at `Bot`'s `LongCamp` label, with the bot's own `AmbushSpot` None; only script assigns `AmbushSpot`, and the stock `PickDestination` sends a bot standing at an ambush point to `LongCamp` without assigning one. |
| UTConsole.AddMessage | 8 | 1 | faithful | As in the `ut` table: the same stock tutorial and console. |
| MinigunCannon.SeePlayer | 5 | 1 | undecided | As `MinigunCannon.SeePlayer` in the `ut` table. |
| TeamCannon.SeePlayer | 5 | 3 | undecided | As in the `ut` table: the one traced warning (AS-HiSpeed) saw a cannon, which has no replication info; AS-Mazon and AS-OceanFloor were not traced, and whom the original raises `SeePlayer` for is not known. |
| TFemale2Bot.Roaming | 1 | 1 | undecided | The same `Bot` `Roaming` body; no traced run logged this class's statement. |

### deusex

| Site | Lines | Maps | Verdict | Grounds |
| :--- | ---: | ---: | :--- | :--- |
| TriggerLight.Trigger | 3 | 1 | faithful | The stock `CameraPoint` command that triggers every actor of an event name calls `A.Trigger(None, None)`, and `TriggerLight`'s `TriggerToggle` state does `SavedTrigger = Other` and `SavedTrigger.BeginEvent()` with no check. |
| LAM.Explode | 2 | 1 | faithful | The stock `ThrownProjectile` `Flying` `Explode`, which `LAM` inherits, casts its owner to `DeusExPlayer` and dereferences it without a check, so a LAM whose owner is not the player reaches None. No traced run was made for this site, and what set the LAM off is not recorded. |
| ShakeTrigger.Trigger | 1 | 1 | faithful | The same `CameraPoint` call passes a None instigator, and the stock `ShakeTrigger.Trigger` does `Instigator.ShakeView(...)` with no check. |

### No longer logged

Sites an earlier committed sweep logged and the current one does not. The
lines and maps are `-` because no current sweep table lists them.

| Site | Lines | Maps | Verdict | Grounds |
| :--- | ---: | ---: | :--- | :--- |
| LaserEmitter.CalcTrace | - | - | engine-side | The stock script dereferences the actor a `TraceTexture` iterator yields, and the engine yielded a null actor for world hits where the script's own `target == Level` branch shows the original yields the level. Fixed by upstream commit 2c1a4f50; the re-run of the sweep (28f08a4c) no longer lists it. |
| ElectricityEmitter.CalcTrace | - | - | engine-side | The same iterator and the same dereference in the stock script, fixed by the same commit; the re-run (28f08a4c) no longer lists it. |
| PlayerStart.PlayTeleportEffect | - | - | engine-side | The engine bound ExtremeDGen's game to a level info the map saved as deleted, so the start spot's `Level.Game` was None. Fixed by commit 14090e6d, which takes the level's first actor as the level info; the re-run of the sweep (f94c1ba6) no longer lists it. |
| SkaarjBerserker.TakeDamage | - | - | undecided | Inherited from the same seven unguarded `ScriptedPawn` state bodies as `Slith.TakeDamage`, but it did not reproduce in two traced runs on ExtremeDark, so neither the state nor the damage source is known. The re-run of the sweep (f94c1ba6) did not log it either, and no engine change is known to account for that; the runs are not deterministic. |
| TFemale1Bot.Roaming | - | - | undecided | The same `Bot` `Roaming` body, which has several unguarded dereferences; no traced run logged this class's statement. The re-sweep (8d225ef3) did not log it, and no engine change is known to account for that; the runs are not deterministic. |

## Sites

### HorseFly.meander

Verdict `undecided`. Lines per map, `unreal`: Bluff 20, Ceremony 10, Chizra 14,
Crashsite2 8, DasaCellars 8, Dig 14, DuskFalls 12, ExtremeDark 8, ExtremeLab 14,
Harobed 6, Nevec 18, SkyCaves 10, SkyTown 14, TheSunspire 10.

Stock `UnrealShare.HorseFly`, state `meander`, label `wander`:

```
wander:
    if ( Owner == None )
        Destroy();
    if (!LineOfSightTo(Owner))
        SetLocation(Owner.Location);

    MoveTo(Owner.Location + HorseflySwarm(Owner).swarmradius * (VRand() +  vect(0,0,0.3)));
```

A fly's `Owner` is the swarm that spawned it. The stock `Engine.Carcass`
destroys the swarm of bugs it carries when it is destroyed itself, and
`UnrealShare.CreatureCarcass` spawns that swarm (a `DeadBodySwarm`) in
`AddFliesAndRats`:

```
    function Destroyed()
    {
...
        if (Bugs != None)
            Bugs.Destroy();
```

```
    function AddFliesAndRats()
...
            Bugs = Spawn(class 'DeadBodySwarm');
```

The engine's `UActor::Destroy` clears the `Owner` of every actor the destroyed
actor owns, so the flies of a destroyed swarm have no owner. The `wander` label
tests `Owner == None` and calls `Destroy()` but does not leave the state code,
and the next two statements dereference `Owner`.

In the sweep committed as 8fcddbd6 (94 lines on nine maps), six of the nine maps
(Bluff, Ceremony, Dig, ExtremeDark, SkyTown and TheSunspire) place no swarm, so
their flies belong to carcass swarms; Chizra, DasaCellars and Harobed (26 of its
94 lines) place swarms of their own, and which swarm orphaned the flies there
was not traced. The five maps the sweep of f94c1ba6 added to the site
(Crashsite2, DuskFalls, ExtremeLab, Nevec, SkyCaves) were not read for swarms.

Evidence: a run on Bluff, with `--trace-script` on `DeadBodySwarm.Destroyed`,
`DeadBodySwarm.Destroy`, `HorseFly.Destroy`, `HorseFly.SetLocation` and
`HorseFly.MoveTo`. The swarm is destroyed at 44.079052; at 44.648392 the fly
calls `Destroy()` (two `trace:` lines) and then each of the two statements logs
one warning. Three flies did this, six warnings in all. The log does not print
a fly's `Owner`; that the destroyed swarm was the fly's owner is inferred from
the timing and from the script:

```
[44.079052] CreatureCarcass.Destroyed: trace: DeadBodySwarm2 DeadBodySwarm buzzing: Destroy()
[44.079052] Carcass.Destroyed: trace: DeadBodySwarm2 DeadBodySwarm buzzing: Destroyed()
[44.079052] Actor.Destroy: trace: DeadBodySwarm2 DeadBodySwarm buzzing: Destroyed()
[44.648392] UnrealShare.HorseFly: trace: HorseFly2 HorseFly meander: Destroy()
[44.648392] UnrealShare.HorseFly: trace: HorseFly2 HorseFly meander: Destroy()
[44.648392] UnrealShare.HorseFly: trace: HorseFly2 HorseFly meander: SetLocation(None)
[44.648392] UnrealShare.HorseFly: Accessed None in HorseFly.meander
[44.648392] UnrealShare.HorseFly: at HorseFly.meander line 64: SetLocation(Owner.Location)
[44.648392] UnrealShare.HorseFly: trace: HorseFly2 HorseFly meander: MoveTo(?)
[44.648392] UnrealShare.HorseFly: Accessed None in HorseFly.meander
[44.648392] UnrealShare.HorseFly: at HorseFly.meander line 64: MoveTo(Add_VectorVector(Owner.Location, Multiply_FloatVector(dynamic cast(Owner).swarmradius, Add_VectorVector(VRand(), { 0.000000, 0.000000, 0.300000 }))))
```

The engine keeps stepping the state code after `Destroy()`: `Frame::Run` in
`SurrealEngine/VM/Frame.cpp` has no test for a deleted actor between
statements, and the comment in `UActor::Destroy`
(`SurrealEngine/Packages/Engine/Actors/UActor.cpp`) asks what should happen to
function calls after `Destroy()`. The stock script's shape (a `Destroy()`
immediately before unguarded uses of `Owner`) reads as written for a state
code that stops there, but that is an inference from the script, not an
observation of the original.

What would decide it: the original engine's rule for state code after
`Destroy()` (and that the original clears the `Owner` of a destroyed actor's
children, as `UActor::Destroy` does here; the script's own `Owner == None` test
is the sign read for it, and other shipped scripts test `Owner` for None the
same way without being checked as evidence), or a run of the original on a map
whose carcasses spawn flies (Bluff or Dig), reading its log for a `HorseFly`
warning after the carcass is gone. No warning in several runs would make these
lines engine-side; the runs are not deterministic, so one run settles nothing.

### SinglePlayer.Killed

Verdict `faithful`. Lines per map, `unreal`: Bluff 4, Ceremony 5, Chizra 2, Dig
1, Dug 1, SkyCaves 1, SkyTown 2, SpireVillage 1.

Stock `UnrealShare.SinglePlayer`, function `Killed`:

```
function Killed(pawn killer, pawn Other, name damageType)
{
    super.Killed(killer, Other, damageType);
    if ( Other.IsA('Nali') )
        killer.PlayerReplicationInfo.Score -= 2;
}
```

The statement runs only when a Nali dies, and `killer.PlayerReplicationInfo` is
None when the killer is a monster, which has no replication info, or None.
Evidence, from the engine log of the Bluff run described under "The runs": a
Krall's `ThrowDamageTarget` kills a pawn at 6.431971 (a monster killer; later
deaths at 13.949867 and 19.577259 come from `Krall.PoundDamageTarget` and
`Skaarj.ClawDamageTarget` the same way), and drowning damage with a null
instigator kills one at 35.471638 (a None killer):

```
[6.431971] Pawn.Died: Accessed None in SinglePlayer.Killed
[6.431971] Pawn.Died: at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2))
[6.431971] Pawn.Died: at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType)
[6.431971] Pawn.Died: at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation)
[6.431971] Pawn.Died: at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType)
[6.431971] Pawn.Died: at ScriptedPawn.MeleeDamageTarget line 1333: Target.TakeDamage(hitdamage, this, HitLocation, pushdir, hacked)
[6.431971] Pawn.Died: at Krall.ThrowDamageTarget line 888: AttackSuccess = MeleeDamageTarget(byteToInt(ThrowDamage), { 0.000000, 0.000000, 0.000000 })
```

```
[35.471638] Pawn.Died: Accessed None in SinglePlayer.Killed
[35.471638] Pawn.Died: at SinglePlayer.Killed line 20: SubtractEqual_FloatFloat(Killer.PlayerReplicationInfo.Score, intToFloat(2))
[35.471638] Pawn.Died: at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType)
[35.471638] Pawn.Died: at Pawn.TakeDamage line 1246: Died(instigatedBy, DamageType, HitLocation)
[35.471638] Pawn.Died: at ScriptedPawn.Retreating.TakeDamage line 3055: TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType)
[35.471638] Pawn.Died: at Pawn.PainTimer line 1512: TakeDamage(5, null, Location, { 0.000000, 0.000000, 0.000000 }, drowned)
```

### UPakShieldEffect.Tick

Verdict `undecided`. Lines per map, `unreal`: Abyss 3, Glacena 3.

`UPakShieldEffect` is a class of `UPak.u` (Return to Na Pali), and no dump of
that package exists on this host, so its script is unread. The sweep keeps no
plaintext engine log, so the site has no callstack either; what there is, is the
smoke log of each map in the sweep of f94c1ba6 (not committed). Both maps log
the line three times, after `STATE 0.0` and before the first `FPS` line:

```
SCRIPT_WARNING Accessed None in UPakShieldEffect.Tick
```

The class chain, read from the export tables of `UPak.u`, `UnrealShare.u` and
`Engine.u`, is `UPakShieldEffect`, `ShieldBeltEffect`, `Effects`, `Actor`. The
export table of `UPak.u` lists a `Tick` function under `UPakShieldEffect`
(beside `PostBeginPlay`, `Timer`, `Destroyed` and the state `OwnerFadeIn`), so
the body that ran is the package's own, not the stock one. The nearest stock
superclass with a dump, `UnrealShare.ShieldBeltEffect`, has a `Tick` that tests
`Owner != None` before it reads `Owner.bHidden`; whether the Return to Na Pali
one does is not known.

What would decide it: a dump of `UPak.u`, which shows the statement that reads a
None, or a plaintext engine log of an Abyss or Glacena run, whose callstack
names it.

### SinglePlayer.ScoreKill

Verdict `faithful`. Lines per map, `unreal`: Bluff 3, Ceremony 2.

Stock `Engine.GameInfo`, function `ScoreKill`, which `SinglePlayer` inherits:

```
function ScoreKill(pawn Killer, pawn Other)
{
    Other.DieCount++;
    if( (killer == Other) || (killer == None) )
        Other.PlayerReplicationInfo.Score -= 1;
    else if ( killer != None )
```

A killer of None or a suicide takes the first branch, and the victim is a
monster, whose `PlayerReplicationInfo` is None. Evidence, from the same Bluff
run, which logged three `ScoreKill` lines: two at 0.025558, where the engine
raised `FellOutOfWorld` for two pawns at the first tick and the stock handler
killed them with a null killer, and the drowning death at 35.471638 above. The
engine raises `FellOutOfWorld` for a pawn whose zone number is 0
(`UActor::HasLeftWorld` in
`SurrealEngine/Packages/Engine/Actors/UActor_Phys.cpp`); the null killer is the
stock `Pawn.FellOutOfWorld`'s own argument, but whether the original raises the
event for those pawns at the first tick was not read:

```
[0.025558] GameInfo.Killed: Accessed None in SinglePlayer.ScoreKill
[0.025558] GameInfo.Killed: at GameInfo.ScoreKill line 1001: SubtractEqual_FloatFloat(Other.PlayerReplicationInfo.Score, intToFloat(1))
[0.025558] GameInfo.Killed: at GameInfo.Killed line 923: ScoreKill(Killer, Other)
[0.025558] GameInfo.Killed: at SinglePlayer.Killed line 20: Killed(Killer, Other, DamageType)
[0.025558] GameInfo.Killed: at Pawn.Died line 1318: Level.Game.Killed(Killer, this, DamageType)
[0.025558] GameInfo.Killed: at Pawn.FellOutOfWorld line 353: Died(null, fell, Location)
```

### Slith.TakeDamage

Verdict `faithful`. Lines per map, `unreal`: Chizra 1.

`UnrealShare.Slith` defines no `TakeDamage`. Seven stock bodies in
`UnrealShare.ScriptedPawn` read `Enemy` without a None check (some before and
some after their call to `Global.TakeDamage`): the states `Patroling`,
`Guarding`, `Acquisition`, `StakeOut`, `TriggerAlarm` and `AlarmPaused`, and the
weave branch of `Charging`. The one the callstack names, stock
`UnrealShare.ScriptedPawn`, state `Patroling`, function `TakeDamage`:

```
State Patroling
{
    function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation,
                        Vector momentum, name damageType)
    {
        Global.TakeDamage(Damage, instigatedBy, hitlocation, momentum, damageType);
        if ( health <= 0 )
            return;
        LastSeenPos = Enemy.Location;
```

The call comes from the stock `Engine.Pawn`, function `BaseChange`, which
stomps the pawn a pawn lands on:

```
singular event BaseChange()
...
    else if (Pawn(Base) != None)
    {
        Base.TakeDamage( (1-Velocity.Z/400)* Mass/Base.Mass, Self,Location,0.5 * Velocity , 'stomped');
        JumpOffPawn();
    }
```

Evidence: a run on Chizra with `--trace-script=Slith.TakeDamage` logged six
`Slith.TakeDamage` warnings (5.168524, 5.461643, 5.847458, 8.081697, 8.121732
and 8.301055), all the same statement, all in `Patroling` on `Slith0`, each
from `Pawn.BaseChange` with a different BiterFish as the instigator and a
damage of 0. The first:

```
[5.168524] UnrealShare.BiterFish: trace: Slith0 Slith Patroling: TakeDamage(0, BiterFish13, ?, ?, 'stomped')
[5.168524] Pawn.BaseChange: trace: Slith0 Slith Patroling: TakeDamage(0, BiterFish13, ?, ?, 'stomped')
[5.168524] Pawn.BaseChange: Accessed None in Slith.TakeDamage
[5.168524] Pawn.BaseChange: at ScriptedPawn.Patroling.TakeDamage line 2055: LastSeenPos = Enemy.Location
[5.168524] Pawn.BaseChange: at Pawn.BaseChange line 687: Base.TakeDamage(floatToInt(Divide_FloatFloat(Multiply_FloatFloat(intToFloat(floatToInt(Subtract_FloatFloat(intToFloat(1), Divide_FloatFloat(Velocity.Z, intToFloat(400))))), Mass), Base.Mass)), this, Location, Multiply_FloatVector(0.500000, Velocity), stomped)
```

The instigator is the fish, not None; `Enemy` is None because a Slith that is
patrolling has no enemy, and the stock body reads it before its own
`Enemy != None` test. The `BaseChange` event is raised by the engine's physics
when a fish comes to rest on the Slith; whether the original's physics puts a
BiterFish on a Slith in the same way was not read.

### BabyCow.PickDestination

Verdict `faithful`. Lines per map, `unreal`: Nevec 1, Ruins 1.

Stock `UnrealShare.BabyCow`, state `Grazing`, function `PickDestination`:

```
    function PickDestination()
    {
        if ( Mom == None )
            Super.PickDestination();
        if ( !LineOfSightTo(mom) )
...
        if (ScaryGuy != None)
            Destination = mom.Destination - 2 * mom.CollisionRadius * Normal(ScaryGuy.Location - mom.Destination);
        else
            Destination = mom.Destination;
    }
```

`mom` is assigned only by the stock `Cow`, when an adult with `bHasBaby` spawns
its baby. The Ruins map places a `BabyCow` itself, and its name table holds
neither `mom` nor `bHasBaby`, so no saved actor sets either. The script has no
`return` after the first branch, so a baby with no mother reaches
`mom.Destination`. Residual: the map's saved data was read, not the loader, so
"no mother" assumes the engine leaves an unsaved object property None.

### UPakTransitionInfo.ScoreKill

Verdict `undecided`. Lines per map, `unreal`: Inter2 1.

`UPakTransitionInfo` is a class of `UPak.u` (Return to Na Pali), and no dump of
that package exists on this host, so its script is unread. The sweep keeps no
plaintext engine log, so the site has no callstack either; what there is, is the
smoke log of Inter2 in the sweep of f94c1ba6 (not committed), which holds the
line once, after `STATE 0.0` and before the first `FPS` line:

```
SCRIPT_WARNING Accessed None in UPakTransitionInfo.ScoreKill
```

The class chain, read from the export tables of `UPak.u`, `UnrealShare.u` and
`Engine.u`, is `UPakTransitionInfo`, `UnrealGameInfo`, `GameInfo`, `Info`,
`Actor`. The export table of `UPak.u` lists `Login`, `SwitchFromUnrealPlayer`,
`AdjustVolume`, `RedirectorSearch` and `Timer` under the class, and no
`ScoreKill` or `Killed`; the stock `UnrealShare.UnrealGameInfo` defines neither.
So the `ScoreKill` that ran is the one `Engine.GameInfo` defines, which its
`Killed` calls:

```
function ScoreKill(pawn Killer, pawn Other)
{
    Other.DieCount++;
    if( (killer == Other) || (killer == None) )
        Other.PlayerReplicationInfo.Score -= 1;
    else if ( killer != None )
    {
        killer.killCount++;
        if ( killer.PlayerReplicationInfo != None )
            killer.PlayerReplicationInfo.Score += 1;
    }
}
```

Two statements can read a None: `Other.DieCount++` when the victim is None, and
`Other.PlayerReplicationInfo.Score -= 1` when the killer is None or the victim
itself and the victim has no replication info, the statement
`SinglePlayer.ScoreKill` reaches for a monster victim. If the caller is the
stock `GameInfo.Killed`, the victim cannot be None: `Killed` reads
`Other.bIsPlayer` as its first statement (line 923 of the stock
`Engine.GameInfo` text) and calls `ScoreKill` as its last (line 991), and the
Inter2 smoke log above holds no `UPakTransitionInfo.Killed` warning, which a
None victim would have logged at the first. Under that caller only the
replication-info read remains, the `SinglePlayer.ScoreKill` pattern, and a None
victim needs a caller inside `UPak.u`, which the decider below already asks
about. The log line names neither a statement, a victim nor a killer, so the
site is not read the way `SinglePlayer.ScoreKill` is.

What would decide it: a plaintext engine log of an Inter2 run, whose callstack
names the statement and its caller, or a dump of `UPak.u`, whose functions would
show whether `UPakTransitionInfo` itself kills a pawn.

### UnrealCTFScoreboard.DrawTrailer

Verdict `faithful`. Lines per map, `ut`: CTF-Tutorial 3234; `ut-bots`:
CTF-Tutorial 3190.

Stock `BotPack.TournamentScoreBoard`, function `DrawTrailer`, which
`UnrealCTFScoreBoard` inherits through `TeamScoreBoard`:

```
function DrawTrailer( canvas Canvas )
...
    if ( (Level.NetMode == NM_Standalone) && Level.Game.IsA('DeathMatchPlus') )
...
        if ( DeathMatchPlus(Level.Game).bRatedGame )
            Canvas.DrawText(DeathMatchPlus(Level.Game).RatedGameLadderObj.SkillText@PlayerOwner.GameReplicationInfo.GameName@MapTitle@MapTitleQuote$Level.Title$MapTitleQuote, true);
```

The stock training game (`BotPack.TrainingCTF`) sets the rated-game flag:

```
event InitGame( string Options, out string Error )
...
    bRatedGame = True;
```

`RatedGameLadderObj` is assigned in one place, `DeathMatchPlus.InitRatedGame`:

```
function InitRatedGame(LadderInventory LadderObj, PlayerPawn LadderPlayer)
{
...
    RatedGameLadderObj = LadderObj;
```

which the stock game calls from `LadderInventory.TravelPostAccept`, for a player
who arrives carrying the ladder item:

```
function TravelPostAccept()
...
        DeathMatchPlus(Level.Game).InitRatedGame(Self, PlayerPawn(Owner));
```

A direct map launch brings no such item. The scoreboard is drawn when the HUD
forces it, and the stock tutorial forces it at its eighth step:

```
function CTFTutEvent7()
{
    ChallengeHUD(Trainee.myHUD).bForceScores = True;
```

`CTF-Tutorial` names `BotPack.TrainingCTF` as its `DefaultGameType`, so a
direct launch runs that tutorial. The DM, DOM and AS tutorial maps name no
training game type, so a direct launch has no tutorial script to run there, and
they log neither this site nor `UTConsole.AddMessage` in either sweep.

The sweep names only the function, and the function has other dereferences, so
that `RatedGameLadderObj` is the None operand is inferred, not observed. The
training game's rated-game flag is what sends `DrawTrailer` into the branch that
reads the ladder object, and the function's other dereferences are not specific
to this map's situation (read from the script text; no traced run discriminates
them); that is why the row stays `faithful`, with the residual. The same warning
on `CTF-Tutorial` in `ut` and `ut-bots` is one site in two sweeps. Residual: the
original's startup for a direct launch of this map is unread.

### UTConsole.AddMessage

Verdict `faithful`, with a residual. Lines per map, `ut`: CTF-Tutorial 8;
`ut-bots`: CTF-Tutorial 8.

Stock `UTMenu.UTConsole`, function `AddMessage`, and the only function that
creates the window it uses, `CreateMessage`:

```
function AddMessage( string NewMessage )
{
    MessageWindow.AddMessage( NewMessage );
}
```

```
function CreateMessage()
{
    MessageWindow = MessageWindow(Root.CreateWindow(Class'MessageWindow', 100, 100, 200, 200));
```

`CreateMessage` is called only from the console's `CreateRootWindow`, which
runs when the UWindow root is created; the stock tutorial calls `AddMessage`
assuming that has happened. No engine code calls `CreateRootWindow`,
`RenderUWindow` or `LaunchUWindow`, so a direct launch with no input never
creates the root. Residual: whether the original native engine creates the
root at startup on a direct launch of `CTF-Tutorial` is unread.

### TeamCannon.SeePlayer

Verdict `undecided`. Lines per map, `ut`: AS-HiSpeed 1, AS-Mazon 1,
AS-OceanFloor 2; `ut-bots`: AS-HiSpeed 1, AS-Mazon 1, AS-OceanFloor 3.

Stock `BotPack.TeamCannon`, state `Idle`, function `SeePlayer`:

```
auto state Idle
...
    function SeePlayer(Actor SeenPlayer)
    {
        if ( SeenPlayer.bCollideActors
            && ((Pawn(SeenPlayer).PlayerReplicationInfo.Team != MyTeam) || !Level.Game.bTeamGame) )
```

`Pawn(SeenPlayer).PlayerReplicationInfo` is None for a pawn with no replication
info. The engine raises `SeePlayer`, every 0.2 to 0.4 s, for each live pawn in
sight other than the receiver and its current enemy, with no test of
`bIsPlayer` (`SurrealEngine/Packages/Engine/Actors/Pawn/UPawn_Tick.cpp`).
Evidence: a run on AS-HiSpeed with four bots and `--trace-script` on
`TeamCannon.SeePlayer` logged one warning, with another cannon as the seen
pawn:

```
[0.151715] <Surreal Engine>: trace: TeamCannon10 TeamCannon Idle: SeePlayer(TeamCannon9)
[0.151715] Botpack.TeamCannon: Accessed None in TeamCannon.SeePlayer
[0.151715] Botpack.TeamCannon: at TeamCannon.Idle.SeePlayer line 189: if (AndAnd_BoolBool(SeenPlayer.bCollideActors, skip(OrOr_BoolBool(NotEqual_IntInt(byteToInt(dynamic cast(SeenPlayer).PlayerReplicationInfo.Team), MyTeam), skip(Not_PreBool(Level.Game.bTeamGame))))))
```

The same run traced four more `SeePlayer` calls on a cannon, each for a bot
(`TMale2Bot0` three times, `TMale1Bot0` once), which have a replication info,
and none logged a warning. The shipped scripts hint that the
original also raises `SeePlayer` for pawns
that are not players, without showing it. Their `SeePlayer` bodies pass
`Pawn(SeenPlayer)` to `SetEnemy`, and `Bot.SetEnemy` rejects a `FlockPawn`
while `ScriptedPawn.SetEnemy` accepts a `ScriptedPawn`; but `SetEnemy` is also
reached from `HearNoise` and from damage.

What would decide it: the original engine's rule for the pawns `SeePlayer` is
raised for. If the original raises it for players only, the warning is
engine-side; if it raises it for every live pawn in sight, the cannons warn in
the original too.

### MinigunCannon.SeePlayer

Verdict `undecided`. Lines per map, `ut`: CTF-High 4; `ut-bots`: CTF-High 5.

`BotPack.MinigunCannon`:

```
class MinigunCannon extends TeamCannon;
```

It defines no `SeePlayer`, so the body and the question are those of
`TeamCannon.SeePlayer`. No run traced
the seen pawn on CTF-High, where the warning is logged; the cannon found in
the `TeamCannon` run is not shown to be what this one sees.

What would decide it: a CTF-High run that traces `MinigunCannon.SeePlayer`
and names its argument, and the original's rule for the pawns `SeePlayer` is
raised for.

### AssaultRandomizer.SpecialCost

Verdict `undecided`. Lines per map, `ut-bots`: AS-HiSpeed 20.

Stock `BotPack.AssaultRandomizer`, state `CostEnabled`, function `SpecialCost`;
the engine calls it from `UPawn::ClearPaths`
(`SurrealEngine/Packages/Engine/Actors/Pawn/UPawn_Path.cpp`), once per path
search for each navigation point with `bSpecialCost` set. The game's `Defender`
is assigned lazily, the first time `Assault.FindPlayerStart` is asked for a
team start:

```
    event int SpecialCost(Pawn Seeker)
    {
        if ( !Seeker.bIsPlayer || (Seeker.PlayerReplicationInfo.Team == Assault(Level.Game).Defender.TeamIndex) )
            return 0;

        return ToggledCost;
    }
```

```
        if ( Defender == None )
...
            Defender = Teams[d];
```

Evidence: a run on AS-HiSpeed with four bots and `--trace-script` on
`AssaultRandomizer.SpecialCost` logged 996 warnings, every one the single
statement of `SpecialCost`, with one bot as the seeker (`TFemale2Bot0`); 168
calls with the other three bots logged no warning. The first:

```
[15.602821] Bot.FindAmbushSpot: trace: AssaultRandomizer2 AssaultRandomizer CostEnabled: SpecialCost(TFemale2Bot0)
[15.602821] Pawn.FindPathToward: Accessed None in AssaultRandomizer.SpecialCost
[15.602821] Pawn.FindPathToward: at AssaultRandomizer.CostEnabled.SpecialCost line 21: if (OrOr_BoolBool(Not_PreBool(Seeker.bIsPlayer), skip(EqualEqual_IntInt(byteToInt(Seeker.PlayerReplicationInfo.Team), dynamic cast(Level.Game).Defender.TeamIndex))))
[15.602821] Pawn.FindPathToward: at Pawn.FindPathToward line 271
[15.602821] Pawn.FindPathToward: at Bot.FindAmbushSpot line 2455: MoveTarget = FindPathToward(AmbushSpot)
[15.602821] Pawn.FindPathToward: at Bot.Roaming.PickDestination line 3019: if (AndAnd_BoolBool(Not_PreBool(bWantsToCamp), skip(FindAmbushSpot())))
[15.602821] Pawn.FindPathToward: at Bot.Roaming line 3387: goto PreBegin
```

The statement has three dereferences (`Seeker.PlayerReplicationInfo`,
`Assault(Level.Game)` and its `Defender`). The last two are the same for every
bot, and three other bots evaluated the statement (168 calls) without a
warning, so by differential the None is `TFemale2Bot0`'s own
`PlayerReplicationInfo`; the log does not print it. Why that bot has none is
the open question: the stock `Pawn.PreBeginPlay` spawns one for a pawn with
`bIsPlayer`, and the log does not show whether the stock script or the engine
left this bot without it.

What would decide it: a trace of that bot's spawn path (`Pawn.PreBeginPlay`)
and its `PlayerReplicationInfo` at first use, and the original's behaviour on
the same map. The sweep of 73df9f26 logged 18 lines on this map, the sweep of
8d225ef3 20 and this run 996, so the count says little.

### TMale2Bot.Roaming

Verdict `faithful`. Lines per map, `ut-bots`: AS-OceanFloor 2, CTF-Command 6.

The `faithful` verdict rests on the statement that the traced CTF-Command run
logged, `TurnTo(Location + AmbushSpot.lookDir)` at `LongCamp`. The sweep of
8d225ef3 also logged 2 lines on AS-OceanFloor, a map no run traced, and which
`Roaming` statement those 2 lines name is not known.

The warning names the state: the statement is in the state code (the labels) of
`BotPack.Bot`'s `Roaming`, shared by the four classes the `ut-bots` sweeps have
logged. Stock `BotPack.Bot`, state `Roaming`, label `LongCamp`:

```
LongCamp:
    bCamping = true;
    Acceleration = vect(0,0,0);
    TweenToWaiting(0.15);
    TurnTo(Location + Ambushspot.lookdir);
    Sleep(CampTime);
    Goto('PreBegin');
```

Evidence: a run on CTF-Command with four bots and `--trace-script` on
`Bot.TurnTo` and `Bot.ShareWith` logged 22 warnings, all `TMale2Bot0` and all
this statement. The first:

```
[45.583836] Botpack.TMale2Bot: trace: TMale2Bot0 TMale2Bot Roaming: TurnTo(?)
[45.583836] Botpack.TMale2Bot: Accessed None in TMale2Bot.Roaming
[45.583836] Botpack.TMale2Bot: at Bot.Roaming line 3387: TurnTo(Add_VectorVector(Location, AmbushSpot.lookDir))
```

`AmbushSpot` is a script property of the bot, and no engine code touches it.
Two stock routes send a bot to `LongCamp`: `FindAmbushSpot` after it has
confirmed a spot,

```
            CampTime = 10.0;
            SightRadius = AmbushSpot.SightRadius;
            GotoState('Roaming', 'LongCamp');
```

and `Roaming`'s `PickDestination`, for a bot standing at an ambush point it
roamed to, which assigns nothing:

```
        if ( (AmbushPoint(RoamTarget) != None)
            && (VSize(Location - RoamTarget.Location) < 2 * CollisionRadius)
            && (FRand() < 0.4) )
        {
            CampTime = 4.0;
            GotoState('Roaming', 'LongCamp');
```

Leaving the state clears the spot:

```
    function EndState()
...
        if ( !bSniping && (AmbushSpot != None) )
        {
            AmbushSpot.taken = false;
            AmbushSpot = None;
```

The engine runs `EndState` only when the state changes (`UObject::GotoState` in
`SurrealEngine/Packages/Core/UObject.cpp`), so on the first route the spot
survives the jump to the label, and a bot with no spot gets to the dereference
by the second route, from stock data. The log keeps one frame for state code
and does not show which route sent the bot.

All 22 warnings come within 0.45 s after a `TurnTo` call of the same bot in
`Acquisition` (it makes 39 in the run; 36 of the 38 gaps between them are under
2 s and run from 0.22 to 0.93 s), and `Roaming.EndState` clears `AmbushSpot`
each time the bot leaves `Roaming` unless it is sniping. What raised those
entries into `Acquisition` (an `EnemyAcquired` from `SeePlayer` or
`HearNoise`) was not traced, and whether the original bounces between the two
states at that cadence was not read; the `SeePlayer` question of the cannon
rows applies. The timing alone does not single the warnings out: 49 of the
bot's 54 `TurnTo` calls in `Roaming` follow an `Acquisition` one within 0.45 s.

### TMale1Bot.Roaming

Verdict `undecided`. Lines per map, `ut-bots`: DM-ArcaneTemple 6, DOM-Cinder 12.

Stock `BotPack.TMale1Bot`:

```
class TMale1Bot extends MaleBotPlus;
```

The same body and the same reason as `TFemale1Bot.Roaming`; no run logged this
class's statement. What would decide it: a run on CTF-EternalCave that logs
this class's warning, with the callstack naming the statement.

### TFemale2Bot.Roaming

Verdict `undecided`. Lines per map, `ut-bots`: CTF-LavaGiant 1.

Stock `BotPack.TFemale2Bot`:

```
class TFemale2Bot extends FemaleBotPlus;
```

The same body and the same reason as `TFemale1Bot.Roaming`; no run logged this
class's statement. What would decide it: a run on AS-Overlord or CTF-LavaGiant
that logs this class's warning, with the callstack naming the statement.

### TriggerLight.Trigger

Verdict `faithful`. Lines per map, `deusex`: 99_Endgame2 3.

Stock `Engine.CameraPoint`, function `EvaluateCommand`, case `CAMCMD_TRIGGER`,
and stock `Engine.TriggerLight`, state `TriggerToggle`, function `Trigger`:

```
        case CAMCMD_TRIGGER:    if (eventName != '')
                                {
                                    foreach AllActors(class'Actor', A, eventName)
                                        A.Trigger(None, None);
                                }
```

```
state() TriggerToggle
...
    function Trigger( actor Other, pawn EventInstigator )
...
        SavedTrigger = Other;
        SavedTrigger.BeginEvent();
```

On 99_Endgame2 a stock camera point whose command is `CAMCMD_TRIGGER` and whose
event name is `Lightshow` triggers every actor with that tag, with a None
`Other`. Three `TriggerLight`s start in `TriggerToggle` with that tag, and each
dereferences `Other` at `SavedTrigger.BeginEvent()`: the three lines.

### ShakeTrigger.Trigger

Verdict `faithful`. Lines per map, `deusex`: 99_Endgame2 1.

Stock `DeusEx.ShakeTrigger`, function `Trigger`:

```
function Trigger(Actor Other, Pawn Instigator)
{
    Instigator.ShakeView(shakeTime, shakeRollMagnitude, shakeVertMagnitude);
```

The same camera point triggers the one `ShakeTrigger` tagged `Lightshow` with a
None instigator, and `Instigator.ShakeView(...)` has no check: the one line.

### LAM.Explode

Verdict `faithful`. Lines per map, `deusex`: 02_NYC_Warehouse 2.

`DeusEx.LAM` defines no `Explode`; the body is stock `DeusEx.ThrownProjectile`,
state `Flying`, function `Explode`:

```
auto simulated state Flying
{
    simulated function Explode(vector HitLocation, vector HitNormal)
...
        player = DeusExPlayer(Owner);

        dist = Abs(VSize(player.Location - Location));
```

When the LAM's owner is not a `DeusExPlayer` the cast gives None and the next
statements dereference it: `player.Location` and then, in whichever branch is
taken, `player.ClientFlash`, so the script would log two lines for one
explosion; the sweep's two lines are consistent with one explosion, not shown to
be. The map `02_NYC_Warehouse` places four LAMs (`LAM0`, `LAM1`, `LAM3` and
`LAM5`), and none carries a saved owner. A LAM with no owner is set off by the
player coming within its proximity radius, or by damage. No traced run was made
for this site; which of the four exploded in the sweep, and why, is not
recorded. The site is not in the Deus Ex summary committed as f0bce687 and first
appears in the re-run committed as 28f08a4c, so a sweep need not set a LAM off;
whether the original sets off the same LAM in the same situation was not read.

### LaserEmitter.CalcTrace

Verdict `engine-side`, fixed. No current sweep lists it. Stock
`DeusEx.LaserEmitter`, function `CalcTrace`:

```
        foreach TraceTexture(class'Actor', target, texName, texGroup, texFlags, HitLocation, HitNormal, EndTrace, StartTrace)
        {
            if ((target.DrawType == DT_None) || target.bHidden)
            {
                // do nothing - keep on tracing
            }
            else if ((target == Level) || target.IsA('Mover'))
```

The `foreach TraceTexture` iterator yields the actor it hits, and `target`
is dereferenced at once; the script's own `target == Level` branch shows that
the original yields the level actor for a world hit. The engine yielded a null
actor for every world hit, which upstream fixed in commit 2c1a4f50 ("Fix
TraceActor bug where no target was returned for world hits"). The Deus Ex
summary committed as f0bce687 lists 1648427 lines on 27 maps; the re-run
committed as 28f08a4c lists none.

### ElectricityEmitter.CalcTrace

Verdict `engine-side`, fixed. No current sweep lists it. Stock
`DeusEx.ElectricityEmitter`, function `CalcTrace`, has the same `foreach
TraceTexture` and the same dereference of `target`, reached through
`LaserEmitter`'s `Tick`:

```
class ElectricityEmitter extends LaserEmitter;
...
        foreach TraceTexture(class'Actor', target, texName, texGroup, texFlags, HitLocation, HitNormal, EndTrace, StartTrace)
...
            else if ((target == Level) || target.IsA('Mover'))
```

The same engine fault and the same fix; the summary
committed as f0bce687 lists 435578 lines on 22 maps and the re-run lists
none.

### PlayerStart.PlayTeleportEffect

Verdict `engine-side`, fixed. No current sweep lists it.

Stock `Engine.PlayerStart`, function `PlayTeleportEffect`, called from the
stock `Engine.GameInfo.Login`:

```
function PlayTeleportEffect(actor Incoming, bool bOut)
{
    Level.Game.PlayTeleportEffect(Incoming, bOut, Level.Game.bDeathMatch );
}
```

```
    // Teleport-in effect.
    StartSpot.PlayTeleportEffect( NewPlayer, true );
```

The only candidates for None are `Level` and `Level.Game` of the start spot.
ExtremeDGen holds several level infos. The engine used the first it found by
name, which the map saved as deleted, while the map's live actors reference
another, so the engine set `Game` on an object no actor reads and
`Level.Game` was None for the start spot. The engine now takes the level's
first actor as the level info, and its `Level info:` line names the pick.

Evidence: the smoke log of ExtremeDGen from before the fix has
`SCRIPT_WARNING Accessed None in PlayerStart.PlayTeleportEffect` between `MAP`
and `LOGIN`, and the log after it has none, with the engine log reading
`Level info: LevelInfo5 (the level's first actor) in ExtremeDGen`. The fix is
commit 14090e6d ("fix(level): take the level's first actor as info"). The re-run
of the sweep (f94c1ba6) logged no warning on ExtremeDGen.

### SkaarjBerserker.TakeDamage

Verdict `undecided`. No current sweep lists it.

`UnrealI.SkaarjBerserker` defines `WhatToDoNext` and `AttitudeToCreature` but
no `TakeDamage`, so the same seven unguarded `ScriptedPawn` state bodies as in
`Slith.TakeDamage` are the candidates. The Berserker fights other monsters:

```
function eAttitude AttitudeToCreature(Pawn Other)
{
    if ( Other.IsA('ScriptedPawn') && !Other.IsA('Pupae') )
        return ATTITUDE_Hate;
    else
        return ATTITUDE_Ignore;
}
```

Two runs on ExtremeDark, 90 s each with
`--trace-script=SkaarjBerserker.TakeDamage`, logged no
`SkaarjBerserker.TakeDamage` warning. In them `SkaarjBerserker2` took traced
`TakeDamage` calls from `SkaarjInfantry0` (never a None instigator) in the
states `Charging`, `Hunting`, `RangedAttack`, `MeleeAttack`, `TakeHit`,
`FallingState` and `Dying`. Only `Charging` is one of the seven, and its
unguarded read is in the weave branch:

```
        else if ( bWasOnGround && (MoveTarget == Enemy) &&
                    (Physics == PHYS_Falling) && (Intelligence == BRAINS_Human) ) //weave
...
            sideDir = Normal( Normal(Enemy.Location - Location) Cross vect(0,0,1) );
```

whose full condition also needs `bWasOnGround`, `Physics == PHYS_Falling` and
`Intelligence == BRAINS_Human` (`MoveTarget == Enemy` is true when both are
None); whether a Berserker's `Intelligence` is
`BRAINS_Human` was not read, so `Charging` may not be a candidate for this
site at all.

What would decide it: a run in which the warning recurs, with the callstack,
which names the state body and its caller. It is stock if the caller passes
a None instigator (zone or drowning damage from `Pawn`'s pain timer) or the
pawn is in one of the seven states with no enemy, and engine-side if the
callstack shows an engine-supplied None.

### TFemale1Bot.Roaming

Verdict `undecided`. No current sweep lists it.

Stock `BotPack.TFemale1Bot`:

```
class TFemale1Bot extends FemaleBotPlus;
```

The same `Bot` `Roaming` body as `TMale2Bot.Roaming`. Its labels also
dereference `MoveTarget`, `OrderObject` and
`InventorySpot(MoveTarget).markedItem`, some of which natives assign, so the
verdict of one statement does not carry to the others. The traced CTF-Command
run, which holds this class on the same map, logged only `TMale2Bot` warnings.
What would decide it: a run that logs this class's warning, whose callstack
names the statement.

The sweep of 73df9f26 logged it on CTF-Command (2 lines) and DOM-Cinder (15).
The re-sweep (8d225ef3) did not, but logged the same `Roaming` body under
`TMale2Bot` on CTF-Command (6 lines) and under `TMale1Bot` on DOM-Cinder (12).
No engine change is known to account for that, and the runs are not
deterministic.

## The runs

The quoted log lines come from single runs of the engine built from commit
b1b35617, one per row of the table below (two for the Berserker). The two
ExtremeDGen runs are the exception: they ran on the branch builds before and
after commit 14090e6d, which b1b35617 later merged. The logs are not committed,
and the runs are not deterministic, so a run that repeats one need not log the
same lines. Each traced run used

```
SurrealEngine --autostart --exit-after=90 --smoke-log=<log> --user-dir=<dir> '--url=<map><options>' [--exec=5:Fire] --trace-script=<entries> "<game folder>"
```

and the plaintext engine log (`SE-Log-LastRun.txt` in the per-user
configuration directory, written when the engine exits) holds the callstacks
and the `trace:` lines. Both are described in `Tools/smoke/README.md`.

| Site | Game and map | URL options and `--exec` | `--trace-script` |
| :--- | :--- | :--- | :--- |
| Slith.TakeDamage | Unreal Gold 226b, Chizra | none | `Slith.TakeDamage` |
| SkaarjBerserker.TakeDamage (two runs) | Unreal Gold 226b, ExtremeDark | none | `SkaarjBerserker.TakeDamage` |
| HorseFly.meander, SinglePlayer.Killed, SinglePlayer.ScoreKill | Unreal Gold 226b, Bluff | none | `DeadBodySwarm.Destroyed,DeadBodySwarm.Destroy,HorseFly.Destroy,HorseFly.SetLocation,HorseFly.MoveTo` |
| TeamCannon.SeePlayer, AssaultRandomizer.SpecialCost | Unreal Tournament 436, AS-HiSpeed | `?NumBots=4` and `--exec=5:Fire` | `TeamCannon.SeePlayer,AssaultRandomizer.SpecialCost` |
| TMale2Bot.Roaming | Unreal Tournament 436, CTF-Command | `?NumBots=4` and `--exec=5:Fire` | `Bot.TurnTo,Bot.ShareWith` |
| PlayerStart.PlayTeleportEffect (the engine before and after commit 14090e6d; no trace) | Unreal Gold 226b, ExtremeDGen | none | none |

## Limits

- The original engine was not run. A `faithful` verdict assumes the original
  runs the same script paths in the same order as this engine; the `Roaming`
  verdict also rests on reading which stock routes can leave `AmbushSpot` None.
- The verdicts of `BabyCow.PickDestination`, `UnrealCTFScoreboard.DrawTrailer`,
  `UTConsole.AddMessage`, `TriggerLight.Trigger`, `ShakeTrigger.Trigger` and
  `LAM.Explode` rest on the stock scripts and, for the first, the two Unreal
  Tournament tutorial sites and the Deus Ex sites, the saved map data; no traced
  run was made for them (the sweeps logged them, but only the lines).
- Whether the original keeps running state code after `Destroy()` is not
  known, which is the question that decides `HorseFly.meander`; that the
  original clears a destroyed swarm's flies' `Owner` the way `UActor::Destroy`
  does here is a sign read from the script's own `Owner == None` test, not an
  observation. Which swarm orphaned the
  flies on the maps that place their own swarms was not traced.
- The runs are not deterministic: monsters and bots fight, so a site may not
  reproduce (`SkaarjBerserker.TakeDamage` did not in two runs) and counts
  differ widely from the sweeps' (`AssaultRandomizer.SpecialCost`: 18 lines in
  the sweep of 73df9f26, 20 in the sweep of 8d225ef3, 996 in one traced run).
- Whom the original raises `SeePlayer` for, when the receiver is a stock UT
  cannon, is not known; the engine raises it for each live pawn in sight.
- The UT scoreboard and console warnings rest on the stock game assuming the
  menu path (a ladder object, a UWindow root); the original's startup for a
  direct launch is unread.
- The `Slith.TakeDamage` call starts from a `BaseChange` event raised by the
  engine's physics. Of the three `SinglePlayer.ScoreKill` lines of the Bluff
  run, two start from a `FellOutOfWorld` event the engine raises at the first
  tick and one from the engine's `PainTimer` event (drowning); the same
  `PainTimer` path gives one of the `SinglePlayer.Killed` lines, and the other
  three Bluff `Killed` lines start from an animation notify
  (`ThrowDamageTarget`, `PoundDamageTarget`, `ClawDamageTarget`), as does a
  Chizra one, or from a `Touch` of a projectile. Whether the original raises any
  of these events in the same situation was not read.
- The `TMale2Bot.Roaming` warnings follow the bot's `Acquisition` entries
  (36 of the 38 gaps between its 39 `TurnTo` calls there are under 1 s); what
  raised those entries was not traced, and whether the original bounces at that
  cadence was not read.
- Which of the four placed LAMs exploded in the Deus Ex sweep, and why, is not
  recorded, and the site appeared only in the re-run, so whether the original
  sets a LAM off in the same situation was not read.
- The Deus Ex sweeps ran on a folder that holds the stock Deus Ex 1112fm
  `DeusEx.u`, `Engine.u` and the maps of the three sites, so the quoted
  scripts and the map facts are the original's. SHA-256 of the files used:
  `DeusEx.u` 257f5c166739b3d13884a8b8e98a47dda458b325fa929b3b3b99be056cb74480,
  `Engine.u` 1742d16f49492a02391fa6a9ca25887f0cc4f30ec9964925226217d8858737da,
  `02_NYC_Warehouse.dx` 2333b0d8a137d0da1ae22ecc24942cd881f9bba7d1e95546e4451a7100b2892c,
  `99_Endgame2.dx` def25b54608fdd8b0bc493aca7f350b793e5e93621ea2a00eec64ef52159a74e.
