#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TARGET_DIR="${PROJECT_ROOT}/images/input"
ZIP_FILE="${TARGET_DIR}/dataset.zip"
KAGGLE_URL="https://www.kaggle.com/api/v1/datasets/download/bhaveshmittal/melanoma-cancer-dataset"

FORCE_DOWNLOAD=false
if [[ "${1:-}" == "--force" ]]; then
    FORCE_DOWNLOAD=true
fi

echo "================================================="
echo " Melanoma PDI — Download do Dataset (Kaggle)"
echo "================================================="
echo "Diretório de destino: ${TARGET_DIR}"

mkdir -p "${TARGET_DIR}"

if [[ "${FORCE_DOWNLOAD}" == false ]] && [[ -d "${TARGET_DIR}/train/Benign" ]] && [[ -d "${TARGET_DIR}/test/Benign" ]]; then
    EXISTING_COUNT=$(find "${TARGET_DIR}/train" "${TARGET_DIR}/test" -type f -iname "*.jpg" | wc -l)
    if [[ "${EXISTING_COUNT}" -ge 13879 ]]; then
        echo "Dataset já está presente e completo (${EXISTING_COUNT} imagens)."
        echo "Use --force para baixar novamente."
        exit 0
    fi
fi

echo "Baixando Melanoma Cancer Dataset do Kaggle..."
if command -v curl >/dev/null 2>&1; then
    curl -L --fail --progress-bar -o "${ZIP_FILE}" "${KAGGLE_URL}"
elif command -v wget >/dev/null 2>&1; then
    wget --show-progress -O "${ZIP_FILE}" "${KAGGLE_URL}"
else
    echo "Erro: 'curl' ou 'wget' não encontrado no sistema." >&2
    exit 1
fi

echo "Extraindo arquivos em ${TARGET_DIR}..."
if command -v unzip >/dev/null 2>&1; then
    unzip -q -o "${ZIP_FILE}" -d "${TARGET_DIR}"
else
    echo "Erro: utilitário 'unzip' não encontrado no sistema." >&2
    rm -f "${ZIP_FILE}"
    exit 1
fi

rm -f "${ZIP_FILE}"

TRAIN_BENIGN=$(find "${TARGET_DIR}/train/Benign" -type f -iname "*.jpg" | wc -l)
TRAIN_MALIGNANT=$(find "${TARGET_DIR}/train/Malignant" -type f -iname "*.jpg" | wc -l)
TEST_BENIGN=$(find "${TARGET_DIR}/test/Benign" -type f -iname "*.jpg" | wc -l)
TEST_MALIGNANT=$(find "${TARGET_DIR}/test/Malignant" -type f -iname "*.jpg" | wc -l)
TOTAL=$((TRAIN_BENIGN + TRAIN_MALIGNANT + TEST_BENIGN + TEST_MALIGNANT))

echo "-------------------------------------------------"
echo "Extração concluída com sucesso!"
echo "  train/Benign    : ${TRAIN_BENIGN}"
echo "  train/Malignant : ${TRAIN_MALIGNANT}"
echo "  test/Benign     : ${TEST_BENIGN}"
echo "  test/Malignant  : ${TEST_MALIGNANT}"
echo "  Total           : ${TOTAL} imagens"
echo "================================================="
