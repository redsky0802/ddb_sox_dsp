#include <deadbeef/deadbeef.h>
#include <soxr.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==========================================================================
 * Foobar2000 SoX Mod 核心数学宏
 * ========================================================================== */
#define TO_3dB(a)       ((1.6e-6*(a)-7.5e-4)*(a)+.646)
#define linear_to_dB(x) (log10(x) * 20.0)

static DB_functions_t *deadbeef;
static DB_dsp_t plugin;

/* ==========================================================================
 * 插件上下文结构体
 * ========================================================================== */
typedef struct {
    ddb_dsp_context_t ctx; 
    soxr_t soxr;
    
    /* 参数数组 (严格对齐 UI 的 6 个控件)
     * [0] 44.1k 基准目标频率
     * [1] 48k 基准目标频率
     * [2] 阻带衰减质量 (Quality)
     * [3] 通带截止点 (Passband -3dB)
     * [4] 相位响应 (Phase Response)
     * [5] 允许混叠 (Allow Aliasing)
     */
    int params[6]; 
    
    int current_in_rate;
    int current_target_rate;
    int channels;
    
    /* FIFO 环形缓冲 */
    float *fifo;
    size_t fifo_cap;
    size_t fifo_len;
    
    /* 输出缓冲 */
    float *out_buf;
    size_t out_cap;
} ddb_soxr_t;

/* ==========================================================================
 * DSP 生命周期管理
 * ========================================================================== */
static ddb_dsp_context_t *ddb_soxr_open(void) {
    ddb_soxr_t *soxr = calloc(1, sizeof(ddb_soxr_t));
    if (soxr) {
        soxr->ctx.plugin = &plugin;
        soxr->params[0] = 2;   // 44.1k 族系默认: 176400 (4x)
        soxr->params[1] = 2;   // 48k   族系默认: 192000 (4x)
        soxr->params[2] = 3;   // Quality: Ultra_168dB
        soxr->params[3] = 950; // Passband (-3dB): 95.0%
        soxr->params[4] = 500; // Phase Response: 50.0% (线性相位)
        soxr->params[5] = 0;   // Allow Aliasing: Off
    }
    return (ddb_dsp_context_t *)soxr;
}

static void ddb_soxr_close(ddb_dsp_context_t *ctx) {
    ddb_soxr_t *soxr = (ddb_soxr_t *)ctx;
    if (!soxr) return;
    
    if (soxr->soxr) soxr_delete(soxr->soxr);
    if (soxr->fifo) free(soxr->fifo);
    if (soxr->out_buf) free(soxr->out_buf);
    free(soxr);
}

/* ==========================================================================
 * DSP 核心处理逻辑 (数据流与重采样)
 * ========================================================================== */
static int ddb_soxr_process(ddb_dsp_context_t *ctx, float *samples, int frames, int maxframes, ddb_waveformat_t *fmt, float *ratio) {
    ddb_soxr_t *soxr = (ddb_soxr_t *)ctx;
    if (!soxr || !fmt || frames <= 0 || !samples || fmt->channels <= 0) return frames;

    int in_rate = fmt->samplerate;
    int target_rate = in_rate;

    /* 1. 自动路由目标采样率 (双基准路由逻辑) */
    if (in_rate % 44100 == 0) {
        static const int r44[] = {44100, 88200, 176400};
        int idx = soxr->params[0];
        target_rate = r44[idx >= 0 && idx <= 2 ? idx : 0];
    } else {
        static const int r48[] = {48000, 96000, 192000};
        int idx = soxr->params[1];
        target_rate = r48[idx >= 0 && idx <= 2 ? idx : 0];
    }

    /* 2. 引擎创建与参数注入 */
    if (!soxr->soxr || soxr->current_in_rate != in_rate || soxr->current_target_rate != target_rate || soxr->channels != fmt->channels) {
        if (soxr->soxr) soxr_delete(soxr->soxr);
        
        soxr_io_spec_t io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
        
        /* 内存清零手工填充，严禁使用 VHQ 宏避免参数冲突 */
        soxr_quality_spec_t q_spec;
        memset(&q_spec, 0, sizeof(q_spec));
        
        double precs[] = {53.0, 47.0, 37.0, 28.0, 20.0, 16.0};
        int q_idx = soxr->params[2];
        if (q_idx < 0 || q_idx > 5) q_idx = 3;
        double bit_depth = precs[q_idx];
        
        /* 祖传数学模型：-3dB 截止点逆向推导 0dB 起点 */
        double bw_3dB_pc = (double)soxr->params[3] / 10.0;
        double rej = bit_depth * linear_to_dB(2.0);
        double bw_0dB_pc = 100.0 - (100.0 - bw_3dB_pc) / TO_3dB(rej);
        
        if (bw_0dB_pc > 99.0) bw_0dB_pc = 99.0;
        if (bw_0dB_pc < 70.0) bw_0dB_pc = 70.0;

        q_spec.precision = bit_depth;
        q_spec.passband_end = bw_0dB_pc / 100.0; 
        q_spec.phase_response = (double)soxr->params[4] / 10.0;
        
        /* 允许混叠时放宽阻带起点，压制预振铃 */
        if (soxr->params[5]) q_spec.stopband_begin = 2.0 - q_spec.passband_end;
        else q_spec.stopband_begin = 1.0;
        
        soxr->soxr = soxr_create(in_rate, target_rate, fmt->channels, NULL, &io_spec, &q_spec, NULL);
        
        if (!soxr->soxr) return frames; /* 引擎异常时原路退避 */
        
        soxr->current_in_rate = in_rate; 
        soxr->current_target_rate = target_rate;
        soxr->channels = fmt->channels;
        soxr->fifo_len = 0; 
    }

    if (!soxr->soxr) return frames;

    /* 3. 修改时钟标签 (屏蔽 ratio 修改，避免与 DeaDBeeF 宿主自带 SRC 的错误 ratio 叠加) */
    if (fmt->samplerate != target_rate) {
        /* 负负得正：宿主即使在直通时也会给出错误 ratio，注释掉此行真实 ratio 以直接抵消宿主 Bug */
        // if (ratio) *ratio *= (float)target_rate / in_rate;
        fmt->samplerate = target_rate;
    }

    /* 4. FIFO 环形缓冲处理 (稳固承载异步数据块) */
    size_t req_fifo = soxr->fifo_len + frames;
    if (req_fifo > soxr->fifo_cap) {
        size_t new_cap = req_fifo * 2;
        soxr->fifo = realloc(soxr->fifo, new_cap * fmt->channels * sizeof(float));
        soxr->fifo_cap = new_cap;
    }
    memcpy(soxr->fifo + (soxr->fifo_len * fmt->channels), samples, frames * fmt->channels * sizeof(float));
    soxr->fifo_len = req_fifo;

    if ((size_t)maxframes > soxr->out_cap) {
        soxr->out_buf = realloc(soxr->out_buf, maxframes * fmt->channels * sizeof(float));
        soxr->out_cap = maxframes;
    }

    size_t idone = 0, odone = 0;
    soxr_process(soxr->soxr, soxr->fifo, soxr->fifo_len, &idone, soxr->out_buf, maxframes, &odone);

    if (idone > 0) {
        if (idone < soxr->fifo_len) {
            memmove(soxr->fifo, soxr->fifo + (idone * fmt->channels), (soxr->fifo_len - idone) * fmt->channels * sizeof(float));
            soxr->fifo_len -= idone;
        } else {
            soxr->fifo_len = 0;
        }
    }

    memcpy(samples, soxr->out_buf, odone * fmt->channels * sizeof(float));
    return (int)odone;
}

