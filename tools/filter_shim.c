/* filter_shim.c -- exports the plugin's real filter so tools/gen_filter_strips.py can measure its response.
 * Built by that script into a throwaway shared library (gcc -shared); it is never part of the plugin. */
#include <string.h>
#include "../src/dsp/dj_filter.h"

/* Impulse response of one filter setting: `n` samples into out[]. type 0 = low-pass, 1 = high-pass. */
void shim_impulse(int type, float cut01, float res01, int curve, int n, float *out)
{
    static dj_filter_t f;                    /* large; static keeps it off a small stack */
    dj_filter_init(&f, DJ_FS, type ? DJ_FLT_HP : DJ_FLT_LP);
    dj_filter_set(&f, cut01, res01, curve);
    dj_filter_reset(&f);                     /* start settled at these values: no smoothing ramp in the measurement */
    float l[DJ_BLOCK], r[DJ_BLOCK];
    for (int done = 0; done < n;) {
        int m = n - done > DJ_BLOCK ? DJ_BLOCK : n - done;
        for (int i = 0; i < m; i++) l[i] = r[i] = (done + i == 0) ? 1.0f : 0.0f;
        dj_filter_process(&f, l, r, m);
        memcpy(out + done, l, (size_t)m * sizeof(float));
        done += m;
    }
}
