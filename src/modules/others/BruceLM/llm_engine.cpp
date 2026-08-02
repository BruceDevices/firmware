/**
 * @file llm_engine.cpp
 * @brief Forward pass for llama2.c-format checkpoints, ported for ESP32-S3.
 */
#include "llm_engine.h"

#include <algorithm>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace bruce_llm {

namespace {

constexpr uint32_t kMagic = 0x616b3432; // "ak42"

void *psram_alloc(size_t n) { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }

struct QuantizedTensor {
    int8_t *q = nullptr;
    float *s = nullptr;
};

struct TokenIndex {
    const char *str;
    int id;
};

int strLookup(const char *str, const std::vector<TokenIndex> &sorted) {
    auto it = std::lower_bound(sorted.begin(), sorted.end(), str, [](const TokenIndex &a, const char *s) {
        return strcmp(a.str, s) < 0;
    });
    if (it != sorted.end() && strcmp(it->str, str) == 0) return it->id;
    return -1;
}

float *readF32Vec(File &f, size_t n) {
    float *buf = (float *)psram_alloc(n * sizeof(float));
    if (!buf) return nullptr;
    f.read((uint8_t *)buf, n * sizeof(float));
    return buf;
}

bool readQuantized(File &f, QuantizedTensor &t, size_t sizePerUnit, int units, int groupSize) {
    size_t groupsPerUnit = sizePerUnit / groupSize;
    size_t totalSize = sizePerUnit * (size_t)units;
    size_t totalGroups = groupsPerUnit * (size_t)units;
    t.q = (int8_t *)psram_alloc(totalSize);
    t.s = (float *)psram_alloc(totalGroups * sizeof(float));
    if (!t.q || !t.s) return false;
    for (int i = 0; i < units; i++) {
        f.read((uint8_t *)(t.q + (size_t)i * sizePerUnit), sizePerUnit);
        f.read((uint8_t *)(t.s + (size_t)i * groupsPerUnit), groupsPerUnit * sizeof(float));
    }
    return true;
}

void dequantize(const QuantizedTensor &t, float *out, size_t n, int groupSize) {
    for (size_t i = 0; i < n; i++) out[i] = t.q[i] * t.s[i / groupSize];
}

QuantizedTensor layerView(const QuantizedTensor &t, size_t elemOffset, int groupSize) {
    QuantizedTensor v;
    v.q = t.q + elemOffset;
    v.s = t.s + elemOffset / groupSize;
    return v;
}

void quantize(QuantizedTensor &t, const float *x, size_t n, int groupSize) {
    size_t nGroups = n / groupSize;
    for (size_t g = 0; g < nGroups; g++) {
        float wmax = 0.0f;
        for (int i = 0; i < groupSize; i++) {
            float v = fabsf(x[g * groupSize + i]);
            if (v > wmax) wmax = v;
        }
        float scale = wmax / 127.0f;
        t.s[g] = scale;
        for (int i = 0; i < groupSize; i++) {
            float v = x[g * groupSize + i] / (scale == 0 ? 1.0f : scale);
            t.q[g * groupSize + i] = (int8_t)roundf(v);
        }
    }
}

void matmulQ(float *out, const float *x, const QuantizedTensor &w, int n, int d, int groupSize) {
    QuantizedTensor xq;
    xq.q = (int8_t *)alloca(n);
    xq.s = (float *)alloca((n / groupSize) * sizeof(float));
    quantize(xq, x, n, groupSize);

    for (int i = 0; i < d; i++) {
        float val = 0.0f;
        int32_t ival = 0;
        int in = i * n;
        for (int j = 0; j <= n - groupSize; j += groupSize) {
            for (int k = 0; k < groupSize; k++) ival += (int32_t)xq.q[j + k] * (int32_t)w.q[in + j + k];
            val += ((float)ival) * w.s[(in + j) / groupSize] * xq.s[j / groupSize];
            ival = 0;
        }
        out[i] = val;
    }
}

void matmulF(float *out, const float *x, const float *w, int n, int d) {
    for (int i = 0; i < d; i++) {
        float val = 0.0f;
        const float *row = w + i * n;
        for (int j = 0; j < n; j++) val += row[j] * x[j];
        out[i] = val;
    }
}

void rmsnorm(float *out, const float *x, const float *weight, int size) {
    float ss = 0.0f;
    for (int i = 0; i < size; i++) ss += x[i] * x[i];
    ss = 1.0f / sqrtf(ss / size + 1e-5f);
    for (int i = 0; i < size; i++) out[i] = weight[i] * (x[i] * ss);
}

void softmax(float *x, int size) {
    float maxv = x[0];
    for (int i = 1; i < size; i++)
        if (x[i] > maxv) maxv = x[i];
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        x[i] = expf(x[i] - maxv);
        sum += x[i];
    }
    for (int i = 0; i < size; i++) x[i] /= sum;
}

} // namespace

