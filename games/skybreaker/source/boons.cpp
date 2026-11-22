// SKYBREAKER – Segen der sieben Götter und Amboss-Verbesserungen
#include <cstring>
#include "meta.hpp"

const char *RARITY_NAME[3] = {"Gewöhnlich", "Selten", "Episch"};
const u32 RARITY_COL[3] = {0xE8E4F8, 0x6AB4FF, 0xC88AFF};
static const float RARITY_POW[3] = {1.0f, 1.5f, 2.0f};

static const char *NAMES[CH_COUNT][5] = {
	{"", "", "", "", ""},
	{"Glutklinge", "Feuerwirbel", "Flammensplitter", "Funkenschritt", "Glutherz"},
	{"Donnerklinge", "Sturmwirbel", "Blitzsplitter", "Wetterleuchten", "Sturmschritt"},
	{"Wellenklinge", "Strudel", "Gischtsplitter", "Brandungsschritt", "Gezeitenfluss"},
	{"Schattenklinge", "Dämmerwirbel", "Nachtsplitter", "Schattenschritt", "Klingenschatten"},
	{"Steinklinge", "Bebenwirbel", "Felssplitter", "Granithaut", "Steinhaut"},
	{"Lichtklinge", "Sternenwirbel", "Lichtsplitter", "Sternenschritt", "Sternensegen"},
	{"Dornenklinge", "Rankenwirbel", "Giftsplitter", "Wurzelschritt", "Wildwuchs"},
};

const char *GOD_LINES[CH_COUNT][2] = {
	{"", ""},
	{"Na endlich, kleine Schmiedin! Lass sie brennen.", "Feuer vergisst nie. Ich auch nicht."},
	{"HA! Schneller, lauter, heller - so gefällt mir das!", "Der Himmel grollt. Für dich."},
	{"Atme. Die Flut kommt, wann sie will.", "Lass sie gegen die Brandung laufen."},
	{"Psst... sie sehen dich nicht kommen.", "Ein Schatten mehr. Wer zählt schon?"},
	{"Gute Klinge. Schlechte Haltung. Hier, nimm das.", "Stein bricht nicht. Merk dir das."},
	{"Die Sterne sehen dich, Seren.", "Ein wenig Licht für deinen Weg."},
	{"...", "Der Wolf mag dich. Ich noch nicht."},
};

const char *boonName(int god, int slot) { return god > 0 && god < CH_COUNT && slot >= 0 && slot <= 4 ? NAMES[god][slot] : "?"; }

static void elemText(int god, float p, char *o, int n) {
	switch (god) {
	case CH_IGNARA: snprintf(o, n, "setzt in Brand (%d Schaden/s)", int(6 * p)); break;
	case CH_THALOR: snprintf(o, n, "Kettenblitz auf 2 Feinde (%d)", int(9 * p)); break;
	case CH_NERYS: snprintf(o, n, "starker Rückstoß, verlangsamt"); break;
	case CH_VEXA: snprintf(o, n, "+%d%% Krit-Chance", int(25 * p)); break;
	case CH_KORRIN: snprintf(o, n, "+%d%% Schaden", int(35 * p)); break;
	case CH_LYRA: snprintf(o, n, "heilt dich um %.1f je Treffer", 1.0f * p); break;
	case CH_SYLV: snprintf(o, n, "vergiftet (stapelt bis 5)"); break;
	default: o[0] = 0;
	}
}

