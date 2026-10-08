/**
 * @file llm_engine.h
 * @brief Minimal on-device inference engine for llama2.c-format checkpoints.
 *
 * Supports any model exported by karpathy/llama2.c's export.py in either
 * version 1 (fp32) or version 2 (int8 / Q8_0 symmetric quantized) format.
 * (version 0 is the legacy header-less format and is not supported - re-export.)
 * Model dimensions (dim, n_layers, n_heads, n_kv_heads, vocab_size, seq_len)
 * are read from the checkpoint header at load time - nothing is hardcoded,
 * so any model that fits in available PSRAM can be loaded.
 */
#pragma once

#include <FS.h>
#include <functional>
#include <memory>

namespace bruce_llm {

// Mirrors llama2.c's Config struct layout (7 x int32, little-endian).
struct LLMConfig {
    int32_t dim;
    int32_t hidden_dim;
    int32_t n_layers;
    int32_t n_heads;
    int32_t n_kv_heads;
    int32_t vocab_size; // negative => unshared classifier weights (fp32 export quirk)
    int32_t seq_len;
};

enum class LLMLoadError {
    None,
    CheckpointNotFound,
    TokenizerNotFound,
    BadMagicOrVersion,
    ConfigTooLarge, // wouldn't fit in available PSRAM
    OutOfMemory,
    // Quantized (v2) export flattens the whole tensor into group_size chunks
    // without regard to row boundaries; our matmul (like reference run.c's)
    // only handles this correctly when every row length is itself a whole
    // number of groups. Rejected here rather than silently corrupting output.
    IncompatibleGroupSize,
};

// Called once per generated token. Return false to cancel generation.
using TokenCallback = std::function<bool(const String &piece)>;

// Sampling knobs. temperature/topP/seed are faithful ports of run.c's own
// Sampler (temperature, nucleus/top-p sampling, seeded xorshift RNG).
// repetitionPenalty is NOT part of upstream llama2.c - it's a common,
// cheap addition (downweights recently-used tokens' logits) that measurably
// helps small models avoid repetition loops.
struct GenerationParams {
    float temperature = 0.8f;       // 0 = greedy/deterministic, higher = more random
    float topP = 0.9f;              // nucleus sampling threshold; >=1.0 or <=0 disables it
    float repetitionPenalty = 1.0f; // 1.0 = disabled, >1.0 discourages repeating recent tokens
    uint32_t seed = 0;              // 0 = reseed from hardware RNG each call (non-reproducible);
                                     // nonzero = deterministic output for the same prompt

    // Chat-finetuned checkpoints (unlike plain story models) are trained on
    // "<user>: ...\n<bot>: ..." pairs - fed raw text with no cue for whose
    // turn it is, they'll hallucinate a whole fake exchange (including their
    // own userTag) before ever answering. When enabled, the prompt is
    // wrapped as userTag + prompt + "\n" + botTag before encoding, and
    // generation stops the moment userTag reappears in the output (the model
    // drifting into a new fake turn) instead of rambling past it.
    bool chatTemplateEnabled = true;
    String userTag = "<user>: ";
    String botTag = "<bot>:";
};

class LLMEngine {
public:
    LLMEngine();
    ~LLMEngine();

    // Loads a checkpoint (.bin, llama2.c format) + matching tokenizer (.bin) from fs.
    // If overrideSafetyChecks is true, loads anyway even when the model looks
    // too large for available PSRAM or the quantized group_size doesn't
    // evenly divide every row length - the caller has warned the user that
    // this may fail or produce garbled output, instead of refusing outright.
    LLMLoadError load(
        FS &fs,
        const String &checkpointPath,
        const String &tokenizerPath,
        bool overrideSafetyChecks = false
    );

    void unload();
    bool isLoaded() const { return loaded; }

    const LLMConfig &config() const { return cfg; }

    // Runs generation from `prompt`, streaming pieces to `onToken`, up to maxTokens
    // (or the model's seq_len, whichever is smaller). Blocking call - designed
    // to be invoked from the UI loop with onToken also servicing UI/cancel checks.
    void generate(
        const String &prompt, int maxTokens, const GenerationParams &params,
        const TokenCallback &onToken
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    LLMConfig cfg{};
    bool loaded = false;
};

} // namespace bruce_llm
