# Vaelmoor — Demo jugable

RPG de rol en pixel art (Godot 4.3). La demo va desde que te despiertas en
tu casa de Tortosa hasta el combate contra el lobo corrupto en el Bosque
Santo, y termina con la primera pista de lo que se avecina.

## Jugar sin instalar nada (Windows)
1. Descarga `builds/Vaelmoor-Windows.zip` del repositorio.
2. Descomprímelo y ejecuta `Vaelmoor/Vaelmoor.exe`.
   (Windows puede avisar de "editor desconocido": *Más información → Ejecutar de todas formas*.)

## Jugar desde Godot
1. Abre Godot 4 (4.3 o superior) → Importar → `game/project.godot`.
2. Pulsa Play (F5).

## Controles
| Acción | Teclado | Mando |
|---|---|---|
| Moverse | WASD / flechas | Cruceta |
| Correr | Mayús | X / Cuadrado |
| Hablar / interactuar / avanzar texto | E, Espacio o Intro | A / Cruz |
| Acelerar texto / saltar intro | X o Retroceso | B / Círculo |
| Pausa | Esc | Start |

## Qué incluye
- **Creación de personaje**: nombre y raza (6 razas; la raza ya queda registrada para el futuro).
- **Prólogo completo**: tu habitación, tu madre, el pueblo de Tortosa con la Catedral Vieja,
  vecinos con los que hablar, Kaelen y Yara uniéndose al grupo y siguiéndote.
- **Decisiones con consecuencias**: cada respuesta mueve la amistad/rivalidad con Kaelen,
  la aprobación de Yara o su corrupción; se resumen al final.
- **Combate por turnos en grupo**: Atacar / Defender / Curar; Kaelen y Yara actúan solos,
  el lobo se enfurece a mitad de combate. Al subir de nivel eliges tu rama
  (Cuerpo a cuerpo, A distancia, DPS, Sanador, Tanque).
- Pixel art, música y efectos originales; luces con parpadeo, partículas,
  profundidad (pasas por delante y detrás de árboles y muebles) y colisiones.

## Cómo está hecho
- `tools/build_assets.py` (en la raíz del repo) genera **todo** el arte, los mapas y el audio
  en `game/assets/` (Python + Pillow + numpy). Para regenerar: `python3 tools/build_assets.py`.
  - `tools/chars.py`: personajes y retratos · `tools/props.py`: muebles, casas, catedral,
    árboles, lobo · `tools/maps.py`: diseño de los mapas · `tools/ui.py`: interfaz y fondos ·
    `tools/audio.py`: música y efectos.
- `scripts/world/World.gd` monta cada mapa desde su JSON (suelo, colisiones, props, luces,
  salidas y zonas de evento).
- `scripts/story/Story.gd` contiene el guion: escenas, diálogos y decisiones.
- `scripts/battle/Battle.gd` es el combate; `scripts/autoload/` el estado, audio, diálogos y transiciones.

Fuente: Pixelify Sans (SIL Open Font License, ver `assets/fonts/OFL.txt`).
