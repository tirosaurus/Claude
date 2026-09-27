# Vaelmoor — Prototipo

Proyecto Godot 4.x. Para probarlo:

1. Abre Godot 4 (Godot 4.2 o superior recomendado).
2. "Importar" y selecciona el archivo `project.godot` de esta carpeta.
3. Pulsa Play (F5). Se abrirá `scenes/World.tscn`.

## Controles
- Mover: WASD o flechas.
- Interactuar: E (acércate a Kaelen o Yara y pulsa E para hablar).

## Qué hay ahora mismo
- Movimiento top-down con colisiones, en una plaza de Tortosa a modo de
  boceto (aún sin catedral ni bosque dibujados, solo placeholders).
- Dos NPCs: **Kaelen** (amigo de la infancia, eje amistad/rivalidad) y
  **Yara** (la amiga común, triángulo de romance con jugador/Kaelen/ambos).
- `GameState.gd` lleva: aprobación general de cada compañero, el eje
  amistad↔rivalidad de Kaelen, la relación aparte de Yara con Kaelen, y
  recalcula automáticamente hacia quién apunta el romance de Yara
  ("player", "kaelen", "both" o "none"). Ver DESIGN.md sección 3.
- Placeholders de color en vez de sprites (aún no hay pixel art).

## Siguiente paso sugerido
Añadir una primera decisión de diálogo real (con ramas que muevan el eje
de rivalidad de Kaelen o la relación de Yara con él), y/o sustituir los
rectángulos de color por sprites de pixel art.
