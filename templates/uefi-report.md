# Xiaomi Mi 8 dipper UEFI 测试报告

## 设备

- 型号：
- 代号：
- Bootloader 状态：
- UEFI commit：
- BootShim commit：
- UEFI FD 大小和 SHA-256：
- boot.img 大小和 SHA-256：
- ESP 分区号、类型、文件系统和大小：
- 测试日期：

## 现象

- 三角感叹号是否出现：
- 完整提示文字：
- 出现在手动 Return 后还是自动倒计时后：
- 等待后关机还是重启：

## UEFI Shell

### map -r

```text

```

### ls \EFI\BOOT

```text

```

### bcfg boot dump

```text

```

### 手动执行 BOOTAA64.EFI

```text

```

## 文件

- BOOTAA64.EFI 是否显示在目录中：
- 电脑端 `file` 识别结果：
- 是否包含 Apple kernel/ADT：
- 是否通过来源审查：

## 屏幕照片

- 三角感叹号画面：
- UEFI Shell 画面：
- BOOTAA64.EFI 执行画面：

## 下一步判断

- 属于无 FAT / 无文件 / 非有效 EFI / BDS 未枚举 / 加载器返回中的哪一种：
- 为什么：
- 下一步唯一应做的实验：
- 回退方法：
