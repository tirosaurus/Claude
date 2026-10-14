# Vaelmoor — versión Game Boy Advance

Port aparte del juego de PC/móvil (no lo sustituye). Esta primera entrega es una **demo jugable**:
dormitorio → casa (madre) → Tortosa (Kaelen) → Bosque Santo (Yara, elección de clase) → combate contra el lobo alfa.

- `Vaelmoor.gba`: la ROM lista para jugar (mGBA, VisualBoyAdvance, Delta en iPhone, o un cartucho flash).
- Controles: cruceta para moverse, **B** para correr, **A** para hablar/examinar/confirmar, **START** en el título.

## Compilar
Requiere `gcc-arm-none-eabi` (con newlib) y Python 3 con Pillow y numpy.

    python3 tools/conv.py   # convierte mapas, sprites, retratos y fuente desde ../game
    make

`tools/gbarun.c` es un ejecutor sin pantalla basado en libmgba para probar la ROM con guiones de botones.
