class_name DB
extends RefCounted
## Datos del juego: razas, clases, habilidades, objetos, enemigos y tiendas.

const RACES := {
	"human": {"name": "Humano", "desc": "Versátiles y tenaces. Su determinación les da golpes críticos más frecuentes.",
		"trait": "Determinación: +10% de probabilidad de crítico.",
		"base": {"hp": 38, "mp": 14, "atk": 9, "def": 5, "mag": 7, "res": 5, "spd": 9},
		"grow": {"hp": 7.0, "mp": 3.0, "atk": 2.0, "def": 1.5, "mag": 1.5, "res": 1.3, "spd": 1.0}},
	"elf": {"name": "Elfo", "desc": "Longevos y afines a la magia. La sangre antigua les devuelve maná cada turno.",
		"trait": "Sangre antigua: recuperas 2 de maná en cada turno.",
		"base": {"hp": 32, "mp": 20, "atk": 8, "def": 4, "mag": 10, "res": 7, "spd": 11},
		"grow": {"hp": 5.5, "mp": 4.0, "atk": 1.5, "def": 1.1, "mag": 2.4, "res": 1.8, "spd": 1.3}},
	"dwarf": {"name": "Enano", "desc": "Duros como la roca de Khazgurim. Inmunes al veneno y difíciles de tumbar.",
		"trait": "Piel de piedra: inmune al veneno y -10% de daño físico recibido.",
		"base": {"hp": 46, "mp": 10, "atk": 10, "def": 7, "mag": 4, "res": 6, "spd": 7},
		"grow": {"hp": 9.0, "mp": 2.0, "atk": 2.2, "def": 2.2, "mag": 0.8, "res": 1.4, "spd": 0.7}},
}

## Sendas (se eligen al nivel 2 junto con la clase)
const BRANCHES := {
	"melee": {"name": "Cuerpo a cuerpo", "desc": "Primera línea: acero, sangre y aguante.", "icon": "icon_melee",
		"bonus": {"hp": 10, "atk": 3, "def": 1}, "classes": ["warrior", "rogue", "paladin"]},
	"ranged": {"name": "Distancia y magia", "desc": "Desde atrás: arcos, hechizos y pactos oscuros.", "icon": "icon_ranged",
		"bonus": {"hp": 4, "mp": 6, "mag": 2, "spd": 2}, "classes": ["mage", "warlock", "hunter"]},
	"support": {"name": "Apoyo", "desc": "Mantener vivo al grupo: curar, proteger y aguantar.", "icon": "icon_healer",
		"bonus": {"hp": 8, "mp": 6, "def": 1, "res": 2}, "classes": ["cleric", "druid", "guardian"]},
}

