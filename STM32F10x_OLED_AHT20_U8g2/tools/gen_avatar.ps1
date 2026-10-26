# 生成 56x56 两帧机器人头像和 16x16 温湿度图标
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Add-Type -AssemblyName System.Drawing
$S = 4
$W = 56; $H = 56
$bmpW = $W*$S; $bmpH = $H*$S

function New-Canvas {
    $bmp = New-Object System.Drawing.Bitmap $bmpW, $bmpH
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Black)
    $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::White), (2*$S)
    $brush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
    $black = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::Black)
    return @{bmp=$bmp; g=$g; pen=$pen; brush=$brush; black=$black}
}

function Draw-Robot([int]$frame) {
    $c = New-Canvas
    $g = $c.g; $pen = $c.pen; $brush = $c.brush; $black = $c.black
    # 天线
    $g.DrawLine($pen, 28*$S, 12*$S, 28*$S, 7*$S)
    if ($frame -eq 0) {
        $g.FillEllipse($brush, 25*$S, 4*$S, 6*$S, 6*$S)
    } else {
        $g.DrawEllipse($pen, 25*$S, 4*$S, 6*$S, 6*$S)
    }
    # 耳朵
    $g.FillRectangle($brush, 3*$S, 25*$S, 4*$S, 10*$S)
    $g.FillRectangle($brush, 49*$S, 25*$S, 4*$S, 10*$S)
    # 头部圆角矩形
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $r = 8*$S
    $x = 7*$S; $y = 13*$S; $w = 42*$S; $h = 35*$S
    $path.AddArc($x, $y, 2*$r, 2*$r, 180, 90)
    $path.AddArc($x+$w-2*$r, $y, 2*$r, 2*$r, 270, 90)
    $path.AddArc($x+$w-2*$r, $y+$h-2*$r, 2*$r, 2*$r, 0, 90)
    $path.AddArc($x, $y+$h-2*$r, 2*$r, 2*$r, 90, 90)
    $path.CloseFigure()
    $g.DrawPath($pen, $path)
    # 眼睛
    if ($frame -eq 0) {
        $g.FillEllipse($brush, 17*$S, 25*$S, 8*$S, 9*$S)
        $g.FillEllipse($brush, 31*$S, 25*$S, 8*$S, 9*$S)
        $g.FillEllipse($black, 20*$S, 28*$S, 3*$S, 4*$S)
        $g.FillEllipse($black, 34*$S, 28*$S, 3*$S, 4*$S)
    } else {
        # 眯眼笑
        $g.DrawArc($pen, 16*$S, 25*$S, 10*$S, 8*$S, 200, 140)
        $g.DrawArc($pen, 30*$S, 25*$S, 10*$S, 8*$S, 200, 140)
    }
    # 嘴
    if ($frame -eq 0) {
        $g.DrawArc($pen, 21*$S, 34*$S, 14*$S, 10*$S, 15, 150)
    } else {
        $g.FillEllipse($brush, 23*$S, 36*$S, 10*$S, 6*$S)
        $g.FillEllipse($black, 24*$S, 38*$S, 8*$S, 2*$S)
    }
    # 腮红/装饰点
    $g.FillEllipse($brush, 12*$S, 34*$S, 3*$S, 3*$S)
    $g.FillEllipse($brush, 41*$S, 34*$S, 3*$S, 3*$S)
    return $c
}

function Downsample([System.Drawing.Bitmap]$bmp) {
    $out = New-Object System.Drawing.Bitmap $W, $H
    for ($y=0; $y -lt $H; $y++) {
        for ($x=0; $x -lt $W; $x++) {
            $sum = 0
            for ($dy=0; $dy -lt $S; $dy++) {
                for ($dx=0; $dx -lt $S; $dx++) {
                    $p = $bmp.GetPixel($x*$S+$dx, $y*$S+$dy)
                    $sum += ($p.R + $p.G + $p.B) / 3
                }
            }
            $avg = $sum / ($S*$S)
            if ($avg -gt 80) { $out.SetPixel($x,$y,[System.Drawing.Color]::White) }
            else { $out.SetPixel($x,$y,[System.Drawing.Color]::Black) }
        }
    }
    return $out
}

function Bitmap-ToRows([System.Drawing.Bitmap]$bmp, [int]$w, [int]$h) {
    $bytes = New-Object System.Collections.Generic.List[byte]
    for ($y=0; $y -lt $h; $y++) {
        $row = New-Object byte[] ([math]::Ceiling($w/8.0))
        for ($x=0; $x -lt $w; $x++) {
            $p = $bmp.GetPixel($x,$y)
            if (($p.R + $p.G + $p.B) -gt 384) {
                $row[[math]::Floor($x/8)] = $row[[math]::Floor($x/8)] -bor (1 -shl (7 - ($x % 8)))
            }
        }
        foreach ($b in $row) { $bytes.Add($b) }
    }
    return $bytes
}

