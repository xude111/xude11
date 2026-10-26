# 从 Adafruit GFX 经典 5x7 字库生成 STM32 用的 font5x7.c
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$src = 'C:\Users\12611\AppData\Local\Temp\opencode\glcdfont.c'
if (-not (Test-Path $src)) { throw "缺少 $src" }
$text = [IO.File]::ReadAllText($src)
$matches = [regex]::Matches($text, '0x([0-9A-Fa-f]{2})')
$bytes = New-Object System.Collections.Generic.List[byte]
foreach ($m in $matches) { $bytes.Add([Convert]::ToByte($m.Groups[1].Value, 16)) }
if ($bytes.Count -lt 256*5) { throw "字库字节数不足: $($bytes.Count)" }
$start = 0x20 * 5
$count = (0x7E - 0x20 + 1)
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('/*')
[void]$sb.AppendLine(' * font5x7.c - 经典 5x7 ASCII 点阵字库')
[void]$sb.AppendLine(' * 数据来源: Adafruit GFX glcdfont.c (BSD License)')
[void]$sb.AppendLine(' * 每个字符 5 列，每列 1 字节，低位在上；显示时右侧补 1 列空白')
[void]$sb.AppendLine(' */')
[void]$sb.AppendLine('#include "font5x7.h"')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('static const uint8_t font5x7_data[95][5] =')
[void]$sb.AppendLine('{')
for ($i = 0; $i -lt $count; $i++) {
    $idx = $start + $i*5
    $vals = @($bytes[$idx], $bytes[$idx+1], $bytes[$idx+2], $bytes[$idx+3], $bytes[$idx+4])
    $hex = ($vals | ForEach-Object { '0x{0:X2}' -f $_ }) -join ', '
    $ch = [char](0x20 + $i)
    $comment = if ($ch -eq '\') { 'backslash' } elseif ($ch -eq '"') { 'quote' } else { [string]$ch }
    $comma = if ($i -lt $count-1) { ',' } else { '' }
    [void]$sb.AppendLine(('    {{ {0} }}{1} /* 0x{2:X2} {3} */' -f $hex, $comma, (0x20+$i), $comment))
}
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('const uint8_t *Font5x7_Get(char c)')
[void]$sb.AppendLine('{')
[void]$sb.AppendLine('    uint8_t u = (uint8_t)c;')
[void]$sb.AppendLine('    if (u < 0x20 || u > 0x7E)')
[void]$sb.AppendLine('    {')
[void]$sb.AppendLine('        u = 0x20;')
[void]$sb.AppendLine('    }')
[void]$sb.AppendLine('    return font5x7_data[u - 0x20];')
[void]$sb.AppendLine('}')
$hdr = @"
/*
 * font5x7.h - 5x7 ASCII 点阵字库接口
 */
#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 返回 5 列点阵指针；不可打印字符按空格处理 */
const uint8_t *Font5x7_Get(char c);

#ifdef __cplusplus
}
#endif

#endif /* FONT5X7_H */
"@
[IO.File]::WriteAllText((Join-Path $root 'Core\Src\font5x7.c'), $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
[IO.File]::WriteAllText((Join-Path $root 'Core\Inc\font5x7.h'), $hdr, (New-Object System.Text.UTF8Encoding($false)))
Write-Host ("生成 5x7 字库: {0} 个字符" -f $count)