## Clases con árbol de talentos.  Cada nodo: tier (1-3), max (rangos), skill (habilidad que desbloquea),
## stats (por rango) o perk [clave, valor por rango].  Nivel 2 del árbol exige 3 puntos gastados en la
## clase y el nivel 3, 6 puntos.
const CLASSES := {
	"warrior": {"name": "Guerrero", "desc": "Fuerza bruta y furia. Daño físico enorme y mucho aguante.", "icon": "icon_melee",
		"weapon": "sword", "bonus": {"hp": 12, "atk": 4, "def": 2}, "starter": "tajo_doble",
		"tree": [
			{"id": "w_furia", "tier": 1, "max": 1, "skill": "furia", "name": "Furia"},
			{"id": "w_fuerza", "tier": 1, "max": 3, "stats": {"atk": 3}, "name": "Fuerza", "desc": "+3 de ataque por rango."},
			{"id": "w_piel", "tier": 1, "max": 3, "stats": {"hp": 10, "def": 2}, "name": "Piel curtida", "desc": "+10 vida y +2 defensa por rango."},
			{"id": "w_carga", "tier": 2, "max": 1, "skill": "carga", "name": "Carga"},
			{"id": "w_remolino", "tier": 2, "max": 1, "skill": "remolino", "name": "Remolino"},
			{"id": "w_sed", "tier": 2, "max": 2, "perk": ["lifesteal", 0.06], "name": "Sed de sangre", "desc": "Tus golpes físicos te curan un 6% del daño por rango."},
			{"id": "w_masacre", "tier": 3, "max": 1, "skill": "masacre", "name": "Masacre"},
			{"id": "w_grito", "tier": 3, "max": 1, "skill": "grito", "name": "Grito de guerra"},
			{"id": "w_juramento", "tier": 3, "max": 1, "skill": "juramento", "name": "Juramento"},
		]},
	"rogue": {"name": "Pícaro", "desc": "Rápido y letal. Críticos, venenos y robos.", "icon": "icon_dps",
		"weapon": "daggers", "bonus": {"atk": 4, "spd": 5}, "starter": "punalada",
		"tree": [
			{"id": "r_robar", "tier": 1, "max": 1, "skill": "robar", "name": "Robar"},
			{"id": "r_agil", "tier": 1, "max": 3, "stats": {"spd": 2, "atk": 1}, "name": "Agilidad", "desc": "+2 velocidad y +1 ataque por rango."},
			{"id": "r_prec", "tier": 1, "max": 3, "perk": ["crit", 0.05], "name": "Precisión", "desc": "+5% de crítico por rango."},
			{"id": "r_sombras", "tier": 2, "max": 1, "skill": "sombras", "name": "Golpe en las sombras"},
			{"id": "r_veneno", "tier": 2, "max": 1, "skill": "hoja_venenosa", "name": "Hoja venenosa"},
			{"id": "r_rapido", "tier": 2, "max": 2, "perk": ["atb_start", 25.0], "name": "Reflejos", "desc": "Empiezas los combates con la barra de tiempo más llena."},
			{"id": "r_mil", "tier": 3, "max": 1, "skill": "mil_cortes", "name": "Mil cortes"},
			{"id": "r_asesinar", "tier": 3, "max": 1, "skill": "asesinar", "name": "Asesinar"},
			{"id": "r_letal", "tier": 3, "max": 2, "perk": ["dmg_phys", 0.1], "name": "Letalidad", "desc": "+10% de daño físico por rango."},
		]},
	"paladin": {"name": "Paladín", "desc": "Acero y fe: golpea con luz, protege y cura.", "icon": "icon_tank",
		"weapon": "sword", "bonus": {"hp": 12, "def": 2, "mag": 3}, "starter": "aux",
		"tree": [
			{"id": "p_juicio", "tier": 1, "max": 1, "skill": "juicio", "name": "Juicio"},
			{"id": "p_fe", "tier": 1, "max": 3, "stats": {"mag": 2, "res": 2}, "name": "Fe", "desc": "+2 magia y +2 resistencia por rango."},
			{"id": "p_armadura", "tier": 1, "max": 3, "stats": {"hp": 10, "def": 2}, "name": "Armadura de fe", "desc": "+10 vida y +2 defensa por rango."},
			{"id": "p_escudo", "tier": 2, "max": 1, "skill": "escudo_sagrado", "name": "Escudo sagrado"},
			{"id": "p_provocar", "tier": 2, "max": 1, "skill": "provocar", "name": "Provocar"},
			{"id": "p_martillo", "tier": 2, "max": 1, "skill": "martillo_luz", "name": "Martillo de luz"},
			{"id": "p_alba", "tier": 3, "max": 1, "skill": "alba", "name": "Luz del alba"},
			{"id": "p_aura", "tier": 3, "max": 2, "perk": ["heal_pow", 0.15], "name": "Aura sagrada", "desc": "+15% a tus curaciones por rango."},
			{"id": "p_resu", "tier": 3, "max": 1, "skill": "resurreccion", "name": "Resurrección"},
		]},
	"mage": {"name": "Mago", "desc": "Fuego, hielo y rayo. La mayor destrucción mágica.", "icon": "icon_dps",
		"weapon": "staff", "bonus": {"mag": 6, "mp": 10}, "starter": "fuego",
		"tree": [
			{"id": "m_hielo", "tier": 1, "max": 1, "skill": "hielo", "name": "Hielo"},
			{"id": "m_int", "tier": 1, "max": 3, "stats": {"mag": 3}, "name": "Intelecto", "desc": "+3 de magia por rango."},
			{"id": "m_medit", "tier": 1, "max": 3, "perk": ["mp_regen", 1.0], "name": "Meditación", "desc": "+1 PM recuperado por turno y rango."},
			{"id": "m_rayo", "tier": 2, "max": 1, "skill": "rayo", "name": "Rayo"},
			{"id": "m_llama", "tier": 2, "max": 1, "skill": "llamarada", "name": "Llamarada"},
			{"id": "m_pot", "tier": 2, "max": 2, "perk": ["dmg_mag", 0.1], "name": "Potencia arcana", "desc": "+10% de daño mágico por rango."},
			{"id": "m_meteoro", "tier": 3, "max": 1, "skill": "meteoro", "name": "Meteoro"},
			{"id": "m_ventisca", "tier": 3, "max": 1, "skill": "ventisca", "name": "Ventisca"},
			{"id": "m_sobre", "tier": 3, "max": 1, "skill": "sobrecarga", "name": "Sobrecarga"},
		]},
	"warlock": {"name": "Brujo", "desc": "Pactos de sombra: maldiciones, venenos y robo de vida.", "icon": "icon_dps",
		"weapon": "staff", "orb": "dark", "bonus": {"mag": 5, "mp": 8, "hp": 6}, "starter": "maldicion",
		"tree": [
			{"id": "k_drenar", "tier": 1, "max": 1, "skill": "drenar", "name": "Drenar"},
			{"id": "k_sombra", "tier": 1, "max": 3, "stats": {"mag": 3}, "name": "Sombra interior", "desc": "+3 de magia por rango."},
			{"id": "k_pacto", "tier": 1, "max": 3, "stats": {"hp": 8, "mp": 5}, "name": "Pacto de sangre", "desc": "+8 vida y +5 PM por rango."},
			{"id": "k_espinas", "tier": 2, "max": 1, "skill": "espinas_negras", "name": "Espinas negras"},
			{"id": "k_debil", "tier": 2, "max": 1, "skill": "debilitar", "name": "Debilitar"},
			{"id": "k_almas", "tier": 2, "max": 2, "perk": ["dmg_mag", 0.1], "name": "Hambre de almas", "desc": "+10% de daño mágico por rango."},
			{"id": "k_abismo", "tier": 3, "max": 1, "skill": "abismo", "name": "Abismo"},
			{"id": "k_plaga", "tier": 3, "max": 1, "skill": "plaga", "name": "Plaga"},
			{"id": "k_cosecha", "tier": 3, "max": 1, "skill": "cosecha", "name": "Cosecha de almas"},
		]},
	"hunter": {"name": "Cazador", "desc": "Arco, trampas y venenos. Debilita a la presa y la remata.", "icon": "icon_ranged",
		"weapon": "bow", "bonus": {"atk": 4, "spd": 4, "hp": 4}, "starter": "disparo_certero",
		"tree": [
			{"id": "h_veneno", "tier": 1, "max": 1, "skill": "flecha_veneno", "name": "Flecha venenosa"},
			{"id": "h_punt", "tier": 1, "max": 3, "stats": {"atk": 2, "spd": 1}, "name": "Puntería", "desc": "+2 ataque y +1 velocidad por rango."},
			{"id": "h_ojo", "tier": 1, "max": 3, "perk": ["crit", 0.05], "name": "Ojo de halcón", "desc": "+5% de crítico por rango."},
			{"id": "h_lluvia", "tier": 2, "max": 1, "skill": "lluvia_flechas", "name": "Lluvia de flechas"},
			{"id": "h_trampa", "tier": 2, "max": 1, "skill": "trampa", "name": "Trampa"},
			{"id": "h_hielo", "tier": 2, "max": 1, "skill": "flecha_hielo", "name": "Flecha de hielo"},
			{"id": "h_caceria", "tier": 3, "max": 1, "skill": "caceria", "name": "Cacería"},
			{"id": "h_tormenta", "tier": 3, "max": 1, "skill": "tormenta", "name": "Tormenta de acero"},
			{"id": "h_instinto", "tier": 3, "max": 2, "perk": ["dmg_phys", 0.1], "name": "Instinto", "desc": "+10% de daño físico por rango."},
		]},
	"cleric": {"name": "Clérigo", "desc": "La mejor sanadora: curas en grupo, resurrección y milagros.", "icon": "icon_healer",
		"weapon": "staff", "orb": "light", "bonus": {"mag": 4, "mp": 10, "res": 3}, "starter": "luz_sanadora",
		"tree": [
			{"id": "c_purif", "tier": 1, "max": 1, "skill": "purificar", "name": "Purificar"},
			{"id": "c_devo", "tier": 1, "max": 3, "stats": {"mag": 2, "mp": 5}, "name": "Devoción", "desc": "+2 magia y +5 PM por rango."},
			{"id": "c_gracia", "tier": 1, "max": 3, "perk": ["heal_pow", 0.1], "name": "Gracia", "desc": "+10% a tus curaciones por rango."},
			{"id": "c_grupo", "tier": 2, "max": 1, "skill": "cura_grupo", "name": "Curación en grupo"},
			{"id": "c_destello", "tier": 2, "max": 1, "skill": "destello", "name": "Destello"},
			{"id": "c_resu", "tier": 2, "max": 1, "skill": "resurreccion", "name": "Resurrección"},
			{"id": "c_milagro", "tier": 3, "max": 1, "skill": "milagro", "name": "Milagro"},
			{"id": "c_santuario", "tier": 3, "max": 2, "perk": ["mp_regen", 1.5], "name": "Santuario", "desc": "Recuperas más PM cada turno."},
			{"id": "c_juicio", "tier": 3, "max": 1, "skill": "juicio", "name": "Juicio"},
		]},
	"druid": {"name": "Druida", "desc": "La voz del bosque: regeneración, zarzas y la furia de la naturaleza.", "icon": "icon_healer",
		"weapon": "staff", "bonus": {"mag": 4, "mp": 8, "hp": 8}, "starter": "regeneracion",
		"tree": [
			{"id": "d_espinas", "tier": 1, "max": 1, "skill": "espinas", "name": "Espinas"},
			{"id": "d_raices", "tier": 1, "max": 3, "stats": {"hp": 8, "res": 1, "mag": 1}, "name": "Raíces hondas", "desc": "+8 vida, +1 resistencia y +1 magia por rango."},
			{"id": "d_savia", "tier": 1, "max": 3, "perk": ["mp_regen", 1.0], "name": "Savia", "desc": "+1 PM recuperado por turno y rango."},
			{"id": "d_bendicion", "tier": 2, "max": 1, "skill": "bendicion", "name": "Bendición del bosque"},
			{"id": "d_enred", "tier": 2, "max": 1, "skill": "enredadera", "name": "Enredadera"},
			{"id": "d_furia", "tier": 2, "max": 2, "perk": ["dmg_mag", 0.1], "name": "Furia del bosque", "desc": "+10% de daño mágico por rango."},
			{"id": "d_despertar", "tier": 3, "max": 1, "skill": "despertar", "name": "Despertar del bosque"},
			{"id": "d_tormenta", "tier": 3, "max": 1, "skill": "tormenta_verde", "name": "Tormenta verde"},
			{"id": "d_resurgir", "tier": 3, "max": 1, "skill": "resurgir", "name": "Resurgir"},
		]},
	"guardian": {"name": "Guardián", "desc": "Un muro viviente: atrae los golpes, los aguanta y devuelve el castigo.", "icon": "icon_tank",
		"weapon": "axe", "bonus": {"hp": 20, "def": 4, "res": 2}, "starter": "provocar",
		"tree": [
			{"id": "g_golpe", "tier": 1, "max": 1, "skill": "golpe_escudo", "name": "Golpe de escudo"},
			{"id": "g_roca", "tier": 1, "max": 3, "stats": {"hp": 12, "def": 3}, "name": "Roca", "desc": "+12 vida y +3 defensa por rango."},
			{"id": "g_resist", "tier": 1, "max": 3, "stats": {"res": 2, "atk": 2}, "name": "Veterano", "desc": "+2 resistencia y +2 ataque por rango."},
			{"id": "g_muro", "tier": 2, "max": 1, "skill": "muro", "name": "Muro"},
			{"id": "g_guardia", "tier": 2, "max": 1, "skill": "guardia", "name": "Guardia férrea"},
			{"id": "g_coraza", "tier": 2, "max": 2, "perk": ["dmg_red", 0.08], "name": "Coraza", "desc": "-8% de daño recibido por rango."},
			{"id": "g_bastion", "tier": 3, "max": 1, "skill": "bastion", "name": "Bastión"},
			{"id": "g_terremoto", "tier": 3, "max": 1, "skill": "terremoto", "name": "Terremoto"},
			{"id": "g_represalia", "tier": 3, "max": 1, "skill": "represalia", "name": "Represalia"},
		]},
}

