
## Parte A — Identificação da equipe e dos materiais conferidos

| Item                                   | Informação                                         |
| -------------------------------------- | -------------------------------------------------- |
| Identificação do grupo                 | Grupo 09                                           |
| Integrantes                            | Henrik Baltazar, Gabriel Teixeira, Gustavo Pedrini |
| Repositório avaliado                   | https://github.com/GabriellViictor/melanoma-pdi    |
| Commit, versão ou estado da entrega M1 | `6b3708a6edb5a4ca4ecbc74e7a57fcd59485e7a3`         |
| Documento de feedback consultado       | Devolutiva da M1 — Grupo 9                         |
| Data da conferência                    | 15/09/2026                                         |
| Nota informada na devolutiva           | 85,00                                              |
| Nota registrada no diário              | 8,50                                               |

## Parte B — Resposta ao feedback

|  Nº | Síntese da observação recebida                                                                      | Classificação do grupo | Evidência ou justificativa | Ação decorrente                                                                          |
| --: | --------------------------------------------------------------------------------------------------- | ---------------------- | -------------------------- | ---------------------------------------------------------------------------------------- |
|   1 | Problema técnico claro, mas há oscilação entre caracterização, classificação e auxílio diagnóstico. | Concordamos            | -                          | Foco em auxílio diagnóstico.                                                             |
|   2 | Quantidade e classes descritas; licença é vaga e um ZIP de dados foi versionado.                    | Concordamos            | -                          | Remover o zip do versionamento, adicionar mais informações sobre a licença e as classes. |
|   3 | Entradas e saídas estão claras; faltam protocolo e metas mensuráveis consolidadas.                  | Concordamos            | -                          | Definir os protocolos e mensurar as metas                                                |
|   4 | Etapas, alternativas, descritores, entradas, saídas e dúvidas são detalhados.                       | Concordamos            | -                          | -                                                                                        |
|   5 | Há inspeção de casos e fundamentação, mas não há processamento próprio verificável.                 | Concordamos            | -                          | Implementar o processamento verificável na próxima etapa.                                |
|   6 | CMake e estrutura existem; execução descrita ainda não corresponde ao programa presente.            | Concordamos            | -                          | Corrigir o versionamento de programas exemplos.                                          |
|   7 | Proposta rica, diagramas e referências; requer revisão de afirmações clínicas e licença.            | Concordamos            | -                          | -                                                                                        |
|   8 | Acesso e duração confirmados; conteúdo e participação requerem conferência.                         | Concordamos            | -                          | -                                                                                        |
|   9 | Histórico distribuído por vários dias e mais de um autor.                                           | Concordamos            | -                          | -                                                                                        |
### Síntese da posição do grupo 
Todos os pontos foram compreendidos, aceitos e nenhum ponto precisa de esclarecimento ou revisão. A crítica que terá maior impacto técnico na continuidade do projeto é a escolha de diagnóstico clínico, pois define o escopo do projeto até a entrega final.

## Parte C — Conferência das pontuações
| Critério   | Peso ou valor máximo | Nível atribuído | Pontuação registrada | Pontuação recalculada pelo grupo | Diferença |
| ---------- | -------------------: | --------------- | -------------------: | -------------------------------: | --------: |
| Critério 1 |                   15 | 11,25           |                11,25 |                                - |         - |
| Critério 2 |                   10 | 7,5             |                  7,5 |                                - |         - |
| Critério 3 |                   10 | 7,5             |                  7,5 |                                - |         - |
| Critério 4 |                   15 | 15              |                   15 |                                - |         - |
| Critério 5 |                   15 | 11,25           |                11,25 |                                - |         - |
| Critério 6 |                   10 | 7,5             |                  7,5 |                                - |         - |
| Critério 7 |                   10 | 10              |                   10 |                                - |         - |
| Critério 8 |                   10 | 10              |                   10 |                                - |         - |
| Critério 9 |                    5 | 5               |                    5 |                                - |         - |
| **Total**  |                  100 | 85              |                   85 |                                - |         - |
- [x] A soma está correta e coincide com a nota da devolutiva.

## Parte D — Conferência da nota registrada no diário

| Verificação                        | Valor |
| ---------------------------------- | ----: |
| Nota recalculada a partir da ficha |  85,0 |
| Nota apresentada na devolutiva     |  85,0 |
| Nota registrada no diário          |   8,5 |
| Diferença identificada             |     0 |
- [x] A nota registrada no diário está correta.
## Parte E — Diagnóstico da situação atual

