# Análise e Segmentação de Lesões Cutâneas para Auxílio no Diagnóstico de Melanoma

**Processamento Digital de Imagens (PDI)**  
**Etapa Atual:** Etapa M2 — Implementação do Pipeline de Pré-processamento, Segmentação e Extração de Características

---

## Integrantes
* **Gabriel Victor Garcia Teixeira**
* **Gustavo Pedrini**
* **Henrik Baltazar**

---

## Problema Investigado

O problema investigado consiste no isolamento e na caracterização quantitativa de lesões cutâneas a partir de imagens dermatoscópicas digitais, buscando mitigar a subjetividade da inspeção visual humana na triagem do melanoma.

O desafio central sob a ótica de Processamento Digital de Imagens (PDI) reside em tratar matrizes de pixels brutas, frequentemente degradadas por ruídos como pelos sobrepostos, variações de iluminação e transições difusas com a pele sadia, para segmentar com precisão os contornos da lesão e extrair descritores matemáticos morfológicos e cromáticos (como assimetria, compacidade de bordas, circularidade, solidez e dispersão de luminância/cores) capazes de auxiliar computacionalmente na diferenciação entre lesões benignas e lesões malignas.

---

## Contexto de Aplicação
O melanoma é a forma mais letal de câncer de pele, cuja taxa de cura está diretamente associada à precocidade do diagnóstico. O conjunto de dados adotado compreende **$13.879$ imagens dermatoscópicas** padronizadas na resolução de $224 \times 224$ pixels, rotuladas entre lesões **benignas** e **malignas (melanoma)**.

A triagem clínica visual tradicional enfrenta desafios críticos:
* **Alta variabilidade inter-examinador:** Lesões em estágios iniciais compartilham características visuais sutis com nevos benignos (pintas comuns);
* **Heterogeneidade morfológica:** Melanomas manifestam polimorfismo acentuado, apresentando crescimento assimétrico, bordas recortadas e multiplicidade de padrões cromáticos.

---

## Objetivo Geral
Desenvolver um pipeline de Processamento Digital de Imagens (PDI) utilizando a linguagem C++ e a biblioteca OpenCV para isolar e analisar lesões cutâneas em imagens dermatoscópicas. O sistema extrai métricas quantitativas e descritores morfológicos e cromáticos das regiões segmentadas, gerando artefatos visuais de cada etapa do processamento e relatórios tabulares (CSV) para análise comparativa.

---

## Arquitetura do Pipeline Implementado (Etapa M2)

A solução computacional implementada no módulo `melanoma::core::ImageProcessor` executa um pipeline completo dividido em cinco etapas sequenciais:

1. **Detecção e Remoção Morfológica de Pelos (`removeHairs`):**
   * Conversão para escala de cinza e aplicação da transformada morfológica **Black-Hat** (elemento estruturante retangular $17 \times 17$) para evidenciar estruturas escuras finas e alongadas sobrepostas à pele e à lesão;
   * Binarização da resposta (limiar $12.0$) seguida de dilatação leve ($3 \times 3$) para englobar as bordas dos fios;
   * Reconstrução das regiões ocluídas por meio do algoritmo de **Inpainting de Telea** (`cv::INPAINT_TELEA`, raio de vizinhança $3$). Pode ser desativada via flag `--no-inpaint` para experimentos comparativos.

2. **Preparação de Canal e Espaço de Cores (`prepareSegmentationChannel`):**
   * Suporte configurável (`--channel <lab|gray>`) para conversão ao espaço de cores **CIE $L^*a^*b^*$** (extraindo o canal de luminância $L^*$) ou **Escala de Cinza**;
   * Inversão matemática da intensidade ($255 - I$) para que a lesão pigmentada (mais escura que a pele sadia) assuma valores altos de primeiro plano.

3. **Segmentação Automática (`segmentOtsu`):**
   * Atenuação de ruídos de alta frequência com filtro **Gaussiano** ($5 \times 5$);
   * Binarização automática global pelo método de **Limiarização de Otsu** (`cv::THRESH_BINARY | cv::THRESH_OTSU`).

