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

1. **Detecção e Remoção Morfológica de Pelos (`detectHairs` & `removeHairs`):**
   * Conversão para escala de cinza e aplicação da metodologia **DullRazor em dois estágios com proteção de núcleo**: extração de vales estreitos via **Black-Hat elíptico ($7 \times 7$)** combinada a filtragem de colinearidade direcional em **$8$ orientações** (elementos estruturantes lineares de $9\text{ px}$), preservando integralmente o interior escuro de lesões pequenas enquanto destaca fibras capilares;
   * Filtragem topológica de componentes conexos (`cv::connectedComponentsWithStats`) sem preenchimento de laços fechados e dilatação elíptica suave ($3 \times 3$);
   * Reconstrução dérmica por meio do algoritmo de **Inpainting de Telea** (`cv::INPAINT_TELEA`, raio $3$), mantendo a imagem restaurada $100\%$ nítida fora da máscara de pelos, e suavização com **Filtro Bilateral** (`cv::bilateralFilter`) aplicada ao ramo de preparação do canal de segmentação. Pode ser desativada via `--no-inpaint` / `--no-bilateral` para experimentos comparativos.

2. **Supressão de Vinhetamento, Correção de Iluminação e Preparação de Canal (`createFieldOfViewMask` & `prepareSegmentationChannel`):**
   * Detecção automática da máscara de Campo de Visão (*Field of View* — FoV) para isolar a abertura central do dermatoscópio e rejeitar cantos brancos/pretos de vinheta e marcações periféricas;
   * Correção de iluminação (*flat-field*) para remover gradientes lentos de sombra nas bordas da imagem;
   * Suporte configurável (`--channel <lab|gray>`) para espaço **CIE $L^*a^*b^*$** (combinando luminância invertida $255 - L^*$ com cromaticidade melanocítica $a^*, b^*$) ou **Escala de Cinza invertida**.

3. **Segmentação Automática (`segmentOtsu` & `segmentGrabCut`):**
   * Suavização gaussiana ($5 \times 5$) e binarização automática pelo método de **Otsu** restrito aos pixels válidos do FoV (`--seg-method otsu`) ou refinamento iterativo por cortes em grafos via **GrabCut** (`--seg-method grabcut`).

4. **Pós-processamento Morfológico e Filtragem Topológica Central (`applyMorphologyAndFilter`):**
   * **Abertura Morfológica** (elemento elíptico $5 \times 5$) para quebrar pontes finas com ruídos periféricos;
   * **Fechamento Morfológico** (elemento elíptico $9 \times 9$) para conectar descontinuidades internas;
   * **Seleção Ponderada de Componente Conexo Central** (`keepLargestConnectedComponent`) ponderando a área pela distância gaussiana ao centro da imagem;
   * **Preenchimento de Orifícios (`fillMaskHoles`)** via reconstrução do contorno externo principal.

