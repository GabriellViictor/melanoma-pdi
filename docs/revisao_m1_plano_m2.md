# Revisão da Etapa M1 e Plano de Ação para a Etapa M2

## Parte A — Identificação da equipe e dos materiais conferidos

| Item | Informação |
| :--- | :--- |
| Identificação do grupo | Grupo 09 |
| Integrantes | Henrik Baltazar, Gabriel Victor Garcia Teixeira, Gustavo Pedrini |
| Repositório avaliado | https://github.com/GabriellViictor/melanoma-pdi |
| Commit, versão ou estado da entrega M1 | `6b3708a6edb5a4ca4ecbc74e7a57fcd59485e7a3` |
| Documento de feedback consultado | Devolutiva da M1 — Grupo 9 |
| Data da conferência | 15/09/2026 |
| Nota informada na devolutiva | 85,00 |
| Nota registrada no diário | 8,50 |

---

## Parte B — Resposta ao feedback

| Nº | Síntese da observação recebida | Classificação do grupo | Evidência ou justificativa | Ação decorrente (implementada na M2) |
| --: | :--- | :--- | :--- | :--- |
| 1 | Problema técnico claro, mas há oscilação entre caracterização, classificação e auxílio diagnóstico. | Concordamos | Na M1, o texto alternava entre triagem clínica e diagnóstico autônomo. | Escopo consolidado em **auxílio à triagem dermatoscópica baseada na regra ABCD** (`README.md`, `docs/PROPOSTA.md` e `docs/relatorio_m2.md`). |
| 2 | Quantidade e classes descritas; licença é vaga e um ZIP de dados foi versionado. | Concordamos | O arquivo `dataset.zip` havia sido versionado e faltava detalhar a licença pública do Kaggle. | `*.zip` e `images/input/train|test` adicionados ao `.gitignore`; scripts `scripts/download_dataset.sh` e `.ps1` criados; licença e distribuição exata ($13.879$ imagens) documentadas. |
| 3 | Entradas e saídas estão claras; faltam protocolo e metas mensuráveis consolidadas. | Concordamos | A proposta M1 listava os módulos, mas sem protocolo experimental fixo e métrica quantitativa. | Protocolo experimental controlado definido (`docs/relatorio_m2.md`) com métricas quantitativas de sobreposição (`IoU`/`Jaccard` e `Dice` em `ImageProcessor::compareMasks`) e invariantes geométricos ABCD. |
| 4 | Etapas, alternativas, descritores, entradas, saídas e dúvidas são detalhados. | Concordamos | Ponto avaliado com nota máxima na M1. | Mantido e expandido com a implementação completa em C++/OpenCV (`src/core/ImageProcessor.cpp`). |
| 5 | Há inspeção de casos e fundamentação, mas não há processamento próprio verificável. | Concordamos | Na M1 havia apenas inspeção visual das amostras `7280.jpg` e `6068.jpg`. | Implementado pipeline completo e verificável em C++/OpenCV gerando $6$ artefatos intermediários por imagem e relatório `metrics.csv`. |
| 6 | CMake e estrutura existem; execução descrita ainda não corresponde ao programa presente. | Concordamos | Na M1 o `CMakeLists.txt` compilava apenas um exemplo básico desconectado do `README.md`. | `CMakeLists.txt` reestruturado com biblioteca `melanoma_core`, CLI `melanoma_pdi` e suíte `melanoma_tests` (CTest), 100% alinhados ao `README.md`. |
| 7 | Proposta rica, diagramas e referências; requer revisão de afirmações clínicas e licença. | Concordamos | Afirmações sobre diagnóstico foram ajustadas para suporte quantitativo à decisão clínica (segunda opinião). | Revisão textual aplicada em `README.md`, `docs/PROPOSTA.md` e `docs/relatorio_m2.md`, incluindo referências clínicas (Nachbar et al., 1994; Lee et al., 1997) e licença do dataset. |
| 8 | Acesso e duração confirmados; conteúdo e participação requerem conferência. | Concordamos | Vídeo M1 entregue e validado. | Para o vídeo da M2 (~10 min), roteiro estruturado garantindo demonstração de execução real e participação equilibrada dos três integrantes. |
| 9 | Histórico distribuído por vários dias e mais de um autor. | Concordamos | Ponto avaliado com nota máxima na M1. | Mantida cadência incremental de commits semânticos por todos os integrantes na M2. |

