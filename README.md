# Pack Medic

**[Português](README.pt-BR.md)**

A small [RE_Kenshi](https://www.nexusmods.com/kenshi/mods/847) plugin for Kenshi: when your medic runs out of first aid kits in the middle of a fight, they take one from the nearest pack animal in the squad and keep healing.

## What it does

Once per second the mod looks at your squad. For each character with the **Medic** or **Repair Robots** job:

1. If they have no kit of the right kind (in their inventory or in the backpack they wear), and
2. someone in the squad needs first aid, a splint or robot repair,
3. the nearest pack animal in the same squad that carries a kit hands **one** over — the best one it has, taken from its inventory or its backpack.

The character goes back to healing on their own. If the kit doesn't fit in their inventory, it stays on the animal.

| Job | Kit |
|---|---|
| Medic | first aid kits and splint kits |
| Repair Robots | robot repair kits |

Medics also **apply splints** on their own: when someone within `@range` needs one, the medic gets a "Splint Injuries" order, just like your click (at most one order per medic and per patient every 20 seconds). The game doesn't let "Splint Injuries" become a permanent job, so the mod does it for the Medic job (idea by Meloo on the Workshop).

Nothing happens while nobody is hurt, so a medic who also works a medical bench won't drain your garru into a crate.

## Install

1. Install **RE_Kenshi v0.3.5 or newer**.
2. Put the `PackMedic` folder in `Kenshi\mods\` (or subscribe on the Steam Workshop).
3. Enable **PackMedic** in the launcher.

## Settings

`Kenshi\mods\PackMedic\settings.txt`:

| Setting | Default | Meaning |
|---|---|---|
| `@range` | 1000 | Max distance between the medic and the animal (world units; ~30 is standing side by side) |
| `@interval` | 1000 | How often the squad is checked, in ms |
| `@debug` | 0 | `1` writes every step to `RE_Kenshi_log.txt` |

## Building

Needs the VC++ 2010 x64 compiler and the KenshiLib + boost 1.60 deps (same setup as [Kenshi Achievements](https://github.com/LucasSKrewer/kenshi-achievements)), then:

```
powershell -ExecutionPolicy Bypass -File build.ps1
```

## License

GPLv3 (inherited from KenshiLib). Fork it, tweak it — a ⭐ is appreciated.
