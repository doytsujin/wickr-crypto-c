/*
 * Copyright © 2012-2017 Wickr Inc.  All rights reserved.
 *
 * This code is being released for EDUCATIONAL, ACADEMIC, AND CODE REVIEW PURPOSES
 * ONLY.  COMMERCIAL USE OF THE CODE IS EXPRESSLY PROHIBITED.  For additional details,
 * please see LICENSE
 *
 * THE CODE IS MADE AVAILABLE "AS-IS" AND WITHOUT ANY EXPRESS OR
 * IMPLIED GUARANTEES AS TO FITNESS, MERCHANTABILITY, NON-
 * INFRINGEMENT OR OTHERWISE. IT IS NOT BEING PROVIDED IN TRADE BUT ON
 * A VOLUNTARY BASIS ON BEHALF OF THE AUTHOR’S PART FOR THE BENEFIT
 * OF THE LICENSEE AND IS NOT MADE AVAILABLE FOR CONSUMER USE OR ANY
 * OTHER USE OUTSIDE THE TERMS OF THIS LICENSE. ANYONE ACCESSING THE
 * CODE SHOULD HAVE THE REQUISITE EXPERTISE TO SECURE THEIR SYSTEM
 * AND DEVICES AND TO ACCESS AND USE THE CODE FOR REVIEW PURPOSES
 * ONLY. LICENSEE BEARS THE RISK OF ACCESSING AND USING THE CODE. IN
 * PARTICULAR, AUTHOR BEARS NO LIABILITY FOR ANY INTERFERENCE WITH OR
 * ADVERSE EFFECT THAT MAY OCCUR AS A RESULT OF THE LICENSEE
 * ACCESSING AND/OR USING THE CODE ON LICENSEE’S SYSTEM.
 */

#ifndef stream_iv_h
#define stream_iv_h

#include "crypto_engine.h"

struct wickr_stream_iv {
    wickr_crypto_engine_t engine;
    wickr_buffer_t *seed;
    wickr_cipher_t cipher;
    uint64_t gen_count;
};

typedef struct wickr_stream_iv wickr_stream_iv_t;

wickr_stream_iv_t *wickr_stream_iv_create(const wickr_crypto_engine_t engine, wickr_cipher_t cipher);
wickr_stream_iv_t *wickr_stream_iv_copy(wickr_stream_iv_t *iv);
void wickr_stream_iv_destroy(wickr_stream_iv_t **iv);

wickr_buffer_t *wickr_stream_iv_generate(wickr_stream_iv_t *iv);

#endif /* stream_iv_h */
