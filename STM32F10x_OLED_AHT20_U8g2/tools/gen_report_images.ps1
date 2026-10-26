#requires -Version 5
<#
  gen_report_images.ps1
  用工程里真实的 16x16 汉字点阵、5x7 ASCII 点阵和 56x56 头像数据，
  重绘 OLED 页面效果图，并生成 I2C 波形、接线图和系统框图。
  所有 OLED 页面均为“程序渲染示意图（非实拍）”，真实照片请由作者补拍。
#>
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root 'docs\images'
if (!(Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

# ---------------------------------------------------------------------------
# 颜色 / 字体 / 基础绘图
# ---------------------------------------------------------------------------
function Col([int]$r, [int]$g, [int]$b, [int]$a = 255) {
    return [System.Drawing.Color]::FromArgb($a, $r, $g, $b)
}
function Fnt([float]$size, [string]$style = 'Regular') {
    $fs = [System.Drawing.FontStyle]::Regular
    if ($style -eq 'Bold') { $fs = [System.Drawing.FontStyle]::Bold }
    return New-Object System.Drawing.Font('Microsoft YaHei UI', $size, $fs, [System.Drawing.GraphicsUnit]::Pixel)
}
function Round-Path([System.Drawing.Rectangle]$r, [int]$rad) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $rad * 2
    $p.AddArc($r.X, $r.Y, $d, $d, 180, 90)
    $p.AddArc($r.Right - $d, $r.Y, $d, $d, 270, 90)
    $p.AddArc($r.Right - $d, $r.Bottom - $d, $d, $d, 0, 90)
    $p.AddArc($r.X, $r.Bottom - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}

# ---------------------------------------------------------------------------
# 解析字库与头像
# ---------------------------------------------------------------------------
$script:cnFont = @{}
$cnText = Get-Content -LiteralPath (Join-Path $root 'Core\Src\font_cn.c') -Raw -Encoding UTF8
foreach ($m in [regex]::Matches($cnText, 'static const uint8_t cn_([0-9A-Fa-f]{4})\[32\]\s*=\s*\{(.*?)\};', 'Singleline')) {
    $code = [Convert]::ToInt32($m.Groups[1].Value, 16)
    $bytes = @()
    foreach ($b in [regex]::Matches($m.Groups[2].Value, '0x([0-9A-Fa-f]{2})')) {
        $bytes += [Convert]::ToInt32($b.Groups[1].Value, 16)
    }
    $script:cnFont[$code] = $bytes
}

$script:ascFont = @()
$ascText = Get-Content -LiteralPath (Join-Path $root 'Core\Src\font5x7.c') -Raw -Encoding UTF8
$ascBlock = [regex]::Match($ascText, 'font5x7_data\[95\]\[5\]\s*=\s*\{(.*?)\};', 'Singleline').Groups[1].Value
foreach ($m in [regex]::Matches($ascBlock, '\{\s*((?:0x[0-9A-Fa-f]{2}\s*,\s*){4}0x[0-9A-Fa-f]{2})\s*\}')) {
    $bytes = @()
    foreach ($b in [regex]::Matches($m.Groups[1].Value, '0x([0-9A-Fa-f]{2})')) {
        $bytes += [Convert]::ToInt32($b.Groups[1].Value, 16)
    }
    $script:ascFont += ,$bytes
}

function Get-Avatar([string]$name) {
    $txt = Get-Content -LiteralPath (Join-Path $root 'Core\Src\avatar.c') -Raw -Encoding UTF8
    $m = [regex]::Match($txt, 'const uint8_t ' + $name + '\[(\d+)\]\s*=\s*\{(.*?)\};', 'Singleline')
    $bytes = @()
    foreach ($b in [regex]::Matches($m.Groups[2].Value, '0x([0-9A-Fa-f]{2})')) {
        $bytes += [Convert]::ToInt32($b.Groups[1].Value, 16)
    }
    return ,$bytes
}
$script:avatar0 = Get-Avatar 'avatar_frame0'
$script:avatar1 = Get-Avatar 'avatar_frame1'
$script:iconTemp = Get-Avatar 'icon_temp'
$script:iconHumi = Get-Avatar 'icon_humi'

# ---------------------------------------------------------------------------
# 128x64 单色显存绘制
# ---------------------------------------------------------------------------
function New-Screen { return ,(New-Object 'bool[]' 8192) }

$script:clipTop = 0
$script:clipBottom = 63
function Set-Clip([int]$top, [int]$bottom) {
    $script:clipTop = $top
    $script:clipBottom = $bottom
}
function Reset-Clip {
    $script:clipTop = 0
    $script:clipBottom = 63
}

function Set-Px($buf, [int]$x, [int]$y) {
    if ($x -ge 0 -and $x -lt 128 -and $y -ge $script:clipTop -and $y -le $script:clipBottom) { $buf[$y * 128 + $x] = $true }
}
function Draw-HLine($buf, [int]$x, [int]$y, [int]$w) {
    for ($i = 0; $i -lt $w; $i++) { Set-Px $buf ($x + $i) $y }
}
function Draw-VLine($buf, [int]$x, [int]$y, [int]$h) {
    for ($i = 0; $i -lt $h; $i++) { Set-Px $buf $x ($y + $i) }
}
function Draw-Frame($buf, [int]$x, [int]$y, [int]$w, [int]$h) {
    Draw-HLine $buf $x $y $w
    Draw-HLine $buf $x ($y + $h - 1) $w
    Draw-VLine $buf $x $y $h
    Draw-VLine $buf ($x + $w - 1) $y $h
}
function Draw-Circle($buf, [int]$cx, [int]$cy, [int]$r) {
    $x = $r; $y = 0; $err = 0
    while ($x -ge $y) {
        Set-Px $buf ($cx + $x) ($cy + $y); Set-Px $buf ($cx + $y) ($cy + $x)
        Set-Px $buf ($cx - $y) ($cy + $x); Set-Px $buf ($cx - $x) ($cy + $y)
        Set-Px $buf ($cx - $x) ($cy - $y); Set-Px $buf ($cx - $y) ($cy - $x)
        Set-Px $buf ($cx + $y) ($cy - $x); Set-Px $buf ($cx + $x) ($cy - $y)
        $y++
        if ($err -le 0) { $err += 2 * $y + 1 } else { $x--; $err += 2 * ($y - $x) + 1 }
    }
}
function Draw-Disc($buf, [int]$cx, [int]$cy, [int]$r) {
    for ($dy = -$r; $dy -le $r; $dy++) {
        for ($dx = -$r; $dx -le $r; $dx++) {
            if ($dx * $dx + $dy * $dy -le $r * $r) { Set-Px $buf ($cx + $dx) ($cy + $dy) }
        }
    }
}
function Draw-Line($buf, [int]$x0, [int]$y0, [int]$x1, [int]$y1) {
    $dx = [Math]::Abs($x1 - $x0); $sx = if ($x0 -lt $x1) { 1 } else { -1 }
    $dy = -[Math]::Abs($y1 - $y0); $sy = if ($y0 -lt $y1) { 1 } else { -1 }
    $err = $dx + $dy
    while ($true) {
        Set-Px $buf $x0 $y0
        if ($x0 -eq $x1 -and $y0 -eq $y1) { break }
        $e2 = 2 * $err
        if ($e2 -ge $dy) { $err += $dy; $x0 += $sx }
        if ($e2 -le $dx) { $err += $dx; $y0 += $sy }
    }
}
function Draw-Char5x7($buf, [int]$x, [int]$y, [char]$ch, [int]$scale = 1) {
    $idx = [int]$ch - 0x20
    if ($idx -lt 0 -or $idx -gt 94) { $idx = 31 }
    $cols = $script:ascFont[$idx]
    for ($c = 0; $c -lt 5; $c++) {
        $bits = $cols[$c]
        for ($r = 0; $r -lt 7; $r++) {
            if ((($bits -shr $r) -band 1) -ne 0) {
                for ($dy = 0; $dy -lt $scale; $dy++) {
                    for ($dx = 0; $dx -lt $scale; $dx++) {
                        Set-Px $buf ($x + $c * $scale + $dx) ($y + $r * $scale + $dy)
                    }
                }
            }
        }
    }
}
function Draw-String5x7($buf, [int]$x, [int]$y, [string]$s, [int]$scale = 1) {
    foreach ($ch in $s.ToCharArray()) {
        Draw-Char5x7 $buf $x $y $ch $scale
        $x += 6 * $scale
    }
}
function Draw-Glyph16($buf, [int]$x, [int]$y, [int]$code) {
    if (-not $script:cnFont.ContainsKey($code)) {
        Draw-Frame $buf $x $y 16 16
        Draw-Char5x7 $buf ($x + 5) ($y + 5) '?' 1
        return
    }
    $g = $script:cnFont[$code]
    for ($r = 0; $r -lt 16; $r++) {
        $b1 = $g[$r * 2]; $b2 = $g[$r * 2 + 1]
        for ($c = 0; $c -lt 8; $c++) {
            if (($b1 -band (0x80 -shr $c)) -ne 0) { Set-Px $buf ($x + $c) ($y + $r) }
            if (($b2 -band (0x80 -shr $c)) -ne 0) { Set-Px $buf ($x + 8 + $c) ($y + $r) }
        }
    }
}
function Draw-Text16($buf, [int]$x, [int]$y, $codes, [bool]$mixed = $true) {
    foreach ($code in $codes) {
        if ($code -lt 0x80) {
            $ay = if ($mixed) { $y + 5 } else { $y }
            Draw-Char5x7 $buf $x $ay ([char]$code) 1
            $x += 6
        } else {
            Draw-Glyph16 $buf $x $y ([int]$code)
            $x += 16
        }
    }
}
function Draw-Bitmap($buf, [int]$x, [int]$y, [int]$w, [int]$h, $data) {
    $bpr = [int][Math]::Ceiling($w / 8.0)
    for ($r = 0; $r -lt $h; $r++) {
        for ($c = 0; $c -lt $w; $c++) {
            $byte = $data[$r * $bpr + [int][Math]::Floor($c / 8)]
            if (($byte -band (0x80 -shr ($c % 8))) -ne 0) { Set-Px $buf ($x + $c) ($y + $r) }
        }
    }
}
function Draw-PlainText($buf, [int]$x, [int]$y, [string]$s) {
    foreach ($ch in $s.ToCharArray()) {
        if ([int]$ch -lt 0x80) { Draw-Char5x7 $buf $x $y $ch 1; $x += 6 }
        else { Draw-Glyph16 $buf $x $y ([int]$ch); $x += 16 }
    }
}

# ---------------------------------------------------------------------------
# 卡片 / OLED 屏幕组件
# ---------------------------------------------------------------------------
function New-Card([string]$title, [string]$subtitle, [int]$w, [int]$h) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $rect = New-Object System.Drawing.Rectangle(0, 0, $w, $h)
    $bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, (Col 250 251 253), (Col 226 232 240), 90)
    $g.FillRectangle($bg, $rect)
    $dot = New-Object System.Drawing.SolidBrush((Col 148 163 184 45))
    for ($yy = 0; $yy -lt $h; $yy += 22) {
        for ($xx = 0; $xx -lt $w; $xx += 22) { $g.FillRectangle($dot, $xx, $yy, 1, 1) }
    }
    $g.FillRectangle((New-Object System.Drawing.SolidBrush((Col 14 165 233))), 0, 0, $w, 6)
    $g.FillRectangle((New-Object System.Drawing.SolidBrush((Col 14 165 233))), 48, 112, 88, 4)
    $ft = Fnt 30 'Bold'; $fs = Fnt 15
    $g.DrawString($title, $ft, (New-Object System.Drawing.SolidBrush((Col 15 23 42))), 48, 32)
    $g.DrawString($subtitle, $fs, (New-Object System.Drawing.SolidBrush((Col 71 85 105))), 48, 76)
    return @{ Bitmap = $bmp; Graphics = $g; Width = $w; Height = $h }
}

