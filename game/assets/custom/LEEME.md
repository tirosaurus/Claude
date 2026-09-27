# Sprites personalizados (IA o dibujo a mano)

Cualquier PNG que pongas aquí **sustituye** al sprite generado por código con el mismo nombre.
Si borras el archivo, el juego vuelve a usar el original. No hace falta tocar código.

| Carpeta | Qué sustituye | Formato |
|---|---|---|
| `chars/<id>.png` | Sprite de mapa de un PNJ o compañero (`kaelen`, `yara`, `aelis`, `brom`, `mother`, `roc`, `thrall`...) | 8 columnas × 4 filas de **20×30**: filas abajo, izquierda, derecha, arriba; columnas reposo, respiración y 6 pasos |
| `battle/<id>.png` | Poses de combate de un compañero | 9 frames de **32×32** en fila, mirando a la **izquierda**: reposo, respiración, carga, golpe, magia, herido, agotado, KO, victoria |
| `enemies/<id>.png` | Enemigo (`larva`, `bat`, `brute`, `root`, `spectre`, `custodian`, `mother_root`, `wisp`, `boar`) | 2 frames en fila, mirando a la **derecha**, cualquier tamaño |
| `portraits/<id>.png` | Retrato de diálogo | **48×48** |
| `player/map_<raza>_<sexo>.png`, `player/battle_...`, `player/portrait_...` | Protagonista (`human_m`, `elf_f`, `dwarf_m`...) | Igual que arriba |

Los sprites personalizados del protagonista y de los compañeros se muestran tal cual: el pelo,
los colores y el equipo del creador solo se aplican a los sprites generados por código.

## Convertir imágenes de IA al formato correcto

`tools/import_sprite.py` recorta, escala, reduce la paleta, añade contorno y coloca cada frame en su sitio:

```bash
# Hoja de 4 direcciones × 4 frames (p. ej. de PixelLab)
python3 tools/import_sprite.py map kaelen kaelen_ia.png --grid 4x4
# Poses de combate (6 poses en una fila)
python3 tools/import_sprite.py battle kaelen poses.png --grid 6x1 --poses idle1=0,idle2=1,windup=2,strike=3,cast=4,hurt=5
# Enemigo
python3 tools/import_sprite.py enemy brute bruto_ia.png --grid 2x1 --height 84
# Retrato
python3 tools/import_sprite.py portrait yara yara_cara.png
```

Después: abre el proyecto en Godot para que importe los PNG nuevos y vuelve a exportar el juego.

## Prompts de ejemplo (PixelLab / Retro Diffusion)

- *"16-bit JRPG character sprite sheet, 4 directions walking, chibi proportions, medieval fantasy young
  swordsman, dark blue messy hair, grey tunic, red scarf, brown boots, transparent background"*
- *"pixel art side view battle poses facing left, idle, attack, casting, hurt, fantasy healer girl with
  long red hair, cream dress, wooden staff with green gem"*
- *"pixel art monster, corrupted purple ogre with glowing crystals on its back, 2 frame idle animation,
  facing right, dark fantasy"*

Consejo: pide fondo transparente, o un fondo liso que la herramienta pueda quitar (`--bg R,G,B`),
y la misma paleta y el mismo tamaño para todos los personajes, para que el estilo sea coherente.
