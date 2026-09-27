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

### Los Moronguls (amenaza base, no facción política)
Vienen de las Tierras del Sur, más allá de todo mapa conocido. No negocian,
no tienen ciudades, no responden a reputación: son la amenaza "de base" del
juego, equivalente a los muertos vivientes de otras ficciones pero con
identidad propia.

- Son una infección viva: cuerpos y bosques "morongulizados" se cubren de
  un tejido oscuro pulsante, mitad carne mitad corteza podrida.
- No conquistan por ejército, **corrompen por contacto**: una zona santa o
  pura, si queda expuesta el tiempo suficiente, empieza a mutar.
- Cuanto más tiempo pasa la historia, más avanza la corrupción por el
  mapa si el jugador no actúa — un reloj narrativo de fondo.
- Su origen y si tienen o no una inteligencia rectora detrás (una especie
  de "Rey Corrupto") es uno de los grandes misterios centrales de la trama,
  y probablemente se conecta con la Torre Negra y la Caída del Imperio de
  Cristal.

---

## 1.5. El pueblo natal: Tortosa

Tortosa es donde empieza la historia, un pueblo de montaña con muy pocos
habitantes, aislado del resto de Vaelmoor por los puertos de montaña. Es el
hogar de la infancia del jugador, de Kaelen y de Yara (ver sección 3).

- **La Catedral Vieja** — una catedral desproporcionadamente enorme para
  un pueblo tan pequeño, en ruinas desde hace generaciones. Nadie recuerda
  bien por qué se construyó algo tan grande aquí ni por qué se abandonó;
  los niños del pueblo (el jugador, Kaelen y Yara) jugaban de pequeños
  entre sus naves derruidas. Guarda secretos relacionados con el Imperio
  de Cristal y probablemente con el propio origen de los Moronguls.
- **El Bosque Milenario (Bosque Santo)** — linda con Tortosa, considerado
  sagrado desde tiempos inmemoriales, protegido por un pacto antiguo que
  mantiene alejada cualquier corrupción. Es el escenario de la infancia
  del trío protagonista y, más adelante, **el primer lugar en corromperse**
  cuando lleguen los Moronguls — el pueblo pasa de ser un remanso de paz a
  primera línea de la amenaza que definirá el juego.
- Tortosa funciona como zona tutorial y como "hogar emocional" al que la
  historia vuelve: cuanto más significa el pueblo para el jugador antes de
  la corrupción, más pesa perderlo (o salvarlo) después.

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

### Progresión: de rama a clase
No se elige una clase cerrada desde el minuto uno. Al empezar solo tienes
acciones básicas (Atacar, Curar, Defender) y vas definiendo tu build por
niveles:

1. **Primer nivel (tutorial del bosque)**: eliges una **rama básica** entre
   Cuerpo a cuerpo, A distancia, DPS, Sanador o Tanque. Esto ya condiciona
   qué acciones/bonus empiezas a desbloquear.
2. **Niveles siguientes**: cada rama se ramifica en subramas más
   específicas y, más adelante, cristaliza en clases completas (Guerrero,
   Paladín, Pícaro, Explorador, Mago Arcano, Nigromante, Clérigo,
   Invocador, Berserker, Bardo...), con nombres y habilidades distintas
   según la raza y facción del jugador.
3. El sistema es deliberadamente abierto al principio para que el jugador
   pruebe combate antes de comprometerse a una identidad de personaje.

### Trasfondo (background)
Determina el punto de partida narrativo, una facción inicial con la que
ya tienes historia (buena o mala) y 1-2 rasgos de diálogo únicos.

### Raza elegible desde el inicio
La raza se elige en la creación de personaje (con Humano como opción por
defecto) y condiciona cómo reacciona el mundo hacia ti — facciones,
diálogos únicos, aceptación o rechazo en ciertas regiones. El diseño fino
de cada raza (bonus, restricciones, quests exclusivas) se hará más
adelante; de momento la elección ya queda registrada en `GameState`.

---

## 3. Compañeros y vínculos

El elenco fijo nace como un trío de la infancia en Tortosa. Los tres
crecieron jugando entre las ruinas de la Catedral Vieja y el Bosque Santo.
Cada compañero tiene, además de la aprobación general (-100 a +100):
- Una "quest personal" que profundiza su trasfondo y puede cambiar su
  destino.
- Posibilidad de **ruptura irreversible** (echarlo del grupo) o de
  **muerte permanente** en momentos clave de la trama (estilo Dragon Age:
  Origins/Inquisition) — un compañero muerto no vuelve, y el mundo/otros
  personajes reaccionan a su ausencia.
- Reactividad cruzada: los compañeros comentan entre ellos y con el
  jugador según raza/clase/decisiones tomadas.

### Kaelen — el amigo de la infancia (eje amistad/rivalidad)
Compañero desde el primer minuto. Su vínculo con el jugador no se mide solo
en aprobación, sino en un **eje independiente de Amistad ↔ Rivalidad**
(-100 amistad total, +100 rivalidad total):

