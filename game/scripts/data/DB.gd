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

const BRANCHES := {
	"melee": {"name": "Cuerpo a cuerpo", "desc": "Espada en primera línea. Más vida y golpes duros.", "icon": "icon_melee",
		"bonus": {"hp": 10, "atk": 3, "def": 1}, "skill": "tajo_doble", "classes": ["knight", "berserker"]},
	"ranged": {"name": "A distancia", "desc": "Arco y precisión. Golpes certeros y rápidos.", "icon": "icon_ranged",
		"bonus": {"hp": 4, "atk": 2, "spd": 3}, "skill": "disparo_certero", "classes": ["archer", "hunter"]},
	"dps": {"name": "DPS", "desc": "Todo al daño: críticos devastadores, pero frágil.", "icon": "icon_dps",
		"bonus": {"atk": 3, "mag": 3, "spd": 2}, "skill": "punalada", "classes": ["rogue", "mage"]},
	"healer": {"name": "Sanador", "desc": "Magia de luz. Curas mucho mejor y proteges a los tuyos.", "icon": "icon_healer",
		"bonus": {"hp": 4, "mp": 8, "mag": 4, "res": 2}, "skill": "luz_sanadora", "classes": ["cleric", "druid"]},
	"tank": {"name": "Tanque", "desc": "Un muro viviente que atrae los golpes y protege al grupo.", "icon": "icon_tank",
		"bonus": {"hp": 18, "def": 4, "res": 2}, "skill": "provocar", "classes": ["paladin", "guardian"]},
}

const CLASSES := {
	"knight": {"name": "Caballero", "desc": "Protector implacable. Aturde y resiste.", "bonus": {"hp": 16, "atk": 4, "def": 4},
		"skills": {5: "guardia", 7: "carga", 9: "juramento"}},
	"berserker": {"name": "Berserker", "desc": "Furia pura. Daño masivo a cambio de defensa.", "bonus": {"hp": 10, "atk": 8},
		"skills": {5: "furia", 7: "remolino", 9: "masacre"}},
	"archer": {"name": "Arquero", "desc": "Lluvia de flechas y disparos helados.", "bonus": {"atk": 5, "spd": 4},
		"skills": {5: "lluvia_flechas", 7: "flecha_hielo", 9: "tormenta"}},
	"hunter": {"name": "Cazador", "desc": "Venenos y trampas. Debilita a la presa antes de rematarla.", "bonus": {"atk": 4, "spd": 3, "hp": 6},
		"skills": {5: "flecha_veneno", 7: "trampa", 9: "caceria"}},
	"rogue": {"name": "Pícaro", "desc": "Roba, golpea desde las sombras y encadena cortes.", "bonus": {"atk": 5, "spd": 5},
		"skills": {5: "robar", 7: "sombras", 9: "mil_cortes"}},
	"mage": {"name": "Mago arcano", "desc": "Fuego, hielo y rayo. Destrucción elemental.", "bonus": {"mag": 8, "mp": 12},
		"skills": {5: "fuego", 6: "hielo", 7: "rayo", 9: "meteoro"}},
	"cleric": {"name": "Clérigo", "desc": "Curaciones en grupo y resurrección.", "bonus": {"mag": 5, "mp": 12, "res": 3},
		"skills": {5: "cura_grupo", 7: "resurreccion", 9: "milagro"}},
	"druid": {"name": "Druida", "desc": "Regeneración y la furia del bosque.", "bonus": {"mag": 5, "mp": 8, "hp": 8},
		"skills": {5: "regeneracion", 7: "espinas", 9: "despertar"}},
	"paladin": {"name": "Paladín", "desc": "Escudos sagrados y juicio de luz.", "bonus": {"hp": 14, "def": 3, "mag": 4},
		"skills": {5: "escudo_sagrado", 7: "juicio", 9: "alba"}},
	"guardian": {"name": "Guardián", "desc": "Un bastión que protege y castiga.", "bonus": {"hp": 22, "def": 5, "res": 3},
		"skills": {5: "muro", 7: "golpe_escudo", 9: "bastion"}},
}

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

const UPGRADES := {
	"arma1": {"name": "Armas de acero (grupo)", "desc": "+4 ataque y magia a todo el grupo.", "price": 220, "kind": "weapon", "tier": 1},
	"armadura1": {"name": "Armaduras de cuero (grupo)", "desc": "+4 defensa y resistencia al grupo.", "price": 200, "kind": "armor", "tier": 1},
	"arma2": {"name": "Armas élficas (grupo)", "desc": "+9 ataque y magia a todo el grupo.", "price": 520, "kind": "weapon", "tier": 2},
	"armadura2": {"name": "Cotas de malla élfica (grupo)", "desc": "+9 defensa y resistencia al grupo.", "price": 480, "kind": "armor", "tier": 2},
}

const SHOPS := {
	"roc": {"name": "Herrería de Maese Roc", "stock": ["pan", "pocion", "antidoto", "eter", "bomba", "pluma", "arma1", "armadura1"]},
	"elves": {"name": "Intercambio élfico", "stock": ["pocion", "pocion_mayor", "eter", "antidoto", "flor_luna_item", "pluma", "arma2", "armadura2"]},
}

const CHESTS := {
	"chest_cath1": {"pocion": 2}, "chest_cath2": {"eter": 1, "gold": 60}, "chest_crypt1": {"pluma": 1},
	"chest_deep1": {"pocion_mayor": 2}, "chest_heart1": {"flor_luna_item": 2}, "chest_heart2": {"eter": 2, "pluma": 1},
}

