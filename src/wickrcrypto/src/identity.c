
#include "identity.h"
#include "private/identity_priv.h"
#include "memory.h"

wickr_identity_t *wickr_identity_create(wickr_identity_type type, wickr_buffer_t *identifier, wickr_ec_key_t *sig_key, wickr_ecdsa_result_t *signature)
{
    if (!identifier || identifier->length >= MAX_IDENTIFIER_LEN || !sig_key) {
        return NULL;
    }
    
    wickr_identity_t *new_identity = wickr_alloc_zero(sizeof(wickr_identity_t));
    
    if (!new_identity) {
        return NULL;
    }
    
    new_identity->type = type;
    new_identity->identifier = identifier;
    new_identity->sig_key = sig_key;
    new_identity->signature = signature;
    
    return new_identity;
}

wickr_ecdsa_result_t *wickr_identity_sign(const wickr_identity_t *identity, const wickr_crypto_engine_t *engine, const wickr_buffer_t *data)
{
    if (!identity || !identity->sig_key->pri_data || !engine || !data) {
        return NULL;
    }
    
    return engine->wickr_crypto_engine_ec_sign(identity->sig_key, data, DIGEST_SHA_512);
}

wickr_identity_t *wickr_node_identity_gen(const wickr_crypto_engine_t *engine, const wickr_identity_t *root_identity, const wickr_buffer_t *identifier)
{
    if (!engine || !root_identity || root_identity->type != IDENTITY_TYPE_ROOT) {
        return NULL;
    }
    
    wickr_ec_key_t *node_sig_key = engine->wickr_crypto_engine_ec_rand_key(engine->default_curve);
    
    if (!node_sig_key) {
        return NULL;
    }
    
    wickr_ecdsa_result_t *node_sig = wickr_identity_sign(root_identity, engine, node_sig_key->pub_data);
    
    if (!node_sig) {
        wickr_ec_key_destroy(&node_sig_key);
        return NULL;
    }
    
    wickr_buffer_t *node_identifier = identifier ? wickr_buffer_copy(identifier) : engine->wickr_crypto_engine_crypto_random(IDENTIFIER_LEN);
    
    if (!node_identifier) {
        wickr_ec_key_destroy(&node_sig_key);
        wickr_ecdsa_result_destroy(&node_sig);
        return NULL;
    }
    
    wickr_identity_t *node_identity = wickr_identity_create(IDENTITY_TYPE_NODE, node_identifier, node_sig_key, node_sig);
    
    if (!node_identity) {
        wickr_ec_key_destroy(&node_sig_key);
        wickr_ecdsa_result_destroy(&node_sig);
        wickr_buffer_destroy(&node_identifier);
    }
    
    return node_identity;
}

wickr_identity_t *wickr_identity_copy(const wickr_identity_t *source)
{
    if (!source) {
        return NULL;
    }
    
    wickr_buffer_t *identifier_copy = wickr_buffer_copy(source->identifier);
    
    if (!identifier_copy) {
        return NULL;
    }
    
    wickr_ec_key_t *sig_key_copy = wickr_ec_key_copy(source->sig_key);
    
    if (!sig_key_copy) {
        wickr_buffer_destroy(&identifier_copy);
        return NULL;
    }
    
    wickr_ecdsa_result_t *sig_copy = wickr_ecdsa_result_copy(source->signature);
    
    wickr_identity_t *copy = wickr_identity_create(source->type, identifier_copy, sig_key_copy, sig_copy);
    
    if (!copy) {
        wickr_buffer_destroy(&identifier_copy);
        wickr_ec_key_destroy(&sig_key_copy);
        wickr_ecdsa_result_destroy(&sig_copy);
    }
    
    return copy;
}

void wickr_identity_destroy(wickr_identity_t **identity)
{
    if (!identity || !*identity) {
        return;
    }
    
    wickr_buffer_destroy(&(*identity)->identifier);
    wickr_ec_key_destroy(&(*identity)->sig_key);
    wickr_ecdsa_result_destroy(&(*identity)->signature);
    wickr_free(*identity);
    *identity = NULL;
}

