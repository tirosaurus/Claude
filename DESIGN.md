# Vaelmoor — Documento de Diseño (v0.1)

## Visión general
RPG de rol en 2D pixel art, vista top-down, estilo medieval-fantástico oscuro
(tono Overlord / Dragon Age). El jugador crea su propio personaje (raza, clase,
apariencia, trasfondo) y se mueve por un mundo con facciones en conflicto,
un elenco de compañeros fijos con los que puede forjar vínculos, romances o
enemistades, y consecuencias permanentes (muerte de compañeros, cambios de
facción, finales múltiples).

Esto es un documento vivo: lo iremos ampliando según juguemos con el prototipo.

---

## 1. El Mundo: Vaelmoor

Un continente fracturado tras la Caída del Imperio de Cristal hace 300 años,
cuando el último Archimago intentó ascender a la divinidad y desgarró el velo
entre planos. Desde entonces:

- Monstruos y no-muertos campan por tierras antaño civilizadas.
- Poderes ocultos (liches, demonios, semidioses) gobiernan en las sombras
  algunas regiones, tal como en Overlord.
- Las facciones "civilizadas" luchan tanto contra las amenazas del Velo
  Roto como entre sí por recursos y territorio.

### Regiones principales (borrador)
1. **El Concilio de Aurelia** — restos del imperio humano, ahora una
   confederación de ciudades-estado gobernadas por nobles y gremios de magos.
2. **Sylvaneth** — bosque ancestral de los elfos, dividido entre la Corte
   de la Savia (elfos "altos", tradicionalistas) y los Renegados de la
   Espina (elfos oscuros, practican magia prohibida).
3. **Khazgurim** — reino subterráneo enano, en guerra fría con la superficie
   por el control de las minas de mithril-sombra.
4. **Las Tierras Quebradas** — territorio anárquico donde surgió una
   Torre Negra gobernada por un ser no-muerto de poder desconocido
   (posible "señor oscuro" al estilo Ainz, con su propia facción de
   monstruos leales).
5. **La Liga Libre de Puerto Ceniza** — ciudad-estado mercantil neutral,
   punto de encuentro de todas las facciones, sede del Gremio de
   Aventureros.

### Facciones jugables/relevantes (para reputación)
- Concilio de Aurelia (orden, ley, sospecha de la magia)
- Corte de la Savia (tradición, pureza élfica)
- Renegados de la Espina (magia prohibida, ambición)
- Khazgurim (honor, comercio, aislacionismo)
- Torre Negra (el "misterio" central, facción monstruosa/oscura)
- Gremio de Aventureros (neutral, mercenarios, tu base de operaciones)

Cada facción tendrá reputación (-100 a +100), quests exclusivas, y
finales de historia influidos por con quién te alías.

---

## 2. Creación de personaje

### Razas jugables (borrador)
- **Humano** — versátil, bonus a reputación social.
- **Elfo (Savia o Espina)** — bonus a magia/destreza según subrama.
- **Enano** — bonus a resistencia y combate cuerpo a cuerpo.
- **Bestial (Therian)** — híbridos animales, bonus a percepción/velocidad,
  mal vistos por el Concilio.
- **Sangre-Oscura (semi-demonio)** — bonus a magia oscura, penalización
  social fuerte, desbloquea diálogos únicos con la Torre Negra.
- **Renacido (no-muerto jugable, raza rara/desbloqueable)** — sin
  necesidad de comer/dormir, inmune a veneno, pero teme y provoca terror;
  la mayoría de facciones "civilizadas" empiezan hostiles.

### Clases (borrador, ampliable con subclases)
- Guerrero, Paladín, Pícaro, Explorador, Mago Arcano, Nigromante,
  Clérigo, Invocador, Berserker, Bardo.

### Trasfondo (background)
Determina el punto de partida narrativo, una facción inicial con la que
ya tienes historia (buena o mala) y 1-2 rasgos de diálogo únicos.

---

## 3. Compañeros y vínculos

Elenco de protagonistas fijos (a definir con nombres/personalidad más
adelante), cada uno con:
- Barra de aprobación/vínculo (-100 a +100) que sube o baja con tus
  decisiones y diálogos.
- Una "quest personal" que profundiza su trasfondo y puede cambiar su
  destino.
- Posibilidad de **romance** en umbrales altos de aprobación (con
  reactividad: celos, rupturas, exclusividad o poliamor si encaja el tono).
- Posibilidad de **ruptura irreversible** (echarlo del grupo) o de
  **muerte permanente** en momentos clave de la trama (estilo Dragon Age:
  Origins/Inquisition) — un compañero muerto no vuelve, y el mundo/otros
  personajes reaccionan a su ausencia.
- Reactividad cruzada: los compañeros comentan entre ellos y con el
  jugador según raza/clase/decisiones tomadas.

---

## 4. Sistemas técnicos a construir (roadmap)

1. **Prototipo (vertical slice actual)**: movimiento top-down, un mapa
   pequeño, un NPC con diálogo básico y una barra de aprobación funcional.
2. Sistema de diálogo ramificado con condiciones (reputación, raza, clase).
3. Sistema de combate (por turnos o acción táctica — a decidir).
4. Sistema de inventario/equipo.
5. Sistema de facciones y reputación global.
6. Sistema de guardado/carga.
7. Generación/gestión de mapas de mundo (varias regiones conectadas).
8. Arte: pixel art de personajes, tiles, UI (a definir estilo: 16x16, 32x32).

---

## 5. Próximos pasos inmediatos
- [x] Documento de diseño inicial.
- [ ] Prototipo Godot: mover un personaje por un mapa con colisiones.
- [ ] Un NPC de prueba con diálogo y barra de aprobación.
- [ ] Definir estilo visual pixel art (resolución de sprite, paleta).
- [ ] Nombrar y perfilar a los primeros 2-3 compañeros fijos.