## sprite: ruta (sin extensión) y hframes; scale en combate
const ENEMIES := {
	"wolf_alpha": {"name": "Lobo corrupto", "sprite": "sprites/wolf_big", "frames": 2, "scale": 2.0, "hp": 170, "atk": 13, "def": 4, "mag": 3, "res": 3, "spd": 10,
		"xp": 90, "gold": 40, "weak": ["fire", "light"], "resist": [], "ai": [["e_mordisco", 5], ["e_feroz", 2]], "boss": true},
	"wolf": {"name": "Lobo sombrío", "sprite": "sprites/wolf_big", "frames": 2, "scale": 1.4, "hp": 70, "atk": 16, "def": 5, "mag": 3, "res": 3, "spd": 13,
		"xp": 28, "gold": 14, "weak": ["fire"], "resist": [], "ai": [["e_mordisco", 5], ["e_feroz", 1]], "drops": [["pocion", 0.2]], "tint": [0.8, 0.75, 0.9]},
	"larva": {"name": "Larva morongul", "sprite": "enemies/larva", "frames": 2, "scale": 2.0, "hp": 44, "atk": 11, "def": 3, "mag": 9, "res": 2, "spd": 8,
		"xp": 26, "gold": 9, "weak": ["fire", "light"], "resist": ["dark"], "ai": [["e_zarpazo", 3], ["e_escupir", 2]], "drops": [["antidoto", 0.25]]},
	"bat": {"name": "Murciélago de esporas", "sprite": "enemies/bat", "frames": 2, "scale": 2.0, "hp": 38, "atk": 12, "def": 2, "mag": 8, "res": 3, "spd": 17,
		"xp": 20, "gold": 10, "weak": ["bolt", "ice"], "resist": [], "ai": [["e_mordisco", 3], ["e_esporas", 1]], "drops": [["pan", 0.3]]},
	"thrall": {"name": "Siervo morongul", "sprite": "chars/thrall", "frames": 0, "scale": 2.4, "hp": 96, "atk": 18, "def": 7, "mag": 6, "res": 5, "spd": 10,
		"xp": 48, "gold": 20, "weak": ["light", "fire"], "resist": ["dark"], "ai": [["e_zarpazo", 4], ["e_savia", 2]], "drops": [["pocion", 0.3]]},
	"brute": {"name": "Bruto morongul", "sprite": "enemies/brute", "frames": 2, "scale": 2.0, "hp": 360, "atk": 14, "def": 9, "mag": 6, "res": 6, "spd": 8,
		"xp": 260, "gold": 150, "weak": ["light"], "resist": ["dark"], "ai": [["e_aplastar", 4], ["e_brutal", 2], ["e_rugido", 1]], "boss": true},
	"root": {"name": "Raíz reptante", "sprite": "enemies/root", "frames": 2, "scale": 1.8, "hp": 88, "atk": 16, "def": 9, "mag": 6, "res": 5, "spd": 7,
		"xp": 34, "gold": 16, "weak": ["fire"], "resist": ["nature", "ice"], "ai": [["e_latigazo", 3], ["e_enredar", 2]], "drops": [["antidoto", 0.2]]},
	"spectre": {"name": "Espectro de cristal", "sprite": "enemies/spectre", "frames": 2, "scale": 1.8, "hp": 66, "atk": 9, "def": 16, "mag": 16, "res": 8, "spd": 12,
		"xp": 36, "gold": 22, "weak": ["bolt", "light"], "resist": ["ice", "phys"], "ai": [["e_rayo_cristal", 4], ["e_lamento", 1]], "drops": [["eter", 0.2]]},
	"custodian": {"name": "Custodio de cristal", "sprite": "enemies/custodian", "frames": 2, "scale": 1.7, "hp": 860, "atk": 18, "def": 12, "mag": 17, "res": 12, "spd": 9,
		"xp": 520, "gold": 300, "weak": ["bolt"], "resist": ["ice"], "ai": [["e_puño_cristal", 3], ["e_haz", 2], ["e_rayo_cristal", 2]], "boss": true},
	"kaelen_dark": {"name": "Kaelen, el Caído", "sprite": "chars/kaelen_dark", "frames": 0, "scale": 3.0, "hp": 1000, "atk": 21, "def": 11, "mag": 14, "res": 11, "spd": 15,
		"xp": 700, "gold": 0, "weak": ["light"], "resist": ["dark"], "ai": [["tajo_sombrio", 3], ["estocada", 3], ["filo_viento", 2], ["e_aullido", 1]], "boss": true},
	"mother_root": {"name": "Madre Raíz", "sprite": "enemies/mother_root", "frames": 2, "scale": 1.5, "hp": 1500, "atk": 22, "def": 11, "mag": 20, "res": 13, "spd": 9,
		"xp": 0, "gold": 0, "weak": ["fire", "light"], "resist": ["dark", "nature"], "ai": [["e_raices", 3], ["e_latido", 2], ["e_escupir", 2]], "boss": true},
}


static func xp_to_next(level: int) -> int:
	return 40 + level * level * 18


static func skill(id: String) -> Dictionary:
	return SKILLS.get(id, {})


static func item(id: String) -> Dictionary:
	if ITEMS.has(id):
		return ITEMS[id]
	return UPGRADES.get(id, {})