function Add-Oled($card, $buf, [int]$x, [int]$y, [int]$scale, [string]$label = '') {
    $g = $card.Graphics
    $w = 128 * $scale; $h = 64 * $scale
    $shadow = New-Object System.Drawing.SolidBrush((Col 15 23 42 40))
    $g.FillRectangle($shadow, $x + 8, $y + 10, $w + 24, $h + 24)
    $bezel = New-Object System.Drawing.SolidBrush((Col 30 41 59))
    $g.FillRectangle($bezel, $x - 12, $y - 12, $w + 24, $h + 24)
    $screen = New-Object System.Drawing.Bitmap(128, 64)
    for ($py = 0; $py -lt 64; $py++) {
        for ($px = 0; $px -lt 128; $px++) {
            if ($buf[$py * 128 + $px]) { $screen.SetPixel($px, $py, [System.Drawing.Color]::White) }
            else { $screen.SetPixel($px, $py, [System.Drawing.Color]::Black) }
        }
    }
    $old = $g.InterpolationMode
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $dst = New-Object System.Drawing.Rectangle($x, $y, $w, $h)
    $g.DrawImage($screen, $dst, 0, 0, 128, 64, [System.Drawing.GraphicsUnit]::Pixel)
    $g.InterpolationMode = $old
    $g.DrawRectangle((New-Object System.Drawing.Pen((Col 71 85 105), 2)), $x - 12, $y - 12, $w + 24, $h + 24)
    if ($label -ne '') {
        $f = Fnt 14 'Bold'
        $g.DrawString($label, $f, (New-Object System.Drawing.SolidBrush((Col 51 65 85))), $x, ($y + $h + 18))
    }
    $screen.Dispose()
}

