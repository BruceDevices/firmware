/**
 * @file llm_engine.h
 * @brief Minimal on-device inference engine for llama2.c-format checkpoints.
 *
 * Supports any model exported by karpathy/llama2.c's export.py in either
 * version 1 (fp32) or version 2 (int8 / Q8_0 symmetric quantized) format.
 * Model dimensions (dim, n_layers, n_heads, n_kv_heads, vocab_size, seq_len)
 * are read from the checkpoint header at load time.
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
    int32_t vocab_size;
    int32_t seq_len;
};

enum class LLMLoadError {
    None,
    CheckpointNotFound,
    TokenizerNotFound,
    BadMagicOrVersion,
    ConfigTooLarge,
    OutOfMemory,
    IncompatibleGroupSize,
};

// Called once per generated token. Return false to cancel generation.
using TokenCallback = std::function<bool(const String &piece)>;

struct GenerationParams {
    float temperature = 0.8f;
    float topP = 0.9f;
    float repetitionPenalty = 1.0f;
    uint32_t seed = 0;

    bool chatTemplateEnabled = true;
    String userTag = "<user>: ";
    String botTag = "<bot>:";
};

class LLMEngine {
public:
    LLMEngine();
    ~LLMEngine();

    LLMLoadError load(
        FS &fs, const String &checkpointPath, const String &tokenizerPath, bool overrideSafetyChecks = false
    );

    void unload();
    bool isLoaded() const { return loaded; }

    const LLMConfig &config() const { return cfg; }

    void generate(
        const String &prompt, int maxTokens, const GenerationParams &params, const TokenCallback &onToken
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    LLMConfig cfg{};
    bool loaded = false;
};

} // namespace bruce_llm
