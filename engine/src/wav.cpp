// FORGE – WAV-Lader (RIFF/PCM16) für eingebettete Musik und Klänge
#include <cstring>
#include "forge/forge.hpp"

namespace forge {
namespace audio {
Sound loadWav(const void *data, size_t size) {
	const u8 *p = (const u8 *)data;
	Sound none;
	if (size < 44 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "WAVE", 4)) return none;
	u16 ch = 1, bits = 16;
	u32 rate = 22050;
	size_t off = 12;
	while (off + 8 <= size) {
		u32 len;
		memcpy(&len, p + off + 4, 4);
		if (!memcmp(p + off, "fmt ", 4) && len >= 16) {
			memcpy(&ch, p + off + 10, 2);
			memcpy(&rate, p + off + 12, 4);
			memcpy(&bits, p + off + 22, 2);
		} else if (!memcmp(p + off, "data", 4)) {
			if (bits != 16 || ch < 1 || ch > 2) return none;
			if (off + 8 + len > size) len = u32(size - off - 8);
			u32 n = len / (2 * ch);
			Sound s = make(n, rate);
			if (!s.data) return none;
			const u8 *d = p + off + 8;
			for (u32 i = 0; i < n; i++) {
				s16 a, b = 0;
				memcpy(&a, d + i * 2 * ch, 2);
				if (ch == 2) memcpy(&b, d + i * 4 + 2, 2);
				s.data[i] = ch == 2 ? s16((a + b) / 2) : a;
			}
			commit(s);
			return s;
		}
		off += 8 + len + (len & 1);
	}
	return none;
}
} // namespace audio
} // namespace forge
