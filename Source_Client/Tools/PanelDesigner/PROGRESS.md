# Panel9 Layout Designer — status (2026-08-17)

Ferramenta visual pra montar interfaces em Lua (`Panel9.lua`) sem ficar
trocando número, recompilando e tirando print. Roda como programa
Python normal ou como `.exe` empacotado — não depende do navegador nem
de mim online.

Este arquivo existe só pra continuar de onde parou, não é doc de usuário
final (isso já está no topo do próprio `panel_designer.py`).

## Onde está tudo

```
Source_Client/
├── Tools/
│   ├── ImageConverter/
│   │   ├── png_to_mu.py      -- PNG → .OZT/.OZJ (com transparência ou opaco)
│   │   ├── remove_bg.py      -- remove fundo preto do Midjourney (flood-fill,
│   │   │                        preserva buracos internos tipo o miolo de um botão)
│   │   └── mu_decode.py      -- .OZT/.OZJ → PNG (o inverso, usado pela ferramenta
│   │                             visual pra carregar os assets reais do jogo direto)
│   └── PanelDesigner/
│       ├── panel_designer.py       -- a ferramenta em si (Tkinter)
│       ├── Panel9LayoutDesigner.exe -- empacotado (PyInstaller), so abrir
│       └── PROGRESS.md             -- este arquivo
├── Lua-Source/
│   ├── System/
│   │   └── Panel9.lua        -- motor de painel 9-slice reutilizavel
│   └── Panel9Demo.lua        -- o unico script "de verdade" ativo hoje
│                                (painel + botao fechar + 1 botao "cancel")
└── Global Debug/Data/Custom/
    ├── Panel9/stone.OZJ      -- fundo do painel (pedra c/ moldura de bronze embutida)
    └── Common/close_btn_x.OZT -- botao de fechar (X dourado)
```

`MenuGrid.lua` e `PrimeCenterDemo.lua` foram removidos a pedido (ficou só
o `Panel9Demo.lua` simples). Se quiser recriar uma tela tipo "PRIME
CENTER" (título + grade de botões), a base pra isso (`MenuGrid`) já foi
feita uma vez nessa sessão — se precisar de novo, é só pedir, não
precisa reinventar.

## Como abrir a ferramenta amanhã

