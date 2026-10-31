# Pack Medic

**[English](README.md)**

Um plugin [RE_Kenshi](https://www.nexusmods.com/kenshi/mods/847) pequeno para Kenshi: quando o médico fica sem kit de primeiros socorros no meio da briga, ele pega um do animal de carga mais próximo do grupo e continua curando.

## O que faz

Uma vez por segundo o mod olha o grupo. Para cada personagem com a tarefa **Médico** ou **Reparar robôs**:

1. se ele não tem kit do tipo certo (no inventário ou na mochila que está usando), e
2. alguém do grupo precisa de primeiros socorros, de tala ou de reparo de robô,
3. o animal de carga mais próximo do mesmo grupo que tem kit entrega **um** — o melhor que tiver, do inventário ou da mochila dele.

O personagem volta a curar sozinho. Se o kit não couber no inventário dele, fica no animal.

| Tarefa | Kit |
|---|---|
| Médico | kits de primeiros socorros e kits de talas |
| Reparar robôs | kits de reparo de robô |

O kit de talas vai pro Médico porque o jogo não deixa "Aplicar talas" virar tarefa fixa (ideia do Meloo na Workshop).

Nada acontece enquanto ninguém está ferido, então um médico que também trabalha na mesa médica não esvazia o garru no baú.

## Instalação

1. Instale o **RE_Kenshi v0.3.5 ou mais novo**.
2. Coloque a pasta `PackMedic` em `Kenshi\mods\` (ou assine na Steam Workshop).
3. Ative o **PackMedic** no launcher.

## Configuração

`Kenshi\mods\PackMedic\settings.txt`:

| Opção | Padrão | O que é |
|---|---|---|
| `@range` | 1000 | Distância máxima entre o médico e o animal (unidades do mundo; ~30 é lado a lado) |
| `@interval` | 1000 | De quanto em quanto tempo o grupo é conferido, em ms |
| `@debug` | 0 | `1` grava cada passo no `RE_Kenshi_log.txt` |

## Compilar

Precisa do compilador VC++ 2010 x64 e das deps KenshiLib + boost 1.60 (mesmo esquema do [Kenshi Achievements](https://github.com/LucasSKrewer/kenshi-achievements)), depois:

```
powershell -ExecutionPolicy Bypass -File build.ps1
```

## Licença

GPLv3 (herdada do KenshiLib). Pode forkear e mexer — uma ⭐ ajuda.