### Síntese da posição do grupo
Todos os pontos da devolutiva M1 foram compreendidos e aceitos integralmente. A principal consolidação de escopo consistiu em posicionar o sistema de forma rigorosa como uma **ferramenta de auxílio à triagem clínica (segunda opinião quantitativa baseada na regra ABCD)**, eliminando ambiguidades sobre diagnóstico médico autônomo e estabelecendo um protocolo experimental reprodutível com código C++/OpenCV verificável.

---

## Parte C — Conferência das pontuações

| Critério | Peso ou valor máximo | Nível atribuído | Pontuação registrada | Pontuação recalculada pelo grupo | Diferença |
| :--- | ---: | ---: | ---: | ---: | ---: |
| Critério 1 | 15 | 11,25 | 11,25 | 11,25 | 0,00 |
| Critério 2 | 10 | 7,50 | 7,50 | 7,50 | 0,00 |
| Critério 3 | 10 | 7,50 | 7,50 | 7,50 | 0,00 |
| Critério 4 | 15 | 15,00 | 15,00 | 15,00 | 0,00 |
| Critério 5 | 15 | 11,25 | 11,25 | 11,25 | 0,00 |
| Critério 6 | 10 | 7,50 | 7,50 | 7,50 | 0,00 |
| Critério 7 | 10 | 10,00 | 10,00 | 10,00 | 0,00 |
| Critério 8 | 10 | 10,00 | 10,00 | 10,00 | 0,00 |
| Critério 9 | 5 | 5,00 | 5,00 | 5,00 | 0,00 |
| **Total** | **100** | **85,00** | **85,00** | **85,00** | **0,00** |

- [x] A soma está correta e coincide com a nota da devolutiva.

---

## Parte D — Conferência da nota registrada no diário

| Verificação | Valor |
| :--- | ---: |
| Nota recalculada a partir da ficha | 85,0 |
| Nota apresentada na devolutiva | 85,0 |
| Nota registrada no diário | 8,5 |
| Diferença identificada | 0,0 |

- [x] A nota registrada no diário está correta.

---

## Parte E — Diagnóstico da situação atual (pós-M1 e status na M2)

| Componente | Situação na M1 | Evidência existente (M2) | Lacuna identificada na M1 e solucionada na M2 |
| :--- | :--- | :--- | :--- |
| Definição e delimitação do problema | Parcial (75%) | `README.md` e `docs/relatorio_m2.md` | Unificada a terminologia em torno de triagem e extração ABCD para auxílio diagnóstico. |
| Objetivo e saída esperada | Parcial (75%) | `src/main.cpp` e `metrics.csv` | Definidos protocolo experimental, $6$ saídas visuais por imagem e CSV com $33$ colunas. |
| Conjunto inicial de imagens | Parcial (75%) | `scripts/download_dataset.sh`, `DatasetManager` | Removido `.zip` do Git, documentada licença pública e validada contagem de $13.879$ imagens. |
| Pipeline proposto | Adequado (100%) | `src/core/ImageProcessor.hpp` e `.cpp` | Pipeline integralmente implementado nas etapas 1 a 4 (Classificador SVM planejado para M3). |
| Evidência de viabilidade | Parcial (75%) | `ctest` (`melanoma_tests`) e artefatos visuais | Substituída inspeção puramente visual por processamento real em C++/OpenCV. |
| Fundamentação e referências | Adequado (100%) | `docs/PROPOSTA.md` e `docs/relatorio_m2.md` | Revisadas afirmações clínicas e detalhada formulação matemática de cada etapa. |
| Organização do repositório | Parcial (75%) | `CMakeLists.txt`, `.gitignore`, `README.md` | Removido código placeholder; alinhados nomes de executáveis (`melanoma_pdi` e `melanoma_tests`). |
| Relatório técnico | Adequado (100%) | `docs/relatorio_m2.md` | Expandido com experimentos controlados, tabelas quantitativas e análise de casos de falha. |
| Página parcial do projeto | Adequado (100%) | `README.md` | Atualizada com instruções completas de CLI, parâmetros e exemplos reprodutíveis. |
| Registro de decisões e limitações | Adequado (100%) | `docs/relatorio_m2.md` (Seções 6 e 7) | Documentadas limitações de pelos densos, lesões amelanóticas e plano para M3. |

