# Deus Ex: save, quit and load across three maps

Chain: 01_NYC_UNATCOIsland to 01_NYC_UNATCOHQ to 02_NYC_BatteryPark. Four engine processes on one user directory, 60 seconds each, slots 1 to 3.

## Steps

| Step | Exit | Verdict | Maps loaded | Loaded slot | Saved to |
| :--- | ---: | :--- | :--- | ---: | :--- |
| save1 | 0 | pass | 01_NYC_UNATCOIsland | - | DeusEx-1112fm-Deus_Ex_GOTY/Save/Save0001/01_NYC_UNATCOIsland.dxs |
| load1-travel | 0 | pass | 01_NYC_UNATCOIsland, 01_NYC_UNATCOIsland, 01_NYC_UNATCOHQ | 1 | DeusEx-1112fm-Deus_Ex_GOTY/Save/Save0002/01_NYC_UNATCOHQ.dxs |
| load2-travel | 0 | pass | 01_NYC_UNATCOIsland, 01_NYC_UNATCOHQ, 02_NYC_BatteryPark | 2 | DeusEx-1112fm-Deus_Ex_GOTY/Save/Save0003/02_NYC_BatteryPark.dxs |
| load3 | 0 | pass | 01_NYC_UNATCOIsland, 02_NYC_BatteryPark | 3 | - |

## Checks

| Check | Map | Inventory |
| :--- | :--- | :--- |
| save1/after-state | 01_NYC_UNATCOIsland | NanoKeyRing |
| load1-travel/after-load | 01_NYC_UNATCOIsland | NanoKeyRing |
| load1-travel/after-travel | 01_NYC_UNATCOHQ | NanoKeyRing |
| load2-travel/after-load | 01_NYC_UNATCOHQ | NanoKeyRing |
| load2-travel/after-travel | 02_NYC_BatteryPark | NanoKeyRing |
| load3/after-load | 02_NYC_BatteryPark | NanoKeyRing |

## Hops

Kept reads as restored for a load and carried for a travel.

| Hop | By | Kept | Lost | Gained | Goal | Note | Travel skipped |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| save1/after-state to load1-travel/after-load | load | NanoKeyRing | - | - | found | found | - |
| load1-travel/after-load to load1-travel/after-travel | travel | NanoKeyRing | - | - | found | found | no |
| load1-travel/after-travel to load2-travel/after-load | load | NanoKeyRing | - | - | found | found | - |
| load2-travel/after-load to load2-travel/after-travel | travel | NanoKeyRing | - | - | found | found | no |
| load2-travel/after-travel to load3/after-load | load | NanoKeyRing | - | - | found | found | - |

## Verdict

Deus Ex: state, inventory and the Deus Ex goal and note survive three maps (01_NYC_UNATCOIsland, 01_NYC_UNATCOHQ, 02_NYC_BatteryPark).

## Limits of the claim

- Items compare by class name without case; the package (`Save<n>` after a load on Unreal Gold and Unreal Tournament, the map's package after a travel and after a load on Deus Ex) and the instance number change and are ignored.
- Health and position are not read, beyond the `STATE` line of the smoke log.
- Inventory beyond the player's `NanoKeyRing` cannot be given from the harness (the game recreates the key ring on a travel), so the inventory witness is the key ring alone.
- A flag's value is not read back: `SetBoolFlagFromString` is called, and survival is witnessed through a goal (`FindGoal`, added as a primary goal so the mission change keeps it) and a note (`GetNote`), which travel the same way.
