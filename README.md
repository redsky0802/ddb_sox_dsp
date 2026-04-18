# DeaDBeeF SoX Resampler (FB2K Core)

本项目为 Linux DeaDBeeF 播放器提供高精度的重采样功能，其核心算法与数学模型深度复刻自 Windows 平台著名的 **Foobar2000 SoX Resampler**。

### 1. 开发愿景
本项目致力于在 Linux 环境下全量复刻 Foobar2000 SoX 插件的声学素质。通过移植其标志性的 Kaiser 窗 FIR 滤波模型，为 PCHiFi 玩家提供精确到 0.1% 的时域控制权，实现无妥协的音频重采样体验。

### 2. 核心逻辑复刻
受限于 DeaDBeeF 的底层架构与 GUI 机制，本插件聚焦于声学核心逻辑的完美移植，并重构了最关键的数据流屏障：

* **TO_3dB 祖传算法**：完整内置了用于通带补偿的多项式数学模型，将用户设定的 -3dB 截止点精准换算为底层引擎所需的 0dB 起点。
* **纯手工引擎注入**：拒绝使用 `libsoxr` 的黑盒预设宏（如 VHQ），采用纯手工参数注入，确保滤波器的数学特性完全符合用户预期。
* **双基准频率路由**：提供 44.1kHz 与 48kHz 两组独立的倍频切换方案（支持 1x/2x/4x），有效避免非整数倍 SRC 带来的相位劣化。
* **基于 FIFO 的平滑吞吐**：引入了坚固的 FIFO 环形缓冲机制，完美承载异步重采样产生的不规则数据块，提供极致平滑的播放体验，同时在物理层面上杜绝了时钟握手失败导致的“快放”异常。

### 3. 编译与安装

**环境依赖：**

根据你的 Linux 发行版，安装相应的运行时（Runtime）与编译时（Build）依赖：

* **Debian / Ubuntu 系**
  * 运行时依赖：`libsoxr0`
  * 编译依赖：`build-essential cmake libsoxr-dev deadbeef-plugins-dev`
  ```bash
  sudo apt install libsoxr0 build-essential cmake libsoxr-dev deadbeef-plugins-dev
  ```

* **Arch Linux 系**
  * 运行时依赖：`soxr`
  * 编译依赖：`base-devel cmake deadbeef`
  ```bash
  sudo pacman -S soxr base-devel cmake deadbeef
  ```

**标准编译与安装步骤：**

本项目采用标准的 CMake 构建系统，默认安装路径已自动配置为用户的本地插件目录（`~/.local/lib/deadbeef/`）。在项目根目录下执行以下命令即可：

```bash
mkdir build && cd build
cmake ..
make
make install
```
*(注：如果需要系统级全局安装供所有用户使用，可在 cmake 时显式指定：`cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/lib/deadbeef/` 并使用 `sudo make install`)*

安装完成后，完全重启 DeaDBeeF。在 `Preferences -> DSP` 菜单中添加并配置 **SoX Resampler (FB2K Core)** 即可生效。

### 4. 宿主配置必读（避免进度条异常）

由于 DeaDBeeF 存在双层重采样机制（宿主自带 SRC + DSP 插件）的时钟比例干扰，为了获得正常的进度条速度与完美的音质，请务必进行以下设置：
1. 进入 DeaDBeeF `首选项 (Preferences) -> 播放 (Playback)`。
2. **开启** `重采样 (Resampling)` 选项。
3. **确保宿主设定的“目标采样率”（Target Sample Rate）与你在 SoX 插件中选择的目标频率完全一致**。

### 5. 参考项目与致谢

本插件的诞生站在了巨人的肩膀上。核心逻辑与工程架构深度参考了以下开源项目，在此致敬：

* **Foobar2000 foo_dsp_resampler** (by *lvqcl*) 
  * 提供了核心的时域换算数学模型（`TO_3dB` 通带补偿算法）及高精度浮点参数处理思路。
* **The SoX Resampler library (libsoxr)** (by *Rob Sykes*)
  * 提供了底层无可挑剔的高精度一维离散信号重采样运算引擎。
* **社区版 DeaDBeeF SoXR 插件**
  * 提供了基础的 DeaDBeeF DSP API 握手规范及内存布局映射参考。
```
