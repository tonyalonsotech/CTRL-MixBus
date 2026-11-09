/* dj_eq_config.h -- the DJ EQ's frequencies, in ONE place.
 *
 * STATUS: PROVISIONAL / UNVERIFIED. Nothing below has been measured on a DJM-900NXS2, and Pioneer DJ's public
 * specifications page for the NXS2 does not publish EQ frequencies or ranges. The only published-looking figures
 * found (HI 13 kHz, MID 1 kHz, LOW 70 Hz, -26..+6 dB) come from a forum post citing the ORIGINAL DJM-900 manual and
 * are themselves unchecked. Per the project rules these must be verified against documentation or measured, not
 * assumed. Procedure and evidence log: docs/EQ_VERIFICATION.md. Until a verified set is committed here, the plugin
 * must not claim to match the hardware's crossover points.
 *
 * Changing these values changes how saved presets SOUND (never how they load); record the change in CHANGELOG. */
#pragma once

#define DJ_EQ_ISO_XOVER_LOW_HZ   250.0   /* PROVISIONAL: LOW | MID  */
#define DJ_EQ_ISO_XOVER_HIGH_HZ  2500.0  /* PROVISIONAL: MID | HIGH */

#define DJ_EQ_LOWSHELF_HZ        250.0   /* PROVISIONAL (EQ mode) */
#define DJ_EQ_MIDBELL_HZ         1000.0  /* PROVISIONAL (matches the unverified 1 kHz figure); bell width also provisional */
#define DJ_EQ_MIDBELL_Q          0.5
#define DJ_EQ_HIGHSHELF_HZ       2500.0  /* PROVISIONAL (EQ mode) */

#define DJ_EQ_RANGE_BOOST_DB     6.0     /* PROVISIONAL: +6 dB (unverified for the NXS2) */
#define DJ_EQ_RANGE_CUT_DB       26.0    /* PROVISIONAL: -26 dB in EQ mode (unverified); ISOLATOR cuts to -inf by design */
