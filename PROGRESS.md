# 移植进度

本页记录已经在真机上验证的结果。未实际启动验证的内容会明确标记为“待验证”。

最后更新：2026-10-25（Asia/Shanghai）

## 状态矩阵

| 阶段 | 状态 | 证据 |
| --- | --- | --- |
| Fastboot + BootShim | 已验证 | Fastboot 可发送并临时启动 boot.img |
| Mu-Silicium UEFI | 已验证 | UEFI 菜单、屏幕和 USB 日志代码可运行 |
| UEFI 到 DarwinBoot | 已验证 | 能进入 XNU，不再停留在 `No Operating System Found` |
| XNU 早期入口 | 已验证 | 出现 kernel console、NVRAM 和 PE lockdown 日志 |
| IOKit CPU 匹配 | 已验证 | `ArmCpu` probe/start 被调用 |
| GIC 映射 | 已验证 | `prepareGic returned`，GIC stage 到达 9 |
| GIC CPU interface 使能 | 未完成 | 停在 `initCPU enter`，尚无 `initCPU returned` |
| 定时器中断 | 未验证 | IRQ ID 仍为 1023，timer 中断计数没有增长 |
| UFS | 未启动 | `UFS TIME ... requests=0` |
| APFS / launchd / 用户态 | 未开始 | 尚未发布根块设备 |

当前阶段是：**XNU IOKit 平台启动，CPU0 的 `ArmCpu::initCPU()` 内部。**

## 已成功构建

GitHub Actions：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/40165586538>

结果：`success`

设备仓库提交：

```text
488d13f fix(armcpu): strip split -D boot progress flag
```

构建产物：

| 文件 | SHA-256 |
| --- | --- |
| artifact.zip | `3459f9b51bca9a160ede898a723e7771ac462e911ab4488fde828984b9cc6122` |
| 原始 kernel.macho | `45902fab48c8f8e520eea11ab0e57f85a9d9bdeaa3b2d1e4b92382e3595f61a7` |
| 入口探针 kernel.macho | `ed91ddbd32704b2147e96f8e75635d9d915f26ed784339b949de2b2b3bf021ba` |
| cache ESP 镜像 | `6e0b1d5846a3ee4dbf2109d555826627ea4556f1578c3ff3f3a54db949cbe703` |

内核中已确认存在：

```text
ArmCpu start enter
ArmCpu boot args returned
ArmCpu prepareGic enter
ArmCpu prepareGic returned
ArmCpu IOCPU::start enter
ArmCpu processor register enter
ArmCpu IRQ controller enter
ArmCpu initCPU enter
```

## 最近一次真机结果

阶段历史：

```text
stage: ArmCpu initCPU enter
4s ArmCpu IOCPU::start returned
4s ArmCpu processor register enter
4s ArmCpu processor register returned
4s ArmCpu IRQ controller enter
4s ArmCpu IRQ controller returned
4s ArmCpu initCPU enter
```

关键状态：

```text
ARMCPU probe=1 start=1 reg=1 GIC stage=9 fail=0 rdy=0
pidr=3b typer=7b0418
GIC raw rng=24 reg=32 cmp=11
    | a00000/10000/a60000/100000 stride=20000 typer=21
CPU max=1 IRQ phase=0 id=1023
UFS TIME ... requests=0
```

## 已确认的结论

1. GIC 地址和设备树属性已经能够解析，`prepareGic()` 成功返回。
2. `IOCPU::start()`、`ml_processor_register()` 和
   `IOCPUInterruptController` 注册都已返回。
3. CPU0 已经进入 `ArmCpu::initCPU(true)`。
4. 停顿位于 `initCPU()` 的 boot CPU 分支，而不是更早的 probe、GIC 映射或 CPU 注册。
5. `rdy=0` 表示 `enableGic()` 尚未设置完成标记，但还需要新增子阶段确认具体位置。
6. `IRQ id=1023` 和 timer 无增长只能说明尚未观察到有效中断，不能单独证明
   GICv3 或 UFS 的根因。
7. UFS `requests=0` 是平台启动尚未推进的结果，当前不应继续修改 UFS 驱动。

## 最新诊断补丁

设备仓库提交：

```text
cf6770b diag(armcpu): trace initCPU and GIC enable substeps
```

新增阶段包括：

```text
ArmCpu enableGic enter
ArmCpu enableGic SRE ready
ArmCpu enableGic priority ready
ArmCpu enableGic sources ready
ArmCpu enableGic group1 ready
ArmCpu enableGic state sampled
ArmCpu enableGic returned
ArmCpu initCPU boot path
ArmCpu initCPU enableGic enter
ArmCpu initCPU enableGic returned
ArmCpu initCPU SGI probe enter
ArmCpu initCPU SGI probe returned
ArmCpu initCPU optional sources enter
ArmCpu initCPU optional sources returned
ArmCpu initCPU timebase enter
ArmCpu initCPU timebase returned
ArmCpu initCPU set state enter
ArmCpu initCPU set state returned
ArmCpu initCPU returned
```

## 正在构建

GitHub Actions：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/40170103530>

记录本页时状态：`in_progress`

预期产出：包含 `cf6770b` 新阶段字符串的 `unixnu-dipper-kernel-1`。

## 下一步

1. 等待构建成功并下载 artifact。
2. 确认内核中存在 `ArmCpu initCPU boot path` 和
   `ArmCpu enableGic enter`。
3. 复制内核并应用入口/panic 屏幕探针。
4. 生成新的 128 MiB、FAT16、4K-sector cache ESP。
5. 通过 `fastboot boot` 临时启动。
6. 记录最后出现的阶段：

```text
卡在 enableGic enter
  -> 检查 GIC system-register 接口或第一条 enableGic 操作

卡在 SRE ready 之前
  -> ICC_SRE_EL1 读取/写入

卡在 priority ready 之前
  -> ICC_CTLR/PMR/BPR1

卡在 sources ready 之前
  -> redistributor ISENABLER0 或 GICD_CTLR MMIO

卡在 group1 ready 之前
  -> ICC_IGRPEN1_EL1

通过 enableGic 后卡在 SGI probe
  -> ICC_SGI1R/HPPIR 自探测路径

通过 SGI probe 后卡在 timebase
  -> CNTV_CTL_EL0

通过 timebase 后卡在 set state
  -> IOCPU 状态转换
```

## 安全记录

- 上轮只写入 `cache` 分区。
- 原 `cache` 分区有 256 MiB 备份。
- 未写入 boot、userdata、unixnu 或 Android system。
- 测试镜像通过 Fastboot 临时启动。
- 本仓库不包含 Apple kernelcache、ADT、根卷或用户数据。