4. **Pós-processamento Morfológico e Filtragem Topológica (`applyMorphologyAndFilter`):**
   * **Fechamento Morfológico** (elemento elíptico $9 \times 9$) para conectar descontinuidades internas ou falhas causadas por variações de pigmentação;
   * **Abertura Morfológica** (elemento elíptico $5 \times 5$) para eliminar espículas periféricas e pequenas ilhas de falso positivo;
   * **Filtro de Maior Componente Conexo** (`cv::connectedComponentsWithStats`) para isolar a lesão principal e descartar artefatos de vinheta nos cantos da imagem;
   * **Preenchimento de Orifícios (`fillMaskHoles`)** via reconstrução do contorno externo principal.

5. **Extração de Características e Descritores (`extractFeatures` & `processBasic`):**
   * **Área ($A$) e Perímetro ($P$):** Contagem de pixels da região conexa e comprimento de arco do contorno principal;
   * **Compacidade (Polsby-Popper):** Medida de irregularidade de borda dada por $\text{Compacidade} = \frac{P^2}{4\pi A}$ (círculo perfeito $= 1.0$, bordas recortadas $> 1.0$);
   * **Circularidade:** Dada por $\text{Circularidade} = \frac{4\pi A}{P^2} \le 1.0$;
   * **Solidez:** Razão entre a área da lesão e a área do seu fecho convexo ($\text{Solidez} = \frac{A}{A_{\text{hull}}}$);
   * **Diâmetro Equivalente:** $\text{Diâmetro} = \sqrt{\frac{4A}{\pi}}$;
   * **Razão de Aspecto e Bounding Box:** Relação $\frac{\text{Largura}}{\text{Altura}}$ do retângulo delimitador mínimo;
   * **Índice de Assimetria:** Alinhamento da máscara pelo eixo principal de inércia obtido via momentos centrais ($\theta = \frac{1}{2}\operatorname{atan2}(2\mu_{11}, \mu_{20} - \mu_{02})$), espelhamento horizontal e cálculo da taxa de não-sobreposição via operação lógica $\text{XOR}$;
   * **Estatísticas Cromáticas e de Luminância:** Médias dos canais $B$, $G$, $R$, média de luminância e desvio-padrão de luminância.

---

