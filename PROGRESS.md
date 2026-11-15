# Xiaomi Mi 8（dipper）UniXNU 移植进度

最后更新：2026-11-15（Asia/Shanghai）

## 快速接手

如果当前聊天上下文已经被压缩，先读：

1. 本文件顶部的 2026-11-15 状态；
2. [UniXNU-dipper 触摸驱动和提交历史](https://github.com/hanyaa380/UniXNU-dipper/tree/diag/io-media-registry)；
3. `NOTES-touch.md`，其中是 FTS/FTM5 硬件协议、HID 约定和诊断方法；
4. 本文件的 APFS 历史章节，保留此前从 UEFI 到根卷挂载的排查记录。

当前一句话状态：

> Mi 8 已完成 UEFI、DarwinBoot、XNU、UFS/APFS、launchd、backboardd、
> SpringBoard 和触摸 HID 全链路。真机触摸在界面中有效，并已进入 iOS
> 初始设置引导。当前主阻塞点转为初次设置完成、GPU 和其余外设。

## 2026-11-15：触摸和初始引导已打通

### 真机结果

更换屏幕后的触摸已在 SpringBoard 和初始设置界面中实际可用：

```text
ST FTS
 -> Mi8Fts / FTM5
 -> native_touch_* seqlock
 -> NativeHidInput
 -> IOHIDEventDriver
 -> backboardd
 -> SpringBoard
 -> iOS Setup
```

这不是只看到控制器 ACK 或内核坐标日志，而是事件已经被 iOS 用户态消费，
可以操作初始引导界面。

### 确认的触摸硬件

TWRP 3.2.3 和 stock FTS 日志确认新屏仍为 ST FTS，并非 Atmel：

| 项目 | 值 |
| --- | --- |
| I2C | `a98000.i2c`，总线 3，地址 `0x49` |
| Device Tree | `/soc/i2c@a98000/fts@49` |
| 输入节点 | `/dev/input/event3` |
| IRQ | `286`，GPIO31，Level |
| Firmware | `0x0045` |
| SVN | `1098` |
| Config | `0x0019` |
| Panel color/vendor | `2` / `R` |
| Lockdown | `48 38 32 04 45 31 52 00` |

`0x4A` 的 Atmel mXT 探测失败。Stock FTS 最终执行 `Sense ON!` 并
`Probe Finished!`；芯片固件版本与 `st_fts_e1.ftb` 相同或更新，因此不应
重刷触摸固件。

### 驱动修复

关键提交：

```text
fb81480 feat(touch): port Dipper ST FTS to NativeHidInput
db77f71 fix(touch): keep ST FTS archive in osfmk include context
a5ff58c fix(touch): start Dipper FTS unconditionally from boot monitor
9736b0b fix(touch): continue FTS init without controller-ready event
450cbf5 fix(hid): register board classes under kext lock
```

`9736b0b` 后真机成功收到：

```text
FTS controller ready: 03 00 00 00 00 00 00 00
FTS chip id: 36 39 ...
FTS system info: ... 45 00 98 10 19 ...
FTS: scanning on (active mode)
fts: finger 0 down ...
fts: finger 0 up ...
```

进一步测试得到多指、移动和完整 down/move/up，约每 10 秒超过 400 个
事件，读取占用约 12.7% CPU。随后发现 SpringBoard 不响应的直接原因是
诊断 payload 为了满足 1000-byte 命令行限制删掉了
`offline-hid-home=1`。该参数实际是 `HidInputProbe.cpp` 创建/启动整个
NativeHidInput 服务的总开关，并非仅 Home 键。

### 可工作的测试 payload

当前已进入初始引导的可工作镜像：

```text
/root/mi8uefi/UniXNU-official-merge/out/dipper-hid-fullconsole/dipper/payload/payload.img
SHA-256: ca0c537bd04ba3750a42c4729e11cb1ae646371be17ff4c69966181b35385dce
```

内核：

```text
GitHub Actions run: 43675993105
/root/mi8uefi/toolchain/dipper-kernel-touch-ready/run-43675993105/dipper/kernel/kernel.macho
SHA-256: 6052384bcd524b6daa6fb576e21614c359e5de2603b23bb885a03cb29a401cda
```

关键参数：

```text
fts=1
fts_steps=6
offline-hid-input=1
offline-hid-home=1
sheng-touch-hid=1
```

另有去掉 VC 参数、改用 `boot-progress=2 sheng-full-console=1` 的诊断镜像：

```text
/root/mi8uefi/UniXNU-official-merge/out/dipper-hid-shengconsole/dipper/payload/payload.img
SHA-256: a7c1362dffdfaabbd66f96012bf5f7dc2e3387d33d8bf9262fd291b71c853451
```

该诊断镜像用于后续排查，不是当前已进入初始引导的基线。

### 仍然需要区分的事项

- `450cbf5` 增加 `OSKextRegisterBoardClasses`，修复板级类注册与并发
  kext 加载的锁顺序风险；对应的新内核构建仍在进行，尚未作为触摸成功
  的必需条件验证。
- `boot-monitor=1` 是 monitor-only，不启动 full-console/touch monitor，
  因此不会显示 `sheng-full-console`；要显示 sheng 屏内日志必须使用
  `boot-progress=2`。
- 当前已进入设置引导，不再继续排查 ST FTS 的 reset、ready、IRQ 或
  NativeHidInput 启动门。

### 下一步

1. 完成初次设置，确认 Home、多指、锁屏滑动和系统手势。
2. 固定并保存可工作的 cache payload 与内核哈希。
3. 完成锁修复内核的真机 A/B 验证后再决定是否替换工作基线。
4. 转向 GPU 驱动；`sheng-gpu-addon=1` 只是资源装载，不代表 A630
   GPU 加速已经可用。

## 设备与目标系统

| 项目 | 值 |
| --- | --- |
| 设备 | Xiaomi Mi 8 标准版 |
| 代号 | `dipper` |
| SoC | Qualcomm SDM845 |
| RAM | 6 GiB |
| UFS | 64 GiB，UFS 2.1 |
| UniXNU 分支 | `hanyaa380/UniXNU-dipper` |
| 启动固件 | Mu-Silicium 的自定义 dipperPkg |
| Apple 用户态来源 | iPadOS 17.7.11（21H461） |
| IPSW 产品 | iPad7,1 |
| DeviceClass | `j120ap` |
| Chip ID | `0x8011` |
| Board ID | 12 |

## 历史：SystemOS/APFS bring-up 阻塞点（2026-10-31）

真机最终显示：

```text
UFS ROOT FAILURE: SystemOS signed volume not found
stage=media publication result=e00002d6

NATIVE_UFS media report: published=1 reads=4 failures=0 media=1
bsdClients=1 rootOwner=1 id=100000115 skips=2

APFS load=0 registered=0 vfsops=0 found=1 loaded=0/1 started=0/1
tag=18 personalities=7 classes=d refresh=1 mediaFound=1

window=OS object=0xfffffff2fe215880
absolute=b3b200000 bytes=16a408600 maxBlock=b52042

window=SystemOS object=0xfffffff2fe21be90
absolute=ca5700000 bytes=c4e00000 maxBlock=626fff

disk0 object=0xfffffff5b15e6100 id=100000170
class=IOMedia content= hint= whole=1 leaf=1 write=0
size=c4e00000 block=512 children=1 uuid=-

parents:
IOMedia
  <- IOBlockStorageDriver(0x10000016f)
  <- NativeUfsBlock(0x10000016c)
  <- NativeUfs(0x10000011b)
  <- IOPlatformExpertDevice(0x100000115)
  <- IORegistryEntry(0x100000100)
```

`NativeUfs.cpp` 在等待 SystemOS UUID 约 30 秒后失败并停止发布 OS。
因此只有 SystemOS whole IOMedia，没有 OS media，也没有
AppleAPFSContainerScheme / AppleAPFSContainer / AppleAPFSVolume。

## 当前已验证事实

- UFS LUN 0 可读取；
- apps SMMU 映射采用：

```text
smr=800f0100
s2cr=00000002
stream=0x100
mask=0xf
size=0x80000
adopt=1
```

- 4 KiB GPT、`unixnu` 分区和 `SHNGROOT` manifest 均正确；
- OS 和 SystemOS 窗口边界正确且互不重叠；
- NativeUfsBlock 以 512-byte sector 对上层发布，底层按 4 KiB UFS LBA 读取；
- 读取经过 `vr_verify_block()` 完整性校验；
- `published=1` 表示 SystemOS block device 已 start/register；
- `rootOwner=1 skips=2` 表示 NativeUfs 单实例门闩正常；
- APFS kext 已 load 和 start；
- APFS 的三个核心 OSMetaClass 已注册；
- APFS personalities 已重新发布并请求过一次重匹配；
- 当前没有 APFS Container/Volume，也没有对应 BSD 子节点；
- 当前停止原因不是内存耗尽，也没有 OOM/分配失败证据。

## APFS 日志字段的正确含义

| 字段 | 已证明 | 未证明 |
| --- | --- | --- |
| `found=1` | APFS kext 对象存在 | scheme 已匹配 |
| `loaded=0/1` | load 调用前后 loaded 状态变化 | 内部全部初始化成功 |
| `started=0/1` | APFS kext start 已调用并成功返回 | scheme 实例存在 |
| `load=0` | `loadKextWithIdentifier()` 返回成功 | Container/Volume 已创建 |
| `tag=18` | APFS 已获得 load tag | IOMedia 已匹配 |
| `personalities=7` | APFS 导出 7 个 personality | 候选进入 probe |
| `classes=d` | 三个核心元类已注册 | 对应实例已创建 |
| `refresh=1` | personalities 已重发并请求重匹配 | 重匹配成功 |
| `mediaFound=1` | 当前 window 有一个 whole IOMedia | APFS 已认领该 media |
| `registered=0` | load 后立即扫描未找到 apfs vfstable | 以后永远不会注册 |
| `vfsops=0` | 上述即时扫描没有 vfsops | `vfs_fsadd` 已失败 |

`classes=d = 1101b`：

```text
bit 0 AppleAPFSContainerScheme                已注册
bit 1 "AppleAPFSContainerSchemeWholeDisk"     正常为 0，它不是 IOClass
bit 2 AppleAPFSContainer                      已注册
bit 3 AppleAPFSVolume                         已注册
```

`refresh=1` 来自 `refreshApfsPersonalities()`。它调用
`updatePersonalitiesInCatalog()`，后者进入
`IOCatalogue::exchangeDrivers(..., true)`，会重新加入 personalities、
调用 `catalogNewDrivers()`、增加 generation。因此它证明“重匹配已请求”，
但不证明 scheme 进入 probe 或 probe 成功。

## 第二轮 APFS 审计结论

详细版本见 `/root/mi8uefi/dp.txt`。

### 只读不是已确认根因

当前 21H461 kernelcache 中的 whole-disk personality 为：

```text
name             AppleAPFSContainerSchemeWholeDisk
IOClass          AppleAPFSContainerScheme
IOProviderClass  IOMedia
IOMatchCategory  IOStorage
IOProbeScore     0x7d0 = 2000
IOPropertyMatch  [ { Whole = true } ]
```

它没有 `Writable=true` 条件。公开 XNU/IOStorageFamily 的被动匹配不会仅因为
`write=0` 拒绝这个 personality。

写保护可能在两个更具体的位置影响 APFS：

1. `IOMedia::handleOpen()` 在 APFS 请求
   `kIOStorageAccessWriter` 时直接拒绝；
2. 如果请求仍进入 `NativeUfsBlock::doAsyncReadWrite()` 且方向不是
   `kIODirectionIn`，该函数直接返回 `kIOReturnNotWritable`。

当前没有日志证明 APFS 请求过 writer open，也没有证明 APFS 发过写请求。

### `failures=0` 不能排除写失败

- 写方向在块设备入口直接返回，不增加 `failures`；
- writer open 可能在 `IOMedia::handleOpen()` 就被拒绝，根本到不了块设备；
- `reads=4` 只能证明四次读取成功，不能证明调用者一定是 APFS。

### 当前最需要追踪

优先级：

1. `IOService::probeCandidates()` 中
   `IOClass == AppleAPFSContainerScheme` 的候选；
2. `IOService::startCandidate()`；
3. `IOMedia::handleOpen()` 的 reader/writer 权限；
4. `NativeUfsBlock::doAsyncReadWrite()` 的请求方向和偏移。

最小日志应包括：

```text
APFS_SCHEME candidate/match/category/module
APFS_SCHEME alloc/init/attach
APFS_SCHEME probe result/score
APFS_SCHEME start result/registered
APFS_OPEN access/writable/result
UFS_IO direction/offset/length/status
APFS service counts: Scheme/Container/Volume/BSD
```

在这些日志出现前，不能把“只读阻止 APFS”写成已确认根因。

## 根镜像与分区

根镜像：

```text
size = 9,382,658,048 bytes = 0x22f400000 = 8.738 GiB
SHA-256 = fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093
```

manifest 窗口：

| 名称 | Offset | Size | 绝对范围 |
| --- | --- | --- | --- |
| OS | `0x100000` | `0x16a408600` | `0xb3b200000..0xca56085ff` |
| SystemOS | `0x16a600000` | `0xc4e00000` | `0xca5700000..0xd6a4fffff` |

TWRP 已验证 4 KiB GPT：

```text
userdata   4063232, 61704192 blocks
unixnu     11776256, 20447232 blocks
UNIXNUEFI  14332160, 524288 blocks
```

`unixnu` 绝对起点为 `0xb3b100000`，与真机日志一致。
根镜像完整放入 `unixnu`，两个窗口均合法且不重叠。

注意：`board.toml` 的 `last_lba=14397695` 确实覆盖到了 ESP，
运行时会从 GPT 重新选择 `unixnu`，所以这不是当前阻塞原因。
不要据此修改分区或重新刷写。

## 源码与工作区状态

设备源码：

<https://github.com/hanyaa380/UniXNU-dipper>

公开进度仓库：

<https://github.com/hanyaa380/dipper-unixnu-bringup>

当前本地 HEAD：

```text
UniXNU-dipper         9c55db1 diag(ufs): expose claim-writable APFS probe switch
dipper-unixnu-bringup 0edfe61 diag(dipper): test APFS with writable media declaration
```

近期关键提交：

```text
0edfe61 test APFS with writable media declaration
1b851f9 test APFS catalogue rematch
07b9e50 add UFS single-owner test config
9c55db1 expose claim-writable APFS probe switch
c05a066 increment refresh counter atomically
bf9642d publish APFS personalities after kernel load
d8adfc9 report APFS load and IOMedia properties
```

## 当前测试产物

最后成功构建并用于诊断的本地产物：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfsrematch/prepared/kernel.plain.macho
/root/mi8uefi/toolchain/dipper-kernel-apfsrematch/prepared/kernel.panic.macho
/root/mi8uefi/toolchain/dipper-kernel-apfsrematch/cache-iokit29apfsrematch.img
/root/mi8uefi/toolchain/dipper-kernel-apfsrematch/artifact.zip
```

SHA-256：

```text
kernel.plain.macho             2e24fac3d5d8e4f0706f6f75b8855d20cfb18e89dcd59cff0a3df7fc5a0b8142
kernel.panic.macho             0655e1dc9530e452bad102fbbee5808c528f7545c77462e47ef27a36e66a1168
cache-iokit29apfsrematch.img   a73f7050baeb90259dcd6979fa5f7af78fee6c1d687a9693e8389cc72ce58ee8
artifact.zip                   272721e786628ec24ceb3dd17699c2684e4f7ce79c077166e8bb5fb62f57a785
```

时间校正：

- 上述 apfsrematch 产物构建于 2026-10-31 07:37；
- `claim-writable` 两个提交在 07:50 才提交；
- 因此上述 apfsrematch 产物不含最新的
  `native-ufs-claim-writable=1` 实验配置；
- 最新已测真机日志仍为 `write=0`，不能用于判断 writable probe 的效果。

## Writable probe 新构建（已下载，待真机测试）

GitHub Actions 运行：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/41146797154>

结果：`success`

设备仓库分支：

```text
hanyaa380/UniXNU-dipper:diag/io-media-registry
9c55db1 diag(ufs): expose claim-writable APFS probe switch
```

下载目录：

```text
/root/mi8uefi/toolchain/dipper-kernel-writableprobe
```

已确认新内核包含：

```text
_native_apfs_load_and_match
_gClaimWritableMedia
NativeUfsBlock::reportWriteProtection
native-ufs-claim-writable
```

已加入 panic screen 的待测镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-writableprobe/cache-iokit30writableprobe.img
```

SHA-256：

```text
kernel.plain.macho           341d52ccae340c9dbabda4dde6d1f70b1776e02324a58c5a866a3d8026b4dca8
kernel.panic.macho           a8c862da045256fbc481deefefaf89331373daa72663a566fdee4960e40d0611
kernel.macho.adt             d4278eeb161754f14eeeacbae17e2d723ebc9e5e4906dca755e9bd1bda4cf8d3
unixnu.toml                  b58f525add0b85ebdb904ef509d668cbd04ece80b7e142ac54d9774ebbbf5c30
cache-iokit30writableprobe.img b675d3beff13e4a53bef40167e4323ddc0c5dfe11b2c287de5cd633fdb6ec9cd
```

FAT 检查结果：

```text
fsck.fat: clean
7 files, 7688/8188 clusters
```

镜像内启动参数已确认包含：

```text
sheng-build-tag=iokit30writableprobe
native-ufs-probe=12
native-ufs-diagnostics=1
native-ufs-claim-writable=1
offline-apfs-load=1
```

第一次真机启动尝试：

```text
fastboot flash cache cache-iokit30writableprobe.img: OKAY
fastboot boot boot-dipper-usblog-persist.img:        OKAY
```

镜像成功写入并 boot，但日志读取器启动后没有看到
`18d1:d00d` vendor USB log 设备，设备随后离开 fastboot/日志 USB 状态。
本轮没有取得 `write` 或 APFS media report 新结果。

下一轮应先启动 USB 日志读取器，再执行 `fastboot boot`，避免错过早期
USB 枚举窗口。

第二次真机启动尝试及修正：

- fastboot 和 Dipper USB log 都使用 `18d1:d00d`；
- 在 fastboot 阶段先启动 `read_usb_log.py` 会占用 fastboot 接口，
  导致 `fastboot boot` 停在 `< waiting for any device >`；
- 释放接口后 boot 命令完成，但日志端点没有重新枚举；
- 因此不能简单地把“先启动读取器”作为修正方案；
- 本轮同样没有取得新日志。

下一步回退到 UFS 持久日志：`sheng-log=1` 已启用时，日志窗口位于
`unixnu` 最后 8192 个 4 KiB block。按当前 GPT：

```text
unixnu first LBA  11776256
unixnu last LBA   14332159
log first LBA     14323968
log size          8192 blocks = 32 MiB
```

可以重启到 TWRP，直接读取这 32 MiB，并用 `SHLOG001` 头解析。

## 2026-10-31 writable probe 真机结果

真机 media report：

```text
NATIVE_UFS media report: published=1 reads=4 failures=0 media=1
bsdClients=1 rootOwner=1 id=100000115 skips=2
APFS load=0 registered=0 vfsops=0 found=1 loaded=0/1 started=0/1
tag=18 personalities=7 classes=d refresh=1 mediaFound=1
disk0: whole=1 leaf=1 write=1 size=c4e00000 block=512 children=1 uuid=-
```

结论：

- `write=1` 证明 `native-ufs-claim-writable=1` 已生效；
- SystemOS whole IOMedia 仍正确发布；
- APFS Container/Volume 仍然没有出现；
- “IOMedia 公布只读属性”可以被排除为唯一阻塞点；
- 如果 APFS 仍尝试写，后续写 I/O 仍会在
  `NativeUfsBlock::doAsyncReadWrite()` 返回 `kIOReturnNotWritable`，
  但这一层当前仍是静默拒绝，需要另行统计。

## 已定位的直接根因：Apple 前缀跳过过宽

文件：

```text
UniXNU-dipper/patches/xnu/zzzz-iokit-candidate-log.patch
```

原条件：

```cpp
if (PE_parse_boot_argn("iokit-skip-apple-cpu", ...) &&
    !strncmp(symbol->getCStringNoCopy(), "Apple", 5)) {
    boot_progress_stage("candidate skipped ...");
    continue;
}
```

启动配置包含：

```text
iokit-skip-apple-cpu=1
```

因此所有以 `Apple` 开头的 IOClass 都会在 alloc 前被跳过，包括：

```text
AppleAPFSContainerScheme
```

这也解释了为什么 APFS kext、personality 和类都正常，但 scheme 从没有
可观测实例。

引入位置：

```text
e9ffdb0 diag(iokit): exclude all Apple CPU candidates on dipper
```

该提交把原本精确的 `AppleARMCPU` 条件扩大成了所有 `"Apple"` 前缀。

本地修复已经完成并通过 `git apply --check`：只跳过以下两个确切的
CPU IOClass：

```text
AppleARMCPU
AppleCPU
```

其他 Apple 驱动不再受影响。

状态：

```text
修复提交                    3ef373f
CI 运行                     41151328380
CI 结果                     success
修复检测                    CI checkout 3ef373f970932ca91c8a534899b2ece3362245f7
原始 kernel SHA-256         05e89d1b74ba9b72e91898e307468789664847e1dde3b4cc088e59225c1de469
测试镜像                    cache-iokit31cpufix.img
测试镜像 SHA-256            9160a82649debce324f2f849a1a6635bc46b9a57dad2a3e669fed45a4988fc2f
真机测试                    已执行，稳定停在 PE_lockdown_iokit
```

产物：

```text
/root/mi8uefi/toolchain/dipper-kernel-cpufix/prepared/kernel.plain.macho
/root/mi8uefi/toolchain/dipper-kernel-cpufix/prepared/kernel.panic.macho
/root/mi8uefi/toolchain/dipper-kernel-cpufix/prepared/kernel.macho.adt
/root/mi8uefi/toolchain/dipper-kernel-cpufix/prepared/unixnu.toml
/root/mi8uefi/toolchain/dipper-kernel-cpufix/cache-iokit31cpufix.img
```

本轮保持与上一轮相同的一个变量以外条件：

```text
native-ufs-claim-writable=1
iokit-skip-apple-cpu=1
```

只把跳过条件从所有 `"Apple"` 前缀收窄为：

```text
AppleARMCPU
AppleCPU
```

该版本连续两次在 7 秒左右停在：

```text
MAIN PE_lockdown_iokit enter
```

尚未进入 NativeUfs/UFS，因此没有到达 APFS scheme。

结论：只保留两个 CPU 类会放行某个在非 Apple 平台上不安全或会阻塞的
非 APFS Apple 驱动。不能使用这种精确白名单跳过方式。

第二次修复 `f8b889e` 恢复此前能够启动到 UFS 的行为：

```text
继续跳过其他 Apple 类
仅豁免 AppleAPFS* 前缀
```

这样 `AppleAPFSContainerScheme`、`AppleAPFSContainer`、
`AppleAPFSVolume` 和 APFS BSD clients 可以进入匹配，而其他 Apple
候选仍保持原来的稳定路径。

状态：

```text
f8b889e 已推送
新 CI 运行 41155749625 成功
内核 SHA-256 ece9996a42ec46d74ff9032121fcb1657eeeec69fb88d25fd7d57070e9475ed0
测试镜像 cache-iokit32apfsallow.img
镜像 SHA-256 34c2f9bb11b40c2b8dcd040135a87b574fc76aad92b68de548da38ed600247a6
真机镜像已写入并 boot
真机结果待屏幕 media report
```

APFS 白名单测试镜像参数：

```text
sheng-build-tag=iokit32apfsallow
native-ufs-claim-writable=1
iokit-skip-apple-cpu=1
```

## APFS 全链路成功（2026-10-31）

`iokit32apfsallow` 真机结果：

```text
NATIVE_UFS media report: published=2 reads=304 failures=0
media=2 bsdClients=7 rootOwner=1 id=100000115 skips=3
APFS load=0 registered=0 vfsops=0 found=1 loaded=0/1 started=0/1
tag=18 personalities=7 classes=d refresh=2 mediaFound=2
```

已实际创建：

```text
AppleAPFSContainerScheme
AppleAPFSContainer
AppleAPFSMedia
AppleAPFSVolume
AppleAPFSMediaBSDClient / APFS volume BSD clients
```

SystemOS：

```text
disk1:
  class=AppleAPFSMedia
  size=c4e00000
  block=4096
  content hint=EF57347C-0000-11AA-AA11-00306543ECAC
  container UUID=E886F82C-EE56-4B52-AF5F-E70BF46320C4

disk1s1:
  class=AppleAPFSVolume
  size=c4e00000
  block=4096
  whole=0 leaf=1
  UUID=8846FE78-2910-4297-AF97-D59BB6F8FC16
```

OS：

```text
disk3:
  class=AppleAPFSMedia
  size=16a400000
  container UUID=50F8A67F-AFC1-4A1B-BCB-B82162674C67F

disk3s1:
  class=AppleAPFSVolume
  size=16a400000
  UUID=04769D26-2CA0-4825-9443-E2E5F88300F2
```

这说明：

- APFS personality、probe、start、Container 和 Volume 全链路已打通；
- `AppleAPFS*` 白名单修复正确；
- `published=2`、`mediaFound=2`；
- 读请求从 4 增至 304，仍无失败；
- 当前 media report 是 `native-ufs-probe=12` 诊断模式的预期 panic。

## 下一测试：进入 root stage

已生成不带 `native-ufs-probe=12` 的 root-stage 镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfswhitelist/cache-iokit33apfsroot.img
```

参数：

```text
sheng-build-tag=iokit33apfsroot
native-ufs-root=1
native-ufs-claim-writable=1
iokit-skip-apple-cpu=1
```

SHA-256：

```text
cache-iokit33apfsroot.img
260702f352143eda8b4fc388698b3c78fb30e826cdd8363b36d6c9ce2bc441cb

unixnu-root.toml
d74e618ddebb4b8a17e8816ed66f40f2c5784f1b12c3ef0b00af556893e6b816
```

下一目标是越过：

```text
SystemOS signed volume not found
stage=media publication
```

进入后续 VFS mountroot。

## Root stage 真机结果（2026-10-31）

`iokit33apfsroot` 已越过 APFS 发布并成功执行：

```text
SystemOS mount enter
SystemOS mount returned
Data mount enter
Data mount returned
```

随后失败：

```text
offline scratch Data mount failed: 2 @kern_exec.cpp:3038
```

已经定位错误来源：

```text
native_owner_data_prepare()
native_ram_block_volume_seed(...) 返回 false
-> error = ENOENT = 2
```

原因不是 SystemOS、APFS 或 VFS mountroot，而是当前 cache payload 没有：

1. `/EFI/UniXNU/Payloads/scratch-data.img`；
2. ADT `/chosen/memory-map/PackedRAMDisk` 占位项。

## Data seed 注入

使用仓库自带 Linux 生成器从 `data-seed.json` 生成了与当前 kernel 编译值一致
的 Data seed：

```text
volume UUID  6A9E5B18-7DA4-4B67-8A5C-3E2D1190C4F7
container    22AE6363-8479-4E23-9F48-CDB0FB9EFB0B
logical size 16 GiB
packed size  90224 bytes
packed SHA  03fff5cd5cc08da6b2034ad4567ccacb477b282d89e9f00148c4ebd5d3280ee7
```

验证结果：

```text
apfsck -cuw: clean
54 directories match data-seed.json
packed/unpacked read-back identical
role: none
encrypted: false
```

ADT 只增加了 16-byte `PackedRAMDisk` 占位项，其他内容保持原 ADT 字节级
往返一致。

新测试镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfswhitelist/cache-iokit34data.img
```

参数：

```text
sheng-build-tag=iokit34data
native-ufs-root=1
native-ufs-claim-writable=1
iokit-skip-apple-cpu=1
initrd=Payloads/scratch-data.img
```

SHA-256：

```text
cache-iokit34data.img
5d5fd10aa5724c2143f27320375f7f28600c41c8a41ef1026229b8e208fd513b

packed ADT
21bebc08b2190529710c3e94a3a9724e04a099f90a09d00dc2781cb9121b5924

Data seed
03fff5cd5cc08da6b2034ad4567ccacb477b282d89e9f00148c4ebd5d3280ee7
```

## Data 成功，launchd 被 MAC/AMFI 拒绝

`iokit34data` 真机结果：

```text
Data mount enter
Data mount returned
temporary watchdog launch policy enter
launching launchd
target pid=1 init exec load macf_trie enter
target pid=1 init exec load macf_trie returned
target pid=1 launchd exec MAC policy REFUSED
unexpected SIGKILL; namespace 9 code 0x8
```

结论：

- Data seed、PackedRAMDisk 和 Data APFS mount 均已成功；
- 当前失败发生在 launchd 的 MAC/AMFI exec 检查；
- `imgp->ip_mac_return != 0` 来自
  `kauth_proc_label_update_execve()`；
- 手工构建 payload 时漏掉了原始 trust cache sidecar。

## Trust cache payload

从同一 21H461 IPSW 提取并组合了两个原始 v2 trust module：

```text
StaticTrustCache module SHA-256
da88460e6e513438d5307e5c526f76497a74754476a24da29f239f0c90120fc2

SystemTrustCache module SHA-256
d121ba89167e03aafd00fa687c30cd8ebc4ccafd10c1408adb2ba3de12403651

combined bundle SHA-256
7416ca4f8377146626bb284e2133c76bee6cb4e3ee0af5d648b186235a7323a7
```

combined hash 与 board 的允许值一致。ADT 已增加 `TrustCache` 占位项，
payload 已增加 `kernel.macho.trustcache`。

新测试镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfswhitelist/cache-iokit35trust.img
```

SHA-256：

```text
cache-iokit35trust.img
043c32fbcbab0aaf23e1e1bc8f03893acea79fd0a3ef22a15f3841203e96a869

ADT
660b35b122967748bb7ce401c02dfe7199094eff5c3cdc2936aad48ab0f46018

trust cache
7416ca4f8377146626bb284e2133c76bee6cb4e3ee0af5d648b186235a7323a7
```

该镜像保持：

```text
Data seed
PackedRAMDisk
TrustCache
native-ufs-root=1
native-ufs-claim-writable=1
APFS allowlist fix
```

真机状态：待测试 launchd MAC exec 是否放行。

## launchd 已通过 AMFI/MAC（2026-10-31）

`iokit35trust` 真机结果：

```text
Data mount enter
Data mount returned
temporary watchdog launch policy enter
temporary watchdog launch policy ready
target pid=1 init exec load_machfile enter
target pid=1 init exec load_machfile returned
p1 init status=0 sig=0 exit=0 ns=0 sub=0
MAIN bsd_init: bsd_do_post - done
```

关键结论：

- trust cache 修复有效；
- 不再出现 `launchd exec MAC policy REFUSED`；
- launchd 已通过 MAC/AMFI，并完成 `load_machfile`；
- `p1 init status=0` 是 exec 替换旧 proc 时的归零记录，
  不再带安全策略拒绝原因；
- `bsd_init` 已完成后续 `bsd_do_post`；
- 当前测试配置没有启用显示、HID、GPU 和完整 userland profile，
  因而不应期待出现 SpringBoard UI。

当前状态：

```text
SystemOS mount  done
Data mount      done
launchd exec    accepted by MAC policy
bsd_do_post     done
```

下一阶段应切换到接近 `board.toml` 的完整 userland 参数，启用：

```text
offline-mobile-graphics-load=1
offline-mobile-graphics-client=1
offline-legacy-register=1
offline-legacy-phase=4
offline-legacy-gop=1
offline-legacy-pm=1
offline-legacy-validate=1
offline-legacy-transfer=1
offline-legacy-cpu-present=1
offline-hid-input=1
offline-hid-home=1
offline-key-events=1
sheng-aks=1
```

并观察 launchd 是否开始拉起 backboardd、runningboardd、SpringBoard。

## 完整 userland 显示测试

已生成：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfswhitelist/cache-iokit36ui.img
```

该镜像保留已经通过的：

```text
APFS allowlist fix
SystemOS mount
PackedRAMDisk + Data seed
TrustCache bundle
native-ufs-root=1
native-ufs-claim-writable=1
```

新增接近 board.toml 的 userland 参数：

```text
offline-mobile-graphics-load=1
offline-mobile-graphics-client=1
offline-legacy-register=1
offline-legacy-phase=4
offline-legacy-gop=1
offline-legacy-pm=1
offline-legacy-validate=1
offline-legacy-transfer=1
offline-legacy-cpu-present=1
offline-hid-input=1
offline-hid-home=1
offline-key-events=1
sheng-aks=1
sheng-staged-apps=1
```

暂不启用 SMP，保持 `cpus=1`，先隔离多核变量。

SHA-256：

```text
cache-iokit36ui.img
b04fcda523f8f641e8a9c4432105cf9ebf8f47b1e7b84e1f61170b9860fc5eef

unixnu.toml
0df85e1803316a4f24343ee255a5cd9d8b340a23fa173b4dd51c581f77bf36bf
```

真机状态：已写入并 boot，等待用户态显示结果。

`iokit36ui` 实测在 `Data mount enter` 后长时间不返回。差异项是启用了：

```text
sheng-staged-apps=1
```

代码位置：

```text
patches/xnu/zzy-staged-system-apps.patch
```

它在 `native_owner_data_prepare()` 返回前逐文件复制 OS 的
`staged_system_apps` 到 Data，并对每个文件执行 `VNOP_FSYNC`。这会造成
明显启动延迟，且不是进入用户态所必需。

已生成关闭 staged-apps 的对照镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfswhitelist/cache-iokit37ui.img
```

SHA-256：

```text
cache-iokit37ui.img
a1be71a737e44af6212993764fffda0dd1ece185a38070708ff52c1542f8b334

unixnu.toml
e27a0bf1f5dc34c6e071a32bd2857e062260e9b294845dca54a52f9e82b1bd9e
```

其余显示、HID、keybag、APFS、Data 和 trust cache 配置保持不变。

## Apple Logo 已出现（2026-10-31）

`iokit37ui` 真机启动后已经显示 Apple Logo。

这证明显式建立了以下完整链路：

```text
XNU
 -> ArmCPU/GIC/UFS/SMMU
 -> APFS Container/Volume
 -> SystemOS read-only mount
 -> Data APFS mount
 -> trust cache / AMFI
 -> launchd exec
 -> NativeLegacyDisplay
 -> framebuffer present
 -> Apple Logo
```

当前不再停留在纯内核日志页。下一观察点是：

```text
启动进度条是否继续
backboardd / runningboardd / SpringBoard 是否启动
是否出现新的用户态 panic、SIGKILL 或自动重启
```

镜像：

```text
cache-iokit37ui.img
a1be71a737e44af6212993764fffda0dd1ece185a38070708ff52c1542f8b334
```

后续观察：Logo 出现后没有进度条。该现象表明内核和 framebuffer 已完成
接管，但用户态启动服务链尚未产生可见进展。当前配置未启用诊断 banner，
因此需要从 `sheng-log=1` 的 UFS 日志读取 launchd、backboardd、
runningboardd、SpringBoard 的进程快照和退出记录，才能判断是等待某个
launch job、进程崩溃，还是用户态初始化停滞。

长时间等待后，Apple Logo 下方出现启动进度条。这说明：

- launchd 没有卡死在 exec；
- 用户态首次启动/数据迁移流程已经继续推进；
- framebuffer、Data、SystemOS 和基础用户态服务保持工作；
- 当前不属于静态死锁，后续只需观察进度条能否完成。

诊断更新：读取 UFS tail 时没有找到 `SHLOG001`，说明这次路径没有成功
写入持久日志（可能未打开或写入前已停在 Logo）。因此改为启用已经链接进
内核的 BootProgress banner。

`iokit38banner` 只把：

```text
sheng-banner=0
```

改为：

```text
sheng-banner=1
```

用于直接显示：

```text
launchd
backboardd
SpringBoard
PreBoard
runningboardd
lsd
keybagd
cfprefsd
containermanagerd
```

的进程、线程、等待和退出状态。

镜像：

```text
cache-iokit38banner.img
0c4b2b71d45dc147c13a953ff70841009a507d3e509ce62e6634b99e7c5d114f
```

### 用户态运行时快照探针

Apple 系统接管显示后，BootProgress banner 不会覆盖系统 UI；TWRP 对整个
`unixnu` 分区扫描 `SHLOG001` 也没有找到日志头。新增探针提交：

```text
eb960e7 diag(ufs): snapshot userland process state
```

参数：

```text
native-ufs-probe=15
native-ufs-runtime-delay=240
```

真机观察 Apple Logo 需要约 150 秒。因此 60 秒会过早触发，改为
240 秒（Logo 后约 90 秒）主动 panic，显示以下进程的线程、运行、等待、continuation、
用户 PC 和退出统计：

```text
launchd
backboardd
SpringBoard
PreBoard
runningboardd
lsd
keybagd
cfprefsd
containermanagerd
kernel_task
```

CI：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/41162252682>

### 屏幕实时 console 对照

用户观察到其他移植项目在启动期间持续显示日志。当前 UI 配置去掉了
`-v`，DarwinBoot 因此让 NativeLegacyDisplay 接管屏幕，只显示 Apple
Logo/进度条。

已生成只恢复屏幕 console 的对照镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfswhitelist/cache-iokit39console.img
```

参数：

```text
-v
debug=0x14f
startup_debug=1
boot-progress=2
sheng-build-tag=iokit39console
```

其余 APFS/Data/trust/UI 参数保持不变。该测试用于确认内核 console 是否
能在整个启动过程中持续刷新，而不是在 Logo 接管后消失。

SHA-256：

```text
cache-iokit39console.img
fa49acbf214868c716d6d7b5d0f6424b912ecd9aae68b32e87419256c7b0cdc8

unixnu.toml
4929b9686e6b0d5630b10f6727611dee76e3e393d3465bdb28233bd389a9e6ef
```

## 用户进程已大量启动，但内存窗口只有 512 MiB

`iokit39console` 真机启动约 20 秒即开始显示大量用户进程：

```text
target pid=217 UserEventAgent exec process signature accepted
target pid=220 launchd exec load_machfile returned
target pid=220 SpringBoard exec process signature accepted
```

说明 SpringBoard 的用户态镜像已进入执行阶段。约 202 秒时：

```text
Process exits: total=327 nonzero-status-or-reason=152
p213 mediaerver sig=9 ns=1 sub=f
p215 timed        sig=9 ns=1 sub=f
p209 containermanaged sig=9 ns=1 sub=f
p218 xpcproxy     sig=9 ns=1 sub=f
MEM free4K=554  (about 2.1 MiB)
UFS requests=220658, verified=52758 blocks (3780 MiB)
```

内核报告的测试内存窗口只有：

```text
MEM totalMiB=512
```

检查当前使用的 ADT 后发现，`/chosen` 中缺少：

```text
darwinboot-memory-size
```

虽然 ADT 已声明：

```text
dram-base=0x80000000
dram-size=0x180000000
```

但 DarwinBoot 仍使用默认的小窗口。`board.toml` 预期的 4.75 GiB 窗口没有
真正传入。当前 `free4K` 只剩约 2 MiB，大量服务以 `sig=9 ns=1 sub=f`
退出，极可能与该 512 MiB 上限有关。

已生成加入以下属性的对照镜像：

```text
darwinboot-memory-size=0x130000000  (4.75 GiB)
```

SHA-256：

```text
cache-iokit40fullmem.img
b13e2e2c3d94f484925ddd54c993d27ed4d6faae90cdb7d5ec5b1af3df94b164

ADT
92d080c27030f092869ce472d0af940d1ab79813e82441d4da40c2a90b95689b

unixnu.toml
584007b0914302538642511bc7dc151c2846b486959b38ec01e8b691abbd79de
```

### 4.75 GiB 内存窗口真机结果

`iokit40fullmem` 启动后再次出现 Apple Logo，并出现会持续推进的进度条，
不再是之前完全静止的进度条。

这说明 `darwinboot-memory-size=0x130000000` 已改变运行时行为，也进一步
支持此前判断：512 MiB 默认窗口导致用户态服务大量因内存压力退出。

当前状态：

```text
Logo                         已出现
进度条                       已出现并缓慢前进
首次启动数据准备             进行中
```

下一观察点：进度条完成、Hello/Setup、锁屏或新的 panic。

`iokit40fullmem` 运行中，用户态图形启动界面接管 framebuffer 后，`-v`
内核日志消失，但进度条持续缓慢移动。说明日志消失是显示 handoff，不是
系统停止。

已生成只打开 BootProgress live banner 的对照镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-apfswhitelist/cache-iokit41fullmembanner.img
```

SHA-256：

```text
cache-iokit41fullmembanner.img
61071fb76535e7b852a21aabca2a8da6ba8fd90b49bfd0e2722380f46d6d66a5

unixnu.toml
ffea032e2b108b26cc357e6c815b95bb20482d57b1c94b90959aa84f8aa78cf6
```

与 `iokit40fullmem` 的唯一区别：

```text
sheng-banner=0 -> 1
```

后续真机进展：`iokit40fullmem` 的启动进度条持续前进，并已接近完成。
当前已不再是“卡在 Logo”或静态进度条状态。

### scanout 后日志 overlay

用户要求进度条与日志同时显示。原 BootProgress banner 只靠异步轮询绘制，
会被 NativeLegacyDisplay 的系统帧覆盖。

新增提交：

```text
7fa53d8 diag(display): repaint logs after scanout
```

实现：

- `NativeLegacyDisplay::finishTransfer()` 在系统帧复制完成后调用
  `boot_progress_after_scanout()`；
- 该回调立即重绘 BootProgress banner；
- 仍沿用每 2 秒一次的节流，避免显著拖慢启动；
- CPU copy、blank frame 和 GPU completion 三条路径都会经过
  `finishTransfer()`。

CI：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/41170212636>

overlay 内核已下载并打包：

```text
/root/mi8uefi/toolchain/dipper-kernel-overlay/cache-iokit42overlay.img
```

保留：

```text
darwinboot-memory-size=0x130000000
Data seed
TrustCache
APFS allowlist
sheng-banner=1
-v
```

SHA-256：

```text
cache-iokit42overlay.img
99474a616e3a809bd7983465b3dccee690d4698c2c636253be45819191e2875e

kernel.panic.macho
da13963f8d841d58c80f535fb77c51d26b76dab0d7e7093c3bf471a865fbc4ba

unixnu.toml
78049e564016ab181e53633ad0df4d7903fbb827a94c44225d51b456cd2cbc26
```

### overlay 真机结果：成功

`iokit42overlay` 真机在进度条界面持续显示诊断 overlay，确认
`finishTransfer()` 后重绘方案有效。

捕获到 SpringBoard 的真实用户态故障：

```text
pid=6 SpringBoard sig=6
LR=fffffff00b8e2e34
cache slide=+18880000
backtrace:
  1a0838708
  1a2ddc724
  1a2b31a28
  1a2b40f38
  1a2b3b638

SESSION terminating due to uncaught exception of type NSException
```

同时：

```text
p5  powerd          sig=6
p170 ScreenTimeAgent sig=6 ns=2 sub=6
p175 PerfPowerServices sig=6 ns=2 sub=6
p176 ScreenTimeAgent sig=6 ns=2 sub=6
```

用户态栈地址去 slide 后落在：

```text
0x187fb8708  CoreFoundation
0x18a55c724  UIKitCore
0x18a2b1a28  UIKitCore
0x18a2c0f38  UIKitCore
0x18a2bb638  UIKitCore
```

说明当前问题已经从内核/挂载推进到 SpringBoard 的 Objective-C/UIKit
初始化异常。下一步应捕获完整的 `NSException` 名称和 reason，并符号化
UIKitCore 栈。

### 后续 overlay 日志（t≈4565 秒）

进度条与日志同时显示保持正常。新捕获的主要崩溃为 ScreenTimeAgent：

```text
CRASH p337 ScreenTimeAgent sig6
PC=1e4b12f74 LR=1f69acfa4
cache slide=+19898000
REASON terminating due to uncaught exception of type NSException
```

未加 slide 的地址映射：

```text
1cb27af74  FitnessCoaching.framework +0x3df74
1dd114fa4  SiriTTS.framework +0x523fa4
18f138708  GeoServices.framework +0x162c708
1dd033714  SiriTTS.framework +0x442714
1dd024e34  SiriTTS.framework +0x433e34
186f3958c  Foundation.framework +0x11958c
```

同时出现：

```text
PerfPowerServices status=100 sig=0 exit=1
ScreenTimeAgent    status=6 sig=6 ns=2 sub=6
FB: 8 swaps/s, mean copy 144529 us
LOAD: user=9%, sys=90%, idle=0%
UFS requests=238627
```

结论：

- overlay 功能正确；
- SpringBoard/ScreenTime/PerfPower 已进入真实用户态；
- 当前是用户态 Objective-C 异常与性能/功耗服务退出循环；
- 单核和 CPU framebuffer 复制仍让系统极慢，但系统仍在运行。

下一步最小诊断建议：仅禁用非关键的 ScreenTimeAgent 和
PerfPowerServices，观察进度条是否能越过当前阶段。

已生成对照镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-overlay/cache-iokit43notime.img
```

只新增：

```text
sheng-disable-jobs=com.apple.ScreenTimeAgent,com.apple.PerfPowerServices
```

SHA-256：

```text
cache-iokit43notime.img
73af1e863f032085c3536f6caace7a1f4c2f9dfa17220b2678f96e6e8a959eb6

unixnu.toml
69ece699391ce7ce039e21807b21799e298f3e2cb5440f7001dce5fca1cdcebc
```

`iokit43notime` 首次启动没有进入 XNU，点击菜单项后返回菜单。

原因已定位：参数值长度从：

```text
iokit42overlay  979 bytes
iokit43notime  1051 bytes
```

增加到 1051，超过 `UniXNU/scripts/unixnu/payload.py` 明确强制的
1000-byte command-line 上限。手工 mcopy 配置绕过了构建器的检查。

修复镜像删除三个非必要参数：

```text
startup_debug=1
sheng-autoreset=2400
native-ufs-diagnostics=1
```

当前参数为 992 bytes，保留：

```text
sheng-disable-jobs=com.apple.ScreenTimeAgent,com.apple.PerfPowerServices
sheng-banner=1
4.75 GiB memory
Data / TrustCache / APFS fixes
```

修复镜像：

```text
cache-iokit43notime-fixed.img
c8c2d034afca5872a3b311b76d6cadf803cf7b4d96f2052e9003eba98424dd97
```

修复后的真机 overlay：

```text
t=4094s crashes=0 exits=6
```

说明禁用 ScreenTimeAgent 和 PerfPowerServices 成功消除了崩溃循环。

剩余退出：

```text
p153 xpcproxy          status=4e00 exit=78
p78  IOMFB_FIR Loader  status=0100 exit=1
p21  prng_seedctl      status=0100 exit=13
```

解释：

- `xpcproxy exit=78` 通常代表 `EX_CONFIG`，即某个 launch job 配置或前置条件失败；
- `IOMFB_FIR Loader` 退出不影响当前 CPU framebuffer 路径；
- `prng_seedctl` 返回 13（EACCES）说明种子注入被拒绝，但当前没有因此崩溃；
- FB 约 8 swaps/s，单帧平均约 15 ms；
- CPU 为 `user=7% sys=92% idle=0%`，仍是单核高负载；
- 自上次检查后 UFS 无新请求，系统可能停在某个用户态等待/重试环节。

下一步若进度条不移动，需要显示 `xpcproxy` 的 job label，而不是继续猜退出码。

### 全量 kernel console 模式

用户希望看到连续完整日志，而不是 overlay 选中的诊断字段。已生成关闭
NativeLegacyDisplay/系统 UI handoff 的纯 console 对照镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-overlay/cache-iokit45fullconsole.img
```

保留：

```text
-v
debug=0x14f
startup_debug=1
boot-progress=2
syslog
APFS / Data / TrustCache / 4.75 GiB memory
```

移除：

```text
offline-mobile-graphics-*
offline-legacy-*
offline-hid-*
sheng-gpu-addon
```

这样系统不会显示 Logo/进度条，也不会让 Apple UI 覆盖 framebuffer；
`-v` kernel console 可以直接持续滚屏。参数仅 555 bytes。

SHA-256：

```text
cache-iokit45fullconsole.img
74b1ff62e24148a07d92cf0c0c90df8e79249ab86681b7f49609f8a5153442df

unixnu-fullconsole.toml
24614fb50a4a1868ad97ac5760fdf979655607148748000e7f6465e12702e0dd
```

### 滚动 console 分屏

新增 `sheng-split-console=1` 模式：

- 顶部约 2/3 保留 Apple Logo、进度条和系统 UI；
- 底部约 1/3 显示滚动 kernel console；
- console ring 从 14 行扩大到 48 行；
- 默认显示最近 32 行，可用 `sheng-split-lines=<8..48>` 调整；
- 每次 NativeLegacyDisplay 完成 scanout 后，由
  `boot_progress_after_scanout()` 立即重绘底部区域。

提交：

```text
209e523 diag(display): add split scrolling console
```

CI：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/41190636754>

分屏镜像已完成：

```text
/root/mi8uefi/toolchain/dipper-kernel-splitconsole/cache-iokit46split.img
```

配置参数 986 bytes，包含：

```text
sheng-banner=1
sheng-split-console=1
sheng-split-lines=32
sheng-disable-jobs=com.apple.ScreenTimeAgent,com.apple.PerfPowerServices
native-ufs-root=1
4.75 GiB memory ADT
Data seed / TrustCache
```

SHA-256：

```text
cache-iokit46split.img
3bafbb44cf720bd75de8549be183a7995c92e09610f249dec936c83e134c6231

kernel.panic.macho
06ae3180e47eb46a9b4b74815ddf4d3e7749d5a3274260632160763db31c6e82

unixnu.toml
4f68325e5b25c7f9849927fe0ebc9d4b13fe99f3ba07f49f21f00526bf479a21
```

真机验证：

```text
顶部：内核/显示画面继续刷新
底部：UniXNU console [iokit46split] t=24s last=32 lines
```

分屏滚动 console 功能已成功。早期日志中：

```text
zone_init: map VM    3685M
zone_init: map RO    1228M
zone_init: map DATA  4913M
VM bootstrap done: 39 maps, 112 entries and 116 holes left
```

也确认 4.75 GiB/更大 VM 映射已经生效。`gPanicBase is still not initialized`
是当前早期日志中的已知诊断提示，不是本次卡死原因。

真机 overlay 最新结果（约 5329 秒）进一步确认：

```text
MEM totalMiB=4864
free4K=448689  (about 1.7 GiB free)
EXIT 248 nonzero=52 crashsig=46
```

内存窗口修复有效，当前不再是内存不足。

失败进程主要是：

```text
lsd        sig=11  (SIGSEGV) ns=2 sub=b
SpringBoard sig=5  (SIGTRAP) ns=2 sub=5
xpcproxy   exec load_machfile returned
```

`ns=2` 是 `OS_REASON_SIGNAL`。代码签名检查已经通过，因此当前问题位于
lsd/SpringBoard 的用户态运行期，而不是 AMFI 或 exec 权限。

最新看到的 UFS 状态：

```text
verify=9801 ms read=44927 ms
verified=1068423 blocks (4173 MiB)
requests=266205
```

分屏滚动 console 构建：

<https://github.com/hanyaa380/UniXNU-build/actions/runs/41190636754>

## 满进度条运行时快照

当前进度条已满，但时间、FB 和 LOAD 继续变化，说明系统仍在运行，不是
内核硬锁。剩余普通退出包括：

```text
p153 xpcproxy status=4e00 exit=78
p78  IOMFB_FIR Loader exit=1
p21  prng_seedctl exit=13
crashes=0
```

已用现有 overlay 内核生成无需重编译的运行时快照镜像：

```text
/root/mi8uefi/toolchain/dipper-kernel-overlay/cache-iokit44runtime.img
```

参数：

```text
native-ufs-probe=15
native-ufs-runtime-delay=240
sheng-disable-jobs=com.apple.ScreenTimeAgent,com.apple.PerfPowerServices
```

启动约 240 秒后会显示：

- launchd/backboardd/SpringBoard/runningboardd 等进程状态；
- 进程等待对象、continuation、用户 PC；
- `boot_progress_spawn_rows()` 的 XPC job label 和退出统计；
- 内存、UFS 计数。

参数长度：

```text
972 bytes
```

镜像 SHA-256：

```text
f444b8fd44e6d14e5df5782b4036fc3c9e9df5b89f9e035431a85ce27453e296
```

## 下一步

### 实验 A：完成已经准备好的 writable probe

配置：

```text
configs/dipper-io-media-registry.toml
```

当前 HEAD 配置包含：

```text
native-ufs-probe=12
native-ufs-claim-writable=1
offline-apfs-load=1
```

预期：

- 若参数生效，media report 应从 `write=0` 变成 `write=1`；
- 如果 APFS Container/Volume 仍不出现，可排除“公开只读属性是唯一阻塞”；
- 如果出现，仍需 probe/start 日志确认具体路径。

注意：该实验只是对照，不要把 `claim-writable=1` 当成生产修复。底层仍是只读
UFS window，真正的写请求仍会被 `NativeUfsBlock` 拒绝。

### 实验 B：增加定向 APFS 生命周期诊断

按 `/root/mi8uefi/dp.txt` 的方案：

1. 在 `IOService::probeCandidates()` 记录 APFS scheme 候选；
2. 在 `IOService::startCandidate()` 记录 start 返回值；
3. 在 `IOMedia::handleOpen()` 记录 reader/writer 权限；
4. 在 `NativeUfsBlock::doAsyncReadWrite()` 增加有界 I/O ring；
5. 输出 SystemOS 子树的 class、provider、registry ID 和 children；
6. 延时复查 APFS VFS 注册。

这一步才能区分：

- personality 根本没进入 probe；
- probe 被调用但返回 NULL；
- start 失败；
- APFS 请求 writer open 被拒；
- start 成功但闭源 APFS 内部没有创建 Container；
- VFS 注册独立失败。

## 不要执行

- 不要因为当前日志就重新分区或扩大 SystemOS/OS；
- 不要修改 GPT；
- 不要关闭 SMMU；
- 不要关闭 AMFI/Image4 等安全机制来“试探修复”；
- 不要声称 `published=1` 等于 APFS 已挂载；
- 不要声称 `failures=0` 等于写路径正常；
- 不要把 `refresh=1` 或 `mediaFound=1` 当成 APFS 匹配成功；
- 不要在未取得 APFS probe/start 证据前重写闭源 APFS 行为；
- 不要分发 Apple kernelcache、ADT、IPSW、根卷或用户数据。

## 文件边界与安全

- 测试镜像通过 `fastboot boot` 临时启动；
- 最近测试只涉及临时 cache 诊断镜像；
- 原 cache 分区有约 256 MiB 备份；
- 未写 boot、userdata、unixnu 或 Android system；
- 公开仓库不包含 Apple 二进制内容；
- 私有 CI 输入仓库 `hanyaa380/dipper-private-inputs` 必须保持私有。
