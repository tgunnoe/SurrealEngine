# Unreal Gold: save, quit and load across three maps

Chain: Vortex2 to Nyleve to Dig. Four engine processes on one user directory, 60 seconds each, slots 1 to 3.

## Steps

| Step | Exit | Verdict | Maps loaded | Loaded slot | Saved to |
| :--- | ---: | :--- | :--- | ---: | :--- |
| save1 | 0 | pass | Vortex2 | - | Unreal-226b-Unreal_Gold/Save/Save1.usa |
| load1-travel | 0 | pass | Vortex2, Vortex2, Nyleve | 1 | Unreal-226b-Unreal_Gold/Save/Save2.usa |
| load2-travel | 0 | pass | Vortex2, Nyleve, Dig | 2 | Unreal-226b-Unreal_Gold/Save/Save3.usa |
| load3 | 0 | pass | Vortex2, Dig | 3 | - |

## Checks

| Check | Map | Inventory |
| :--- | :--- | :--- |
| save1/after-state | Vortex2 | StingerAmmo, Stinger |
| load1-travel/after-load | Vortex2 | StingerAmmo, Stinger |
| load1-travel/after-travel | Nyleve | DefaultAmmo, DispersionPistol, Stinger, StingerAmmo |
| load2-travel/after-load | Nyleve | DefaultAmmo, DispersionPistol, Stinger, StingerAmmo |
| load2-travel/after-travel | Dig | StingerAmmo, DispersionPistol, Stinger, DefaultAmmo |
| load3/after-load | Dig | StingerAmmo, DispersionPistol, Stinger, DefaultAmmo |

## Hops

Kept reads as restored for a load and carried for a travel.

| Hop | By | Kept | Lost | Gained | Travel skipped |
| :--- | :--- | :--- | :--- | :--- | :--- |
| save1/after-state to load1-travel/after-load | load | StingerAmmo, Stinger | - | - | - |
| load1-travel/after-load to load1-travel/after-travel | travel | StingerAmmo, Stinger | - | DefaultAmmo, DispersionPistol | no |
| load1-travel/after-travel to load2-travel/after-load | load | DefaultAmmo, DispersionPistol, Stinger, StingerAmmo | - | - | - |
| load2-travel/after-load to load2-travel/after-travel | travel | DefaultAmmo, DispersionPistol, Stinger, StingerAmmo | - | - | no |
| load2-travel/after-travel to load3/after-load | load | StingerAmmo, DispersionPistol, Stinger, DefaultAmmo | - | - | - |

## Verdict

Unreal Gold: state and inventory survive three maps (Vortex2, Nyleve, Dig).

## Limits of the claim

- Items compare by class name without case; the package (`Save<n>` after a load on Unreal Gold and Unreal Tournament, the map's package after a travel and after a load on Deus Ex) and the instance number change and are ignored.
- Health and position are not read, beyond the `STATE` line of the smoke log.