## Clases antiguas (partidas guardadas de versiones previas) -> nuevas
const LEGACY_CLASSES := {"knight": "warrior", "berserker": "warrior", "archer": "hunter", "hunter": "hunter",
	"rogue": "rogue", "mage": "mage", "cleric": "cleric", "druid": "druid", "paladin": "paladin", "guardian": "guardian"}
const LEGACY_BRANCHES := {"melee": "melee", "ranged": "ranged", "dps": "melee", "healer": "support", "tank": "support"}


static func class_branch(cid: String) -> String:
	for b in BRANCHES:
		if BRANCHES[b]["classes"].has(cid):
			return b
	return ""


static func tree_node(cid: String, nid: String) -> Dictionary:
	for n in CLASSES[cid]["tree"]:
		if n["id"] == nid:
			return n
	return {}


const MEMBERS := {
	"kaelen": {"name": "Kaelen", "role": "Espadachín",
		"base": {"hp": 42, "mp": 12, "atk": 11, "def": 5, "mag": 3, "res": 4, "spd": 11},
		"grow": {"hp": 7.0, "mp": 2.0, "atk": 2.3, "def": 1.5, "mag": 0.6, "res": 1.0, "spd": 1.1},
		"skills": {1: "estocada", 4: "grito", 7: "filo_viento"}},
	"yara": {"name": "Yara", "role": "Curandera",
		"base": {"hp": 32, "mp": 22, "atk": 5, "def": 4, "mag": 11, "res": 8, "spd": 10},
		"grow": {"hp": 5.0, "mp": 4.0, "atk": 0.8, "def": 1.0, "mag": 2.4, "res": 1.8, "spd": 1.0},
		"skills": {1: "flor_luna", 2: "destello", 3: "purificar", 4: "bendicion", 7: "resurgir"},
		"dark_skills": {1: "flor_negra", 2: "maldicion", 3: "drenar", 5: "espinas_negras", 7: "resurgir"}},
	"aelis": {"name": "Aelis", "role": "Arquera élfica",
		"base": {"hp": 36, "mp": 16, "atk": 12, "def": 4, "mag": 9, "res": 6, "spd": 15},
		"grow": {"hp": 5.5, "mp": 3.0, "atk": 2.2, "def": 1.0, "mag": 1.6, "res": 1.4, "spd": 1.4},
		"skills": {1: "disparo_doble", 5: "flecha_lunar", 8: "lluvia_flechas"}},
	"brom": {"name": "Brom", "role": "Guerrero enano",
		"base": {"hp": 56, "mp": 8, "atk": 13, "def": 10, "mag": 2, "res": 5, "spd": 6},
		"grow": {"hp": 9.0, "mp": 1.5, "atk": 2.2, "def": 2.4, "mag": 0.4, "res": 1.2, "spd": 0.6},
		"skills": {1: "martillazo", 5: "provocar", 8: "terremoto"}},
}