5. **Extração de Características e Descritores ABCD (`extractFeatures` & `extractColorFeatures`):**
   * **A — Assimetria:** Translação do centroide para o centro do canvas e alinhamento pelos eixos principais de inércia ($\theta = \frac{1}{2}\operatorname{atan2}(2\mu_{11}, \mu_{20} - \mu_{02})$), reflexão biaxial (eixos maior e menor) via $\text{XOR}$ normalizado em $[0, 1]$, e cálculo dos **7 Momentos Invariantes de Hu** (`cv::HuMoments`);
   * **B — Irregularidade de Borda:** Compacidade de Polsby-Popper ($\frac{P^2}{4\pi A} \ge 1.0$), Circularidade ($\frac{4\pi A}{P^2} \le 1.0$), Solidez ($\frac{A}{A_{\text{hull}}}$) e **Variância Radial Normalizada** das distâncias do centroide ao contorno;
   * **C — Variação Cromática (restrita à máscara da lesão):** Média e desvio-padrão estritamente dentro da ROI segmentada nos espaços $BGR$, $L^*a^*b^*$ e $HSV$, além da contagem de **Clusters Cromáticos ABCD** (branco, vermelho, castanho-claro, castanho-escuro, azul-acinzentado e preto);
   * **D — Dimensões e Diâmetro:** Área ($A$), Perímetro ($P$), Diâmetro Equivalente ($\sqrt{4A/\pi}$), Razão de Aspecto e comprimentos dos **Eixos Maior e Menor** da elipse ajustada via `cv::fitEllipse`.

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
* **Etapa M2 (Concluída):** Implementação completa das Etapas 1 a 4 do pipeline de PDI em C++/OpenCV (`ImageProcessor`), interface de linha de comando (CLI) flexível para execução em imagem única, diretórios ou lotes do dataset, geração automática de $6$ artefatos visuais intermediários por imagem, exportação de $33$ colunas de métricas ABCD em CSV, métricas quantitativas de sobreposição (`IoU`/`Dice`), experimentos controlados e [relatório técnico M2](./docs/relatorio_m2.md).
* **Etapa M3 (Planejada):** Extração de atributos ABCD em lote sobre as $13.879$ imagens, implementação do escore clínico TDS e treinamento/avaliação do classificador supervisionado (`cv::ml::SVM` / `cv::ml::RTrees`) com foco em alta sensibilidade diagnóstica.

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
│   ├── revisao_m1_plano_m2.md  # Rastreabilidade do feedback M1, protocolo experimental e plano M2
│   ├── relatorio_m2.md         # Relatório técnico parcial M2 (métodos, experimentos, tabelas e plano M3)
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
  --image <path>             Processa uma unica imagem especifica
  --input-dir <path>         Processa todas as imagens encontradas no diretorio informado
  --dataset <path>           Caminho para a raiz do dataset (padrao: images/input)
  --samples <n>              Numero de amostras por classe ao ler o dataset (padrao: 2)
  --output-dir <path>        Diretorio para salvar saidas e artefatos (padrao: images/results)
  --channel <lab|gray>       Canal para segmentacao (padrao: lab)
  --seg-method <otsu|grabcut> Metodo de segmentacao da lesao (padrao: otsu)
  --no-inpaint               Desativa a remocao de pelos (util para experimentos comparativos)
  --no-bilateral             Desativa o filtro bilateral pos-inpainting
  --no-vignette-filter       Desativa a supressao de vinhetamento e correcao de iluminacao
  --csv <path>               Caminho do arquivo CSV de metricas (padrao: <output-dir>/metrics.csv)
  -h, --help                 Exibe a mensagem de ajuda
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

Além das imagens, todas as métricas ABCD extraídas são consolidadas no arquivo **`metrics.csv`** contendo as colunas:
`image_id, split, label, area_px, perimeter_px, compactness, circularity, solidity, border_radial_var, equiv_diameter_px, major_axis_px, minor_axis_px, aspect_ratio, asymmetry_major, asymmetry_minor, asymmetry_index, hu1, hu2, color_clusters, lesion_mean_l, lesion_std_l, lesion_mean_a, lesion_std_a, lesion_mean_bstar, lesion_std_bstar, lesion_mean_b, lesion_mean_g, lesion_mean_r, bbox_x, bbox_y, bbox_w, bbox_h, success`.

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
* [PROPOSTA.md](./docs/PROPOSTA.md) — Proposta original e fundamentação do projeto (Etapa M1)
* [revisao_m1_plano_m2.md](./docs/revisao_m1_plano_m2.md) — Resposta ponto a ponto ao feedback da M1 e protocolo experimental da M2
* [relatorio_m2.md](./docs/relatorio_m2.md) — Relatório técnico completo da Etapa M2 (fundamentação matemática, experimentos controlados, métricas IoU/Dice, análise de falhas e plano M3)

---

## Uso de IA
* **Ferramenta:** Gemini;
* **Finalidade:** Ferramenta de produtividade, pesquisa eficiente, estruturação de documentação e sugestões de implementação;
* Os slides utilizados no vídeo foram produzidos com auxílio de IA, revisados e modificados pelos integrantes;
* Respostas e formulações matemáticas verificadas através das referências bibliográficas listadas na [proposta](./docs/PROPOSTA.md).