wickr_buffer_t *wickr_identity_serialize(const wickr_identity_t *identity)
{
    if (!identity) {
        return NULL;
    }
    
    Wickr__Proto__Identity *proto_identity = wickr_identity_to_proto(identity);
    
    if (!proto_identity) {
        return NULL;
    }
    
    size_t packed_size = wickr__proto__identity__get_packed_size(proto_identity);
    
    wickr_buffer_t *packed_buffer = wickr_buffer_create_empty(packed_size);
    
    if (!packed_buffer) {
        wickr_identity_proto_free(proto_identity);
        return NULL;
    }
    
    wickr__proto__identity__pack(proto_identity, packed_buffer->bytes);
    wickr_identity_proto_free(proto_identity);
    
    return packed_buffer;
}

wickr_identity_t *wickr_identity_create_from_buffer(const wickr_buffer_t *buffer, const wickr_crypto_engine_t *engine)
{
    if (!buffer) {
        return NULL;
    }
    
    Wickr__Proto__Identity *proto_identity = wickr__proto__identity__unpack(NULL, buffer->length, buffer->bytes);
    
    if (!proto_identity) {
        return NULL;
    }
    
    wickr_identity_t *return_identity = wickr_identity_create_from_proto(proto_identity, engine);
    wickr__proto__identity__free_unpacked(proto_identity, NULL);
    
    return return_identity;
}

wickr_fingerprint_t *wickr_identity_get_fingerprint(const wickr_identity_t *identity,
                                                    wickr_crypto_engine_t engine)
{
    if (!identity) {
        return NULL;
    }
    
    return wickr_fingerprint_gen(engine, identity->sig_key,
                                 identity->identifier, WICKR_FINGERPRINT_TYPE_SHA512);
}

wickr_fingerprint_t *wickr_identity_get_bilateral_fingerprint(const wickr_identity_t *identity,
                                                              const wickr_identity_t *remote_identity,
                                                              wickr_crypto_engine_t engine)
{
    if (!identity || !remote_identity) {
        return NULL;
    }
    
    wickr_fingerprint_t *local_fingerprint = wickr_fingerprint_gen(engine,
                                                                   identity->sig_key,
                                                                   identity->identifier,
                                                                   WICKR_FINGERPRINT_TYPE_SHA512);
    
    if (!local_fingerprint) {
        return NULL;
    }
    
    wickr_fingerprint_t *remote_fingerprint = wickr_fingerprint_gen(engine,
                                                                    remote_identity->sig_key,
                                                                    remote_identity->identifier,
                                                                    WICKR_FINGERPRINT_TYPE_SHA512);
    
    if (!remote_fingerprint) {
        wickr_fingerprint_destroy(&local_fingerprint);
        return NULL;
    }
    
    wickr_fingerprint_t *merged_fingerprint = wickr_fingerprint_gen_bilateral(engine,
                                                                              local_fingerprint,
                                                                              remote_fingerprint,
                                                                              WICKR_FINGERPRINT_TYPE_SHA512);
    
    wickr_fingerprint_destroy(&remote_fingerprint);
    wickr_fingerprint_destroy(&local_fingerprint);
    
    return merged_fingerprint;
}

wickr_identity_chain_t *wickr_identity_chain_create(wickr_identity_t *root, wickr_identity_t *node)
{
    if (!root || !node) {
        return NULL;
    }
    
    wickr_identity_chain_t *new_chain = wickr_alloc_zero(sizeof(wickr_identity_chain_t));
    
    if (!new_chain) {
        return NULL;
    }
    
    new_chain->root = root;
    new_chain->node = node;
    new_chain->status = IDENTITY_CHAIN_STATUS_UNKNOWN;
    
    return new_chain;
}