| Componente                          | Situação atual | Evidência existente                              | Lacuna identificada                 |
| ----------------------------------- | -------------- | ------------------------------------------------ | ----------------------------------- |
| Definição e delimitação do problema | Adequado       |                                                  |                                     |
| Objetivo e saída esperada           | Adequado       |                                                  |                                     |
| Conjunto inicial de imagens         | Adequado       |                                                  |                                     |
| Pipeline proposto                   | Adequado       |                                                  |                                     |
| Evidência de viabilidade            | Adequado       |                                                  |                                     |
| Fundamentação e referências         | Parcial        | Falta de referência clínica e licença do projeto | Adicionar mais material de pesquisa |
| Organização do repositório          | Parcial        | Presença de programa exemplo e zip               | Deve ser removido                   |
| Relatório técnico                   | Adequado       |                                                  |                                     |
| Página parcial do projeto           | Adequado       |                                                  |                                     |
| Registro de decisões e limitações   | Adequado       |                                                  |                                     |

## Parte F — Plano de ação para a M2
| Prioridade | Problema identificado na M1  | Ação de correção                                  | Responsável     | Prazo  | Evidência de conclusão |
| ---------- | ---------------------------- | ------------------------------------------------- | --------------- | ------ | ---------------------- |
| Essencial  | referência clinica e licença | pesquisa e inclusão no repositório                | Gustavo Pedrini | 7 dias | Commit no repositório  |
| Importante | Programa exemplo e zip       | Remover o programa exemplo e o zip do repositório | Gustavo Pedrini | 7 dias | Commit no repositório  |

---

| Entrega ou resultado da M2       | Tarefa concreta | Responsável principal | Colaboradores | Prazo | Evidência esperada                                | Dependências ou riscos |
| -------------------------------- | --------------- | --------------------- | ------------- | ----- | ------------------------------------------------- | ---------------------- |
| Implementação inicial executável |                 |                       |               |       | Comando documentado e execução registrada         |                        |
| Segmentação                      |                 |                       |               |       | Imagem de máscara/resultado e parâmetros          |                        |
| Morfologia matemática            |                 |                       |               |       | Comparação antes/depois e justificativa           |                        |
| Extração de características      |                 |                       |               |       | Tabela, CSV ou registro dos descritores           |                        |
| Experimentos com parâmetros      |                 |                       |               |       | Configurações comparadas sob o mesmo protocolo    |                        |
| Testes e casos difíceis          |                 |                       |               |       | Imagens, resultados e análise de erros            |                        |
| Relatório técnico parcial        |                 |                       |               |       | Texto atualizado com métodos e resultados         |                        |
| Página parcial                   |                 |                       |               |       | Estado atual, comparação visual e próximos passos |                        |
| Organização e reprodutibilidade  |                 |                       |               |       | README, dependências e comandos atualizados       |                        |

---

| Elemento | Definição do grupo |
|---|---|
| Pergunta do experimento | |
| Imagens ou amostra utilizadas | |
| Método principal | |
| Configurações ou parâmetros comparados | |
| Resultado esperado | |
| Métrica ou critério de comparação | |
| Forma de registrar os resultados | |
| Limitações já previstas | |

---

| Integrante     | Responsabilidades principais                 | Artefatos ou partes sob sua responsabilidade | Como o trabalho será validado pelo grupo        |
| -------------- | -------------------------------------------- | -------------------------------------------- | ----------------------------------------------- |
| Gabriel Medina | Atenção aos prazos, organização das reuniões | Repositório e agenda                         | Atendimento dos prazos e carga de trabalho leve |
| Pedrini        | Pesquisa                                     | Material de estudo                           | Fonte de conhecimento                           |
| Henrik         | Código                                       | Código                                       | Códigos adicionados no repositório              |
## Parte G — Compromisso da equipe

> Declaramos que revisamos a devolutiva da M1, conferimos as pontuações e a nota registrada no diário, identificamos as correções necessárias e definimos um plano de ação para a M2. As discordâncias apresentadas neste documento estão acompanhadas de evidências localizáveis na entrega avaliada. Os integrantes conhecem as tarefas, os prazos e os resultados esperados para a próxima etapa.