## target: enemy, enemies, ally, allies, self, ally_dead
## kind: phys, mag, heal, buff, status, revive, steal, cure, drain
const SKILLS := {
	"aux": {"name": "Primeros auxilios", "desc": "Cura a un aliado.", "mp": 4, "target": "ally", "kind": "heal", "power": 0.9, "base": 12, "el": "light"},
	"tajo_doble": {"name": "Tajo doble", "desc": "Dos cortes rápidos.", "mp": 5, "target": "enemy", "kind": "phys", "power": 0.8, "hits": 2},
	"disparo_certero": {"name": "Disparo certero", "desc": "Nunca falla. Crítico frecuente.", "mp": 4, "target": "enemy", "kind": "phys", "power": 1.5, "crit": 0.3},
	"punalada": {"name": "Puñalada", "desc": "Golpe traicionero muy potente.", "mp": 5, "target": "enemy", "kind": "phys", "power": 1.8, "crit": 0.2},
	"luz_sanadora": {"name": "Luz sanadora", "desc": "Cura mucho a un aliado.", "mp": 5, "target": "ally", "kind": "heal", "power": 1.5, "base": 16, "el": "light"},
	"provocar": {"name": "Provocar", "desc": "Atrae los ataques y endurece tu defensa.", "mp": 3, "target": "self", "kind": "buff", "status": ["taunt", "protect"], "turns": 3},
	"guardia": {"name": "Guardia férrea", "desc": "Protege a todo el grupo.", "mp": 8, "target": "allies", "kind": "buff", "status": ["protect"], "turns": 3},
	"carga": {"name": "Carga", "desc": "Embestida que puede aturdir.", "mp": 8, "target": "enemy", "kind": "phys", "power": 2.0, "inflict": "stun", "chance": 0.35},
	"juramento": {"name": "Juramento", "desc": "Golpe legendario y protección.", "mp": 18, "target": "enemy", "kind": "phys", "power": 3.2, "self_status": ["protect"]},
	"furia": {"name": "Furia", "desc": "Más ataque, menos defensa.", "mp": 4, "target": "self", "kind": "buff", "status": ["rage"], "turns": 4},
	"remolino": {"name": "Remolino", "desc": "Golpea a todos los enemigos.", "mp": 9, "target": "enemies", "kind": "phys", "power": 1.0},
	"masacre": {"name": "Masacre", "desc": "Destrucción total.", "mp": 18, "target": "enemies", "kind": "phys", "power": 1.9},
	"lluvia_flechas": {"name": "Lluvia de flechas", "desc": "Flechas sobre todos los enemigos.", "mp": 9, "target": "enemies", "kind": "phys", "power": 0.95},
	"flecha_hielo": {"name": "Flecha de hielo", "desc": "Disparo helado.", "mp": 7, "target": "enemy", "kind": "phys", "power": 1.7, "el": "ice"},
	"tormenta": {"name": "Tormenta de acero", "desc": "Dos lluvias de flechas.", "mp": 18, "target": "enemies", "kind": "phys", "power": 1.1, "hits": 2},
	"flecha_veneno": {"name": "Flecha venenosa", "desc": "Daña y envenena.", "mp": 6, "target": "enemy", "kind": "phys", "power": 1.1, "inflict": "poison", "chance": 0.85, "el": "nature"},
	"trampa": {"name": "Trampa", "desc": "Aturde al objetivo.", "mp": 7, "target": "enemy", "kind": "status", "inflict": "stun", "chance": 0.65},
	"caceria": {"name": "Cacería", "desc": "Brutal contra presas envenenadas.", "mp": 16, "target": "enemy", "kind": "phys", "power": 2.6, "vs_poison": 1.6},
	"robar": {"name": "Robar", "desc": "Roba un objeto u oro.", "mp": 2, "target": "enemy", "kind": "steal"},
	"sombras": {"name": "Golpe en las sombras", "desc": "Daño oscuro devastador.", "mp": 8, "target": "enemy", "kind": "phys", "power": 2.2, "el": "dark"},
	"mil_cortes": {"name": "Mil cortes", "desc": "Seis cortes a enemigos al azar.", "mp": 18, "target": "enemies_random", "kind": "phys", "power": 0.75, "hits": 6},
	"fuego": {"name": "Fuego", "desc": "Llamas sobre un enemigo.", "mp": 6, "target": "enemy", "kind": "mag", "power": 1.7, "el": "fire"},
	"hielo": {"name": "Hielo", "desc": "Estaca de hielo.", "mp": 6, "target": "enemy", "kind": "mag", "power": 1.7, "el": "ice"},
	"rayo": {"name": "Rayo", "desc": "Relámpago sobre todos.", "mp": 10, "target": "enemies", "kind": "mag", "power": 1.15, "el": "bolt"},
	"meteoro": {"name": "Meteoro", "desc": "Fuego del cielo sobre todos.", "mp": 24, "target": "enemies", "kind": "mag", "power": 2.3, "el": "fire"},
	"cura_grupo": {"name": "Curación en grupo", "desc": "Cura a todo el grupo.", "mp": 10, "target": "allies", "kind": "heal", "power": 0.9, "base": 12, "el": "light"},
	"resurreccion": {"name": "Resurrección", "desc": "Revive con la mitad de vida.", "mp": 14, "target": "ally_dead", "kind": "revive", "ratio": 0.5},
	"milagro": {"name": "Milagro", "desc": "Cura por completo a todos y limpia estados.", "mp": 30, "target": "allies", "kind": "heal", "power": 9.0, "base": 999, "cure": true, "el": "light"},
	"regeneracion": {"name": "Regeneración", "desc": "Cura un poco cada turno.", "mp": 6, "target": "ally", "kind": "buff", "status": ["regen"], "turns": 5},
	"espinas": {"name": "Espinas", "desc": "Zarzas sobre todos, pueden envenenar.", "mp": 10, "target": "enemies", "kind": "mag", "power": 1.0, "el": "nature", "inflict": "poison", "chance": 0.3},
	"despertar": {"name": "Despertar del bosque", "desc": "Cura y regenera a todos.", "mp": 24, "target": "allies", "kind": "heal", "power": 1.2, "base": 20, "add_status": ["regen"], "el": "nature"},
	"escudo_sagrado": {"name": "Escudo sagrado", "desc": "Protege a todo el grupo.", "mp": 9, "target": "allies", "kind": "buff", "status": ["protect"], "turns": 3},
	"juicio": {"name": "Juicio", "desc": "Luz sagrada contra un enemigo.", "mp": 8, "target": "enemy", "kind": "mag", "power": 1.8, "el": "light"},
	"alba": {"name": "Luz del alba", "desc": "Luz que hiere enemigos y cura aliados.", "mp": 24, "target": "enemies", "kind": "mag", "power": 1.8, "el": "light", "heal_party": 0.5},
	"muro": {"name": "Muro", "desc": "Atraes golpes y los reduces a la mitad.", "mp": 6, "target": "self", "kind": "buff", "status": ["taunt", "protect", "wall"], "turns": 3},
	"golpe_escudo": {"name": "Golpe de escudo", "desc": "Golpe que debilita al enemigo.", "mp": 6, "target": "enemy", "kind": "phys", "power": 1.4, "inflict": "weak", "chance": 0.7},
	"bastion": {"name": "Bastión", "desc": "Protección y regeneración para todos.", "mp": 20, "target": "allies", "kind": "buff", "status": ["protect", "regen"], "turns": 4},
	"estocada": {"name": "Estocada", "desc": "Un golpe preciso de Kaelen.", "mp": 4, "target": "enemy", "kind": "phys", "power": 1.5},
	"grito": {"name": "Grito de guerra", "desc": "Aumenta el ataque del grupo.", "mp": 8, "target": "allies", "kind": "buff", "status": ["rage"], "turns": 3},
	"filo_viento": {"name": "Filo del viento", "desc": "Un tajo que alcanza a todos.", "mp": 9, "target": "enemies", "kind": "phys", "power": 1.05},
	"tajo_sombrio": {"name": "Tajo sombrío", "desc": "Un corte envuelto en savia negra.", "mp": 10, "target": "enemy", "kind": "phys", "power": 2.0, "el": "dark"},
	"flor_luna": {"name": "Flor de luna", "desc": "Cura a un aliado.", "mp": 4, "target": "ally", "kind": "heal", "power": 1.4, "base": 14, "el": "light"},
	"destello": {"name": "Destello", "desc": "Luz que hiere a un enemigo.", "mp": 4, "target": "enemy", "kind": "mag", "power": 1.25, "el": "light"},
	"purificar": {"name": "Purificar", "desc": "Limpia estados alterados.", "mp": 3, "target": "ally", "kind": "cure"},
	"bendicion": {"name": "Bendición del bosque", "desc": "Cura a todo el grupo.", "mp": 9, "target": "allies", "kind": "heal", "power": 0.8, "base": 10, "el": "light"},
	"resurgir": {"name": "Resurgir", "desc": "Revive a un aliado.", "mp": 12, "target": "ally_dead", "kind": "revive", "ratio": 0.4},
	"flor_negra": {"name": "Flor negra", "desc": "Cura, pero algo menos.", "mp": 5, "target": "ally", "kind": "heal", "power": 1.0, "base": 8, "el": "dark"},
	"maldicion": {"name": "Maldición", "desc": "Envenena y hiere con sombra.", "mp": 5, "target": "enemy", "kind": "mag", "power": 0.9, "el": "dark", "inflict": "poison", "chance": 0.9},
	"drenar": {"name": "Drenar", "desc": "Roba vida al enemigo.", "mp": 6, "target": "enemy", "kind": "drain", "power": 1.4, "el": "dark"},
	"espinas_negras": {"name": "Espinas negras", "desc": "Sombra sobre todos los enemigos.", "mp": 10, "target": "enemies", "kind": "mag", "power": 1.2, "el": "dark"},
	"disparo_doble": {"name": "Disparo doble", "desc": "Dos flechas rápidas.", "mp": 4, "target": "enemy", "kind": "phys", "power": 0.85, "hits": 2},
	"flecha_lunar": {"name": "Flecha lunar", "desc": "Una flecha bañada en luz de luna.", "mp": 6, "target": "enemy", "kind": "mag", "power": 1.6, "el": "light"},
	"martillazo": {"name": "Martillazo", "desc": "Un golpe que puede aturdir.", "mp": 5, "target": "enemy", "kind": "phys", "power": 1.6, "inflict": "stun", "chance": 0.3},
	"terremoto": {"name": "Terremoto", "desc": "Hace temblar a todos los enemigos.", "mp": 10, "target": "enemies", "kind": "phys", "power": 1.15},
	"hoja_venenosa": {"name": "Hoja venenosa", "desc": "Corte que envenena casi seguro.", "mp": 5, "target": "enemy", "kind": "phys", "power": 1.2, "inflict": "poison", "chance": 0.9},
	"asesinar": {"name": "Asesinar", "desc": "Un golpe mortal con críticos muy probables.", "mp": 16, "target": "enemy", "kind": "phys", "power": 3.0, "crit": 0.4},
	"martillo_luz": {"name": "Martillo de luz", "desc": "Golpe sagrado que puede aturdir.", "mp": 8, "target": "enemy", "kind": "phys", "power": 1.7, "el": "light", "inflict": "stun", "chance": 0.35},
	"llamarada": {"name": "Llamarada", "desc": "Fuego sobre todos los enemigos.", "mp": 12, "target": "enemies", "kind": "mag", "power": 1.3, "el": "fire"},
	"ventisca": {"name": "Ventisca", "desc": "Tormenta de hielo sobre todos.", "mp": 22, "target": "enemies", "kind": "mag", "power": 2.1, "el": "ice"},
	"sobrecarga": {"name": "Sobrecarga", "desc": "Un rayo descomunal sobre un enemigo.", "mp": 18, "target": "enemy", "kind": "mag", "power": 3.4, "el": "bolt"},
	"debilitar": {"name": "Debilitar", "desc": "Reduce el ataque y la magia de un enemigo.", "mp": 5, "target": "enemy", "kind": "status", "inflict": "weak", "chance": 0.85},
	"abismo": {"name": "Abismo", "desc": "Sombra devoradora sobre todos.", "mp": 22, "target": "enemies", "kind": "mag", "power": 2.2, "el": "dark"},
	"plaga": {"name": "Plaga", "desc": "Envenena a todos los enemigos.", "mp": 12, "target": "enemies", "kind": "mag", "power": 0.6, "el": "dark", "inflict": "poison", "chance": 0.85},
	"cosecha": {"name": "Cosecha de almas", "desc": "Roba mucha vida a un enemigo.", "mp": 16, "target": "enemy", "kind": "drain", "power": 2.6, "el": "dark"},
	"enredadera": {"name": "Enredadera", "desc": "Raíces que atrapan al enemigo.", "mp": 7, "target": "enemy", "kind": "status", "inflict": "stun", "chance": 0.6},
	"tormenta_verde": {"name": "Tormenta verde", "desc": "La furia del bosque sobre todos.", "mp": 20, "target": "enemies", "kind": "mag", "power": 2.0, "el": "nature"},
	"represalia": {"name": "Represalia", "desc": "Golpe tremendo que además te protege.", "mp": 14, "target": "enemy", "kind": "phys", "power": 2.6, "self_status": ["protect", "taunt"]},
	# enemigos
	"e_mordisco": {"name": "Mordisco", "target": "enemy", "kind": "phys", "power": 1.0},
	"e_feroz": {"name": "Mordisco feroz", "target": "enemy", "kind": "phys", "power": 1.6},
	"e_aullido": {"name": "Aullido", "target": "self", "kind": "buff", "status": ["rage"], "turns": 3},
	"e_escupir": {"name": "Escupitajo de savia", "target": "enemy", "kind": "mag", "power": 0.8, "el": "dark", "inflict": "poison", "chance": 0.6},
	"e_esporas": {"name": "Esporas", "target": "enemies", "kind": "mag", "power": 0.45, "el": "nature", "inflict": "poison", "chance": 0.35},
	"e_zarpazo": {"name": "Zarpazo", "target": "enemy", "kind": "phys", "power": 1.1},
	"e_savia": {"name": "Beber savia", "target": "enemy", "kind": "drain", "power": 1.0, "el": "dark"},
	"e_brutal": {"name": "Golpe brutal", "target": "enemies", "kind": "phys", "power": 0.95},
	"e_aplastar": {"name": "Aplastar", "target": "enemy", "kind": "phys", "power": 1.5},
	"e_rugido": {"name": "Rugido", "target": "self", "kind": "buff", "status": ["rage"], "turns": 3},
	"e_enredar": {"name": "Enredar", "target": "enemy", "kind": "phys", "power": 0.6, "inflict": "stun", "chance": 0.45},
	"e_latigazo": {"name": "Latigazo", "target": "enemy", "kind": "phys", "power": 1.2},
	"e_rayo_cristal": {"name": "Rayo de cristal", "target": "enemy", "kind": "mag", "power": 1.4, "el": "ice"},
	"e_lamento": {"name": "Lamento", "target": "enemies", "kind": "status", "inflict": "weak", "chance": 0.5},
	"e_haz": {"name": "Haz prismático", "target": "enemies", "kind": "mag", "power": 1.05, "el": "light"},
	"e_puño_cristal": {"name": "Puño de cristal", "target": "enemy", "kind": "phys", "power": 1.5},
	"e_escudo": {"name": "Escudo de cristal", "target": "self", "kind": "buff", "status": ["protect"], "turns": 4},
	"e_latido": {"name": "Latido negro", "target": "enemies", "kind": "mag", "power": 1.0, "el": "dark"},
	"e_raices": {"name": "Raíces", "target": "enemy", "kind": "phys", "power": 1.3, "inflict": "stun", "chance": 0.3},
	"e_invocar": {"name": "Brotes", "target": "self", "kind": "summon"},
	"e_regenerar": {"name": "Regenerar", "target": "self", "kind": "buff", "status": ["regen"], "turns": 4},
}