wickr_identity_chain_t *wickr_identity_chain_copy(const wickr_identity_chain_t *source)
{
    if (!source) {
        return NULL;
    }
    
    wickr_identity_t *root_copy = wickr_identity_copy(source->root);
    
    if (!root_copy) {
        return NULL;
    }
    
    wickr_identity_t *node_copy = wickr_identity_copy(source->node);
    
    if (!node_copy) {
        wickr_identity_destroy(&root_copy);
        return NULL;
    }
    
    wickr_buffer_t *cache_copy = wickr_buffer_copy(source->_status_cache);
    
    if (source->_status_cache && !cache_copy) {
        wickr_identity_destroy(&root_copy);
        wickr_identity_destroy(&node_copy);
        return NULL;
    }
    
    wickr_identity_chain_t *copy = wickr_identity_chain_create(root_copy, node_copy);
    
    if (!copy) {
        wickr_identity_destroy(&root_copy);
        wickr_identity_destroy(&node_copy);
        wickr_buffer_destroy(&cache_copy);
        return NULL;
    }
    
    copy->_status_cache = cache_copy;
    copy->status = source->status;
    
    return copy;
}

bool wickr_identity_chain_validate(wickr_identity_chain_t *chain, const wickr_crypto_engine_t *engine)
{
    if (!chain || !engine) {
        return false;
    }
    
    /* Check to see if we need to recalculate the status of the chain */
    if (wickr_identity_chain_has_valid_cache(chain, engine)) {
        return chain->status == IDENTITY_CHAIN_STATUS_VALID;
    }
    
    bool is_valid = engine->wickr_crypto_engine_ec_verify(chain->node->signature, chain->root->sig_key, chain->node->sig_key->pub_data);
    
    chain->status = is_valid ? IDENTITY_CHAIN_STATUS_VALID : IDENTITY_CHAIN_STATUS_INVALID;
    wickr_identity_chain_update_status_cache(chain, engine);
    
    return is_valid;
}

static wickr_buffer_t *__wickr_identity_chain_serialize(const wickr_identity_chain_t *identity_chain, bool include_private)
{
    if (!identity_chain) {
        return NULL;
    }
    
    Wickr__Proto__IdentityChain *proto_identity = NULL;
    
    if (include_private) {
        proto_identity = wickr_identity_chain_to_private_proto(identity_chain);
    } else {
        proto_identity = wickr_identity_chain_to_proto(identity_chain);
    }
    
    if (!proto_identity) {
        return NULL;
    }
    
    size_t packed_size = wickr__proto__identity_chain__get_packed_size(proto_identity);
    
    wickr_buffer_t *packed_buffer = wickr_buffer_create_empty(packed_size);
    
    if (!packed_buffer) {
        wickr_identity_chain_proto_free(proto_identity);
        return NULL;
    }
    
    wickr__proto__identity_chain__pack(proto_identity, packed_buffer->bytes);
    wickr_identity_chain_proto_free(proto_identity);
    
    return packed_buffer;
}

wickr_buffer_t *wickr_identity_chain_serialize(const wickr_identity_chain_t *identity_chain)
{
    return __wickr_identity_chain_serialize(identity_chain, false);
}

wickr_buffer_t *wickr_identity_chain_serialize_private(const wickr_identity_chain_t *identity_chain)
{
    return __wickr_identity_chain_serialize(identity_chain, true);
}

wickr_identity_chain_t *wickr_identity_chain_create_from_buffer(const wickr_buffer_t *buffer, const wickr_crypto_engine_t *engine)
{
    if (!buffer) {
        return NULL;
    }
    
    Wickr__Proto__IdentityChain *proto_identity = wickr__proto__identity_chain__unpack(NULL, buffer->length, buffer->bytes);
    
    if (!proto_identity) {
        return NULL;
    }
    
    wickr_identity_chain_t *return_chain = wickr_identity_chain_create_from_proto(proto_identity, engine);
    wickr__proto__identity_chain__free_unpacked(proto_identity, NULL);
    
    return return_chain;
}

void wickr_identity_chain_destroy(wickr_identity_chain_t **chain)
{
    if (!chain || !*chain) {
        return;
    }
    
    wickr_identity_destroy(&(*chain)->node);
    wickr_identity_destroy(&(*chain)->root);
    wickr_buffer_destroy(&(*chain)->_status_cache);
    
    wickr_free(*chain);
    *chain = NULL;
}
