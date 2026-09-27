# Vaelmoor — Prototipo

Proyecto Godot 4.x. Para probarlo:

1. Abre Godot 4 (Godot 4.2 o superior recomendado).
2. "Importar" y selecciona el archivo `project.godot` de esta carpeta.
3. Pulsa Play (F5). Se abrirá `scenes/World.tscn`.

## Controles
- Mover: WASD o flechas.
- Interactuar: E (habla con el NPC "Kaelen" acercándote y pulsando E).

## Qué hay ahora mismo
- Movimiento top-down con colisiones contra los límites del mapa.
- Un NPC (Kaelen, uno de los compañeros fijos) con diálogo básico.
- Sistema de "vínculo/aprobación" (`GameState.gd`) que sube al hablar con
  él; en el futuro determinará romance, ruptura o eventos de historia.
- Placeholders de color en vez de sprites (aún no hay pixel art).

## Siguiente paso sugerido
Sustituir los rectángulos de color por sprites de pixel art reales, y
añadir un segundo NPC / una primera decisión de diálogo con consecuencias.