struct LLMEngine::Impl {
    bool quantized = false;
    bool legacy = false;
    int groupSize = 0;
    bool sharedClassifier = true;

    float *tokEmbF = nullptr;
    float *rmsAttW = nullptr, *rmsFfnW = nullptr, *rmsFinalW = nullptr;
    float *wqF = nullptr, *wkF = nullptr, *wvF = nullptr, *woF = nullptr;
    float *w1F = nullptr, *w2F = nullptr, *w3F = nullptr;
    float *wclsF = nullptr;

    QuantizedTensor tokEmbQ;
    QuantizedTensor wqQ, wkQ, wvQ, woQ;
    QuantizedTensor w1Q, w2Q, w3Q;
    QuantizedTensor wclsQ;
    float *tokEmbDequant = nullptr;

    float *x = nullptr, *xb = nullptr, *xb2 = nullptr;
    float *hb = nullptr, *hb2 = nullptr;
    float *q = nullptr, *att = nullptr, *logits = nullptr;
    float *keyCache = nullptr, *valCache = nullptr;

    std::unique_ptr<char *[]> vocab;
    std::unique_ptr<float[]> vocabScores;
    int vocabSize = 0;
    std::vector<TokenIndex> sortedVocab;

    ~Impl() { freeAll(); }

    void freeAll() {
        auto f = [](void *p) {
            if (p) heap_caps_free(p);
        };
        f(tokEmbF);
        f(rmsAttW);
        f(rmsFfnW);
        f(rmsFinalW);
        f(wqF);
        f(wkF);
        f(wvF);
        f(woF);
        f(w1F);
        f(w2F);
        f(w3F);
        if (wclsF != tokEmbF) f(wclsF);
        f(tokEmbQ.q);
        f(tokEmbQ.s);
        f(wqQ.q);
        f(wqQ.s);
        f(wkQ.q);
        f(wkQ.s);
        f(wvQ.q);
        f(wvQ.s);
        f(woQ.q);
        f(woQ.s);
        f(w1Q.q);
        f(w1Q.s);
        f(w2Q.q);
        f(w2Q.s);
        f(w3Q.q);
        f(w3Q.s);
        f(wclsQ.q);
        f(wclsQ.s);
        f(tokEmbDequant);
        f(x);
        f(xb);
        f(xb2);
        f(hb);
        f(hb2);
        f(q);
        f(att);
        f(logits);
        f(keyCache);
        f(valCache);
        if (vocab) {
            for (int i = 0; i < vocabSize; i++)
                if (vocab[i]) free(vocab[i]);
        }
    }
};

LLMEngine::LLMEngine() : impl(std::make_unique<Impl>()) {}
LLMEngine::~LLMEngine() = default;

