// Goo Alert - Sound-Synthese (wird in main.c eingebunden)
enum { S_POP, S_SHOOT, S_BOOM, S_ZAP, S_FREEZE, S_COIN, S_PLACE, S_UPGRADE, S_WAVE, S_LEAK, S_WIN, S_LOSE, S_CLICK, S_ERR, NSND };
typedef struct { s16 *data; u32 n; } Sample;
static Sample snd[NSND];
static ndspWaveBuf wbuf[12];
static bool sndOk;
static int sndCh;
#define SRATE 22050
#define NCH 12

static void genSnd(int id, float dur) {
	u32 n = (u32)(dur * SRATE);
	s16 *d = (s16 *)linearAlloc(n * sizeof(s16));
	if (!d) return;
	float ph = 0, lp = 0;
	for (u32 i = 0; i < n; i++) {
		float t = (float)i / SRATE, p = t / dur, v = 0, f = 440;
		float nz = (rand() & 1023) / 512.0f - 1;
		switch (id) {
		case S_POP: f = 700 - 500 * p; v = sinf(ph) * (1 - p) * (1 - p) + nz * 0.25f * expf(-t * 120); break;
		case S_SHOOT: f = 1800 - 900 * p; v = sinf(ph) * expf(-t * 70) * 0.35f + nz * 0.15f * expf(-t * 90); break;
		case S_BOOM: lp += (nz - lp) * 0.15f; f = 60; v = (lp * 1.8f + sinf(ph) * 0.6f) * expf(-t * 7); break;
		case S_ZAP: f = 220 + 80 * sinf(t * 300); v = ((sinf(ph) > 0 ? 0.5f : -0.5f) + nz * 0.3f) * (1 - p) * 0.6f; break;
		case S_FREEZE: f = 2400 + 600 * sinf(t * 40); v = sinf(ph) * (1 - p) * 0.3f + nz * 0.1f * (1 - p); break;
		case S_COIN: f = t < 0.06f ? 1320 : 1760; v = (sinf(ph) + 0.3f * sinf(ph * 2)) * expf(-fmodf(t, 0.06f) * 25) * 0.6f; break;
		case S_PLACE: f = 180 - 80 * p; v = sinf(ph) * expf(-t * 25) + nz * 0.2f * expf(-t * 60); break;
		case S_UPGRADE: {
			static const float nt[4] = {660, 880, 1100, 1320};
			int k = (int)(t / 0.06f);
			if (k > 3) k = 3;
			f = nt[k];
			v = sinf(ph) * expf(-fmodf(t, 0.06f) * 20) * 0.6f;
			break;
		}
		case S_WAVE: f = t < 0.15f ? 392 : 523; v = (sinf(ph) + 0.4f * sinf(ph * 2) + 0.2f * sinf(ph * 3)) * fminf(1, t * 30) * (1 - p) * 0.45f; break;
		case S_LEAK: f = 140 - 60 * p; v = (sinf(ph) > 0 ? 0.5f : -0.5f) * (1 - p) * 0.6f; break;
		case S_WIN: {
			static const float nt[6] = {523, 659, 784, 1047, 784, 1047};
			int k = (int)(t / 0.13f);
			if (k > 5) k = 5;
			f = nt[k];
			v = (sinf(ph) + 0.3f * sinf(ph * 2)) * (k == 5 ? 1 - (t - 0.65f) / (dur - 0.65f) : 0.8f) * 0.55f;
			break;
		}
		case S_LOSE: f = 300 * powf(0.5f, p * 1.5f); v = (sinf(ph) + 0.4f * sinf(ph * 1.5f)) * (1 - p) * 0.5f; break;
		case S_CLICK: f = 1000; v = sinf(ph) * expf(-t * 80) * 0.5f; break;
		case S_ERR: f = 150; v = (sinf(ph) > 0 ? 0.4f : -0.4f) * (1 - p); break;
		}
		ph += 2 * PI * f / SRATE;
		if (ph > 2 * PI * 64) ph -= 2 * PI * 64;
		if (v > 1) v = 1;
		if (v < -1) v = -1;
		d[i] = (s16)(v * 24000);
	}
	DSP_FlushDataCache(d, n * sizeof(s16));
	snd[id].data = d;
	snd[id].n = n;
}
static void sndInit(void) {
	if (R_FAILED(ndspInit())) return;
	sndOk = true;
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	for (int ch = 0; ch < NCH; ch++) {
		ndspChnReset(ch);
		ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
		ndspChnSetRate(ch, SRATE);
		ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
	}
	static const float dur[NSND] = {0.09f, 0.05f, 0.45f, 0.14f, 0.3f, 0.16f, 0.12f, 0.28f, 0.45f, 0.25f, 1.1f, 1.0f, 0.04f, 0.1f};
	for (int i = 0; i < NSND; i++) genSnd(i, dur[i]);
}
static void play(int id, float pitch, float vol) {
	if (!sndOk || !SD.sound || !snd[id].data) return;
	int ch = sndCh;
	sndCh = (sndCh + 1) % NCH;
	ndspChnWaveBufClear(ch);
	ndspChnSetRate(ch, SRATE * pitch);
	float mix[12] = {0};
	mix[0] = mix[1] = vol;
	ndspChnSetMix(ch, mix);
	memset(&wbuf[ch], 0, sizeof wbuf[ch]);
	wbuf[ch].data_vaddr = snd[id].data;
	wbuf[ch].nsamples = snd[id].n;
	ndspChnWaveBufAdd(ch, &wbuf[ch]);
}
static void sndExit(void) {
	if (!sndOk) return;
	ndspExit();
	for (int i = 0; i < NSND; i++) if (snd[i].data) linearFree(snd[i].data);
}
static float frnd(void) { return (rand() & 0xFFFF) / 65535.0f; }