---

## Parte F — Plano de ação executado na M2

### 1. Correções das pendências da M1

| Prioridade | Problema identificado na M1 | Ação de correção | Responsável | Prazo | Evidência de conclusão |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Essencial | Ausência de processamento próprio verificável e desalinhamento entre CMake e README | Implementação de `DatasetManager`, `ImageProcessor`, CLI `melanoma_pdi` e testes CTest | Henrik Baltazar | Concluído | `CMakeLists.txt`, `src/core/ImageProcessor.cpp`, `src/main.cpp`, `tests/test_dataset_io.cpp` |
| Essencial | Arquivo `.zip` de dados versionado e falta de automação de download/licença | Exclusão do `.zip` via `.gitignore`, criação dos scripts `download_dataset` e documentação da licença | Gustavo Pedrini & Henrik Baltazar | Concluído | `.gitignore`, `scripts/download_dataset.sh`, `scripts/download_dataset.ps1`, `README.md` |
| Importante | Falta de protocolo experimental consolidado e metas mensuráveis | Definição de experimentos controlados (com/sem inpainting; CIE $L^*a^*b^*$ vs. Grayscale; Otsu vs. GrabCut) e métricas `IoU`/`Dice` | Gabriel Teixeira & Gustavo Pedrini | Concluído | `docs/relatorio_m2.md` e `ImageProcessor::compareMasks` |

---

### 2. Planejamento e entregas da Etapa M2

| Entrega ou resultado da M2 | Tarefa concreta | Responsável principal | Colaboradores | Prazo | Evidência esperada | Dependências ou riscos |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Implementação inicial executável | Desenvolver CLI `melanoma_pdi` e gerenciador `DatasetManager` com resolução automática de diretórios | Henrik Baltazar | Gabriel Teixeira | Concluído | Compilação limpa via CMake/Ninja e execução CLI documentada no `README.md` | Compatibilidade entre versões do OpenCV 4.x e 5.x (resolvida via `__has_include`) |
| Segmentação | Implementar preparação de canal CIE $L^*a^*b^*$, máscara de Campo de Visão (FoV), correção flat-field, Otsu e GrabCut | Henrik Baltazar | Gustavo Pedrini | Concluído | Artefatos `<id>_3_channel.png`, `<id>_4_raw_otsu.png`, `<id>_5_final_mask.png` e `<id>_6_contour_overlay.png` | Vinhetamento escuro de lente (`6068.jpg`) e bordas difusas |
| Morfologia matemática | Implementar detecção de pelos *DullRazor* (Black-Hat $7\times 7$ + filtro direcional em $8$ orientações + proteção de núcleo) e abertura/fechamento da máscara | Henrik Baltazar | Gabriel Teixeira | Concluído | Artefatos `<id>_1_hair_mask.png` e `<id>_2_inpainted.png` preservando o interior de lesões escuras (`1.jpg`) e limpando pelos (`7280.jpg`) | Fios muito espessos ou da mesma cor do núcleo da lesão |
| Extração de características | Implementar descritores ABCD completos (Assimetria biaxial centrada no centroide, Momentos de Hu, Compacidade, Variância Radial, Eixos de Elipse e Cores na ROI) | Henrik Baltazar | Gustavo Pedrini | Concluído | Geração automática do arquivo `metrics.csv` com $33$ colunas e validação via CTest | Sensibilidade do perímetro a ruídos residuais de pelos |
| Experimentos com parâmetros | Executar comparações controladas variando `--no-inpaint`, `--channel <lab|gray>`, `--no-vignette-filter` e `--seg-method <otsu|grabcut>` | Gustavo Pedrini | Henrik Baltazar, Gabriel Teixeira | Concluído | Tabelas comparativas e interpretação técnica em `docs/relatorio_m2.md` | Manter protocolo e amostras constantes entre execuções |
| Testes e casos difíceis | Avaliar casos de sucesso (`6068.jpg`, `1.jpg`) e casos desafiadores/limitações (`7280.jpg`, `6299.jpg`) + teste sintético com `IoU`/`Dice` | Gustavo Pedrini | Henrik Baltazar | Concluído | Seção de análise de casos de sucesso e falha em `docs/relatorio_m2.md` e suíte `melanoma_tests` | Ausência de máscaras ground-truth pixel a pixel no dataset original do Kaggle |
| Relatório técnico parcial | Redigir `docs/relatorio_m2.md` cobrindo evolução M1$\to$M2, protocolo, métodos, experimentos e plano M3 | Gustavo Pedrini | Gabriel Teixeira, Henrik Baltazar | Concluído | Arquivo `docs/relatorio_m2.md` completo e referenciado no `README.md` | Alinhamento com todos os critérios da rubrica M2 |
| Página parcial e Reprodutibilidade | Atualizar `README.md` com instruções de compilação, flags da CLI, exemplos práticos e descrição das saídas | Gabriel Teixeira | Henrik Baltazar | Concluído | `README.md` atualizado e testado em Linux e Windows | Nenhuma |