const STATUS := {
	"poison": {"name": "Veneno", "icon": "st_poison"},
	"stun": {"name": "Aturdido", "icon": "st_stun"},
	"regen": {"name": "Regeneración", "icon": "st_regen"},
	"protect": {"name": "Protección", "icon": "st_protect"},
	"rage": {"name": "Furia", "icon": "st_rage"},
	"taunt": {"name": "Provocación", "icon": "st_taunt"},
	"weak": {"name": "Debilidad", "icon": "st_weak"},
	"wall": {"name": "Muro", "icon": "st_protect"},
}

const ITEMS := {
	"pan": {"name": "Pan de hogaza", "desc": "Cura 30 de vida.", "icon": "it_bread", "price": 12, "target": "ally", "heal": 30},
	"pocion": {"name": "Poción", "desc": "Cura 60 de vida.", "icon": "it_potion", "price": 30, "target": "ally", "heal": 60},
	"pocion_mayor": {"name": "Poción mayor", "desc": "Cura 180 de vida.", "icon": "it_potion2", "price": 100, "target": "ally", "heal": 180},
	"eter": {"name": "Éter", "desc": "Recupera 25 de maná.", "icon": "it_ether", "price": 70, "target": "ally", "mp": 25},
	"antidoto": {"name": "Antídoto", "desc": "Cura el veneno.", "icon": "it_antidote", "price": 15, "target": "ally", "cure": true},
	"pluma": {"name": "Pluma de fénix", "desc": "Revive con el 40% de vida.", "icon": "it_feather", "price": 180, "target": "ally_dead", "revive": 0.4},
	"flor_luna_item": {"name": "Flor de luna", "desc": "Cura 50 a todo el grupo y limpia el veneno.", "icon": "it_flower", "price": 90, "target": "allies", "heal": 50, "cure": true},
	"bomba": {"name": "Bomba de fuego", "desc": "70 de daño de fuego a todos los enemigos.", "icon": "it_bomb", "price": 60, "target": "enemies", "damage": 70, "el": "fire"},
}