/* ==========================================================================
 * UI 参数交互与配置
 * ========================================================================== */
static int ddb_num_params(void) { 
    return 6; 
}

static void ddb_set_param(ddb_dsp_context_t *ctx, int p, const char *val) {
    ddb_soxr_t *soxr = (ddb_soxr_t *)ctx;
    if (!soxr || !val) return;
    
    /* 拦截索引 3(Passband) 和 4(Phase) 放大 10 倍保留一位小数精度 */
    if (p == 3 || p == 4) soxr->params[p] = (int)(atof(val) * 10.0 + 0.5);
    else soxr->params[p] = atoi(val);
}

static void ddb_get_param(ddb_dsp_context_t *ctx, int p, char *val, int sz) {
    ddb_soxr_t *soxr = (ddb_soxr_t *)ctx;
    if (!soxr) return;
    
    /* 索引 3 和 4 还原为浮点字符串 */
    if (p == 3 || p == 4) snprintf(val, sz, "%.1f", (double)soxr->params[p] / 10.0);
    else snprintf(val, sz, "%d", soxr->params[p]);
}

static void ddb_reset(ddb_dsp_context_t *ctx) {
    if (ctx) ((ddb_soxr_t *)ctx)->current_in_rate = 0;
}

/* 包含显示名称的 Dialog 描述字符串 (代替不稳定的 get_param_name 回调) */
static const char settings_dlg[] =
    "property \"Target Rate (44.1k base)\" select[3] 0 2 44100 88200 176400;\n"
    "property \"Target Rate (48k base)\" select[3] 1 2 48000 96000 192000;\n"
    "property \"Quality\" select[6] 2 3 Best_53_319dB Ultra_47_282dB Ultra_37_222dB Ultra_168dB High_120dB Fast_96dB;\n"
    "property \"Passband (-3dB) 70.0-99.9%\" spinbtn[70.0,99.9,0.1] 3 95.0;\n"
    "property \"Phase Response 0.0-50.0%\" spinbtn[0.0,50.0,0.1] 4 50.0;\n"
    "property \"Allow Aliasing\" checkbox 5 0;\n";

/* ==========================================================================
 * 插件注册
 * ========================================================================== */
static DB_dsp_t plugin = {
    .plugin.api_vmajor = 1, 
    .plugin.api_vminor = 0,
    .plugin.version_major = 1, 
    .plugin.version_minor = 32,
    .plugin.type = DB_PLUGIN_DSP, 
    .plugin.id = "soxr_fb2k_core", 
    .plugin.name = "SoX Resampler (FB2K Core)",
    
    .num_params = ddb_num_params, 
    .get_param_name = NULL, /* 暴力安全处理：设为 NULL 彻底杜绝 GTK3 渲染解引用闪退 */
    .set_param = ddb_set_param, 
    .get_param = ddb_get_param,
    .configdialog = settings_dlg, 
    
    .open = ddb_soxr_open, 
    .close = ddb_soxr_close, 
    .process = ddb_soxr_process, 
    .reset = ddb_reset,
};

DB_plugin_t* ddb_sox_dsp_load(DB_functions_t *api) {
    deadbeef = api;
    return (DB_plugin_t *)&plugin;
}