- **Ruta de amistad (Harry & Ron)**: decisiones leales, compartir el
  mérito, defenderlo en público. Kaelen se convierte en el mejor apoyo
  posible: fiable, con su propio arco de crecimiento, dispuesto a
  sacrificarse por el jugador.
- **Ruta de rivalidad (Naruto & Sasuke)**: decisiones egoístas, competir
  por reconocimiento, elegir una facción/bando opuesto al suyo, o dejar
  que la corrupción de Tortosa le toque más duro a él que a ti. Kaelen se
  vuelve cada vez más distante, puede acabar abandonando el grupo por
  voluntad propia, uniéndose a una facción enemiga, o convirtiéndose en un
  **jefe/antagonista recurrente** en vez de un compañero de la party.
- El eje es de doble sentido: se puede caer en rivalidad y luego
  reconciliarse (con esfuerzo), pero pasado cierto umbral algunas rupturas
  se vuelven permanentes (igual que la muerte de un compañero).
- Es romanceable si el jugador lo desea (compatible con la ruta de
  amistad alta, incompatible con la de rivalidad).

### Yara — la amiga común (triángulo)
Yara creció con ambos por igual y no toma partido de forma automática: su
relación con el jugador y con Kaelen evoluciona en paralelo, no en
exclusiva.

- Tiene su propia aprobación con el jugador **y** su propia relación con
  Kaelen (llevada aparte, afectada por la ruta de amistad/rivalidad de
  éste).
- Variables que deciden su romance: aprobación del jugador con ella,
  estado de la relación Kaelen-jugador (amistad, rivalidad o ruptura),
  aprobación de Kaelen con ella, y decisiones explícitas de diálogo
  (declaraciones, celos, honestidad).
- Rutas posibles: romance solo con el jugador, romance solo con Kaelen
  (el jugador queda como confidente/tercero), un **trío consentido** si
  la ruta de amistad de Kaelen es alta y ambos lo aceptan explícitamente,
  o ninguna de las anteriores si el jugador prioriza la amistad platónica
  o la aleja por decisiones tomadas.
- Si Kaelen muere o se convierte en enemigo, Yara reacciona de forma
  fuerte y duradera (duelo, resentimiento hacia el jugador si lo considera
  responsable, o unión aún mayor si lo apoyó hasta el final).

### Compañeros futuros
Más adelante (Concilio de Aurelia, Khazgurim, etc.) se sumarán compañeros
reclutables fuera del trío inicial, con su propia aprobación y arcos, pero
Kaelen y Yara son el núcleo emocional fijo de la historia.

---

## 3.5. Prólogo jugable (el que existe ahora mismo)

Flujo actual del prototipo, estilo "despertar en tu casa" tipo Pokémon:

1. **Creación de personaje** — nombre y raza (Humano por defecto).
2. **Dormitorio** — una cama interactuable ("ya has dormido bastante") y
   unas escaleras que bajan a la casa.
3. **Casa** — tu madre te comenta que hace buen día y te pregunta si has
   descansado bien; te anima a salir a ver a tus amigos.
4. **Plaza de Tortosa** — Kaelen te espera aburrido y te propone ir al
   Bosque Santo. Hasta no hablar con él, el camino al bosque queda
   bloqueado (con un mensaje recordándotelo).
5. **Bosque Santo** — encuentras a Yara cogiendo flores; los tres vais de
   aventura. Aparece una bestia del bosque (aún no un morongul — el bosque
   todavía no está corrompido) con la que se libra el primer combate:
   Atacar / Curar / Defender. Al ganar y subir de nivel, se elige la
   primera rama de progresión (sección 2).

Todo con placeholders de color; el foco de esta fase es la estructura
narrativa y los sistemas (flags de historia, combate básico, progresión),
no el arte.

---

## 4. Sistemas técnicos a construir (roadmap)

1. **Prototipo (vertical slice actual)**: prólogo completo (arriba),
   combate por turnos básico, sistema de niveles y ramas iniciales.
2. Sistema de diálogo ramificado con condiciones (reputación, raza, clase).
3. Ampliar el combate: habilidades específicas por rama/clase, más
   enemigos, dificultad progresiva.
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
- [x] Nombrar y perfilar a los primeros 2-3 compañeros fijos (Kaelen, Yara).
- [x] Prólogo jugable: casa, madre, plaza, invitación de Kaelen, bosque
      con Yara, primer combate y elección de rama al subir de nivel.
- [ ] Primera decisión de diálogo real con ramas narrativas (mover
      rivalidad de Kaelen o la relación/corrupción de Yara según lo que
      se elija responder).
- [ ] Escena de la Catedral Vieja en ruinas (aún no visitada).
- [ ] Sprites de pixel art reales para jugador, Kaelen, Yara y madre.