LLMLoadError LLMEngine::load(
    FS &fs, const String &checkpointPath, const String &tokenizerPath, bool overrideSafetyChecks
) {
    unload();
    impl = std::make_unique<Impl>();

    if (!fs.exists(checkpointPath)) return LLMLoadError::CheckpointNotFound;
    if (!fs.exists(tokenizerPath)) return LLMLoadError::TokenizerNotFound;

    File cf = fs.open(checkpointPath, FILE_READ);
    if (!cf) return LLMLoadError::CheckpointNotFound;

    uint32_t magic = 0;
    cf.read((uint8_t *)&magic, 4);
    bool legacy = (magic != kMagic);

    int32_t version = 0;
    uint8_t sharedClassifierByte = 1;
    int32_t groupSize = 0;

    if (legacy) {
        cf.seek(0, SeekSet);
        LLMConfig c{};
        cf.read((uint8_t *)&c, sizeof(LLMConfig));
        sharedClassifierByte = (c.vocab_size > 0) ? 1 : 0;
        c.vocab_size = c.vocab_size > 0 ? c.vocab_size : -c.vocab_size;
        cfg = c;
    } else {
        cf.read((uint8_t *)&version, 4);
        if (version != 1 && version != 2) {
            cf.close();
            return LLMLoadError::BadMagicOrVersion;
        }

        LLMConfig c{};
        cf.read((uint8_t *)&c, sizeof(LLMConfig));
        cfg = c;

        cf.read((uint8_t *)&sharedClassifierByte, 1);
        if (version == 2) cf.read((uint8_t *)&groupSize, 4);
        cf.seek(256, SeekSet);
    }

    impl->legacy = legacy;
    impl->quantized = (version == 2);
    impl->groupSize = groupSize;
    impl->sharedClassifier = sharedClassifierByte != 0;

    int dim = cfg.dim, hidden = cfg.hidden_dim, layers = cfg.n_layers;
    int heads = cfg.n_heads, kvHeads = cfg.n_kv_heads, vocab = cfg.vocab_size, seqLen = cfg.seq_len;
    int headSize = dim / heads;
    int kvDim = (dim * kvHeads) / heads;

    if (!overrideSafetyChecks && impl->quantized &&
        (dim % groupSize != 0 || kvDim % groupSize != 0 || hidden % groupSize != 0)) {
        cf.close();
        return LLMLoadError::IncompatibleGroupSize;
    }

    size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t approxBytes = (size_t)vocab * dim * (impl->quantized ? 1 : 4) +
                         (size_t)layers * dim * dim * 4 * (impl->quantized ? 1 : 4) +
                         (size_t)layers * dim * hidden * 3 * (impl->quantized ? 1 : 4);
    if (!overrideSafetyChecks && approxBytes > psramFree * 0.85) {
        cf.close();
        return LLMLoadError::ConfigTooLarge;
    }

    if (!impl->quantized && impl->legacy) {
        impl->tokEmbF = readF32Vec(cf, (size_t)vocab * dim);
        impl->rmsAttW = readF32Vec(cf, (size_t)layers * dim);
        impl->wqF = readF32Vec(cf, (size_t)layers * dim * (heads * headSize));
        impl->wkF = readF32Vec(cf, (size_t)layers * dim * (kvHeads * headSize));
        impl->wvF = readF32Vec(cf, (size_t)layers * dim * (kvHeads * headSize));
        impl->woF = readF32Vec(cf, (size_t)layers * (heads * headSize) * dim);
        impl->rmsFfnW = readF32Vec(cf, (size_t)layers * dim);
        impl->w1F = readF32Vec(cf, (size_t)layers * dim * hidden);
        impl->w2F = readF32Vec(cf, (size_t)layers * hidden * dim);
        impl->w3F = readF32Vec(cf, (size_t)layers * dim * hidden);
        impl->rmsFinalW = readF32Vec(cf, dim);
        cf.seek((size_t)seqLen * (headSize / 2) * sizeof(float) * 2, SeekCur);
        impl->wclsF = impl->sharedClassifier ? impl->tokEmbF : readF32Vec(cf, (size_t)vocab * dim);
    } else if (!impl->quantized) {
        impl->rmsAttW = readF32Vec(cf, (size_t)layers * dim);
        impl->rmsFfnW = readF32Vec(cf, (size_t)layers * dim);
        impl->rmsFinalW = readF32Vec(cf, dim);
        impl->tokEmbF = readF32Vec(cf, (size_t)vocab * dim);
        impl->wqF = readF32Vec(cf, (size_t)layers * dim * (heads * headSize));
        impl->wkF = readF32Vec(cf, (size_t)layers * dim * (kvHeads * headSize));
        impl->wvF = readF32Vec(cf, (size_t)layers * dim * (kvHeads * headSize));
        impl->woF = readF32Vec(cf, (size_t)layers * (heads * headSize) * dim);
        impl->w1F = readF32Vec(cf, (size_t)layers * dim * hidden);
        impl->w2F = readF32Vec(cf, (size_t)layers * hidden * dim);
        impl->w3F = readF32Vec(cf, (size_t)layers * dim * hidden);
        impl->wclsF = impl->sharedClassifier ? impl->tokEmbF : readF32Vec(cf, (size_t)vocab * dim);
    } else {
        int gs = groupSize;
        impl->rmsAttW = readF32Vec(cf, (size_t)layers * dim);
        impl->rmsFfnW = readF32Vec(cf, (size_t)layers * dim);
        impl->rmsFinalW = readF32Vec(cf, dim);

        readQuantized(cf, impl->tokEmbQ, (size_t)vocab * dim, 1, gs);
        readQuantized(cf, impl->wqQ, (size_t)dim * (heads * headSize), layers, gs);
        readQuantized(cf, impl->wkQ, (size_t)dim * (kvHeads * headSize), layers, gs);
        readQuantized(cf, impl->wvQ, (size_t)dim * (kvHeads * headSize), layers, gs);
        readQuantized(cf, impl->woQ, (size_t)(heads * headSize) * dim, layers, gs);
        readQuantized(cf, impl->w1Q, (size_t)dim * hidden, layers, gs);
        readQuantized(cf, impl->w2Q, (size_t)hidden * dim, layers, gs);
        readQuantized(cf, impl->w3Q, (size_t)dim * hidden, layers, gs);
        if (!impl->sharedClassifier) readQuantized(cf, impl->wclsQ, (size_t)vocab * dim, 1, gs);

        impl->tokEmbDequant = (float *)psram_alloc((size_t)vocab * dim * sizeof(float));
        if (impl->tokEmbDequant) dequantize(impl->tokEmbQ, impl->tokEmbDequant, (size_t)vocab * dim, gs);
    }
    cf.close();

    File tf = fs.open(tokenizerPath, FILE_READ);
    if (!tf) return LLMLoadError::TokenizerNotFound;
    int32_t maxTokLen = 0;
    tf.read((uint8_t *)&maxTokLen, 4);
    impl->vocabSize = vocab;
    impl->vocab = std::make_unique<char *[]>(vocab);
    impl->vocabScores = std::make_unique<float[]>(vocab);
    for (int i = 0; i < vocab; i++) {
        tf.read((uint8_t *)&impl->vocabScores[i], 4);
        int32_t len = 0;
        tf.read((uint8_t *)&len, 4);
        char *s = (char *)malloc(len + 1);
        tf.read((uint8_t *)s, len);
        s[len] = '\0';
        impl->vocab[i] = s;
    }
    tf.close();

    impl->sortedVocab.resize(vocab);
    for (int i = 0; i < vocab; i++) impl->sortedVocab[i] = {impl->vocab[i], i};
    std::sort(
        impl->sortedVocab.begin(), impl->sortedVocab.end(), [](const TokenIndex &a, const TokenIndex &b) {
            return strcmp(a.str, b.str) < 0;
        }
    );

    impl->x = (float *)psram_alloc(dim * sizeof(float));
    impl->xb = (float *)psram_alloc(dim * sizeof(float));
    impl->xb2 = (float *)psram_alloc(dim * sizeof(float));
    impl->hb = (float *)psram_alloc(hidden * sizeof(float));
    impl->hb2 = (float *)psram_alloc(hidden * sizeof(float));
    impl->q = (float *)psram_alloc(dim * sizeof(float));
    impl->att = (float *)psram_alloc(heads * seqLen * sizeof(float));
    impl->logits = (float *)psram_alloc(vocab * sizeof(float));
    impl->keyCache = (float *)psram_alloc((size_t)layers * seqLen * kvDim * sizeof(float));
    impl->valCache = (float *)psram_alloc((size_t)layers * seqLen * kvDim * sizeof(float));

    if (!impl->x || !impl->xb || !impl->xb2 || !impl->hb || !impl->hb2 || !impl->q || !impl->att ||
        !impl->logits || !impl->keyCache || !impl->valCache) {
        return LLMLoadError::OutOfMemory;
    }

    loaded = true;
    return LLMLoadError::None;
}

