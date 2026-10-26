# 生成两个单目标 Keil 工程（共用同一套源码与 HAL）
#   Task1_SSD1306.uvprojx : 自定义 SSD1306 驱动 + 汉字 + AHT20 + 滚动
#   Task3_U8g2.uvprojx    : 裁剪版 U8g2 + demo + 四向滚动 + 头像
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$templatePath = 'C:\Users\12611\Documents\Default Project\STM32F10x_OLED_U8g2\MDK-ARM\OLED_U8g2.uvprojx'
$mdk = Join-Path $root 'MDK-ARM'

$halFiles = @(
    'stm32f1xx_hal.c','stm32f1xx_hal_cortex.c','stm32f1xx_hal_dma.c','stm32f1xx_hal_exti.c',
    'stm32f1xx_hal_flash.c','stm32f1xx_hal_flash_ex.c','stm32f1xx_hal_gpio.c','stm32f1xx_hal_gpio_ex.c',
    'stm32f1xx_hal_i2c.c','stm32f1xx_hal_pwr.c','stm32f1xx_hal_rcc.c','stm32f1xx_hal_rcc_ex.c',
    'stm32f1xx_hal_uart.c','stm32f1xx_hal_tim.c','stm32f1xx_hal_tim_ex.c'
)

$u8g2Files = @(
    'u8g2_setup.c','u8g2_buffer.c','u8g2_hvline.c','u8g2_box.c','u8g2_line.c','u8g2_circle.c',
    'u8g2_arc.c','u8g2_polygon.c','u8g2_kerning.c','u8g2_intersection.c','u8g2_bitmap.c',
    'u8g2_cleardisplay.c','u8g2_d_setup_min.c','u8g2_d_memory_min.c','u8g2_ll_hvline.c',
    'u8x8_capture.c','u8g2_font.c','u8g2_fonts_min.c','u8x8_setup.c','u8x8_byte.c','u8x8_cad.c',
    'u8x8_display.c','u8x8_gpio.c','u8x8_string.c','u8x8_u8toa.c','u8x8_u16toa.c','u8x8_8x8.c',
    'u8x8_d_ssd1306_128x64_noname.c'
)

function New-FileNode($doc, [string]$name, [string]$path, [int]$type) {
    $file = $doc.CreateElement('File')
    $n = $doc.CreateElement('FileName'); $n.InnerText = $name; [void]$file.AppendChild($n)
    $t = $doc.CreateElement('FileType'); $t.InnerText = [string]$type; [void]$file.AppendChild($t)
    $p = $doc.CreateElement('FilePath'); $p.InnerText = $path; [void]$file.AppendChild($p)
    return $file
}

function Add-Group($doc, $groups, [string]$groupName, $fileNodes) {
    $g = $doc.CreateElement('Group')
    $gn = $doc.CreateElement('GroupName'); $gn.InnerText = $groupName; [void]$g.AppendChild($gn)
    $fs = $doc.CreateElement('Files')
    foreach ($f in $fileNodes) { [void]$fs.AppendChild($f) }
    [void]$g.AppendChild($fs)
    [void]$groups.AppendChild($g)
}

function Build-Project([string]$name, [string]$defines, [string]$outDir, [string[]]$appFiles, [bool]$withU8g2) {
    [xml]$doc = Get-Content -LiteralPath $templatePath -Raw -Encoding UTF8
    $target = $doc.Project.Targets.Target
    $target.TargetName = $name
    $target.TargetOption.TargetCommonOption.OutputName = $name
    $target.TargetOption.TargetCommonOption.OutputDirectory = $outDir
    $target.TargetOption.TargetCommonOption.ListingPath = $outDir
    $target.TargetOption.TargetCommonOption.CreateHexFile = '1'
    $target.TargetOption.TargetArmAds.Cads.VariousControls.Define = $defines
    $target.TargetOption.TargetArmAds.Aads.VariousControls.Define = $defines
    $target.TargetOption.TargetArmAds.Cads.VariousControls.IncludePath = '../Core/Inc;../Drivers/STM32F1xx_HAL_Driver/Inc;../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy;../Drivers/CMSIS/Device/ST/STM32F1xx/Include;../Drivers/CMSIS/Include;../Drivers/U8g2'

    $groups = $target.Groups
    $groups.RemoveAll()

    Add-Group $doc $groups 'Application/MDK-ARM' @((New-FileNode $doc 'startup_stm32f103xb.s' 'startup_stm32f103xb.s' 2))

    $core = @()
    foreach ($f in $appFiles) { $core += (New-FileNode $doc $f ('../Core/Src/' + $f) 1) }
    Add-Group $doc $groups 'Application/User/Core' $core

    $hal = @()
    foreach ($f in $halFiles) { $hal += (New-FileNode $doc $f ('../Drivers/STM32F1xx_HAL_Driver/Src/' + $f) 1) }
    Add-Group $doc $groups 'Drivers/STM32F1xx_HAL_Driver' $hal

    Add-Group $doc $groups 'Drivers/CMSIS' @((New-FileNode $doc 'system_stm32f1xx.c' '../Core/Src/system_stm32f1xx.c' 1))

    if ($withU8g2) {
        $u8 = @()
        foreach ($f in $u8g2Files) { $u8 += (New-FileNode $doc $f ('../Drivers/U8g2/' + $f) 1) }
        Add-Group $doc $groups 'Drivers/U8g2' $u8
    }

    $outPath = Join-Path $mdk ($name + '.uvprojx')
    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Indent = $true
    $settings.IndentChars = '  '
    $settings.Encoding = New-Object System.Text.UTF8Encoding($false)
    $writer = [System.Xml.XmlWriter]::Create($outPath, $settings)
    $doc.Save($writer)
    $writer.Close()
    Write-Host "Generated $outPath"
}

Build-Project 'Task1_SSD1306' 'USE_HAL_DRIVER,STM32F103xB' '.\Objects_Task1\' @(
    'main.c','task1_app.c','board.c','aht20.c','oled_ssd1306.c',
    'font_cn.c','font5x7.c','avatar.c','stm32f1xx_it.c','stm32f1xx_hal_msp.c'
) $false

Build-Project 'Task3_U8g2' 'USE_HAL_DRIVER,STM32F103xB,TASK3_BUILD' '.\Objects_Task3\' @(
    'main.c','task3_app.c','board.c','aht20.c',
    'font_cn.c','font5x7.c','avatar.c','stm32f1xx_it.c','stm32f1xx_hal_msp.c'
) $true

foreach ($d in @('Objects_Task1','Objects_Task3')) {
    $p = Join-Path $mdk $d
    if (!(Test-Path $p)) { New-Item -ItemType Directory -Path $p | Out-Null }
}
