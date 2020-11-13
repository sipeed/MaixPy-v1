/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_PROCESSING_AGC_LEGACY_ANALOG_AGC_H_
#define MODULES_AUDIO_PROCESSING_AGC_LEGACY_ANALOG_AGC_H_

//#define MIC_LEVEL_FEEDBACK
#ifdef WEBRTC_AGC_DEBUG_DUMP
#include <stdio.h>
#endif


#include <stdint.h>  // NOLINT(build/include)
#include <string.h>

#ifdef WEBRTC_AGC_DEBUG_DUMP
#include <stdio.h>
#endif

#include <stdint.h>  // NOLINT(build/include)
#include <assert.h>

// allpass filter coefficients.
static const uint16_t kResampleAllpass1[3] = {3284, 24441, 49528};
static const uint16_t kResampleAllpass2[3] = {12199, 37471, 60255};


typedef struct {
    int32_t downState[8];
    int16_t HPstate;
    int16_t counter;
    int16_t logRatio;           // log( P(active) / P(inactive) ) (Q10)
    int16_t meanLongTerm;       // Q10
    int32_t varianceLongTerm;   // Q8
    int16_t stdLongTerm;        // Q10
    int16_t meanShortTerm;      // Q10
    int32_t varianceShortTerm;  // Q8
    int16_t stdShortTerm;       // Q10
} AgcVad;                     // total = 54 bytes

typedef struct {
    int32_t capacitorSlow;
    int32_t capacitorFast;
    int32_t gain;
    int32_t gainTable[32];
    int16_t gatePrevious;
    int16_t agcMode;
    AgcVad vadNearend;
    AgcVad vadFarend;
#ifdef WEBRTC_AGC_DEBUG_DUMP
    FILE* logFile;
    int frameCounter;
#endif
} DigitalAgc;

int32_t WebRtcAgc_InitDigital(DigitalAgc *digitalAgcInst, int16_t agcMode);

int32_t WebRtcAgc_ProcessDigital(DigitalAgc *digitalAgcInst,
                                 const int16_t *const *inNear,
                                 size_t num_bands,
                                 int16_t *const *out,
                                 uint32_t FS,
                                 int16_t lowLevelSignal);

int32_t WebRtcAgc_AddFarendToDigital(DigitalAgc *digitalAgcInst,
                                     const int16_t *inFar,
                                     size_t nrSamples);

void WebRtcAgc_InitVad(AgcVad *vadInst);

int16_t WebRtcAgc_ProcessVad(AgcVad *vadInst,    // (i) VAD state
                             const int16_t *in,  // (i) Speech signal
                             size_t nrSamples);  // (i) number of samples

int32_t WebRtcAgc_CalculateGainTable(int32_t *gainTable,         // Q16
                                     int16_t compressionGaindB,  // Q0 (in dB)
                                     int16_t targetLevelDbfs,    // Q0 (in dB)
                                     uint8_t limiterEnable,
                                     int16_t analogTarget);

// Errors
#define AGC_UNSPECIFIED_ERROR 18000
#define AGC_UNSUPPORTED_FUNCTION_ERROR 18001
#define AGC_UNINITIALIZED_ERROR 18002
#define AGC_NULL_POINTER_ERROR 18003
#define AGC_BAD_PARAMETER_ERROR 18004

// Warnings
#define AGC_BAD_PARAMETER_WARNING 18050

enum {
    kAgcModeUnchanged,
    kAgcModeAdaptiveAnalog,
    kAgcModeAdaptiveDigital,
    kAgcModeFixedDigital
};

enum {
    kAgcFalse = 0, kAgcTrue
};

typedef struct {
    int16_t targetLevelDbfs;    // default 3 (-3 dBOv)
    int16_t compressionGaindB;  // default 9 dB
    uint8_t limiterEnable;      // default kAgcTrue (on)
} WebRtcAgcConfig;

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This function analyses the number of samples passed to
 * farend and produces any error code that could arise.
 *
 * Input:
 *      - agcInst           : AGC instance.
 *      - samples           : Number of samples in input vector.
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error.
 */
