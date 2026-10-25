# 移植进度与返校继续检查点

最后更新：2026-10-25（Asia/Shanghai）

本页区分两条工作线：

- **本项目 dipper 调试分支**：已经进入 XNU，当前卡在 APFS 卷发布。
- **从 `No Operating System Found` 开始的协作者**：仍应先完成 UEFI FAT/EFI
  冒烟测试。

## 当前结论

本项目已经完成：

```text
Fastboot
  -> BootShim
  -> Mu-Silicium UEFI
  -> UniXNU
  -> DarwinBoot
  -> XNU
  -> ArmCpu / GICv3 / timer
  -> UFS / SMMU / GPT
  -> OS + SystemOS 根窗口发布
```

当前停在：

```text
UFS ROOT FAILURE: SystemOS signed volume not found
```

根镜像、UFS 和 SMMU 都已验证正常；失败原因是 APFS personalities 没有进入
IOKit catalog，导致 APFS 没有为 `Whole=1` 的根窗口创建卷 media。

## 源码检查点

设备源码：

<https://github.com/hanyaa380/UniXNU-dipper>

返校检查点提交：

```text
2b37a90 feat(uefi): persist dipper USB log checkpoints
```

Git tag：

<https://github.com/hanyaa380/UniXNU-dipper/releases/tag/school-checkpoint-2026-10-25>

也可以用：

```sh
git clone --branch school-checkpoint-2026-10-25 \
  https://github.com/hanyaa380/UniXNU-dipper.git
```

关键提交：

```text
2b37a90 feat(uefi): persist dipper USB log checkpoints
bf9642d fix(apfs): publish APFS personalities after kernel load
d8adfc9 diag(ufs): report APFS load and IOMedia properties
cf6770b diag(armcpu): trace initCPU and GIC enable substeps
488d13f fix(armcpu): strip split -D boot progress flag
8791cbe fix(armcpu): remove split boot progress macro arguments
81da04a diag(armcpu): trace boot CPU startup stages
e9ffdb0 diag(iokit): exclude all Apple CPU candidates on dipper
```

## 状态矩阵

| 阶段 | 状态 | 关键证据 |
| --- | --- | --- |
| Fastboot + BootShim | 已验证 | 临时 boot.img 可启动 |
| Mu-Silicium UEFI | 已验证 | 菜单、GOP、USB 日志可用 |
| DarwinBoot → XNU | 已验证 | 可进入 XNU 控制台 |
| XNU 早期入口 | 已验证 | kernel console、NVRAM、PE lockdown |
| ArmCpu probe/start | 已验证 | 全部外层和内部 start 阶段返回 |
| GICv3 映射 | 已验证 | `prepareGic returned` |
| GIC CPU interface | 已验证 | `enableGic returned`，`rdy=1` |
| CPU `initCPU()` | 已验证 | `initCPU returned` |
| SGI 可选自探测 | 已识别硬件限制 | 开启时硬锁，启动配置已移除 |
| UFS 控制器 | 已验证 | SCSI READ(10) 完成，`ocs=0` |
| SMMU stream | 已验证 | `smr=800f0100`、`s2cr=00000002` |
| GPT / 根窗口 | 已验证 | 找到并发布 OS、SystemOS 两个窗口 |
| 根窗口 I/O | 已验证 | `IO reads=8 failures=0` |
| APFS kext load | 已验证 | `APFS load=0` |
| APFS IOKit personalities | 修复已提交，待真机验证 | `bf9642d` |
| SystemOS/Data 挂载 | 未完成 | 等待 APFS 卷 media |
| dyld / launchd | 未开始 | 依赖根卷 |
| iOS 用户态 | 未开始 | 尚未进入 |
| Android/iOS 双系统 | 未开始 | 先完成用户态启动 |

## 最新真机证据

UFS/SMMU 成功：

```text
CMD n=19 type=01 op=28 lun=00 ocs=00 rsp=21000000 scsi=0000
SMMU smr=800f0100 s2cr=00000002
WINDOWS found=2 prepared=2 published=2 bytes=c4e00000
IO reads=8 failures=0
```

APFS 诊断：

```text
NATIVE_UFS media report: published=2 reads=8 failures=0 media=2
APFS load=0 registered=0 vfsops=0
disk0 content= hint= whole=1 leaf=1 size=c4e00000 uuid=-
disk1 content= hint= whole=1 leaf=1 size=c4e00000 uuid=-
```

根因：

- `APFS load=0` 表示 APFS kext 已经成功加载。
- `registered=0` 是加载调用后的即时 VFS 检查结果。
- `disk0/disk1` 的 `Whole=1`，说明 APFS whole-disk personality 本应匹配。
- C 接口 `OSKextLoadKextWithIdentifier()` 默认 `startMatchingOpt =
  kOSKextExcludeAll`，只 load，不把 APFS personalities 发送给 IOKit。