function Add-Badge($card, [int]$x, [int]$y, [string]$text, [System.Drawing.Color]$color) {
    $g = $card.Graphics
    $f = Fnt 13 'Bold'
    $sz = $g.MeasureString($text, $f)
    $bw = [int]$sz.Width + 24
    $rect = New-Object System.Drawing.Rectangle($x, $y, $bw, 30)
    $path = Round-Path $rect 12
    $brush = New-Object System.Drawing.SolidBrush((Col $color.R $color.G $color.B 28))
    $g.FillPath($brush, $path)
    $g.DrawPath((New-Object System.Drawing.Pen($color, 1)), $path)
    $g.DrawString($text, $f, (New-Object System.Drawing.SolidBrush($color)), ($x + 12), ($y + 6))
}

function Add-Note($card, [string]$text) {
    $g = $card.Graphics
    $f = Fnt 13
    $g.DrawString($text, $f, (New-Object System.Drawing.SolidBrush((Col 100 116 139))), 48, ($card.Height - 42))
}

function Save-Card($card, [string]$name) {
    $path = Join-Path $outDir $name
    $card.Bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $card.Graphics.Dispose()
    $card.Bitmap.Dispose()
    Write-Host ("[OK] " + $name)
}

# ---------------------------------------------------------------------------
# 页面 1：任务一 学生信息
# ---------------------------------------------------------------------------
function Scene-Task1Info {
    $buf = New-Screen
    Draw-Text16 $buf 32 0 @(0x5B66, 0x751F, 0x4FE1, 0x606F)
    Draw-HLine $buf 0 17 128
    Draw-Text16 $buf 40 21 @(0x5F90, 0x6B63, 0x5FC3)
    Draw-String5x7 $buf 28 39 '632407030612'
    Draw-Text16 $buf 36 47 @(0x901A, 0x4FE1, [int][char]'2', [int][char]'4', [int][char]'0', [int][char]'3')

    $card = New-Card '任务一 · 学生信息页' '自定义 SSD1306 驱动 · 16×16 宋体点阵 · I2C1 400 kHz' 960 600
    Add-Oled $card $buf 224 160 4
    Add-Badge $card 48 470 '128×64 单色 OLED' (Col 14 116 144)
    Add-Badge $card 300 470 'SSD1306  0x3C' (Col 21 128 61)
    Add-Badge $card 540 470 '汉字 16×16 + ASCII 5×7' (Col 180 83 9)
    Add-Note $card '程序渲染示意图（非实拍）：按工程内真实字库逐点重绘，显示内容与烧录后一致。'
    Save-Card $card 'task1_info.png'
}

