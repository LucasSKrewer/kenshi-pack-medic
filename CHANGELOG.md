# Changelog

## [1.1.0] — 2026-10-31

- Medics now **apply splints** on their own: when someone nearby needs one, the medic gets a "Splint Injuries" order (taking a splint kit from the pack animal first if they have none). One order per medic and per patient every 20 s, only for patients within `@range`. The game doesn't let "Splint Injuries" become a permanent job, so it goes with the Medic job. Idea by Meloo on the Steam Workshop.
- `@debug` also lists each character's job ids and the splint check.

## [1.0.0] — 2026-10-31

First release.

- Characters with the **Medic** job take a first aid kit from the nearest pack animal in the squad when they run out and someone needs first aid.
- Characters with the **Repair Robots** job do the same with robot repair kits.
- Kits are found in the animal's inventory and in the backpack it wears; the best kit goes first, one at a time.
- `settings.txt`: `@range`, `@interval`, `@debug`.
