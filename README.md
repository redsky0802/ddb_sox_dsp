# DeaDBeeF SoX Resampler (FB2K Core)

[English](#english) | [中文](#chinese)

<a name="english"></a>
## English

High-precision resampling DSP plugin for DeaDBeeF player on Linux. The core algorithms and mathematical models are provided by **libsoxr**, ensuring the same audio quality and filtering characteristics as the famous **Foobar2000 SoX Resampler**.

### Core Logic
* **Engine Parity**: Powered by `libsoxr`. Mathematically identical to the Foobar2000 plugin, providing professional-grade 1D resampling.
* **Manual Parameter Injection**: Bypasses opaque presets. Manually injects physical parameters (Passband, Phase Response, Precision) for predictable filtering.
* **Dual-Base Routing**: Independent target rates for 44.1kHz and 48kHz families (1x/2x/4x) to ensure integer-ratio resampling.
* **FIFO Smoothing**: A robust ring buffer handles asynchronous data chunks, ensuring jitter-free playback and preventing "chipmunk" speed anomalies.

### Compilation & Installation
**Dependencies**: `deadbeef >= 1.10.1`, `libsoxr-dev`, `deadbeef-plugins-dev`, `cmake`.

```bash
mkdir build && cd build
cmake ..
make
make install
```

*Default install path: `~/.local/lib/deadbeef/`.*

### DeaDBeeF Configuration

Due to the double-resampling architecture of DeaDBeeF, please follow these steps:

1.  Go to `Preferences -> Playback`.
2.  **Enable** `Override samplerate`.
3.  **Enable** `Based on input separate`, to distinguish 48k/44.1k bases.
4.  **Match the Rates**: Ensure the Host's "Target rate" for both groups **exactly matches** the target frequencies selected in the SoX plugin.

### Credits

  * **lvqcl**: For the original Foobar2000 plugin logic and `TO_3dB` model.
  * **Rob Sykes**: For the magnificent `libsoxr` engine.
  * **silentlexx**: For the plugin `deadbeef_soxr` in early days.

### AI Assistance

The initial three commits were all extracted from the AI LLM dialogue (Gemini 3.1 Pro), and the debugging process was extremely tedious. I'll have to find a smarter helper next time.jpg

<a name="chinese"></a>
## 中文

为 Linux DeaDBeeF 播放器打造的高精度重采样 DSP 插件。其核心算法与数学模型完全由 **libsoxr** 提供，确保其音质与滤波特性与 Windows 平台著名的 **Foobar2000 SoX Resampler** 完全同源。

### 核心逻辑

  * **算法同源**：完全采用 `libsoxr` 引擎，在数学层面实现与 Foobar2000 原版插件的像素级对齐。
  * **纯手工参数注入**：屏蔽不透明的预设宏，手工注入物理参数（通带、相位响应、精度），确保滤波特性符合预期。
  * **双基准频率路由**：针对 44.1kHz 与 48kHz 族系提供独立的目标频率设置（支持 1x/2x/4x），有效避免非整数倍 SRC 劣化。
  * **基于 FIFO 的平滑吞吐**：引入坚固的 FIFO 环形缓冲机制，完美承载异步数据块，保障播放极其平滑并杜绝“快放”异常。

### 编译与安装

**环境依赖**:  `deadbeef >= 1.10.1`，`libsoxr-dev`，`deadbeef-plugins-dev`，`cmake`。

```bash
mkdir build && cd build
cmake ..
make
make install
```

*默认安装路径：`~/.local/lib/deadbeef/`*

### DeaDBeeF 配置

受 DeaDBeeF 双层重采样架构限制，请务必进行以下设置：

1.  进入 `首选项 -> 播放`。
2.  **开启** `覆盖采样率`。
3.  **开启** `根据输入采样率`，分别设置 48kHz 和 44.1kHz 的整数倍的目标频率。
4.  **频率对齐**：确保宿主设置的两个目标采样率与你在 SoX 插件中选择的目标频率**完全一致**。

### 致谢

  * **lvqcl**: Foobar2000 SoX 插件作者，提供了核心数学模型参考。
  * **Rob Sykes**: `libsoxr` 引擎作者。
  * **silentlexx**: `deadbeef_soxr` 老插件作者。

### AI 协助

最初的 3 个 commit 全部摘录自 AI 大语言模型对话（Gemini 3.1 Pro)，调试过程极为折磨。下一次得找个更聪明点的帮手.jpg