---

### 3. Protocolo Experimental da Etapa M2

| Elemento | Definição do grupo |
| :--- | :--- |
| **Pergunta do experimento** | **Exp. 1:** A remoção morfológica de pelos (*DullRazor* em dois estágios + Inpainting de Telea) reduz artefatos de borda e preserva o núcleo da lesão em comparação à segmentação sem inpainting (`--no-inpaint`)? <br>**Exp. 2:** O canal perceptual CIE $L^*a^*b^*$ com supressão de vinhetamento (FoV + *flat-field*) e limiarização de Otsu supera a escala de cinza bruta (`--channel gray --no-vignette-filter`) e como se compara ao refinamento por grafos (`--seg-method grabcut`)? |
| **Imagens ou amostra utilizadas** | **Conjunto de benchmark qualitativo e de estresse:** `docs/7280.jpg` (Malignant com densa cobertura de pelos cruzados) e `docs/6068.jpg` (Benign com anel de vinhetamento, régua dermatoscópica e marcação cirúrgica verde). <br>**Amostra estratificada de avaliação do dataset ($N = 12$):** $3$ amostras determinísticas de cada subconjunto (`train/Benign`: `1`, `10`, `100`; `train/Malignant`: `1`, `10`, `100`; `test/Benign`: `6299`, `6300`, `6301`; `test/Malignant`: `5602`, `5603`, `5604`) + **Imagem sintética controlada** com máscara *ground-truth* exata em `melanoma_tests`. |
| **Método principal** | Pipeline modular em 5 estágios: (1) Black-Hat $7\times 7$ + colinearidade direcional em $8$ ângulos + proteção de núcleo + Inpainting de Telea; (2) Filtro Bilateral + Canal CIE $L^*a^*b^*$ realçado para melanina com máscara FoV e correção *flat-field*; (3) Limiarização de Otsu restrita ao FoV; (4) Abertura ($5\times 5$), Fechamento ($9\times 9$), seleção de componente conexo ponderada pela distância central e preenchimento de buracos; (5) Extração de descritores ABCD. |
| **Configurações ou parâmetros comparados** | **Configuração A (Padrão Proposta):** `--channel lab --seg-method otsu` (com inpainting e filtro de vinheta ativos). <br>**Configuração B (Sem Remoção de Pelos):** `--no-inpaint --channel lab --seg-method otsu`. <br>**Configuração C (Sem Supressão de Vinheta / Escala de Cinza):** `--channel gray --no-vignette-filter --seg-method otsu`. <br>**Configuração D (Segmentação por Grafos):** `--channel lab --seg-method grabcut`. |
| **Resultado esperado** | Espera-se que a Configuração A isole corretamente o nevo central em `6068.jpg` (rejeitando vinheta e tinta cirúrgica), preserve integralmente o núcleo escuro de lesões pequenas (`train/Benign/1.jpg`) ao remover fios isolados, e alcance $\text{IoU} > 0,40$ / $\text{Dice} > 0,55$ no benchmark sintético com pelos sobrepostos. |
| **Métrica ou critério de comparação** | 1. **Métricas de sobreposição espacial:** *Intersection over Union* ($\text{IoU}$) e coeficiente de *Sørensen-Dice* ($\text{Dice}$) via `ImageProcessor::compareMasks` sobre máscara de referência sintética e concordância entre métodos; <br>2. **Estabilidade de descritores morfológicos ABCD:** Área ($A$), Perímetro ($P$), Compacidade ($\frac{P^2}{4\pi A}$), Circularidade ($\frac{4\pi A}{P^2}$), Solidez, Assimetria Biaxial ($\in [0, 1]$) e Variância Radial; <br>3. **Taxa de sucesso de segmentação válida** (`success = 1` no lote avaliado). |
| **Forma de registrar os resultados** | Geração automatizada dos $6$ artefatos PNG por imagem (`_1_hair_mask` a `_6_contour_overlay`), exportação tabular em `metrics.csv` ($33$ colunas) e consolidação analítica em tabelas comparativas em `docs/relatorio_m2.md`. |
| **Limitações já previstas** | 1. Em lesões com rede densa de pelos pretos sobre pigmentação castanha (`7280.jpg`), resíduos de inpainting na periferia ainda elevam a compacidade da borda; <br>2. Lesões muito extensas ou descentralizadas que tocam a borda da imagem (`test/Benign/6299.jpg`) sofrem truncamento pelo limite do FoV; <br>3. O dataset Kaggle possui rótulos de classe (`Benign`/`Malignant`), mas não fornece máscaras *ground-truth* manuais pixel a pixel para as $13.879$ imagens. |

