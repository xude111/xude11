# 生成 16x16 汉字点阵字库（从 Windows 宋体 16px 点阵提取）
# 用法：powershell -ExecutionPolicy Bypass -File tools\gen_font_cn.ps1
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$charsPath = Join-Path $PSScriptRoot 'chars_cn.txt'
$outC = Join-Path $root 'Core\Src\font_cn.c'
$outH = Join-Path $root 'Core\Inc\font_cn.h'
$outPreview = Join-Path $PSScriptRoot 'font_cn_preview.txt'

Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public class HzRender {
    [DllImport("gdi32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr CreateFontW(int h,int w,int esc,int ori,int weight,uint italic,uint underline,uint strike,uint charset,uint outp,uint clipp,uint quality,uint pitch,string face);
    [DllImport("gdi32.dll")] public static extern IntPtr SelectObject(IntPtr hdc, IntPtr obj);
    [DllImport("gdi32.dll")] public static extern bool DeleteObject(IntPtr obj);
    [DllImport("gdi32.dll")] public static extern bool DeleteDC(IntPtr hdc);
    [DllImport("gdi32.dll")] public static extern uint SetTextColor(IntPtr hdc, uint color);
    [DllImport("gdi32.dll")] public static extern uint SetBkColor(IntPtr hdc, uint color);
    [DllImport("gdi32.dll")] public static extern int SetBkMode(IntPtr hdc, int mode);
    [DllImport("gdi32.dll", CharSet=CharSet.Unicode)] public static extern bool TextOutW(IntPtr hdc,int x,int y,string s,int len);
    [DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleDC(IntPtr hdc);
    [DllImport("gdi32.dll")] public static extern IntPtr CreateDIBSection(IntPtr hdc, ref BITMAPINFO bmi, uint usage, out IntPtr bits, IntPtr hSection, uint offset);
    [StructLayout(LayoutKind.Sequential)] public struct BITMAPINFOHEADER {
        public uint biSize; public int biWidth; public int biHeight; public ushort biPlanes; public ushort biBitCount;
        public uint biCompression; public uint biSizeImage; public int biXPelsPerMeter; public int biYPelsPerMeter;
        public uint biClrUsed; public uint biClrImportant;
    }
    [StructLayout(LayoutKind.Sequential)] public struct BITMAPINFO { public BITMAPINFOHEADER bmiHeader; public uint bmiColors; }

    public static byte[][] RenderOne(string ch, string face, int quality) {
        int w = 16, h = 16;
        IntPtr screen = CreateCompatibleDC(IntPtr.Zero);
        BITMAPINFO bmi = new BITMAPINFO();
        bmi.bmiHeader.biSize = (uint)Marshal.SizeOf(typeof(BITMAPINFOHEADER));
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = 0;
        IntPtr bits;
        IntPtr dib = CreateDIBSection(screen, ref bmi, 0, out bits, IntPtr.Zero, 0);
        IntPtr old = SelectObject(screen, dib);
        byte[] zero = new byte[w*h*4];
        Marshal.Copy(zero, 0, bits, zero.Length);
        IntPtr font = CreateFontW(-16, 0, 0, 0, 400, 0, 0, 0, 134, 0, 0, (uint)quality, 0, face);
        IntPtr oldFont = SelectObject(screen, font);
        SetBkMode(screen, 1);
        SetTextColor(screen, 0x00FFFFFF);
        SetBkColor(screen, 0);
        TextOutW(screen, 0, 0, ch, 1);
        byte[][] result = new byte[16][];
        for (int y = 0; y < 16; y++) {
            result[y] = new byte[16];
            for (int x = 0; x < 16; x++) {
                int off = (y*w + x)*4;
                int b = Marshal.ReadByte(bits, off);
                int g = Marshal.ReadByte(bits, off+1);
                int r = Marshal.ReadByte(bits, off+2);
                result[y][x] = (byte)(((r+g+b)/3) > 96 ? 1 : 0);
            }
        }
        SelectObject(screen, oldFont);
        DeleteObject(font);
        SelectObject(screen, old);
        DeleteObject(dib);
        DeleteDC(screen);
        return result;
    }
}
"@ -IgnoreWarnings

$text = [IO.File]::ReadAllText($charsPath, [Text.Encoding]::UTF8)
$seen = @{}
$chars = New-Object System.Collections.Generic.List[char]
foreach ($ch in $text.ToCharArray()) {
    if ([int]$ch -lt 32) { continue }
    $key = [int]$ch
    if (-not $seen.ContainsKey($key)) {
        $seen[$key] = $true
        $chars.Add($ch)
    }
}
if ($chars.Count -eq 0) { throw 'chars_cn.txt 中没有字符' }

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('/*')
[void]$sb.AppendLine(' * font_cn.c - 16x16 汉字点阵字库（自动生成，请勿手工修改）')
[void]$sb.AppendLine(' * 生成工具: tools/gen_font_cn.ps1')
[void]$sb.AppendLine(' * 字形来源: Windows 宋体 16px 点阵，按 16x16 单元提取')
[void]$sb.AppendLine(' * 编码方式: 每行 2 字节，高位在前（MSB first），共 32 字节/字')
[void]$sb.AppendLine(' */')
[void]$sb.AppendLine('#include "font_cn.h"')
[void]$sb.AppendLine('')

$preview = New-Object System.Text.StringBuilder
$tableLines = New-Object System.Collections.Generic.List[string]

for ($i = 0; $i -lt $chars.Count; $i++) {
    $ch = $chars[$i]
    $code = [int]$ch
    $name = 'cn_{0:X4}' -f $code
    $glyph = [HzRender]::RenderOne([string]$ch, 'SimSun', 3)
    $bytes = New-Object System.Collections.Generic.List[byte]
    [void]$preview.AppendLine(('=== {0} U+{1:X4} ===' -f $ch, $code))
    for ($y = 0; $y -lt 16; $y++) {
        $b1 = 0; $b2 = 0
        $line = ''
        for ($x = 0; $x -lt 16; $x++) {
            if ($glyph[$y][$x]) {
                $line += '#'
                if ($x -lt 8) { $b1 = $b1 -bor (1 -shl (7 - $x)) }
                else { $b2 = $b2 -bor (1 -shl (15 - $x)) }
            } else {
                $line += '.'
            }
        }
        $bytes.Add([byte]$b1)
        $bytes.Add([byte]$b2)
        [void]$preview.AppendLine($line)
    }
    [void]$preview.AppendLine(('HEX: ' + (($bytes | ForEach-Object { '0x{0:X2}' -f $_ }) -join ',')))
    [void]$preview.AppendLine('')

    [void]$sb.AppendLine(('static const uint8_t {0}[32] = {{' -f $name))
    for ($y = 0; $y -lt 16; $y++) {
        $b1 = $bytes[$y*2]; $b2 = $bytes[$y*2+1]
        $comma = if ($y -lt 15) { ',' } else { '' }
        [void]$sb.AppendLine(('  0x{0:X2},0x{1:X2}{2}' -f $b1, $b2, $comma))
    }
    [void]$sb.AppendLine('};')
    [void]$sb.AppendLine('')
    $tableLines.Add(('  {{ 0x{0:X4}, {1} }},' -f $code, $name))
}

[void]$sb.AppendLine('typedef struct')
[void]$sb.AppendLine('{')
[void]$sb.AppendLine('    uint16_t code;')
[void]$sb.AppendLine('    const uint8_t *bitmap;')
[void]$sb.AppendLine('} FontCN_Glyph;')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('static const FontCN_Glyph font_cn_table[] =')
[void]$sb.AppendLine('{')
foreach ($line in $tableLines) { [void]$sb.AppendLine($line) }
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#define FONT_CN_COUNT (sizeof(font_cn_table) / sizeof(font_cn_table[0]))')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('const uint8_t *FontCN_Get(uint16_t code)')
[void]$sb.AppendLine('{')
[void]$sb.AppendLine('    uint16_t i;')
[void]$sb.AppendLine('    for (i = 0; i < FONT_CN_COUNT; i++)')
[void]$sb.AppendLine('    {')
[void]$sb.AppendLine('        if (font_cn_table[i].code == code)')
[void]$sb.AppendLine('        {')
[void]$sb.AppendLine('            return font_cn_table[i].bitmap;')
[void]$sb.AppendLine('        }')
[void]$sb.AppendLine('    }')
[void]$sb.AppendLine('    return 0;')
[void]$sb.AppendLine('}')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('uint16_t FontCN_Count(void)')
[void]$sb.AppendLine('{')
[void]$sb.AppendLine('    return (uint16_t)FONT_CN_COUNT;')
[void]$sb.AppendLine('}')
[void]$sb.AppendLine('')

$hdr = @"
/*
 * font_cn.h - 16x16 汉字点阵字库接口
 */
#ifndef FONT_CN_H
#define FONT_CN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 按 Unicode 码点查找 16x16 点阵，未找到返回 0 */
const uint8_t *FontCN_Get(uint16_t code);

/* 字库中汉字数量 */
uint16_t FontCN_Count(void);

#ifdef __cplusplus
}
#endif

#endif /* FONT_CN_H */
"@

[IO.File]::WriteAllText($outC, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
[IO.File]::WriteAllText($outH, $hdr, (New-Object System.Text.UTF8Encoding($false)))
[IO.File]::WriteAllText($outPreview, $preview.ToString(), (New-Object System.Text.UTF8Encoding($false)))
Write-Host ("生成 {0} 个汉字点阵 -> {1}" -f $chars.Count, $outC)
