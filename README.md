# Análise e Segmentação de Lesões Cutâneas para Auxílio no Diagnóstico de Melanoma

**Processamento Digital de Imagens (PDI)**  
**Etapa:** Definição, Escopo, Viabilidade e Planejamento

---

## Integrantes
* **Gabriel Victor Garcia Teixeira**
* **Gustavo Pedrini**
* **Henrik Baltazar**

---

## Problema Investigado

O problema investigado consiste no isolamento e na caracterização quantitativa de lesões cutâneas a partir de imagens dermatoscópicas digitais, buscando mitigar a subjetividade da inspeção visual humana na triagem do melanoma.
O desafio central sob a ótica de Processamento Digital de Imagens (PDI) reside em tratar matrizes de pixels brutas, frequentemente degradadas por ruídos como pelos sobrepostos, variações de iluminação e transições difusas com a pele sadia, para segmentar com precisão os contornos da lesão e extrair descritores matemáticos morfológicos e cromáticos (como assimetria, compacidade de bordas e dispersão de cores) capazes de diferenciar computacionalmente lesões benignas de lesões malignas.

---

## Contexto de Aplicação
O melanoma é a forma mais letal de câncer de pele, cuja taxa de cura está diretamente associada à precocidade do diagnóstico. O conjunto de dados adotado compreende **13.900 imagens dermatoscópicas** padronizadas na resolução de $224 \times 224$ pixels, rotuladas entre lesões **benignas** e **malignas (melanoma)**.

A triagem clínica visual tradicional enfrenta desafios críticos:
* **Alta variabilidade inter-examinador:** Lesões em estágios iniciais compartilham características visuais sutis com nevos benignos (pintas comuns);
* **Heterogeneidade morfológica:** Melanomas manifestam polimorfismo acentuado, apresentando crescimento assimétrico, bordas recortadas e multiplicidade de padrões cromáticos.

---

## Objetivo Geral

---

## Visão Resumida da Solução Proposta


---

## Conjunto e Origem das Imagens
* **Origem:** [Melanoma Cancer Dataset (Kaggle)](https://www.kaggle.com/datasets/bhaveshmittal/melanoma-cancer-dataset/data).
* **Composição:** Mais de 10.000 imagens dermatoscópicas divididas entre as classes `Benign` (Nevos benignos) e `Malignant` (Melanoma).
* **Formato e Resolução:** Imagens coloridas (RGB) em formato `.jpg` / `.png`.
* **Condições de Uso:** O dataset está disponível publicamente para fins educacionais e de pesquisa científica.

---

##  Estágio Atual do Projeto (M1)


---

## Organização do Repositório

```text
melanoma-pdi/
├── CMakeLists.txt              # Configuração do CMake para compilação C++ e OpenCV
├── README.md                   # Síntese navegável e porta de entrada do projeto
├── .gitignore                  # Arquivos e pastas ignoradas pelo controle de versão
├── docs/
│   └── proposta.md             # Documento formal e detalhado da proposta M1
├── images/
   ├── input/                  # Amostras de imagens dermatoscópicas para testes
   └── results/                # Resultados visuais dos processamentos preliminares
```

---

## Tecnologias Previstas e Utilizadas
* **Linguagem:** C++ 
* **Biblioteca de Visão Computacional:** OpenCV 4.x
* **Sistema de Build:** CMake (versão mínima 3.20)
* **Controle de Versão:** Git & GitHub

---

## Instruções de Compilação e Execução

---

## Vídeo de Apresentação da Etapa M1
* **Link do Vídeo

---

## Documentação Adicional

