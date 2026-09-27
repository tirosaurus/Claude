# Vaelmoor — Crónicas del Velo Roto

RPG de rol en pixel art (Godot 4.3) con combate estilo Final Fantasy, historia
ramificada y varios finales.

## Jugar
- **Windows sin instalar nada:** descomprime `builds/Vaelmoor-Windows.zip` y ejecuta `Vaelmoor/Vaelmoor.exe`.
- **Desde Godot:** Importar → `game/project.godot` → Play (F5).

## Controles
| Acción | Teclado | Mando |
|---|---|---|
| Moverse | WASD / flechas | Cruceta |
| Correr | Mayús | X / Cuadrado |
| Hablar / confirmar | E, Espacio o Intro | A / Cruz |
| Volver / acelerar texto | X o Retroceso | B / Círculo |
| Menú (grupo, objetos, **guardar**) | Esc | Start |
| Combate automático | Mayús (en combate) | |

## Qué hay en el juego
- **Creador de personaje:** Humano, Elfo o Enano (cada uno con su cuerpo, su retrato y un rasgo propio),
  hombre o mujer, 7 peinados, 10 colores de pelo, 5 tonos de piel, 6 colores de ropa y barba.
- **Combate por turnos activos (ATB):** barras de tiempo, controlas a todo el grupo, menú con iconos
  (Atacar, Habilidades, Objetos, Defender, Huir), eliges objetivo con una mano (también a quién curas),
  elementos y debilidades, críticos, estados alterados (veneno, aturdido, regeneración, protección, furia,
  provocación, debilidad), jefes con fases e invocaciones, y modo automático.
- **Progresión:** rama al nivel 2 (Cuerpo a cuerpo, A distancia, DPS, Sanador, Tanque) y clase al nivel 5
  (Caballero, Berserker, Arquero, Cazador, Pícaro, Mago arcano, Clérigo, Druida, Paladín, Guardián),
  cada una con sus habilidades hasta el nivel 9.
- **Compañeros:** Kaelen (amigo o rival), Yara (curandera... o bruja), y según tus decisiones Aelis
  (arquera élfica) o Brom (guerrero enano).
- **Historia en 4 actos:** Tortosa, el ataque nocturno de los Moronguls, la Catedral y su cripta, el
  bosque corrupto, el campamento élfico y el Corazón del Bosque.
- **Decisiones con consecuencias** y **7 finales** (muy bueno, bueno, agridulce, dos malos y dos fatales),
  con epílogo del destino de cada personaje y romance.
- **Guardado** en 3 ranuras desde el menú (Esc) o en las hogueras; **Continuar/Cargar** desde el título.
- Tiendas, oro, objetos, mejoras de equipo, cofres y encuentros aleatorios.
- **v3:** sprites nuevos más grandes y detallados (mapa 20x30 y poses de combate: ataque, magia, herido,
  agotado, KO y victoria, con el arma de cada rama), enemigos redibujados con volumen y dos nuevos
  (fuego fatuo y jabalí), interfaz de combate estilo JRPG clásico con barras de vida/PM y retratos,
  efectos por elemento, banda sonora orquestal (cuerdas, metales, coro, timbales) y menú de **Opciones**
  (volumen, velocidad de texto y de combate, pantalla completa). El maná ya no se rellena al subir de nivel.
- **v4:** eliges senda y clase con Yara antes del lobo (Guerrero, Pícaro, Paladín, Mago, Brujo, Cazador,
  Clérigo, Druida, Guardián), cada una con un **árbol de 9 talentos** y puntos al subir de nivel; **equipo**
  para todo el grupo (arma, escudo, cabeza, cuerpo, anillo) que se ve en el personaje, con **rarezas**
  (Básico, Común, Raro, Muy raro, Divino, Legendario) y armas legendarias con historia; la **Cueva de los
  Susurros** (mazmorra opcional); dificultad Fácil/Normal/Difícil; PM que se recuperan por turno; caminar
  con 6 fases; madre de tu raza; el ataque nocturno animado; y el menú (Esc) disponible siempre.

## Cómo está hecho
Todo el arte, los mapas y el audio se generan con `tools/build_assets.py` (Python + Pillow + numpy):
`chars2.py` (rig de personajes por piezas, poses de combate y capas del creador; `chars.py` retratos),
`enemies2.py` (enemigos esculpidos con volumen), `props.py`/`props2.py` (escenarios),
`maps.py`/`maps2.py` (diseño de mapas), `ui.py` (interfaz y fondos), `orch.py` + `songs.py`
(sintetizador orquestal y las 16 pistas en OGG) y `sfx2.py` (efectos de sonido).
Requisitos: `pip install pillow numpy scipy soundfile`.

En Godot: `scripts/data/DB.gd` (razas, clases, habilidades, objetos, enemigos), `scripts/battle/` (combate),
`scripts/world/` (mapas, jugador, PNJ), `scripts/story/maps/` (guion por mapa), `scripts/ui/` (menús,
creador, finales) y `scripts/autoload/` (estado, guardado, diálogos, audio, apariencia).

Fuente: Pixelify Sans (SIL Open Font License, ver `assets/fonts/OFL.txt`).
