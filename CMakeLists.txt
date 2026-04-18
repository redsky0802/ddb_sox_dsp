cmake_minimum_required(VERSION 3.10)
project(DeaDBeeF_SoX_Resampler C)

# 启用 C99 标准 (DeaDBeeF 插件标准)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# 强制启用位置无关代码 (-fPIC，共享库必需)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 添加编译优化与警告标志
add_compile_options(-Wall -O3)

# 查找 libsoxr 库
find_library(SOXR_LIB soxr REQUIRED)

# 定义动态库目标 (使用 MODULE 专门用于运行时加载的插件)
add_library(ddb_sox_dsp MODULE ddb_sox_dsp.c)

# 去除 CMake 默认的 "lib" 前缀，使输出文件严格为 ddb_sox_dsp.so
set_target_properties(ddb_sox_dsp PROPERTIES PREFIX "")

# 链接必要的库: soxr 和数学库 m (源码中使用了 log10 等数学函数)
target_link_libraries(ddb_sox_dsp PRIVATE ${SOXR_LIB} m)

# 核心逻辑：拦截默认安装路径，将其重定向至用户本地的 DeaDBeeF 插件目录
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local/lib/deadbeef" CACHE PATH "Default plugin install path" FORCE)
endif()

# 安装指令：直接放置在 PREFIX 根目录
install(TARGETS ddb_sox_dsp LIBRARY DESTINATION ".")
