# Unreal Tournament: save, quit and load across three maps

Chain: DM-Deck16][ to DM-Turbine to DM-Codex. Four engine processes on one user directory, 60 seconds each, slots 1 to 3.

## Steps

| Step | Exit | Verdict | Maps loaded | Loaded slot | Saved to |
| :--- | ---: | :--- | :--- | ---: | :--- |
| save1 | 0 | pass | DM-Deck16][ | - | UnrealTournament-436-Unreal_Tournament_GOTY/Save/Save1.usa |
| load1-travel | 0 | pass | DM-Deck16][, DM-Deck16][, DM-Turbine | 1 | UnrealTournament-436-Unreal_Tournament_GOTY/Save/Save2.usa |
| load2-travel | 0 | pass | DM-Deck16][, DM-Turbine, DM-Codex | 2 | UnrealTournament-436-Unreal_Tournament_GOTY/Save/Save3.usa |
| load3 | 0 | pass | DM-Deck16][, DM-Codex | 3 | - |

## Checks

| Check | Map | Inventory |
| :--- | :--- | :--- |
| save1/after-state | DM-Deck16][ | ShockCore, ShockRifle, ImpactHammer, Miniammo, enforcer |
| load1-travel/after-load | DM-Deck16][ | ShockCore, ShockRifle, ImpactHammer, miniammo, enforcer |
| load1-travel/after-travel | DM-Turbine | ImpactHammer, Miniammo, enforcer |
| load2-travel/after-load | DM-Turbine | ImpactHammer, Miniammo, enforcer |
| load2-travel/after-travel | DM-Codex | ImpactHammer, Miniammo, enforcer |
| load3/after-load | DM-Codex | ImpactHammer, Miniammo, enforcer |

## Hops

Kept reads as restored for a load and carried for a travel.

| Hop | By | Kept | Lost | Gained | Offered | Travel skipped |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| save1/after-state to load1-travel/after-load | load | ShockCore, ShockRifle, ImpactHammer, Miniammo, enforcer | - | - | - | - |
| load1-travel/after-load to load1-travel/after-travel | travel | ImpactHammer, miniammo, enforcer | ShockCore, ShockRifle | - | ShockCore0, ShockRifle0, ImpactHammer0, Miniammo0, enforcer0 | no |
| load1-travel/after-travel to load2-travel/after-load | load | ImpactHammer, Miniammo, enforcer | - | - | - | - |
| load2-travel/after-load to load2-travel/after-travel | travel | ImpactHammer, Miniammo, enforcer | - | - | ImpactHammer0, enforcer0, Miniammo0 | no |
| load2-travel/after-travel to load3/after-load | load | ImpactHammer, Miniammo, enforcer | - | - | - | - |

## Verdict

Unreal Tournament: loads restore what was saved; travels discard the carried inventory by game rule (lost on a travel: ShockCore, ShockRifle).

## Limits of the claim

- Items compare by class name without case; the package (`Save<n>` after a load on Unreal Gold and Unreal Tournament, the map's package after a travel and after a load on Deus Ex) and the instance number change and are ignored.
- Health and position are not read, beyond the `STATE` line of the smoke log.
- The deathmatch game type discards travelled inventory by game rule: `DeathMatchPlus.AcceptInventory` destroys every item but a ladder item and `AddDefaultInventory` gives the defaults again once the player is ready. A class listed as kept on a travel hop is therefore a default given again, not a carried item. The Offered column is the `--trace-script=Inventory.TravelPreAccept` trace, the witness that the engine carried the items to the acceptance (empty when the engine skipped the transfer); the destruction itself is not seen.