void boonDesc(const Offer &o, char *out, int n) {
	if (o.anvil >= 0) { snprintf(out, n, "%s", anvilDesc(o.anvil)); return; }
	float p = RARITY_POW[o.rarity];
	char e[96];
	elemText(o.god, p, e, sizeof e);
	if (o.upgrade) { snprintf(out, n, "Verstärkt deinen Segen um +%d%%.", int(p * 50)); return; }
	switch (o.slot) {
	case SL_ATTACK: snprintf(out, n, "Deine Angriffe: %s.", e); break;
	case SL_SPECIAL:
		switch (o.god) {
		case CH_IGNARA: snprintf(out, n, "Wirbel hinterlässt einen Feuerring, der %d/s brennt.", int(15 * p)); break;
		case CH_THALOR: snprintf(out, n, "Wirbel entfesselt einen Tornado, der Feinde jagt und einsaugt (%.0f s).", 3 + p); break;
		case CH_NERYS: snprintf(out, n, "Strudel: Wirbel zieht alle Feinde in der Nähe zu dir und bremst sie."); break;
		case CH_VEXA: snprintf(out, n, "Zwei Schattenklone wirbeln neben dir mit (%d Schaden).", int(26 * p)); break;
		case CH_KORRIN: snprintf(out, n, "Wirbel löst eine Schockwelle aus, die durch den Raum rollt (%d).", int(30 * p)); break;
		case CH_LYRA: snprintf(out, n, "Wirbel schießt 6 Sternenstrahlen in alle Richtungen (%d).", int(18 * p)); break;
		case CH_SYLV: snprintf(out, n, "Ranken halten getroffene Feinde 2 s fest und vergiften sie."); break;
		}
		break;
	case SL_CAST:
		switch (o.god) {
		case CH_IGNARA: snprintf(out, n, "Splitter explodiert beim Aufprall (%d) und setzt in Brand.", int(18 * p)); break;
		case CH_THALOR: snprintf(out, n, "Statt zu werfen: Ein Blitz trifft automatisch den nächsten Feind (%d).", int(44 * p)); break;
		case CH_NERYS: snprintf(out, n, "Splitter prallt bis zu 3x von Feind zu Feind."); break;
		case CH_VEXA: snprintf(out, n, "Wirft 3 zielsuchende Schattendolche (je %d).", int(18 * p)); break;
		case CH_KORRIN: snprintf(out, n, "Ein Felsbrocken rollt durch alle Feinde und stößt sie weg (%d).", int(50 * p)); break;
		case CH_LYRA: snprintf(out, n, "Ein Stern kreist %.0f s um dich und trifft alle, die er berührt.", 6 + p * 2); break;
		case CH_SYLV: snprintf(out, n, "Ruft einen Geistwolf, der %.0f s an deiner Seite kämpft.", 8 + p * 2); break;
		default: snprintf(out, n, "Dein Splitterwurf: %s.", e);
		}
		break;
	case SL_DASH:
		switch (o.god) {
		case CH_IGNARA: snprintf(out, n, "Ausweichen hinterlässt eine Explosion (%d).", int(14 * p)); break;
		case CH_THALOR: snprintf(out, n, "Am Ende des Ausweichens schlägt ein Blitz ein (%d).", int(18 * p)); break;
		case CH_NERYS: snprintf(out, n, "Ausweichen endet mit einer Welle, die Feinde wegstößt."); break;
		case CH_VEXA: snprintf(out, n, "Nach dem Ausweichen ist dein nächster Treffer kritisch."); break;
		case CH_KORRIN: snprintf(out, n, "Nach dem Ausweichen: -40%% Schaden für %.1f s.", 1.3f * p); break;
		case CH_LYRA: snprintf(out, n, "Jedes Ausweichen heilt dich um %.1f.", 2.5f * p); break;
		case CH_SYLV: snprintf(out, n, "Ausweichen lässt Wurzeln zurück: Feinde festgehalten + Gift."); break;
		}
		break;
	case SL_PASSIVE:
		switch (o.god) {
		case CH_IGNARA: snprintf(out, n, "Brennende Feinde erleiden +25%% Schaden."); break;
		case CH_THALOR: snprintf(out, n, "+15%% Angriffstempo, +10%% Lauftempo."); break;
		case CH_NERYS: snprintf(out, n, "Verlangsamte Feinde erleiden +25%% Schaden."); break;
		case CH_VEXA: snprintf(out, n, "Kritische Treffer: +60%% Schaden."); break;
		case CH_KORRIN: snprintf(out, n, "+30 max. Leben und sofortige Heilung."); break;
		case CH_LYRA: snprintf(out, n, "Heilt 15%% Leben nach jedem Raum."); break;
		case CH_SYLV: snprintf(out, n, "Vergiftete Feinde verbreiten Gift, wenn sie fallen."); break;
		}
		break;
	}
}