# ---------------------------------------------------------------------------
# 页面 2：任务一 AHT20 温湿度（效果图）
# ---------------------------------------------------------------------------
function Scene-Task1Sensor {
    $buf = New-Screen
    Draw-Text16 $buf 24 0 @(0x6E29, 0x6E7F, 0x5EA6, 0x68C0, 0x6D4B)
    Draw-HLine $buf 0 17 128
    Draw-Bitmap $buf 6 22 16 16 $script:iconTemp
    Draw-String5x7 $buf 28 23 '25.6 C' 2
    Draw-Bitmap $buf 6 40 16 16 $script:iconHumi
    Draw-String5x7 $buf 28 41 '55.2 %' 2
    Draw-String5x7 $buf 40 57 'AHT20 OK'

    $card = New-Card 'AHT20 温湿度显示效果图' '任务一 · 传感器页面 · 25.6 °C / 55.2 %RH（程序渲染示意）' 1200 700
    Add-Oled $card $buf 80 190 4

    $g = $card.Graphics
    $panel = New-Object System.Drawing.Rectangle(700, 180, 430, 340)
    $path = Round-Path $panel 16
    $g.FillPath((New-Object System.Drawing.SolidBrush((Col 255 255 255 230))), $path)
    $g.DrawPath((New-Object System.Drawing.Pen((Col 203 213 225), 2)), $path)

    $fLabel = Fnt 15; $fBig = Fnt 52 'Bold'; $fUnit = Fnt 20 'Bold'; $fSmall = Fnt 13
    $g.DrawString('温度 TEMPERATURE', $fLabel, (New-Object System.Drawing.SolidBrush((Col 71 85 105))), 735, 210)
    $g.DrawString('25.6', $fBig, (New-Object System.Drawing.SolidBrush((Col 220 38 38))), 730, 236)
    $g.DrawString('°C', $fUnit, (New-Object System.Drawing.SolidBrush((Col 220 38 38))), 930, 268)
    $g.DrawString('湿度 HUMIDITY', $fLabel, (New-Object System.Drawing.SolidBrush((Col 71 85 105))), 735, 330)
    $g.DrawString('55.2', $fBig, (New-Object System.Drawing.SolidBrush((Col 2 132 199))), 730, 356)
    $g.DrawString('%RH', $fUnit, (New-Object System.Drawing.SolidBrush((Col 2 132 199))), 930, 388)

    Add-Badge $card 700 480 'AHT20  0x38' (Col 21 128 61)
    Add-Badge $card 875 480 'I2C1  PB6/PB7' (Col 14 116 144)
    Add-Badge $card 80 550 '与 OLED 共用 I2C1 总线' (Col 180 83 9)
    Add-Note $card '程序渲染示意图（非实拍）：温湿度数值为演示值，实际读数由 AHT20 上电测量得到。'
    Save-Card $card 'task1_sensor.png'
}