int WebRtcAgc_GetAddFarendError(void *state, size_t samples);

/*
 * This function processes a 10 ms frame of far-end speech to determine
 * if there is active speech. The length of the input speech vector must be
 * given in samples (80 when FS=8000, and 160 when FS=16000, FS=32000 or
 * FS=48000).
 *
 * Input:
 *      - agcInst           : AGC instance.
 *      - inFar             : Far-end input speech vector
 *      - samples           : Number of samples in input vector
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_AddFarend(void *agcInst, const int16_t *inFar, size_t samples);

/*
 * This function processes a 10 ms frame of microphone speech to determine
 * if there is active speech. The length of the input speech vector must be
 * given in samples (80 when FS=8000, and 160 when FS=16000, FS=32000 or
 * FS=48000). For very low input levels, the input signal is increased in level
 * by multiplying and overwriting the samples in inMic[].
 *
 * This function should be called before any further processing of the
 * near-end microphone signal.
 *
 * Input:
 *      - agcInst           : AGC instance.
 *      - inMic             : Microphone input speech vector for each band
 *      - num_bands         : Number of bands in input vector
 *      - samples           : Number of samples in input vector
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_AddMic(void *agcInst,
                     int16_t *const *inMic,
                     size_t num_bands,
                     size_t samples);

/*
 * This function replaces the analog microphone with a virtual one.
 * It is a digital gain applied to the input signal and is used in the
 * agcAdaptiveDigital mode where no microphone level is adjustable. The length
 * of the input speech vector must be given in samples (80 when FS=8000, and 160
 * when FS=16000, FS=32000 or FS=48000).
 *
 * Input:
 *      - agcInst           : AGC instance.
 *      - inMic             : Microphone input speech vector for each band
 *      - num_bands         : Number of bands in input vector
 *      - samples           : Number of samples in input vector
 *      - micLevelIn        : Input level of microphone (static)
 *
 * Output:
 *      - inMic             : Microphone output after processing (L band)
 *      - inMic_H           : Microphone output after processing (H band)
 *      - micLevelOut       : Adjusted microphone level after processing
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_VirtualMic(void *agcInst,
                         int16_t *const *inMic,
                         size_t num_bands,
                         size_t samples,
                         int32_t micLevelIn,
                         int32_t *micLevelOut);

/*
 * This function processes a 10 ms frame and adjusts (normalizes) the gain both
 * analog and digitally. The gain adjustments are done only during active
 * periods of speech. The length of the speech vectors must be given in samples
 * (80 when FS=8000, and 160 when FS=16000, FS=32000 or FS=48000). The echo
 * parameter can be used to ensure the AGC will not adjust upward in the
 * presence of echo.
 *
 * This function should be called after processing the near-end microphone
 * signal, in any case after any echo cancellation.
 *
 * Input:
 *      - agcInst           : AGC instance
 *      - inNear            : Near-end input speech vector for each band
 *      - num_bands         : Number of bands in input/output vector
 *      - samples           : Number of samples in input/output vector
 *      - inMicLevel        : Current microphone volume level
 *      - echo              : Set to 0 if the signal passed to add_mic is
 *                            almost certainly free of echo; otherwise set
 *                            to 1. If you have no information regarding echo
 *                            set to 0.
 *
 * Output:
 *      - outMicLevel       : Adjusted microphone volume level
 *      - out               : Gain-adjusted near-end speech vector
 *                          : May be the same vector as the input.
 *      - saturationWarning : A returned value of 1 indicates a saturation event
 *                            has occurred and the volume cannot be further
 *                            reduced. Otherwise will be set to 0.
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_Process(void *agcInst,
                      const int16_t *const *inNear,
                      size_t num_bands,
                      size_t samples,
                      int16_t *const *out,
                      int32_t inMicLevel,
                      int32_t *outMicLevel,
                      int16_t echo,
                      uint8_t *saturationWarning);

/*
 * This function sets the config parameters (targetLevelDbfs,
 * compressionGaindB and limiterEnable).
 *
 * Input:
 *      - agcInst           : AGC instance
 *      - config            : config struct
 *
 * Output:
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_set_config(void *agcInst, WebRtcAgcConfig config);

/*
 * This function returns the config parameters (targetLevelDbfs,
 * compressionGaindB and limiterEnable).
 *
 * Input:
 *      - agcInst           : AGC instance
 *
 * Output:
 *      - config            : config struct
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_get_config(void *agcInst, WebRtcAgcConfig *config);

/*
 * This function creates and returns an AGC instance, which will contain the
 * state information for one (duplex) channel.
 */