int makeOffers(int god, Offer *out, float luck, Rng &r) {
	extern Arena AR;
	int cand[5], n = 0;
	for (int s = 0; s < 5; s++) {
		if (s == SL_PASSIVE && AR.mods.passive[god]) continue;
		cand[n++] = s;
	}
	for (int i = n - 1; i > 0; i--) { int j = r.irange(0, i); int t = cand[i]; cand[i] = cand[j]; cand[j] = t; }
	int k = n < 3 ? n : 3;
	for (int i = 0; i < k; i++) {
		Offer &o = out[i];
		o = Offer();
		o.god = god;
		o.slot = cand[i];
		float x = r.f();
		o.rarity = x < 0.08f + luck ? 2 : x < 0.33f + luck * 1.5f ? 1 : 0;
		o.upgrade = o.slot < SL_COUNT && AR.mods.slotGod[o.slot] == god;
	}
	return k;
}

void applyOffer(Mods &m, const Offer &o, Arena &ar) {
	if (o.anvil >= 0) {
		switch (o.anvil) {
		case 0: m.wide = true; break;
		case 1: m.finisher = true; break;
		case 2: m.doubleSpin = true; break;
		case 3: m.fanCast = true; break;
		case 4: m.bladeWave = true; break;
		case 5: m.dashCharges++; m.dashRecharge *= 0.75f; ar.hero.dashCharges++; break;
		case 6: m.dmg *= 1.15f; m.crit += 0.05f; break;
		}
		return;
	}
	float p = RARITY_POW[o.rarity];
	if (o.slot < SL_COUNT) {
		if (o.upgrade) m.slotPow[o.slot] += 0.5f * p;
		else { m.slotGod[o.slot] = o.god; m.slotPow[o.slot] = p; }
	} else {
		m.passive[o.god] = true;
		if (o.god == CH_KORRIN) { m.maxHp += 30; ar.heal(30); }
	}
}

static const char *ANVIL_NAME[7] = {"Weite Hiebe", "Donnernder Abschluss", "Doppelwirbel", "Splitterfächer", "Windklinge", "Schneller Atem", "Sternenstahl"};
static const char *ANVIL_DESC[7] = {
	"Angriffe reichen 30% weiter.", "3. Kombo-Schlag: +60% Schaden und Schockwelle.", "Der Wirbel trifft zweimal.",
	"Splitterwurf wirft 3 Splitter im Fächer.", "Jeder Hieb schickt eine Klingenwelle los.", "+1 Ausweich-Ladung, lädt schneller.",
	"+15% Schaden und +5% Krit-Chance."};
const char *anvilName(int i) { return ANVIL_NAME[i]; }
const char *anvilDesc(int i) { return ANVIL_DESC[i]; }

int makeAnvilOffers(const Mods &m, Offer *out, Rng &r) {
	bool own[7] = {m.wide, m.finisher, m.doubleSpin, m.fanCast, m.bladeWave, false, false};
	int cand[7], n = 0;
	for (int i = 0; i < 7; i++) if (!own[i]) cand[n++] = i;
	for (int i = n - 1; i > 0; i--) { int j = r.irange(0, i); int t = cand[i]; cand[i] = cand[j]; cand[j] = t; }
	int k = n < 3 ? n : 3;
	for (int i = 0; i < k; i++) { out[i] = Offer(); out[i].anvil = cand[i]; }
	return k;
}