# ---------------------------------------------------------------------------
# 页面 3：任务一 长文本软件滚动（三帧胶片）
# ---------------------------------------------------------------------------
$script:longText = @(
    [int][char]'H', [int][char]'e', [int][char]'l', [int][char]'l', [int][char]'o',
    0xFF0C,
    0x6B22, 0x8FCE, 0x6765, 0x5230,
    0x91CD, 0x5E86, 0x4EA4, 0x901A, 0x5927, 0x5B66,
    0x7269, 0x8054, 0x7F51,
    [int][char]'2', [int][char]'0', [int][char]'5',
    0x5B9E, 0x8BAD, 0x5BA4,
    0xFF01
)

function Scene-Task1Scroll {
    $card = New-Card '任务一 · 长文本水平软件滚动' '逐帧清屏 + 显存整体左移 · 55 px/s · 超出屏幕部分由驱动裁剪' 1200 560
    $offsets = @(96, 40, -16)
    $labels = @('t = 0.0 s', 't = 1.0 s', 't = 2.0 s')
    for ($i = 0; $i -lt 3; $i++) {
        $buf = New-Screen
        Draw-Text16 $buf 24 0 @(0x957F, 0x6587, 0x672C, 0x6EDA, 0x52A8)
        Draw-HLine $buf 0 17 128
        Draw-Text16 $buf $offsets[$i] 24 $script:longText
        Draw-HLine $buf 0 44 128
        Draw-String5x7 $buf 16 57 'SOFTWARE SCROLL'
        Add-Oled $card $buf (60 + $i * 370) 180 2 $labels[$i]
    }
    Add-Badge $card 60 480 '文本宽度 336 px' (Col 14 116 144)
    Add-Badge $card 300 480 '滚动行程 464 px' (Col 21 128 61)
    Add-Badge $card 540 480 '标题/分隔线固定' (Col 180 83 9)
    Add-Note $card '程序渲染示意图（非实拍）：三帧按 55 px/s 的滚动位置重绘，展示长文本从右向左进入屏幕。'
    Save-Card $card 'task1_scroll.png'
}

# ---------------------------------------------------------------------------
# 页面 4：任务一 SSD1306 硬件滚动
# ---------------------------------------------------------------------------
function Scene-Task1HwScroll {
    $buf = New-Screen
    Draw-String5x7 $buf 19 0 'HARDWARE SCROLL'
    Draw-String5x7 $buf 8 16 'SSD1306 0123456789'
    Draw-String5x7 $buf 4 26 'HARDWARE SCROLL DEMO'
    Draw-String5x7 $buf 4 36 '<<< LEFT / RIGHT >>>'
    Draw-String5x7 $buf 0 46 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    Draw-String5x7 $buf 10 57 '128x64  I2C 400kHz'

    $card = New-Card '任务一 · SSD1306 硬件滚动' '0x26/0x27 水平滚动 · 0x29/0x2A 对角滚动 · 页 1~6 参与，页 0/7 固定' 1000 620
    Add-Oled $card $buf 244 160 4
    Add-Badge $card 48 490 '0x2F 启动滚动' (Col 21 128 61)
    Add-Badge $card 260 490 '0x2E 停止滚动' (Col 220 38 38)
    Add-Badge $card 470 490 '无需 CPU 持续刷新' (Col 14 116 144)
    Add-Note $card '程序渲染示意图（非实拍）：硬件滚动只能滚动整页显存，不能直接滚动任意长度文本，故另做软件滚动页。'
    Save-Card $card 'task1_hwscroll.png'
}

# ---------------------------------------------------------------------------
# 任务三页面
# ---------------------------------------------------------------------------
function Scene-Task3Info {
    $buf = New-Screen
    Draw-String5x7 $buf 10 0 'U8g2 PORT' 2
    Draw-HLine $buf 0 16 128
    Draw-Text16 $buf 40 20 @(0x5F90, 0x6B63, 0x5FC3)
    Draw-String5x7 $buf 28 38 '632407030612'
    Draw-Text16 $buf 36 48 @(0x901A, 0x4FE1, [int][char]'2', [int][char]'4', [int][char]'0', [int][char]'3')

    $card = New-Card '任务三 · U8g2 学号姓名页' '裁剪版 U8g2 · 全缓冲模式 · SSD1306 128×64 · 6×12 / 7×14B 字体' 960 600
    Add-Oled $card $buf 224 160 4
    Add-Badge $card 48 470 'U8g2 全缓冲 1 KB' (Col 14 116 144)
    Add-Badge $card 300 470 'I2C 字节回调' (Col 21 128 61)
    Add-Badge $card 520 470 '中英混排' (Col 180 83 9)
    Add-Note $card '程序渲染示意图（非实拍）：汉字由自定义 16×16 字库绘制，英文字符串使用 U8g2 内置字体。'
    Save-Card $card 'task3_info.png'
}