## Equipo.  slot: weapon, shield, head, body, ring.  kind (armas): sword, axe, daggers, staff, bow.
## weight (cuerpo/cabeza): light, medium, heavy, cloth.  look: aspecto visible en el sprite.
const EQUIP := {
	# Espadas
	"espada_hierro": {"rarity": "basic", "lvl": 1, "name": "Espada de hierro", "slot": "weapon", "kind": "sword", "tier": 1, "price": 80, "stats": {"atk": 4}},
	"espada_acero": {"rarity": "common", "lvl": 3, "name": "Espada de acero", "slot": "weapon", "kind": "sword", "tier": 2, "price": 260, "stats": {"atk": 9}},
	"espada_runica": {"rarity": "rare", "lvl": 5, "name": "Espada rúnica", "slot": "weapon", "kind": "sword", "tier": 3, "price": 640, "stats": {"atk": 15, "mag": 4}},
	"hoja_alba": {"rarity": "divine", "lvl": 7, "name": "Hoja del Alba", "slot": "weapon", "kind": "sword", "tier": 3, "price": 0, "stats": {"atk": 22, "mag": 6, "spd": 2}},
	# Hachas
	"hacha_hierro": {"rarity": "basic", "lvl": 1, "name": "Hacha de hierro", "slot": "weapon", "kind": "axe", "tier": 1, "price": 90, "stats": {"atk": 6, "spd": -1}},
	"hacha_guerra": {"rarity": "common", "lvl": 3, "name": "Hacha de guerra", "slot": "weapon", "kind": "axe", "tier": 2, "price": 280, "stats": {"atk": 12, "spd": -1}},
	"hacha_runica": {"rarity": "rare", "lvl": 5, "name": "Hacha rúnica de Khazgurim", "slot": "weapon", "kind": "axe", "tier": 3, "price": 660, "stats": {"atk": 19, "def": 2}},
	# Dagas
	"dagas_hierro": {"rarity": "basic", "lvl": 1, "name": "Dagas de hierro", "slot": "weapon", "kind": "daggers", "tier": 1, "price": 80, "stats": {"atk": 3, "spd": 1}},
	"dagas_sombra": {"rarity": "common", "lvl": 3, "name": "Dagas de sombra", "slot": "weapon", "kind": "daggers", "tier": 2, "price": 270, "stats": {"atk": 8, "spd": 2}},
	"colmillos": {"rarity": "rare", "lvl": 5, "name": "Colmillos del lobo", "slot": "weapon", "kind": "daggers", "tier": 3, "price": 620, "stats": {"atk": 14, "spd": 4}},
	# Bastones
	"baston_roble": {"rarity": "basic", "lvl": 1, "name": "Bastón de roble", "slot": "weapon", "kind": "staff", "tier": 1, "price": 80, "stats": {"mag": 4, "mp": 4}},
	"baston_cristal": {"rarity": "common", "lvl": 3, "name": "Bastón de cristal", "slot": "weapon", "kind": "staff", "tier": 2, "price": 280, "stats": {"mag": 9, "mp": 8}},
	"baston_ancestral": {"rarity": "rare", "lvl": 5, "name": "Bastón ancestral", "slot": "weapon", "kind": "staff", "tier": 3, "price": 660, "stats": {"mag": 15, "mp": 12, "res": 2}},
	"baston_sombra": {"rarity": "divine", "lvl": 7, "name": "Cayado de la sombra", "slot": "weapon", "kind": "staff", "tier": 3, "price": 0, "orb": "dark", "stats": {"mag": 17, "mp": 8}},
	# Arcos
	"arco_corto": {"rarity": "basic", "lvl": 1, "name": "Arco corto", "slot": "weapon", "kind": "bow", "tier": 1, "price": 80, "stats": {"atk": 4, "spd": 1}},
	"arco_largo": {"rarity": "common", "lvl": 3, "name": "Arco largo", "slot": "weapon", "kind": "bow", "tier": 2, "price": 270, "stats": {"atk": 9, "spd": 1}},
	"arco_elfico": {"rarity": "rare", "lvl": 5, "name": "Arco élfico", "slot": "weapon", "kind": "bow", "tier": 3, "price": 640, "stats": {"atk": 15, "spd": 3}},
	# Escudos
	"escudo_madera": {"rarity": "basic", "lvl": 1, "name": "Escudo de madera", "slot": "shield", "tier": 1, "look": "wood", "price": 60, "stats": {"def": 3}},
	"escudo_hierro": {"rarity": "common", "lvl": 3, "name": "Escudo de hierro", "slot": "shield", "tier": 2, "look": "iron", "price": 220, "stats": {"def": 6, "spd": -1}},
	"egida": {"rarity": "rare", "lvl": 5, "name": "Égida de la Savia", "slot": "shield", "tier": 3, "look": "aegis", "price": 560, "stats": {"def": 10, "res": 5}},
	# Cabeza
	"gorro_cuero": {"rarity": "basic", "lvl": 1, "name": "Gorro de cuero", "slot": "head", "weight": "light", "look": "cap", "price": 50, "stats": {"def": 1, "hp": 6}},
	"capucha_sombra": {"rarity": "rare", "lvl": 5, "name": "Capucha de sombra", "slot": "head", "weight": "light", "look": "hood", "price": 240, "stats": {"spd": 2, "mag": 2, "def": 1}},
	"yelmo_hierro": {"rarity": "common", "lvl": 3, "name": "Yelmo de hierro", "slot": "head", "weight": "heavy", "look": "helm", "price": 150, "stats": {"def": 3, "hp": 12}},
	"yelmo_acero": {"rarity": "rare", "lvl": 5, "name": "Yelmo de acero", "slot": "head", "weight": "heavy", "look": "helm_gold", "price": 420, "stats": {"def": 6, "hp": 20, "res": 1}},
	"diadema": {"rarity": "common", "lvl": 3, "name": "Diadema de plata", "slot": "head", "weight": "cloth", "look": "circlet", "price": 180, "stats": {"mag": 3, "mp": 8}},
	"diadema_luna": {"rarity": "rare", "lvl": 5, "name": "Diadema lunar", "slot": "head", "weight": "cloth", "look": "circlet_moon", "price": 480, "stats": {"mag": 6, "mp": 14, "res": 2}},
	# Cuerpo
	"jubon_cuero": {"rarity": "basic", "lvl": 1, "name": "Jubón de cuero", "slot": "body", "weight": "light", "look": "leather", "price": 70, "stats": {"def": 3, "hp": 8}},
	"cota_malla": {"rarity": "common", "lvl": 3, "name": "Cota de malla", "slot": "body", "weight": "medium", "look": "chain", "price": 240, "stats": {"def": 6, "hp": 16, "spd": -1}},
	"armadura_placas": {"rarity": "rare", "lvl": 5, "name": "Armadura de placas", "slot": "body", "weight": "heavy", "look": "plate", "price": 520, "stats": {"def": 11, "hp": 26, "spd": -2}},
	"armadura_mithril": {"rarity": "very_rare", "lvl": 6, "name": "Armadura de mithril", "slot": "body", "weight": "medium", "look": "mithril", "price": 0, "stats": {"def": 14, "hp": 32, "res": 4}},
	"tunica_mago": {"rarity": "basic", "lvl": 1, "name": "Túnica de aprendiz", "slot": "body", "weight": "cloth", "look": "robe", "price": 90, "stats": {"mag": 3, "mp": 10, "res": 2}},
	"tunica_sabio": {"rarity": "rare", "lvl": 5, "name": "Túnica del sabio", "slot": "body", "weight": "cloth", "look": "robe_sage", "price": 460, "stats": {"mag": 7, "mp": 16, "res": 4, "def": 2}},
	"tunica_sombra": {"rarity": "rare", "lvl": 5, "name": "Túnica de sombra", "slot": "body", "weight": "light", "look": "robe_dark", "price": 380, "stats": {"mag": 6, "spd": 2, "def": 3, "mp": 8}},
	# Anillos
	"anillo_vida": {"rarity": "common", "lvl": 3, "name": "Anillo de vida", "slot": "ring", "price": 200, "stats": {"hp": 25}},
	"anillo_mana": {"rarity": "common", "lvl": 3, "name": "Anillo de maná", "slot": "ring", "price": 220, "stats": {"mp": 15}},
	"anillo_fuerza": {"rarity": "common", "lvl": 3, "name": "Anillo de fuerza", "slot": "ring", "price": 240, "stats": {"atk": 5}},
	"anillo_sabio": {"rarity": "rare", "lvl": 5, "name": "Anillo del sabio", "slot": "ring", "price": 240, "stats": {"mag": 5}},
	"anillo_veloz": {"rarity": "rare", "lvl": 5, "name": "Anillo del viento", "slot": "ring", "price": 260, "stats": {"spd": 4}},
	"anillo_guardian": {"rarity": "rare", "lvl": 5, "name": "Anillo guardián", "slot": "ring", "price": 280, "stats": {"def": 4, "res": 4}},
	"aurora": {"rarity": "legendary", "lvl": 8, "name": "Aurora", "slot": "weapon", "kind": "sword", "tier": 3, "price": 0,
		"stats": {"atk": 26, "mag": 8, "spd": 2},
		"lore": "Espada de Ermengol el Sellador, que cerró el Velo bajo la Catedral de Tortosa hace mil años. Su filo aún guarda la primera luz del mundo."},
	"rompemontanas": {"rarity": "legendary", "lvl": 8, "name": "Rompemontañas", "slot": "weapon", "kind": "axe", "tier": 3, "price": 0,
		"stats": {"atk": 30, "def": 4},
		"lore": "Hacha de Thrain Barbahierro, primer rey de Khazgurim, antepasado de Brom. Con ella abrió la montaña para fundar la ciudad enana."},
	"susurro_ilvane": {"rarity": "legendary", "lvl": 8, "name": "Susurro de Ilvanë", "slot": "weapon", "kind": "bow", "tier": 3, "price": 0,
		"stats": {"atk": 26, "spd": 5},
		"lore": "El arco de Ilvanë, primera centinela de la Corte de la Savia. Se dice que sus flechas nunca hicieron ruido."},
	"lagrimas_selen": {"rarity": "legendary", "lvl": 8, "name": "Lágrimas de Selen", "slot": "weapon", "kind": "daggers", "tier": 3, "price": 0,
		"stats": {"atk": 24, "spd": 7},
		"lore": "Dagas gemelas de Selen, la Sombra del Imperio de Cristal, que asesinó a su emperador y lloró por él hasta su muerte."},
	"baculo_pacto": {"rarity": "legendary", "lvl": 8, "name": "Báculo del Pacto", "slot": "weapon", "kind": "staff", "tier": 3, "price": 0,
		"orb": "light", "stats": {"mag": 28, "mp": 20, "res": 4},
		"lore": "Tallado de la primera rama del Árbol Madre por la druida que selló el Pacto entre el bosque y los hombres."},
	"muralla_tortosa": {"rarity": "legendary", "lvl": 8, "name": "Muralla de Tortosa", "slot": "shield", "tier": 3, "look": "aegis", "price": 0,
		"stats": {"def": 16, "res": 8},
		"lore": "El escudo de la primera guardia de Tortosa, forjado con las campanas fundidas de la vieja Catedral."},
	"corona_anciana": {"rarity": "legendary", "lvl": 8, "name": "Corona de la Anciana", "slot": "head", "weight": "cloth", "look": "circlet_moon", "price": 0,
		"stats": {"mag": 12, "mp": 24, "res": 4},
		"lore": "La llevaron todas las Ancianas de la Savia, hasta que la última se perdió en el Corazón del Bosque."},
	"coraza_olvidado": {"rarity": "legendary", "lvl": 8, "name": "Coraza del Rey Olvidado", "slot": "body", "weight": "heavy", "look": "mithril", "price": 0,
		"stats": {"def": 22, "hp": 50, "res": 5},
		"lore": "Armadura del último rey humano de Vaelmoor, cuyo nombre borraron los Moronguls de todas las crónicas."},
	"manto_estrellas": {"rarity": "legendary", "lvl": 8, "name": "Manto de las Estrellas", "slot": "body", "weight": "cloth", "look": "robe_sage", "price": 0,
		"stats": {"mag": 12, "mp": 25, "res": 8, "def": 4},
		"lore": "Túnica de los astrólogos del Imperio de Cristal, bordada con mapas de cielos que ya no existen."},
	"anillo_velo": {"rarity": "legendary", "lvl": 8, "name": "Anillo del Velo", "slot": "ring", "price": 0,
		"stats": {"hp": 30, "mp": 20, "atk": 6, "mag": 6, "spd": 2},
		"lore": "Un fragmento del propio Velo engarzado en plata. Quien lo lleva oye susurros del otro lado."},
	# El arma del Soberano: solo aparece por un camino secreto
	"overlord_sword": {"rarity": "overlord", "lvl": 1, "name": "Overlord · Filo del Soberano", "slot": "weapon", "kind": "sword", "tier": 3, "price": 0,
		"stats": {"atk": 99999}, "lore": "Nadie sabe quién la forjó. Dicen que existía antes que el Velo, antes que el bosque, antes que el mundo."},
	"overlord_axe": {"rarity": "overlord", "lvl": 1, "name": "Overlord · Hacha del Soberano", "slot": "weapon", "kind": "axe", "tier": 3, "price": 0,
		"stats": {"atk": 99999}, "lore": "Nadie sabe quién la forjó. Dicen que existía antes que el Velo, antes que el bosque, antes que el mundo."},
	"overlord_daggers": {"rarity": "overlord", "lvl": 1, "name": "Overlord · Colmillos del Soberano", "slot": "weapon", "kind": "daggers", "tier": 3, "price": 0,
		"stats": {"atk": 99999}, "lore": "Nadie sabe quién las forjó. Dicen que existían antes que el Velo, antes que el bosque, antes que el mundo."},
	"overlord_staff": {"rarity": "overlord", "lvl": 1, "name": "Overlord · Cetro del Soberano", "slot": "weapon", "kind": "staff", "tier": 3, "price": 0,
		"orb": "dark", "stats": {"mag": 99999, "atk": 99999}, "lore": "Nadie sabe quién lo forjó. Dicen que existía antes que el Velo, antes que el bosque, antes que el mundo."},
	"overlord_bow": {"rarity": "overlord", "lvl": 1, "name": "Overlord · Arco del Soberano", "slot": "weapon", "kind": "bow", "tier": 3, "price": 0,
		"stats": {"atk": 99999}, "lore": "Nadie sabe quién lo forjó. Dicen que existía antes que el Velo, antes que el bosque, antes que el mundo."},
	"anillo_savia": {"rarity": "very_rare", "lvl": 6, "name": "Anillo de savia", "slot": "ring", "price": 0, "stats": {"hp": 15, "mp": 10, "mag": 2, "atk": 2}},
}
const RARITY := {
	"basic": {"name": "Básico", "color": Color(0.45, 0.42, 0.4)},
	"common": {"name": "Común", "color": Color(0.2, 0.45, 0.2)},
	"rare": {"name": "Raro", "color": Color(0.15, 0.35, 0.8)},
	"very_rare": {"name": "Muy raro", "color": Color(0.5, 0.2, 0.7)},
	"divine": {"name": "Divino", "color": Color(0.75, 0.55, 0.05)},
	"legendary": {"name": "Legendario", "color": Color(0.85, 0.35, 0.05)},
	"overlord": {"name": "Overlord", "color": Color(0.8, 0.05, 0.1)},
}
const SLOT_NAMES := {"weapon": "Arma", "shield": "Escudo", "head": "Cabeza", "body": "Cuerpo", "ring": "Anillo"}
const STAT_NAMES := {"hp": "Vida", "mp": "PM", "atk": "Ataque", "def": "Defensa", "mag": "Magia", "res": "Resist.", "spd": "Velocidad"}