void *WebRtcAgc_Create(void);

/*
 * This function frees the AGC instance created at the beginning.
 *
 * Input:
 *      - agcInst           : AGC instance.
 */
void WebRtcAgc_Free(void *agcInst);

/*
 * This function initializes an AGC instance.
 *
 * Input:
 *      - agcInst           : AGC instance.
 *      - minLevel          : Minimum possible mic level
 *      - maxLevel          : Maximum possible mic level
 *      - agcMode           : 0 - Unchanged
 *                          : 1 - Adaptive Analog Automatic Gain Control -3dBOv
 *                          : 2 - Adaptive Digital Automatic Gain Control -3dBOv
 *                          : 3 - Fixed Digital Gain 0dB
 *      - fs                : Sampling frequency
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */
int WebRtcAgc_Init(void *agcInst,
                   int32_t minLevel,
                   int32_t maxLevel,
                   int16_t agcMode,
                   uint32_t fs);

#if defined(__cplusplus)
}
#endif

/* Analog Automatic Gain Control variables:
 * Constant declarations (inner limits inside which no changes are done)
 * In the beginning the range is narrower to widen as soon as the measure
 * 'Rxx160_LP' is inside it. Currently the starting limits are -22.2+/-1dBm0
 * and the final limits -22.2+/-2.5dBm0. These levels makes the speech signal
 * go towards -25.4dBm0 (-31.4dBov). Tuned with wbfile-31.4dBov.pcm
 * The limits are created by running the AGC with a file having the desired
 * signal level and thereafter plotting Rxx160_LP in the dBm0-domain defined
 * by out=10*log10(in/260537279.7); Set the target level to the average level
 * of our measure Rxx160_LP. Remember that the levels are in blocks of 16 in
 * Q(-7). (Example matlab code: round(db2pow(-21.2)*16/2^7) )
 */
#define RXX_BUFFER_LEN 10

static const int16_t kMsecSpeechInner = 520;
static const int16_t kMsecSpeechOuter = 340;

static const int16_t kNormalVadThreshold = 400;

static const int16_t kAlphaShortTerm = 6;  // 1 >> 6 = 0.0156
static const int16_t kAlphaLongTerm = 10;  // 1 >> 10 = 0.000977