function Scene-Task3Demo {
    $buf = New-Screen
    Draw-String5x7 $buf 4 4 'U8g2 Demo'
    Draw-String5x7 $buf 92 4 '128x64'
    Draw-HLine $buf 0 16 128
    Draw-Frame $buf 2 20 60 40
    Draw-Circle $buf 32 40 16
    Draw-Line $buf 32 40 41 30
    Draw-Disc $buf 41 30 3
    Draw-Disc $buf 32 40 2
    for ($i = 0; $i -lt 28; $i++) { Set-Px $buf (68 + $i) 21 }
    for ($i = 0; $i -lt 28; $i++) { Set-Px $buf (68 + $i) 22 }
    Draw-String5x7 $buf 68 30 'U8g2' 2
    Draw-String5x7 $buf 68 48 'STM32F103'
    Draw-String5x7 $buf 68 57 'SSD1306'

    $card = New-Card '任务三 · U8g2 Demo 页面' '几何图形 + 内置字体 + 动画指针 · 50 ms 刷新一帧' 960 600
    Add-Oled $card $buf 224 160 4
    Add-Badge $card 48 470 'DrawFrame / Circle / Line' (Col 14 116 144)
    Add-Badge $card 340 470 'ncenB14 / 7x14B / 5x7' (Col 21 128 61)
    Add-Note $card '程序渲染示意图（非实拍）：动画指针按 16 点正弦表旋转，实际字体形状以 U8g2 内置字模为准。'
    Save-Card $card 'task3_demo.png'
}

function Scene-Task3Scroll {
    $card = New-Card '任务三 · 四向滚动演示' '左 / 右 / 上 / 下四个方向 · 3 秒切换一次 · 负坐标裁剪后绘制' 1100 760
    $codes = @(0x56DB, 0x5411, 0x6EDA, 0x52A8, 0x6F14, 0x793A)
    $labels = @('LEFT  从右向左', 'RIGHT  从左向右', 'UP  从下向上', 'DOWN  从上向下')
    for ($i = 0; $i -lt 4; $i++) {
        $buf = New-Screen
        Draw-String5x7 $buf 28 2 '4-DIR SCROLL'
        Draw-HLine $buf 0 10 128
        Set-Clip 11 54
        switch ($i) {
            0 { Draw-Text16 $buf 24 27 $codes }
            1 { Draw-Text16 $buf -24 27 $codes }
            2 {
                $top = -8
                for ($k = 0; $k -lt 6; $k++) { Draw-Glyph16 $buf 56 ($top + $k * 18) $codes[$k] }
            }
            3 {
                $top = 12
                for ($k = 0; $k -lt 6; $k++) { Draw-Glyph16 $buf 56 ($top + $k * 18) $codes[$k] }
            }
        }
        Reset-Clip
        Draw-String5x7 $buf 56 57 $labels[$i].Split(' ')[0]
        $col = $i % 2; $row = [int][Math]::Floor($i / 2)
        Add-Oled $card $buf (65 + $col * 550) (165 + $row * 285) 3 $labels[$i]
    }
    Add-Note $card '程序渲染示意图（非实拍）：四帧分别对应四个滚动方向，箭头方向由页面底部英文提示。'
    Save-Card $card 'task3_scroll.png'
}

function Scene-Task3Avatar {
    $card = New-Card '任务三 · 动画头像页面' '两帧 56×56 机器人头像 · 每 2 秒眨眼一次 · 右侧显示学号姓名班级' 1040 620
    $frames = @($script:avatar0, $script:avatar1)
    $labels = @('第 1 帧 · 睁眼', '第 2 帧 · 眨眼')
    for ($i = 0; $i -lt 2; $i++) {
        $buf = New-Screen
        Draw-String5x7 $buf 28 0 '632407030612'
        Draw-Bitmap $buf 2 7 56 56 $frames[$i]
        Draw-Text16 $buf 80 12 @(0x5934, 0x50CF)
        Draw-Text16 $buf 74 32 @(0x5F90, 0x6B63, 0x5FC3)
        Draw-Text16 $buf 70 48 @(0x901A, 0x4FE1, [int][char]'2', [int][char]'4', [int][char]'0', [int][char]'3')
        Add-Oled $card $buf (70 + $i * 500) 170 3 $labels[$i]
    }
    Add-Badge $card 70 500 '56×56 XBMP' (Col 14 116 144)
    Add-Badge $card 260 500 '2 帧循环' (Col 21 128 61)
    Add-Badge $card 420 500 '1.55 s + 0.45 s' (Col 180 83 9)
    Add-Note $card '程序渲染示意图（非实拍）：头像点阵来自工程 avatar.c，两帧仅眼部图案不同。'
    Save-Card $card 'task3_avatar.png'
}