## Conjunto e Origem das Imagens
* **Origem:** [Melanoma Cancer Dataset (Kaggle)](https://www.kaggle.com/datasets/bhaveshmittal/melanoma-cancer-dataset/data).
* **Composição Validada (`DatasetManager`):** Total de **$13.879$ imagens** dermatoscópicas divididas em:
  * `train/Benign`: $6.289$ imagens
  * `train/Malignant`: $5.590$ imagens
  * `test/Benign`: $1.000$ imagens
  * `test/Malignant`: $1.000$ imagens
* **Formato e Resolução:** Imagens coloridas (RGB/BGR) padronizadas em $224 \times 224$ pixels ($3$ canais) em formato `.jpg` / `.jpeg` (suporte adicional a `.png` e `.bmp` em diretórios avulsos).
* **Imagens de Demonstração Integradas:** Caso o dataset completo não esteja baixado localmente, o programa utiliza automaticamente as imagens de demonstração incluídas no repositório (`docs/7280.jpg` e `docs/6068.jpg`).

---

## Estágio Atual do Projeto
* **Etapa M1 (Concluída):** Definição do problema, escopo, estudo de viabilidade, [proposta detalhada](./docs/PROPOSTA.md), scripts de automação do dataset e estrutura base de leitura e validação com CMake/CTest.
* **Etapa M2 (Atual):** Implementação completa do pipeline de PDI em C++/OpenCV (`ImageProcessor`), interface de linha de comando (CLI) flexível para execução em imagem única, diretórios ou lotes do dataset, geração automática de $6$ artefatos visuais intermediários por imagem, exportação de métricas em CSV e testes unitários sintéticos do pipeline.

---

## Organização do Repositório

~~~text
melanoma-pdi/
├── CMakeLists.txt              # Configuração do CMake para compilação C++, OpenCV e CTest
├── README.md                   # Documentação principal e guia de uso do projeto
├── .gitignore                  # Arquivos e pastas ignorados pelo controle de versão
├── scripts/
│   ├── download_dataset.sh     # Script Linux/macOS para download e extração do dataset Kaggle
│   └── download_dataset.ps1    # Script Windows (PowerShell) para download e extração do dataset
├── docs/
│   ├── PROPOSTA.md             # Documento formal e detalhado da proposta M1
│   ├── 6068.jpg                # Imagem dermatoscópica de demonstração (Benign)
│   └── 7280.jpg                # Imagem dermatoscópica de demonstração (Malignant)
├── images/
│   ├── input/                  # Diretório de entrada (dataset extraído em train/ e test/)
│   └── results/                # Artefatos visuais gerados (*_1_hair_mask.png a *_6_contour_overlay.png) e metrics.csv
├── src/
│   ├── main.cpp                # Ponto de entrada CLI da aplicação (Etapa M2)
│   ├── io/
│   │   ├── DatasetManager.hpp  # Declarações de inspeção, listagem e leitura do dataset
│   │   └── DatasetManager.cpp  # Implementação do gerenciamento de dados
│   └── core/
│       ├── ImageProcessor.hpp  # Declarações do pipeline de PDI, estruturas de opções e métricas
│       └── ImageProcessor.cpp  # Implementação de remoção de pelos, Otsu, morfologia e features
└── tests/
    └── test_dataset_io.cpp     # Suíte de testes automatizados (pipeline sintético, dataset e leitura)
~~~

---

## Tecnologias Utilizadas
* **Linguagem:** C++ (Padrão C++17 / `std::filesystem`)
* **Biblioteca de Visão Computacional:** OpenCV 4.x (`core`, `imgcodecs`, `imgproc`, `photo`)
* **Sistema de Build e Testes:** CMake (mínimo 3.20) & CTest
* **Controle de Versão:** Git & GitHub

---

## Instruções de Obtenção dos Dados, Compilação e Execução

### 1. Download do Dataset (Opcional para Teste Rápido)
O conjunto de dados completo não é versionado no Git. Você pode rodar o programa imediatamente com as imagens de demonstração da pasta `docs/` ou baixar o dataset completo em `images/input/` usando os scripts:

**Linux / macOS:**
~~~bash
./scripts/download_dataset.sh
~~~

**Windows (PowerShell):**
~~~powershell
powershell -ExecutionPolicy Bypass -File .\scripts\download_dataset.ps1
~~~

### 2. Compilação
~~~bash
cmake -S . -B build/unix-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/unix-debug
~~~

### 3. Execução da Aplicação (CLI)
O executável `melanoma_pdi` oferece suporte a diferentes modos de operação e parâmetros ajustáveis:

~~~text
Uso: ./build/unix-debug/melanoma_pdi [opcoes]

Opcoes disponiveis:
  --image <path>       Processa uma unica imagem especifica
  --input-dir <path>   Processa todas as imagens encontradas no diretorio informado
  --dataset <path>     Caminho para a raiz do dataset (padrao: images/input)
  --samples <n>        Numero de amostras por classe ao ler o dataset (padrao: 2)
  --output-dir <path>  Diretorio para salvar saidas e artefatos (padrao: images/results)
  --channel <lab|gray> Canal para segmentacao por Otsu (padrao: lab)
  --no-inpaint         Desativa a remocao de pelos (util para experimentos comparativos)
  --csv <path>         Caminho do arquivo CSV de metricas (padrao: <output-dir>/metrics.csv)
  -h, --help           Exibe a mensagem de ajuda
~~~

#### Exemplos Práticos de Uso:

* **Execução padrão (usa o dataset se disponível ou faz fallback automático para as imagens de demonstração):**
  ~~~bash
  ./build/unix-debug/melanoma_pdi
  ~~~

* **Processar uma única imagem específica salvando resultados em `images/results`:**
  ~~~bash
  ./build/unix-debug/melanoma_pdi --image docs/7280.jpg --output-dir images/results
  ~~~

* **Experimento comparativo sem remoção de pelos e usando canal em escala de cinza:**
  ~~~bash
  ./build/unix-debug/melanoma_pdi --image docs/7280.jpg --no-inpaint --channel gray --output-dir images/results_no_inpaint
  ~~~

* **Processar amostras do dataset completo ($5$ imagens por classe):**
  ~~~bash
  ./build/unix-debug/melanoma_pdi --dataset images/input --samples 5 --channel lab
  ~~~

* **Processar todas as imagens de um diretório customizado:**
  ~~~bash
  ./build/unix-debug/melanoma_pdi --input-dir docs --output-dir images/results
  ~~~

---

## Saídas e Artefatos Gerados

Para cada imagem processada (ex.: `7280.jpg`), o sistema salva automaticamente **$6$ artefatos visuais** no diretório de saída (`images/results/` por padrão), permitindo inspecionar passo a passo a eficácia do pipeline:

1. `<id>_1_hair_mask.png` — Máscara binária dos pelos detectados pelo filtro morfológico Black-Hat;
2. `<id>_2_inpainted.png` — Imagem colorida restaurada após a remoção de pelos via Inpainting de Telea;
3. `<id>_3_channel.png` — Canal de segmentação preparado e invertido ($L^*$ invertido ou Escala de Cinza invertida);
4. `<id>_4_raw_otsu.png` — Máscara binária bruta resultante da limiarização automática de Otsu;
5. `<id>_5_final_mask.png` — Máscara final refinada após fechamento, abertura, filtragem de maior componente conexo e preenchimento de buracos;
6. `<id>_6_contour_overlay.png` — Imagem após inpainting com o contorno final da lesão demarcado em verde.

Além das imagens, todas as métricas extraídas são consolidadas e anexadas ao arquivo **`metrics.csv`** contendo as colunas:
`image_id, split, label, area_px, perimeter_px, compactness, circularity, solidity, equiv_diameter_px, aspect_ratio, asymmetry_index, mean_luminance, std_luminance, mean_b, mean_g, mean_r, bbox_x, bbox_y, bbox_w, bbox_h, success`.

---

## Suíte de Testes Automatizados

Para executar todos os testes automatizados via CTest:
~~~bash
ctest --test-dir build/unix-debug --output-on-failure -V
~~~

O executável de testes (`test_dataset_io`) valida três frentes:
1. **Teste Sintético do Pipeline (`--test-pipeline`):** Gera uma imagem sintética em memória com fundo simulando pele, uma lesão elíptica escura e um traço simulando pelo sobreposto, verificando se a detecção de pelos, a segmentação e a consistência matemática dos descritores ($\text{Compacidade} \ge 1.0$ e $\text{Área} > 100\text{ px}$) funcionam perfeitamente mesmo sem o dataset baixado;
2. **Integridade do Dataset (`--test-dataset`):** Verifica a estrutura de diretórios e a contagem exata das $13.879$ imagens quando o dataset está presente;
3. **Smoke Test de Leitura e Processamento (`--test-processing`):** Valida o carregamento de matrizes OpenCV, dimensões padronizadas ($224 \times 224 \times 3$) e extração de luminância em lote.

---

## Vídeo de Apresentação da Etapa M1
* [Assista no YouTube](https://youtu.be/oI2tWe0_nkA)

---

## Documentação Adicional
* [PROPOSTA.md](./docs/PROPOSTA.md)

---

## Uso de IA
* **Ferramenta:** Gemini;
* **Finalidade:** Ferramenta de produtividade, pesquisa eficiente, estruturação de documentação e sugestões de implementação;
* Os slides utilizados no vídeo foram produzidos com auxílio de IA, revisados e modificados pelos integrantes;
* Respostas e formulações matemáticas verificadas através das referências bibliográficas listadas na [proposta](./docs/PROPOSTA.md).