void LLMEngine::unload() {
    impl = std::make_unique<Impl>();
    loaded = false;
}

namespace {

int sampleArgmax(const float *probabilities, int n) {
    int best = 0;
    for (int i = 1; i < n; i++)
        if (probabilities[i] > probabilities[best]) best = i;
    return best;
}

int sampleMult(const float *probabilities, int n, float coin) {
    float cdf = 0.0f;
    for (int i = 0; i < n; i++) {
        cdf += probabilities[i];
        if (coin < cdf) return i;
    }
    return n - 1;
}

int sampleTopp(const float *probabilities, int n, float topP, float coin) {
    std::vector<std::pair<float, int>> probIndex;
    probIndex.reserve(n);
    float cutoff = (1.0f - topP) / (n - 1);
    for (int i = 0; i < n; i++)
        if (probabilities[i] >= cutoff) probIndex.push_back({probabilities[i], i});
    std::sort(probIndex.begin(), probIndex.end(), [](auto &a, auto &b) { return a.first > b.first; });

    float cumulative = 0.0f;
    int lastIdx = (int)probIndex.size() - 1;
    for (size_t i = 0; i < probIndex.size(); i++) {
        cumulative += probIndex[i].first;
        if (cumulative > topP) {
            lastIdx = (int)i;
            break;
        }
    }

    float r = coin * cumulative;
    float cdf = 0.0f;
    for (int i = 0; i <= lastIdx; i++) {
        cdf += probIndex[i].first;
        if (r < cdf) return probIndex[i].second;
    }
    return probIndex[lastIdx].second;
}

uint32_t randomU32(uint64_t &state) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return (uint32_t)((state * 0x2545F4914F6CDD1Dull) >> 32);
}
float randomF32(uint64_t &state) { return (randomU32(state) >> 8) / 16777216.0f; }