# ---------------------------------------------------------------------------
# I2C 波形示意图（回答 Keil 虚拟逻辑分析仪问题）
# ---------------------------------------------------------------------------
function Fill-RoundRect($g, [int]$x, [int]$y, [int]$w, [int]$h, [int]$rad, $fill, $border, [int]$bw = 1) {
    $rect = [System.Drawing.Rectangle]::new($x, $y, $w, $h)
    $path = Round-Path $rect $rad
    if ($null -ne $fill) {
        $g.FillPath((New-Object System.Drawing.SolidBrush($fill)), $path)
    }
    if ($null -ne $border) {
        $g.DrawPath((New-Object System.Drawing.Pen($border, $bw)), $path)
    }
}

function Draw-WaveSegments($g, $segments, [int]$yHigh, [int]$yLow, $pen) {
    $pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Miter
    foreach ($seg in $segments) {
        $y = if ($seg.level -eq 1) { $yHigh } else { $yLow }
        $g.DrawLine($pen, $seg.x1, $y, $seg.x2, $y)
    }
    for ($i = 0; $i -lt $segments.Count - 1; $i++) {
        $a = $segments[$i]
        $b = $segments[$i + 1]
        if ($a.level -ne $b.level) {
            $ya = if ($a.level -eq 1) { $yHigh } else { $yLow }
            $yb = if ($b.level -eq 1) { $yHigh } else { $yLow }
            $g.DrawLine($pen, $b.x1, $ya, $b.x1, $yb)
        }
    }
}

function Draw-Bracket($g, [int]$x1, [int]$x2, [int]$y, [string]$label, $color) {
    $pen = New-Object System.Drawing.Pen($color, 1)
    $g.DrawLine($pen, $x1, $y, $x2, $y)
    $g.DrawLine($pen, $x1, $y, $x1, ($y + 6))
    $g.DrawLine($pen, $x2, $y, $x2, ($y + 6))
    $f = Fnt 12 'Bold'
    $sz = $g.MeasureString($label, $f)
    $g.DrawString($label, $f, (New-Object System.Drawing.SolidBrush($color)),
                  ($x1 + ($x2 - $x1 - $sz.Width) / 2), ($y - 24))
}