typedef struct {
    // Configurable parameters/variables
    uint32_t fs;                // Sampling frequency
    int16_t compressionGaindB;  // Fixed gain level in dB
    int16_t targetLevelDbfs;    // Target level in -dBfs of envelope (default -3)
    int16_t agcMode;            // Hard coded mode (adaptAna/adaptDig/fixedDig)
    uint8_t limiterEnable;      // Enabling limiter (on/off (default off))
    WebRtcAgcConfig defaultConfig;
    WebRtcAgcConfig usedConfig;

    // General variables
    int16_t initFlag;
    int16_t lastError;

    // Target level parameters
    // Based on the above: analogTargetLevel = round((32767*10^(-22/20))^2*16/2^7)
    int32_t analogTargetLevel;    // = RXX_BUFFER_LEN * 846805;       -22 dBfs
    int32_t startUpperLimit;      // = RXX_BUFFER_LEN * 1066064;      -21 dBfs
    int32_t startLowerLimit;      // = RXX_BUFFER_LEN * 672641;       -23 dBfs
    int32_t upperPrimaryLimit;    // = RXX_BUFFER_LEN * 1342095;      -20 dBfs
    int32_t lowerPrimaryLimit;    // = RXX_BUFFER_LEN * 534298;       -24 dBfs
    int32_t upperSecondaryLimit;  // = RXX_BUFFER_LEN * 2677832;      -17 dBfs
    int32_t lowerSecondaryLimit;  // = RXX_BUFFER_LEN * 267783;       -27 dBfs
    uint16_t targetIdx;           // Table index for corresponding target level
#ifdef MIC_LEVEL_FEEDBACK
    uint16_t targetIdxOffset;  // Table index offset for level compensation
#endif
    int16_t analogTarget;  // Digital reference level in ENV scale

    // Analog AGC specific variables
    int32_t filterState[8];  // For downsampling wb to nb
    int32_t upperLimit;      // Upper limit for mic energy
    int32_t lowerLimit;      // Lower limit for mic energy
    int32_t Rxx160w32;       // Average energy for one frame
    int32_t Rxx16_LPw32;     // Low pass filtered subframe energies
    int32_t Rxx160_LPw32;    // Low pass filtered frame energies
    int32_t Rxx16_LPw32Max;  // Keeps track of largest energy subframe
    int32_t Rxx16_vectorw32[RXX_BUFFER_LEN];  // Array with subframe energies
    int32_t Rxx16w32_array[2][5];  // Energy values of microphone signal
    int32_t env[2][10];            // Envelope values of subframes

    int16_t Rxx16pos;          // Current position in the Rxx16_vectorw32
    int16_t envSum;            // Filtered scaled envelope in subframes
    int16_t vadThreshold;      // Threshold for VAD decision
    int16_t inActive;          // Inactive time in milliseconds
    int16_t msTooLow;          // Milliseconds of speech at a too low level
    int16_t msTooHigh;         // Milliseconds of speech at a too high level
    int16_t changeToSlowMode;  // Change to slow mode after some time at target
    int16_t firstCall;         // First call to the process-function
    int16_t msZero;            // Milliseconds of zero input
    int16_t msecSpeechOuterChange;  // Min ms of speech between volume changes
    int16_t msecSpeechInnerChange;  // Min ms of speech between volume changes
    int16_t activeSpeech;           // Milliseconds of active speech
    int16_t muteGuardMs;            // Counter to prevent mute action
    int16_t inQueue;                // 10 ms batch indicator

    // Microphone level variables
    int32_t micRef;         // Remember ref. mic level for virtual mic
    uint16_t gainTableIdx;  // Current position in virtual gain table
    int32_t micGainIdx;     // Gain index of mic level to increase slowly
    int32_t micVol;         // Remember volume between frames
    int32_t maxLevel;       // Max possible vol level, incl dig gain
    int32_t maxAnalog;      // Maximum possible analog volume level
    int32_t maxInit;        // Initial value of "max"
    int32_t minLevel;       // Minimum possible volume level
    int32_t minOutput;      // Minimum output volume level
    int32_t zeroCtrlMax;    // Remember max gain => don't amp low input
    int32_t lastInMicLevel;

    int16_t scale;  // Scale factor for internal volume levels
#ifdef MIC_LEVEL_FEEDBACK
    int16_t numBlocksMicLvlSat;
    uint8_t micLvlSat;
#endif
    // Structs for VAD and digital_agc
    AgcVad vadMic;
    DigitalAgc digitalAgc;

#ifdef WEBRTC_AGC_DEBUG_DUMP
    FILE* fpt;
    FILE* agcLog;
    int32_t fcount;
#endif

    int16_t lowLevelSignal;
} LegacyAgc;

#endif  // MODULES_AUDIO_PROCESSING_AGC_LEGACY_ANALOG_AGC_H_
