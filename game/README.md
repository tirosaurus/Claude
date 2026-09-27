# Vaelmoor — Prototipo

Proyecto Godot 4.x. Para probarlo:

1. Abre Godot 4 (Godot 4.2 o superior recomendado).
2. "Importar" y selecciona el archivo `project.godot` de esta carpeta.
3. Pulsa Play (F5). Empieza en `scenes/CharacterCreation.tscn`.

## Controles
- Mover: WASD o flechas.
- Interactuar: E (habla con NPCs, la cama, etc.).
- En combate: botones en pantalla (Atacar / Curar / Defender).

## Flujo actual del prólogo
1. **Creación de personaje** — nombre y raza.
2. **Dormitorio** — interactúa con la cama, luego baja las escaleras.
3. **Casa** — tu madre te saluda; sal por la puerta.
4. **Plaza de Tortosa** — habla con **Kaelen** (te invita al bosque); el
   camino al bosque se desbloquea al hacerlo.
5. **Bosque Santo** — habla con **Yara** (cogiendo flores) y luego busca
   la "bestia del bosque" para el primer combate. Al ganar y subir de
   nivel, elige tu primera rama: Cuerpo a cuerpo / A distancia / DPS /
   Sanador / Tanque.

## Qué lleva el motor de estado (`GameState.gd`)
- Datos del personaje: nombre, raza elegida.
- Progresión: nivel, XP, HP/maná, rama elegida.
- Aprobación de cada compañero, el eje amistad↔rivalidad de Kaelen, la
  relación aparte de Yara con Kaelen, la corrupción de Yara (curandera ↔
  bruja oscura) y hacia quién apunta su romance (jugador/Kaelen/ambos).
- `quest_flags`: flags narrativos simples (p. ej. `kaelen_invited`,
  `yara_met`) para desbloquear puertas y diálogos.
- Reputación de facciones.

Ver `DESIGN.md` (en la raíz del repo) para el diseño completo.

## Qué falta (siguiente en la lista)
- Sprites de pixel art reales (todo son placeholders de color).
- Una primera decisión de diálogo con ramas reales (que mueva la
  rivalidad de Kaelen o la corrupción/relación de Yara según lo elegido).
- La Catedral Vieja en ruinas.