function Scene-I2CWaveform {
    $card = New-Card 'I2C 总线时序与 Keil 虚拟逻辑分析仪' '以 OLED 地址 0x78（7 位 0x3C）为例 · 地址 + ACK + 数据 + ACK + STOP' 1240 800
    $g = $card.Graphics

    Fill-RoundRect $g 50 130 1140 420 16 (Col 9 15 23) (Col 51 65 85) 2

    $gridPen = New-Object System.Drawing.Pen((Col 30 41 59), 1)
    for ($x = 140; $x -le 1140; $x += 44) { $g.DrawLine($gridPen, $x, 160, $x, 530) }
    for ($y = 170; $y -le 530; $y += 40) { $g.DrawLine($gridPen, 70, $y, 1150, $y) }

    $g.DrawString('I2C 逻辑分析仪视图', (Fnt 16 'Bold'),
                  (New-Object System.Drawing.SolidBrush((Col 148 163 184))), 80, 150)

    $sclHigh = 240; $sclLow = 280
    $sdaHigh = 380; $sdaLow = 420

    # SCL：空闲高 -> 起始后的低 -> 18 个时钟 -> STOP 前高
    $scl = @()
    $scl += @{ x1 = 120; x2 = 180; level = 1 }
    $scl += @{ x1 = 180; x2 = 200; level = 0 }
    for ($k = 0; $k -lt 18; $k++) {
        $a = 200 + $k * 44
        $scl += @{ x1 = $a; x2 = ($a + 22); level = 1 }
        $scl += @{ x1 = ($a + 22); x2 = ($a + 44); level = 0 }
    }
    $scl += @{ x1 = 992; x2 = 1060; level = 1 }

    # SDA：起始 1->0，地址 0x78，ACK，数据 0xA5，ACK，STOP 0->1
    $bits = @(0,1,1,1,1,0,0,0, 0, 1,0,1,0,0,1,0,1, 0)
    $sda = @()
    $sda += @{ x1 = 120; x2 = 160; level = 1 }
    $sda += @{ x1 = 160; x2 = 180; level = 0 }
    for ($k = 0; $k -lt 18; $k++) {
        $a = 180 + $k * 44
        $sda += @{ x1 = $a; x2 = ($a + 44); level = $bits[$k] }
    }
    $sda += @{ x1 = 972; x2 = 1020; level = 0 }
    $sda += @{ x1 = 1020; x2 = 1060; level = 1 }

    Draw-WaveSegments $g $scl $sclHigh $sclLow (New-Object System.Drawing.Pen((Col 250 204 21), 3))
    Draw-WaveSegments $g $sda $sdaHigh $sdaLow (New-Object System.Drawing.Pen((Col 34 211 238), 3))

    $fLabel = Fnt 16 'Bold'
    $g.DrawString('SCL', $fLabel, (New-Object System.Drawing.SolidBrush((Col 250 204 21))), 78, 228)
    $g.DrawString('SDA', $fLabel, (New-Object System.Drawing.SolidBrush((Col 34 211 238))), 78, 368)

    $fBit = Fnt 10
    $g.DrawString('1', $fBit, (New-Object System.Drawing.SolidBrush((Col 148 163 184))), 105, 232)
    $g.DrawString('0', $fBit, (New-Object System.Drawing.SolidBrush((Col 148 163 184))), 105, 272)
    $g.DrawString('1', $fBit, (New-Object System.Drawing.SolidBrush((Col 148 163 184))), 105, 372)
    $g.DrawString('0', $fBit, (New-Object System.Drawing.SolidBrush((Col 148 163 184))), 105, 412)

    Draw-Bracket $g 180 532 210 '地址 0x78（写）' (Col 147 197 253)
    Draw-Bracket $g 532 576 210 'ACK' (Col 251 191 36)
    Draw-Bracket $g 576 928 210 '数据 0xA5（示例）' (Col 147 197 253)
    Draw-Bracket $g 928 972 210 'ACK' (Col 251 191 36)

    for ($k = 0; $k -lt 18; $k++) {
        $cx = 180 + $k * 44 + 22
        $v = [string]$bits[$k]
        $col = if (($k -eq 8) -or ($k -eq 17)) { Col 251 191 36 } else { Col 203 213 225 }
        $sz = $g.MeasureString($v, $fBit)
        $g.DrawString($v, $fBit, (New-Object System.Drawing.SolidBrush($col)), ($cx - $sz.Width / 2), 350)
    }

    $dashPen = New-Object System.Drawing.Pen((Col 251 146 60), 1)
    $dashPen.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dash
    $g.DrawLine($dashPen, 160, 220, 160, 440)
    $g.DrawLine($dashPen, 1020, 220, 1020, 440)
    $g.FillEllipse((New-Object System.Drawing.SolidBrush((Col 251 146 60))), 155, 375, 10, 10)
    $g.FillEllipse((New-Object System.Drawing.SolidBrush((Col 251 146 60))), 1015, 375, 10, 10)
    $g.DrawString('START', (Fnt 13 'Bold'), (New-Object System.Drawing.SolidBrush((Col 251 146 60))), 130, 450)
    $g.DrawString('STOP', (Fnt 13 'Bold'), (New-Object System.Drawing.SolidBrush((Col 251 146 60))), 992, 450)

    $g.DrawString('SDA 只在 SCL 低电平期间改变；START/STOP 是唯一允许在 SCL 高电平期间跳变的情况。',
                  (Fnt 12), (New-Object System.Drawing.SolidBrush((Col 148 163 184))), 80, 505)

    Fill-RoundRect $g 50 570 1140 150 14 (Col 255 255 255) (Col 203 213 225) 2
    $g.DrawString('Keil 虚拟逻辑分析仪读数要点', (Fnt 16 'Bold'),
                  (New-Object System.Drawing.SolidBrush((Col 15 23 42))), 75, 585)
    $lines = @(
        '① 只显示 GPIO 高低电平，不会自动解码 I2C 地址、数据和 ACK，需要按 SCL 的 9 个时钟手工分组。',
        '② 有真实从机：START → 地址 0x78 的 8 位 → 第 9 个时钟 ACK=0 → 数据位 → STOP。',
        '③ 无真实从机：SDA 可能一直为高，或在 ACK 位收不到低电平，HAL 返回 HAL_ERROR / HAL_TIMEOUT。'
    )
    for ($i = 0; $i -lt 3; $i++) {
        $g.DrawString($lines[$i], (Fnt 13), (New-Object System.Drawing.SolidBrush((Col 51 65 85))), 75, (615 + $i * 30))
    }

    Add-Note $card '波形为按 I2C 规范绘制的示意图，不是示波器实拍；实际边沿和抖动以硬件测量为准。'
    Save-Card $card 'i2c_waveform.png'
}

# ---------------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------------
Scene-Task1Sensor
Scene-I2CWaveform

Write-Host ('[DONE] 图片输出目录: ' + $outDir)