- Mais rápido: clica duas vezes em `Tools\PanelDesigner\Panel9LayoutDesigner.exe`.
- Pra mexer no código: `python panel_designer.py` de dentro da pasta
  `Tools\PanelDesigner\` (precisa `pip install pillow numpy`).
- Depois de editar `panel_designer.py`, pra atualizar o `.exe`:
  ```
  cd Tools\PanelDesigner
  pyinstaller --onefile --windowed --paths "../ImageConverter" --name Panel9LayoutDesigner panel_designer.py
  copy dist\Panel9LayoutDesigner.exe .
  rmdir /s /q build dist
  del Panel9LayoutDesigner.spec
  ```

## O que já funciona (testado ou corrigido nesta sessão)

- **Fundo do painel**: arraste um asset na "Fundo" (quadradinho na
  lateral) pra trocar a textura de fundo. Ajusta o tamanho do painel
  automaticamente pro tamanho real da imagem.
- **Botão de fechar**: arraste um asset em cima do X (no canvas) pra
  trocar a textura dele.
- **Botões (elementos)**: arraste um asset em qualquer outro lugar do
  painel pra criar um botão novo — arrastável, redimensionável (puxa o
  cantinho azul), selecionável, aparece na lista "ELEMENTOS" da lateral.
- **Biblioteca**: mostra miniaturas dos 606 arquivos `.OZT`/`.OZJ` de
  `Data\Interface` (arte nativa do próprio jogo), carrega em segundo
  plano sem travar a abertura.
- **Histórico ("timeline")**: cada troca de fundo/botão de fechar e cada
  redimensionamento vira uma linha na lista "HISTORICO". Clica pra
  voltar pra aquele ponto. "Remover selecionado" apaga uma entrada sem
  necessariamente mudar o estado atual.
- **Código Lua**: atualiza ao vivo na caixa de texto, incluindo o bloco
  `Panel9Demo_Elements` quando há botões colocados. Botão "Copiar
  codigo" bota na área de transferência.
- **Salvar e implantar**: edita o `Panel9Demo.lua` direto (só as linhas
  de tamanho/posição do painel e botão de fechar — nunca os elementos,
  isso é sempre manual) e já criptografa pro jogo pegar. Não depende de
  Python instalado nem no `.exe` (a criptografia foi reimplementada
  dentro do próprio programa).

## Bugs achados e corrigidos nesta sessão (pra não repetir o mesmo caminho)

1. **Lag ao redimensionar** — corrigido: reduz a imagem de origem uma vez
   só ao carregar, usa reamostragem mais rápida (BILINEAR) durante o
   arrastar ao vivo.
2. **`.OZT` decodificava devagar** — corrigido com numpy (era um loop
   pixel a pixel em Python puro, ~600x mais lento).
3. **Matemática de redimensionar errada** — o painel fica sempre
   centralizado, e a fórmula antiga não levava isso em conta (cada frame
   compondo sobre uma referência já deslocada). Corrigida pra resolver
   direto a partir do centro fixo da tela.
4. **Área de "soltar" do botão de fechar roubava soltas do painel** — o
   botão de fechar se sobrepõe de propósito ao canto do painel
   (visual), mas a detecção de onde você soltou usava a caixa inteira
   dele. Reduzida pra 60% central, só na hora de soltar.
5. **Arte de canto ornamentado não alinhava com a moldura da pedra** —
   são duas gerações independentes do Midjourney, nunca vão bater
   pixel a pixel. Decisão: removido o canto separado, ficou só a
   moldura embutida na própria textura de pedra (que já tem cantinhos
   discretos e fica limpo sozinha).
6. **Sobra de pedra irregular além da linha de bronze** — corrigido
   recortando a imagem de origem bem rente à moldura (achei o ponto
   exato medindo pixels, não no olho).

## Pendências / não testado ainda (começar por aqui amanhã)

- **O arrastar do mouse em si nunca foi testado por um humano depois
  dos últimos 3-4 rounds de correção** (esse ambiente não tem tela
  gráfica pra eu simular clique/arrastar — só testei a lógica por
  script, sem interface). Prioridade #1: abrir o `.exe` e testar
  redimensionar (handle do painel), mover o botão de fechar, e
  arrastar um asset da biblioteca pros 3 alvos (fundo/fechar/painel).
- O botão "cancel" recém-adicionado ao `Panel9Demo.lua` (fecha o
  painel) ainda não foi visto em jogo.
- Só existe **um** elemento (`cancel`) no painel de exemplo hoje — a
  tabela `Panel9Demo_Elements` e o campo `action` por botão já estão
  prontos pra receber mais, sem precisar mexer no loop genérico.
- A versão web (artifact "Panel9 Layout Designer",
  `https://claude.ai/code/artifact/a9efe7bc-eb84-4b57-83bb-fef6b8c23721`)
  ficou **desatualizada** — parei de manter ela quando o foco virou o
  `.exe` standalone. Se quiser continuar usando a web em vez do `.exe`,
  avisar pra eu sincronizar as mesmas correções lá.
- Não tem como redimensionar/mover a JANELA da ferramenta em si pra
  telas menores ainda (não testado se ela cabe em resoluções baixas).

## Ideias pra continuar (não pedidas ainda, só anotando)

- Adicionar mais botões ao `Panel9Demo.lua` via a ferramenta (ex: um
  "confirm" do lado do "cancel"?).
- Suporte a texto por elemento (título/label desenhado junto com o
  botão), já que `DrawText` existe no motor mas a ferramenta não expõe
  isso ainda.
- Talvez trazer de volta o `MenuGrid`/`PrimeCenterDemo` (removido a
  pedido) se a ideia de uma tela com título + grade de botões voltar à
  tona.