function Rows-To-Icon([string[]]$rows) {
    $bmp = New-Object System.Drawing.Bitmap 16,16
    for ($y=0; $y -lt 16; $y++) {
        for ($x=0; $x -lt 16; $x++) {
            if ($rows[$y][$x] -eq '1') {
                $bmp.SetPixel($x,$y,[System.Drawing.Color]::White)
            } else {
                $bmp.SetPixel($x,$y,[System.Drawing.Color]::Black)
            }
        }
    }
    return $bmp
}

function Draw-IconTemp {
    # 16x16 温度计：细管 + 实心球泡 + 右侧刻度
    $rows = @(
        '0000000000000000',
        '0000000110000000',
        '0000000110000000',
        '0000000110100000',
        '0000000110000000',
        '0000000110100000',
        '0000000110000000',
        '0000000110100000',
        '0000000110000000',
        '0000011111100000',
        '0000111111110000',
        '0000111111110000',
        '0000111111110000',
        '0000111111110000',
        '0000011111100000',
        '0000001111000000'
    )
    return Rows-To-Icon $rows
}

function Draw-IconHumi {
    # 16x16 水滴：尖顶 + 圆底
    $rows = @(
        '0000000000000000',
        '0000000100000000',
        '0000000100000000',
        '0000001110000000',
        '0000011111000000',
        '0000011111000000',
        '0000111111100000',
        '0000111111100000',
        '0001111111110000',
        '0001111111110000',
        '0001111111110000',
        '0001111111110000',
        '0000111111100000',
        '0000011111000000',
        '0000001110000000',
        '0000000000000000'
    )
    return Rows-To-Icon $rows
}

$bmp0 = Draw-Robot 0; $bmp1 = Draw-Robot 1
$d0 = Downsample $bmp0.bmp; $d1 = Downsample $bmp1.bmp
$b0 = Bitmap-ToRows $d0 56 56
$b1 = Bitmap-ToRows $d1 56 56
$it = Draw-IconTemp; $ih = Draw-IconHumi
$bt = Bitmap-ToRows $it 16 16
$bh = Bitmap-ToRows $ih 16 16

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('/*')
[void]$sb.AppendLine(' * avatar.c - 56x56 两帧机器人头像 + 16x16 温湿度图标')
[void]$sb.AppendLine(' * 自动生成: tools/gen_avatar.ps1')
[void]$sb.AppendLine(' * 每行按字节对齐，高位在左（MSB first）')
[void]$sb.AppendLine(' */')
[void]$sb.AppendLine('#include "avatar.h"')
[void]$sb.AppendLine('')
function Emit-Array($name, $bytes, $perLine) {
    [void]$sb.AppendLine(("const uint8_t {0}[{1}] = {{" -f $name, $bytes.Count))
    for ($i=0; $i -lt $bytes.Count; $i+=$perLine) {
        $end = [Math]::Min($i+$perLine-1, $bytes.Count-1)
        $vals = @()
        for ($j=$i; $j -le $end; $j++) { $vals += ('0x{0:X2}' -f $bytes[$j]) }
        $comma = if ($end -lt $bytes.Count-1) { ',' } else { '' }
        [void]$sb.AppendLine(('    ' + ($vals -join ',') + $comma))
    }
    [void]$sb.AppendLine('};')
    [void]$sb.AppendLine('')
}
Emit-Array 'avatar_frame0' $b0 14
Emit-Array 'avatar_frame1' $b1 14
Emit-Array 'icon_temp' $bt 16
Emit-Array 'icon_humi' $bh 16
$hdr = @"
/*
 * avatar.h - 动画头像与图标点阵
 */
#ifndef AVATAR_H
#define AVATAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t avatar_frame0[392];
extern const uint8_t avatar_frame1[392];
extern const uint8_t icon_temp[32];
extern const uint8_t icon_humi[32];

#ifdef __cplusplus
}
#endif

#endif /* AVATAR_H */
"@
[IO.File]::WriteAllText((Join-Path $root 'Core\Src\avatar.c'), $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
[IO.File]::WriteAllText((Join-Path $root 'Core\Inc\avatar.h'), ($hdr + "`n"), (New-Object System.Text.UTF8Encoding($false)))

# 生成预览图（8 倍放大，并排两帧）
$preview = New-Object System.Drawing.Bitmap (2*56*8+30), (56*8+30)
$pg = [System.Drawing.Graphics]::FromImage($preview)
$pg.Clear([System.Drawing.Color]::FromArgb(24,24,28))
$pg.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$pg.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$pg.DrawImage($d0, 10, 15, 56*8, 56*8)
$pg.DrawImage($d1, 10+56*8+10, 15, 56*8, 56*8)
$pg.Dispose()
$preview.Save((Join-Path $root 'docs\images\avatar_preview.png'), [System.Drawing.Imaging.ImageFormat]::Png)
$preview.Dispose()
Write-Host "生成头像帧: $($b0.Count) / $($b1.Count) 字节"
