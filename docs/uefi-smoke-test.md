# UEFI 冒烟测试

## 目标

在接触 XNU 之前，证明固件可以：

1. 枚举块设备和 FAT 卷；
2. 读取标准路径；
3. 执行 AArch64 EFI 应用；
4. 通过 BDS 自动启动该应用。

## 建议的 ESP 文件结构

```text
ESP/
  EFI/
    BOOT/
      BOOTAA64.EFI
    UniXNU/
      unixnu.toml
      kernel.macho
      kernel.macho.adt
```

第一阶段只放 `BOOTAA64.EFI`。`kernel.macho` 和 ADT 等 UEFI 冒烟测试通过后再加入。

## Hello EFI

`examples/HelloEfi` 和 `examples/EfiProbe` 是 AArch64 EDK2 应用源码。

- `HelloEfi` 只验证固件能执行应用。
- `EfiProbe` 枚举 Simple File System 句柄，并尝试打开
  `\EFI\BOOT\BOOTAA64.EFI`。

将编译出的 `.efi` 复制到 ESP 的标准路径：

```text
\EFI\BOOT\BOOTAA64.EFI
```

## 真机步骤

1. 使用 `fastboot boot` 启动包含 Mu-Silicium 的临时 boot 镜像。
2. 在 UEFI Shell 中执行 `map -r`。
3. 进入目标 FAT 卷并列出 `\EFI\BOOT`。
4. 手动执行 `BOOTAA64.EFI`。
5. 返回 Shell 后重启，检查 BDS 是否自动执行。

## 通过标准

以下证据必须同时存在：

```text
map -r 输出中存在 FAT 卷
ls \EFI\BOOT 输出中存在 BOOTAA64.EFI
手动执行 BOOTAA64.EFI 成功
bcfg boot dump 中存在对应启动项
重启后该程序可自动执行
```

通过后再进入 UniXNU 配置、DarwinBoot 和 XNU 阶段。