int sampleToken(float *logits, int n, float temperature, float topP, uint64_t &rngState) {
    if (temperature <= 0.0f) return sampleArgmax(logits, n);
    for (int i = 0; i < n; i++) logits[i] /= temperature;
    softmax(logits, n);
    float coin = randomF32(rngState);
    if (topP <= 0.0f || topP >= 1.0f) return sampleMult(logits, n, coin);
    return sampleTopp(logits, n, topP, coin);
}

void applyRepetitionPenalty(float *logits, const std::vector<int> &history, float penalty) {
    if (penalty == 1.0f) return;
    for (int id : history) {
        if (logits[id] > 0) logits[id] /= penalty;
        else logits[id] *= penalty;
    }
}

String decodePiece(const char *piece) {
    unsigned int byteVal;
    if (piece[0] == '<' && sscanf(piece, "<0x%02X>", &byteVal) == 1) {
        char c = (char)byteVal;
        return String(c);
    }
    return String(piece);
}

void encodeBpe(
    const String &text, char **vocab, const float *vocabScores, const std::vector<TokenIndex> &sortedVocab,
    int *outIds, int &outCount
) {
    outCount = 0;
    outIds[outCount++] = 1;
    if (text.length() == 0) return;
    outIds[outCount++] = strLookup(" ", sortedVocab);

    char strBuffer[5];
    size_t strLen = 0;
    const char *text_c = text.c_str();

    for (const char *c = text_c; *c != '\0'; c++) {
        if ((*c & 0xC0) != 0x80) strLen = 0;
        strBuffer[strLen++] = *c;
        strBuffer[strLen] = '\0';

        if ((unsigned char)(*(c + 1)) != 0 && (*(c + 1) & 0xC0) == 0x80 && strLen < 4) continue;

        int id = strLookup(strBuffer, sortedVocab);
        if (id != -1) {
            outIds[outCount++] = id;
        } else {
            for (size_t i = 0; i < strLen; i++) outIds[outCount++] = (unsigned char)strBuffer[i] + 3;
        }
        strLen = 0;
    }

    char mergeBuf[256];
    for (;;) {
        float bestScore = -1e10f;
        int bestId = -1, bestIdx = -1;
        for (int i = 0; i < outCount - 1; i++) {
            snprintf(mergeBuf, sizeof(mergeBuf), "%s%s", vocab[outIds[i]], vocab[outIds[i + 1]]);
            int id = strLookup(mergeBuf, sortedVocab);
            if (id != -1 && vocabScores[id] > bestScore) {
                bestScore = vocabScores[id];
                bestId = id;
                bestIdx = i;
            }
        }
        if (bestIdx == -1) break;
        outIds[bestIdx] = bestId;
        for (int i = bestIdx + 1; i < outCount - 1; i++) outIds[i] = outIds[i + 1];
        outCount--;
    }
}

} // namespace

