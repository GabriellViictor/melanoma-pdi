param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ProjectRoot = Split-Path -Parent $ScriptDir
$TargetDir = Join-Path $ProjectRoot "images\input"
$ZipFile = Join-Path $TargetDir "dataset.zip"
$KaggleUrl = "https://www.kaggle.com/api/v1/datasets/download/bhaveshmittal/melanoma-cancer-dataset"

Write-Host "================================================="
Write-Host " Melanoma PDI — Download do Dataset (Kaggle)"
Write-Host "================================================="
Write-Host "Diretorio de destino: $TargetDir"

if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

$TrainBenignDir = Join-Path $TargetDir "train\Benign"
$TrainMalignantDir = Join-Path $TargetDir "train\Malignant"
$TestBenignDir = Join-Path $TargetDir "test\Benign"
$TestMalignantDir = Join-Path $TargetDir "test\Malignant"

if ((-not $Force) -and (Test-Path $TrainBenignDir) -and (Test-Path $TestBenignDir)) {
    $ExistingCount = (Get-ChildItem -Path (Join-Path $TargetDir "train"), (Join-Path $TargetDir "test") -Filter "*.jpg" -Recurse -File -ErrorAction SilentlyContinue).Count
    if ($ExistingCount -ge 13879) {
        Write-Host "Dataset ja esta presente e completo ($ExistingCount imagens)."
        Write-Host "Use -Force para baixar novamente."
        exit 0
    }
}

Write-Host "Baixando Melanoma Cancer Dataset do Kaggle..."
if (Get-Command "curl.exe" -ErrorAction SilentlyContinue) {
    & curl.exe -L --fail -o $ZipFile $KaggleUrl
    if ($LASTEXITCODE -ne 0) {
        throw "Falha ao baixar o dataset via curl.exe."
    }
} else {
    Invoke-WebRequest -Uri $KaggleUrl -OutFile $ZipFile
}

Write-Host "Extraindo arquivos em $TargetDir..."
Expand-Archive -Path $ZipFile -DestinationPath $TargetDir -Force

if (Test-Path $ZipFile) {
    Remove-Item -Path $ZipFile -Force
}

$TrainBenign = (Get-ChildItem -Path $TrainBenignDir -Filter "*.jpg" -File).Count
$TrainMalignant = (Get-ChildItem -Path $TrainMalignantDir -Filter "*.jpg" -File).Count
$TestBenign = (Get-ChildItem -Path $TestBenignDir -Filter "*.jpg" -File).Count
$TestMalignant = (Get-ChildItem -Path $TestMalignantDir -Filter "*.jpg" -File).Count
$Total = $TrainBenign + $TrainMalignant + $TestBenign + $TestMalignant

Write-Host "-------------------------------------------------"
Write-Host "Extracao concluida com sucesso!"
Write-Host "  train/Benign    : $TrainBenign"
Write-Host "  train/Malignant : $TrainMalignant"
Write-Host "  test/Benign     : $TestBenign"
Write-Host "  test/Malignant  : $TestMalignant"
Write-Host "  Total           : $Total imagens"
Write-Host "================================================="