## Qué puede llevar cada clase / compañero
const EQUIP_RULES := {
	"warrior": {"weapons": ["sword", "axe"], "shield": true, "weights": ["light", "medium", "heavy"]},
	"rogue": {"weapons": ["daggers", "sword"], "shield": false, "weights": ["light", "medium"]},
	"paladin": {"weapons": ["sword", "axe"], "shield": true, "weights": ["light", "medium", "heavy"]},
	"mage": {"weapons": ["staff"], "shield": false, "weights": ["cloth", "light"]},
	"warlock": {"weapons": ["staff", "daggers"], "shield": false, "weights": ["cloth", "light"]},
	"hunter": {"weapons": ["bow", "daggers"], "shield": false, "weights": ["light", "medium"]},
	"cleric": {"weapons": ["staff"], "shield": true, "weights": ["cloth", "light"]},
	"druid": {"weapons": ["staff"], "shield": false, "weights": ["cloth", "light"]},
	"guardian": {"weapons": ["axe", "sword"], "shield": true, "weights": ["light", "medium", "heavy"]},
	"none": {"weapons": ["sword", "axe", "daggers", "staff", "bow"], "shield": true, "weights": ["cloth", "light", "medium", "heavy"]},
	"kaelen": {"weapons": ["sword"], "shield": true, "weights": ["light", "medium", "heavy"]},
	"yara": {"weapons": ["staff"], "shield": false, "weights": ["cloth", "light"]},
	"aelis": {"weapons": ["bow", "daggers"], "shield": false, "weights": ["light", "medium"]},
	"brom": {"weapons": ["axe"], "shield": true, "weights": ["light", "medium", "heavy"]},
}
const CLASS_STARTER_GEAR := {
	"warrior": ["espada_hierro", "jubon_cuero"], "rogue": ["dagas_hierro", "jubon_cuero"],
	"paladin": ["espada_hierro", "escudo_madera"], "mage": ["baston_roble", "tunica_mago"],
	"warlock": ["baston_roble", "tunica_mago"], "hunter": ["arco_corto", "jubon_cuero"],
	"cleric": ["baston_roble", "tunica_mago"], "druid": ["baston_roble", "tunica_mago"],
	"guardian": ["hacha_hierro", "escudo_madera"],
}
const MEMBER_START_GEAR := {
	"kaelen": ["espada_hierro", "jubon_cuero"], "yara": ["baston_roble"], "aelis": ["arco_largo", "jubon_cuero", "gorro_cuero"],
	"brom": ["hacha_guerra", "cota_malla", "yelmo_hierro", "escudo_madera"],
}


static func equip_allowed(rule_key: String, eid: String) -> bool:
	var e: Dictionary = EQUIP[eid]
	var r: Dictionary = EQUIP_RULES.get(rule_key, EQUIP_RULES["none"])
	match str(e["slot"]):
		"weapon":
			return r["weapons"].has(e["kind"])
		"shield":
			return bool(r["shield"])
		"head", "body":
			return r["weights"].has(e["weight"])
	return true


const UPGRADES := {
	"arma1": {"name": "Armas de acero (grupo)", "desc": "+4 ataque y magia a todo el grupo.", "price": 220, "kind": "weapon", "tier": 1},
	"armadura1": {"name": "Armaduras de cuero (grupo)", "desc": "+4 defensa y resistencia al grupo.", "price": 200, "kind": "armor", "tier": 1},
	"arma2": {"name": "Armas élficas (grupo)", "desc": "+9 ataque y magia a todo el grupo.", "price": 520, "kind": "weapon", "tier": 2},
	"armadura2": {"name": "Cotas de malla élfica (grupo)", "desc": "+9 defensa y resistencia al grupo.", "price": 480, "kind": "armor", "tier": 2},
}

const SHOPS := {
	"roc": {"name": "Herrería de Maese Roc", "stock": ["pan", "pocion", "antidoto", "eter", "bomba", "pluma",
		"espada_acero", "hacha_guerra", "dagas_sombra", "baston_cristal", "arco_largo", "escudo_hierro",
		"yelmo_hierro", "gorro_cuero", "cota_malla", "jubon_cuero", "tunica_mago", "diadema", "anillo_vida", "anillo_fuerza"]},
	"elves": {"name": "Intercambio élfico", "stock": ["pocion", "pocion_mayor", "eter", "antidoto", "flor_luna_item", "pluma",
		"espada_runica", "hacha_runica", "colmillos", "baston_ancestral", "arco_elfico", "egida", "yelmo_acero",
		"capucha_sombra", "diadema_luna", "armadura_placas", "tunica_sabio", "tunica_sombra",
		"anillo_mana", "anillo_sabio", "anillo_veloz", "anillo_guardian"]},
}

