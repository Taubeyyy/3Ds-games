// Gibt die Kartendaten als JSON aus (fuer den Grafik-Generator)
#include "../source/td.h"
#include <stdio.h>
int main(void) {
	printf("[");
	for (int m = 0; m < NMAP; m++) {
		const MapDef *d = &MAPS[m];
		printf("%s{\"theme\":%d,\"pts\":[", m ? "," : "", d->theme);
		for (int i = 0; i < d->n; i++) printf("%s[%g,%g]", i ? "," : "", d->pts[i][0], d->pts[i][1]);
		printf("]}");
	}
	printf("]\n");
	return 0;
}