void LLMEngine::generate(
    const String &prompt, int maxTokens, const GenerationParams &params, const TokenCallback &onToken
) {
    if (!loaded) return;
    Impl *m = impl.get();
    uint64_t rngState =
        params.seed != 0 ? (uint64_t)params.seed : (((uint64_t)esp_random() << 32) | esp_random());
    std::vector<int> history;
    int dim = cfg.dim, hidden = cfg.hidden_dim, layers = cfg.n_layers;
    int heads = cfg.n_heads, kvHeads = cfg.n_kv_heads, vocab = cfg.vocab_size, seqLen = cfg.seq_len;
    int headSize = dim / heads;
    int kvDim = (dim * kvHeads) / heads;
    int kvMul = heads / kvHeads;
    int gs = m->groupSize;

    String effectivePrompt =
        params.chatTemplateEnabled ? (params.userTag + prompt + "\n" + params.botTag) : prompt;

    int *promptIds = (int *)alloca(sizeof(int) * (effectivePrompt.length() + 2));
    int nPrompt = 0;
    encodeBpe(effectivePrompt, m->vocab.get(), m->vocabScores.get(), m->sortedVocab, promptIds, nPrompt);

    int steps = maxTokens < seqLen ? maxTokens : seqLen;
    int token = promptIds[0];
    String tailBuffer;

    for (int pos = 0; pos < steps; pos++) {
        memcpy(
            m->x, (m->quantized ? m->tokEmbDequant : m->tokEmbF) + (size_t)token * dim, dim * sizeof(float)
        );

        for (int l = 0; l < layers; l++) {
            rmsnorm(m->xb, m->x, m->rmsAttW + l * dim, dim);

            float *kRow = m->keyCache + ((size_t)l * seqLen + pos) * kvDim;
            float *vRow = m->valCache + ((size_t)l * seqLen + pos) * kvDim;

            if (!m->quantized) {
                matmulF(m->q, m->xb, m->wqF + (size_t)l * dim * dim, dim, dim);
                matmulF(kRow, m->xb, m->wkF + (size_t)l * dim * kvDim, dim, kvDim);
                matmulF(vRow, m->xb, m->wvF + (size_t)l * dim * kvDim, dim, kvDim);
            } else {
                matmulQ(m->q, m->xb, layerView(m->wqQ, (size_t)l * dim * dim, gs), dim, dim, gs);
                matmulQ(kRow, m->xb, layerView(m->wkQ, (size_t)l * dim * kvDim, gs), dim, kvDim, gs);
                matmulQ(vRow, m->xb, layerView(m->wvQ, (size_t)l * dim * kvDim, gs), dim, kvDim, gs);
            }

            for (int h = 0; h < heads; h++) {
                for (int i = 0; i < headSize; i += 2) {
                    float freq = 1.0f / powf(10000.0f, (float)i / headSize);
                    float val = pos * freq;
                    float fcr = cosf(val), fci = sinf(val);
                    int base = h * headSize;
                    if (h < kvHeads) {
                        float v0 = kRow[base + i], v1 = kRow[base + i + 1];
                        kRow[base + i] = v0 * fcr - v1 * fci;
                        kRow[base + i + 1] = v0 * fci + v1 * fcr;
                    }
                    float v0 = m->q[base + i], v1 = m->q[base + i + 1];
                    m->q[base + i] = v0 * fcr - v1 * fci;
                    m->q[base + i + 1] = v0 * fci + v1 * fcr;
                }
            }

            for (int h = 0; h < heads; h++) {
                float *qh = m->q + h * headSize;
                float *attRow = m->att + h * seqLen;
                for (int t = 0; t <= pos; t++) {
                    float *kt = m->keyCache + ((size_t)l * seqLen + t) * kvDim + (h / kvMul) * headSize;
                    float score = 0.0f;
                    for (int i = 0; i < headSize; i++) score += qh[i] * kt[i];
                    attRow[t] = score / sqrtf((float)headSize);
                }
                softmax(attRow, pos + 1);
                float *out = m->xb2 + h * headSize;
                memset(out, 0, headSize * sizeof(float));
                for (int t = 0; t <= pos; t++) {
                    float *vt = m->valCache + ((size_t)l * seqLen + t) * kvDim + (h / kvMul) * headSize;
                    float a = attRow[t];
                    for (int i = 0; i < headSize; i++) out[i] += a * vt[i];
                }
            }

            if (!m->quantized) matmulF(m->xb, m->xb2, m->woF + (size_t)l * dim * dim, dim, dim);
            else matmulQ(m->xb, m->xb2, layerView(m->woQ, (size_t)l * dim * dim, gs), dim, dim, gs);

            for (int i = 0; i < dim; i++) m->x[i] += m->xb[i];

            rmsnorm(m->xb, m->x, m->rmsFfnW + l * dim, dim);
            if (!m->quantized) {
                matmulF(m->hb, m->xb, m->w1F + (size_t)l * dim * hidden, dim, hidden);
                matmulF(m->hb2, m->xb, m->w3F + (size_t)l * dim * hidden, dim, hidden);
            } else {
                matmulQ(m->hb, m->xb, layerView(m->w1Q, (size_t)l * dim * hidden, gs), dim, hidden, gs);
                matmulQ(m->hb2, m->xb, layerView(m->w3Q, (size_t)l * dim * hidden, gs), dim, hidden, gs);
            }
            for (int i = 0; i < hidden; i++) {
                float v = m->hb[i];
                v *= 1.0f / (1.0f + expf(-v));
                m->hb[i] = v * m->hb2[i];
            }
            if (!m->quantized) matmulF(m->xb, m->hb, m->w2F + (size_t)l * hidden * dim, hidden, dim);
            else matmulQ(m->xb, m->hb, layerView(m->w2Q, (size_t)l * hidden * dim, gs), hidden, dim, gs);

            for (int i = 0; i < dim; i++) m->x[i] += m->xb[i];
        }

        rmsnorm(m->x, m->x, m->rmsFinalW, dim);
        if (!m->quantized) matmulF(m->logits, m->x, m->wclsF, dim, vocab);
        else matmulQ(m->logits, m->x, m->sharedClassifier ? m->tokEmbQ : m->wclsQ, dim, vocab, gs);

        int nextToken;
        if (pos + 1 < nPrompt) {
            nextToken = promptIds[pos + 1];
        } else {
            applyRepetitionPenalty(m->logits, history, params.repetitionPenalty);
            nextToken = sampleToken(m->logits, vocab, params.temperature, params.topP, rngState);
        }

        if (pos >= nPrompt) {
            String piece = decodePiece(m->vocab[token]);
            if (params.chatTemplateEnabled && params.userTag.length() > 0) {
                String combined = tailBuffer + piece;
                int tagPos = combined.indexOf(params.userTag);
                if (tagPos != -1) {
                    int emitLen = tagPos - (int)tailBuffer.length();
                    if (emitLen > 0) onToken(piece.substring(0, emitLen));
                    return;
                }
                tailBuffer = combined;
                int maxKeep = (int)params.userTag.length() * 2 + 8;
                if ((int)tailBuffer.length() > maxKeep)
                    tailBuffer = tailBuffer.substring(tailBuffer.length() - maxKeep);
            }
            if (!onToken(piece)) return;
        }

        if (pos >= nPrompt - 1 && nextToken == 2) return;
        if (pos >= nPrompt - 1 && nextToken == 1) return;
        history.push_back(token);
        if (history.size() > 64) history.erase(history.begin());
        token = nextToken;
    }
}

} // namespace bruce_llm