const CHESTS := {
	"chest_cath1": {"pocion": 2, "yelmo_hierro": 1}, "chest_cath2": {"eter": 1, "gold": 60, "diadema": 1},
	"chest_crypt1": {"pluma": 1, "anillo_vida": 1},
	"chest_deep1": {"pocion_mayor": 2, "hoja_alba": 1}, "chest_elf1": {"susurro_ilvane": 1, "eter": 2},
	"chest_heart1": {"flor_luna_item": 2, "coraza_olvidado": 1, "anillo_savia": 1},
	"chest_heart2": {"eter": 2, "pluma": 1, "baculo_pacto": 1, "corona_anciana": 1},
	"chest_cave1": {"armadura_mithril": 1, "pocion_mayor": 2}, "chest_cave2": {"anillo_velo": 1, "eter": 2},
	"chest_cave3": {"manto_estrellas": 1, "pluma": 1}, "chest_cave4": {"aurora": 1, "muralla_tortosa": 1},
	"chest_cave5": {"rompemontanas": 1, "lagrimas_selen": 1, "gold": 500},
}

## sprite: ruta (sin extensión) y hframes; scale en combate
const ENEMIES := {
	"wolf_alpha": {"name": "Lobo corrupto", "sprite": "sprites/wolf_big", "frames": 2, "scale": 2.0, "hp": 170, "atk": 13, "def": 4, "mag": 3, "res": 3, "spd": 10,
		"xp": 90, "gold": 40, "weak": ["fire", "light"], "resist": [], "ai": [["e_mordisco", 5], ["e_feroz", 2]], "boss": true},
	"wolf": {"name": "Lobo sombrío", "sprite": "sprites/wolf_big", "frames": 2, "scale": 1.4, "hp": 70, "atk": 16, "def": 5, "mag": 3, "res": 3, "spd": 13,
		"xp": 28, "gold": 14, "weak": ["fire"], "resist": [], "ai": [["e_mordisco", 5], ["e_feroz", 1]], "drops": [["pocion", 0.2]], "tint": [0.8, 0.75, 0.9]},
	"larva": {"name": "Larva morongul", "sprite": "enemies/larva", "frames": 2, "scale": 1.8, "hp": 44, "atk": 11, "def": 3, "mag": 9, "res": 2, "spd": 8,
		"xp": 26, "gold": 9, "weak": ["fire", "light"], "resist": ["dark"], "ai": [["e_zarpazo", 3], ["e_escupir", 2]], "drops": [["antidoto", 0.25]]},
	"bat": {"name": "Murciélago de esporas", "sprite": "enemies/bat", "frames": 2, "scale": 1.6, "hp": 38, "atk": 12, "def": 2, "mag": 8, "res": 3, "spd": 17,
		"xp": 20, "gold": 10, "weak": ["bolt", "ice"], "resist": [], "ai": [["e_mordisco", 3], ["e_esporas", 1]], "drops": [["pan", 0.3]]},
	"thrall": {"name": "Siervo morongul", "sprite": "battle/thrall", "frames": -1, "scale": 2.0, "hp": 96, "atk": 18, "def": 7, "mag": 6, "res": 5, "spd": 10,
		"xp": 48, "gold": 20, "weak": ["light", "fire"], "resist": ["dark"], "ai": [["e_zarpazo", 4], ["e_savia", 2]], "drops": [["pocion", 0.3]]},
	"brute": {"name": "Bruto morongul", "sprite": "enemies/brute", "frames": 2, "scale": 1.55, "hp": 360, "atk": 14, "def": 9, "mag": 6, "res": 6, "spd": 8,
		"xp": 260, "gold": 150, "weak": ["light"], "resist": ["dark"], "ai": [["e_aplastar", 4], ["e_brutal", 2], ["e_rugido", 1]], "boss": true},
	"root": {"name": "Raíz reptante", "sprite": "enemies/root", "frames": 2, "scale": 1.6, "hp": 88, "atk": 16, "def": 9, "mag": 6, "res": 5, "spd": 7,
		"xp": 34, "gold": 16, "weak": ["fire"], "resist": ["nature", "ice"], "ai": [["e_latigazo", 3], ["e_enredar", 2]], "drops": [["antidoto", 0.2]]},
	"spectre": {"name": "Espectro de cristal", "sprite": "enemies/spectre", "frames": 2, "scale": 1.5, "hp": 66, "atk": 9, "def": 16, "mag": 16, "res": 8, "spd": 12,
		"xp": 36, "gold": 22, "weak": ["bolt", "light"], "resist": ["ice", "phys"], "ai": [["e_rayo_cristal", 4], ["e_lamento", 1]], "drops": [["eter", 0.2]]},
	"custodian": {"name": "Custodio de cristal", "sprite": "enemies/custodian", "frames": 2, "scale": 1.35, "hp": 860, "atk": 18, "def": 12, "mag": 17, "res": 12, "spd": 9,
		"xp": 520, "gold": 300, "weak": ["bolt"], "resist": ["ice"], "ai": [["e_puño_cristal", 3], ["e_haz", 2], ["e_rayo_cristal", 2]], "boss": true},
	"kaelen_dark": {"name": "Kaelen, el Caído", "sprite": "battle/kaelen_dark", "frames": -1, "scale": 2.5, "hp": 1000, "atk": 21, "def": 11, "mag": 14, "res": 11, "spd": 15,
		"xp": 700, "gold": 0, "weak": ["light"], "resist": ["dark"], "ai": [["tajo_sombrio", 3], ["estocada", 3], ["filo_viento", 2], ["e_aullido", 1]], "boss": true},
	"wisp": {"name": "Fuego fatuo corrupto", "sprite": "enemies/wisp", "frames": 2, "scale": 1.8, "hp": 52, "atk": 6, "def": 6, "mag": 15, "res": 12, "spd": 14,
		"xp": 30, "gold": 14, "weak": ["ice", "light"], "resist": ["fire", "dark"], "ai": [["e_lamento", 2], ["e_escupir", 3]], "drops": [["eter", 0.25]]},
	"boar": {"name": "Jabalí de cristal negro", "sprite": "enemies/boar", "frames": 2, "scale": 1.7, "hp": 110, "atk": 18, "def": 10, "mag": 2, "res": 4, "spd": 9,
		"xp": 40, "gold": 18, "weak": ["fire", "bolt"], "resist": ["phys"], "ai": [["e_feroz", 3], ["e_mordisco", 3]], "drops": [["pan", 0.4], ["pocion", 0.2]]},
	"mother_root": {"name": "Madre Raíz", "sprite": "enemies/mother_root", "frames": 2, "scale": 1.25, "hp": 1500, "atk": 22, "def": 11, "mag": 20, "res": 13, "spd": 9,
		"xp": 0, "gold": 0, "weak": ["fire", "light"], "resist": ["dark", "nature"], "ai": [["e_raices", 3], ["e_latido", 2], ["e_escupir", 2]], "boss": true},
}


static func xp_to_next(level: int) -> int:
	return 40 + level * level * 18


static func skill(id: String) -> Dictionary:
	return SKILLS.get(id, {})


static func item(id: String) -> Dictionary:
	if ITEMS.has(id):
		return ITEMS[id]
	if EQUIP.has(id):
		var e: Dictionary = EQUIP[id].duplicate()
		var rz: String = str(e.get("rarity", "basic"))
		e["desc"] = "%s · %s · Nv %d · %s" % [RARITY[rz]["name"], SLOT_NAMES[e["slot"]], int(e.get("lvl", 1)), stats_text(e["stats"])]
		if e.has("lore"):
			e["desc"] += "\n" + str(e["lore"])
		e["icon"] = equip_icon(id)
		return e
	return UPGRADES.get(id, {})


static func stats_text(st: Dictionary) -> String:
	var parts: Array = []
	for k in ["hp", "mp", "atk", "def", "mag", "res", "spd"]:
		if st.has(k):
			parts.append("%s %+d" % [STAT_NAMES[k], int(st[k])])
	return ", ".join(parts)


static func equip_icon(id: String) -> String:
	var e: Dictionary = EQUIP[id]
	match str(e["slot"]):
		"weapon":
			return "eq_" + str(e["kind"])
		"shield":
			return "eq_shield"
		"head":
			return "eq_circlet" if str(e["weight"]) == "cloth" else ("eq_hood" if str(e["look"]) == "hood" else "eq_helm")
		"body":
			return "eq_robe" if str(e["look"]).begins_with("robe") else "eq_armor"
	return "eq_ring"