---

### 4. Divisão de responsabilidades na equipe

| Integrante | Responsabilidades principais | Artefatos ou partes sob sua responsabilidade | Como o trabalho será validado pelo grupo |
| :--- | :--- | :--- | :--- |
| **Gabriel Victor Garcia Teixeira** | Gestão de prazos, organização do repositório, documentação de uso (`README.md`) e estruturação do pipeline inicial | `README.md`, `CMakeLists.txt`, revisão de reprodutibilidade e preparação da apresentação M2 | Verificação de build limpo, execução dos comandos do `README.md` e revisão cruzada pelos integrantes |
| **Gustavo Pedrini** | Pesquisa bibliográfica e clínica, especificação do protocolo experimental, análise das métricas ABCD e redação técnica | `docs/PROPOSTA.md`, `docs/revisao_m1_plano_m2.md`, `docs/relatorio_m2.md` e interpretação dos casos de sucesso/falha | Conferência de aderência à rubrica M2, exatidão das equações matemáticas e revisão textual |
| **Henrik Baltazar** | Arquitetura de software C++, implementação e refinamento dos algoritmos de PDI em OpenCV e suíte de testes automatizados | `src/io/DatasetManager.*`, `src/core/ImageProcessor.*`, `src/main.cpp`, `tests/test_dataset_io.cpp` e scripts de dataset | Aprovação em 100% dos testes automatizados (`ctest`) e inspeção visual das máscaras geradas |

---

## Parte G — Compromisso da equipe

> Declaramos que revisamos a devolutiva da M1, conferimos as pontuações e a nota registrada no diário, identificamos as correções necessárias e executamos o plano de ação para a M2. Todas as melhorias e experimentos encontram-se implementados e documentados de forma reprodutível no repositório. Os integrantes conhecem as tarefas, os prazos e os resultados esperados para a conclusão do projeto na Etapa M3.