修复：

```text
bf9642d fix(apfs): publish APFS personalities after kernel load
```

改为 C++ `OSKext::loadKextWithIdentifier(..., kOSKextExcludeNone,
kOSKextExcludeNone, ...)`，从而发送 APFS personalities 并启动匹配。

## 正在构建

GitHub Actions：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/40177699244>

记录本页时状态：`in_progress`

注意：该运行由 `bf9642d` 触发，但 GitHub workflow checkout 使用设备仓库
`main`。`2b37a90` 只增加 UEFI 日志文件，不改变这次内核源码中的 APFS 修复。

## 已验证的 APFS 诊断内核

运行：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/40175066339>

结果：`success`

| 文件 | SHA-256 |
| --- | --- |
| artifact.zip | `223501a809d4f0ca9a552f1e1d7939e3953d1b5b933e3ad4dd72f9033bfe5bb8` |
| 原始 kernel.macho | `1d5123c85bae82ca280e97ecb841538a78807c7c12691af786708954be982ce0` |
| 入口探针 kernel.macho | `6f115eb33c2c9ff67953091e2a9a455283712c9cb030e8184f439cc55913daa1` |
| 媒体报告 cache 镜像 | `14f8739386085413f333ee7e2c54433c1333e6078c03a33f5f98a3e1839843a4` |

## 根镜像验证

本地 `userdata-root.img`：

```text
image_bytes = 9382658048
image_sha256 = fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093
```

窗口：

| 名称 | Offset | Size | 结构 |
| --- | --- | --- | --- |
| OS | `0x100000` | `0x16a408600` | GPT 磁盘 |
| SystemOS | `0x16a600000` | `0xc4e00000` | 直接以 `NXSB` 开头的 APFS 容器 |

SystemOS 中存在预期 UUID：

```text
8846FE78-2910-4297-AF97-D59BB6F8FC16
```

因此当前失败不是镜像缺卷或 UUID 写错。

## 返校后继续步骤

1. 打开 Actions `40177699244`，确认构建成功。
2. 下载 `unixnu-dipper-kernel-1`。
3. 确认内核包含：

```text
fix(apfs): publish APFS personalities after kernel load
native_apfs_load_and_match
APFS load=%d registered=%u vfsops=%llx
```

4. 对 kernel 副本应用入口探针：

```sh
UNIXNU_UEFI=/root/mi8uefi/Mu-Silicium \
python3 scripts/patch-xnu-entry-probe.py kernel.panic.macho \
  --panic-screen --no-reset
```

5. 使用 [configs/dipper-apfs-stage.toml](configs/dipper-apfs-stage.toml) 创建
   128 MiB FAT16、4K-sector ESP。
6. `fastboot flash cache <image>` 后执行 `fastboot boot
   boot-dipper-usblog-persist.img`。
7. 首选确认媒体报告：APFS personalities 应开始匹配，随后出现 APFS 容器和卷 media。
8. 根启动配置应看到：

```text
SystemOS volume 8846FE78-2910-4297-AF97-D59BB6F8FC16 ready
```

9. 通过后再处理 SystemOS mount、Data mount、dyld、launchd 和显示/触控/网络。

## 构建工作流参数

仓库：`hanyaa380/UniXNU-build`

工作流：`macos.yml`

参数：

```text
device_repository=hanyaa380/UniXNU-dipper
device_ref=main
defconfig=dipper_defconfig
steps=kernel
xcode=26.6
```

`pre_build`：

```sh
set -euo pipefail
./unixnu ipsw fetch KernelCache OS Cryptex1,SystemOS
curl -fL --retry 5 --retry-all-errors --connect-timeout 20 \
  -o "$RUNNER_TEMP/fajita-kernel.zip" \
  "https://nightly.link/johnpanos/UniXNU/actions/runs/39734235637/unixnu-fajita_defconfig.zip"
rm -rf "$RUNNER_TEMP/fajita-kernel"
unzip -q "$RUNNER_TEMP/fajita-kernel.zip" -d "$RUNNER_TEMP/fajita-kernel"
echo "FAJITA_STORAGE_CANDIDATE=$RUNNER_TEMP/fajita-kernel/fajita/kernel/kernel.macho" \
  >> "$GITHUB_ENV"
```

## 安全与文件边界

- 只写临时 `cache` 分区。
- 原 cache 有 256 MiB 备份。
- 未写 boot、userdata、unixnu 或 Android system。
- 仓库不包含 Apple kernelcache、ADT、IPSW、iOS 根卷或用户数据。
- 内核和根镜像应由协作者从合法来源自行构建/提取。
