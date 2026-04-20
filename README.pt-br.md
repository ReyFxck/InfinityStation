# InfinityStation – Frontend SNES para PlayStation 2

[![English](https://img.shields.io/badge/English-red)](README.md)
[![Português-BR](https://img.shields.io/badge/Português--BR-blue)](README.pt-BR.md)

InfinityStation é um frontend/launcher em desenvolvimento para executar um núcleo SNES no PlayStation 2, com foco em desempenho, modularização e usabilidade no hardware original.

> **Nota**
> Este projeto ainda está em desenvolvimento (**W.I.P**). O foco atual está em limpeza de código, modularização e melhorias de desempenho e usabilidade. Consulte a seção **Status atual e limitações** para entender o estágio atual do projeto.

## Índice

- [Pré-requisitos](#pré-requisitos)
- [Instalação](#instalação)
- [Comandos úteis](#comandos-úteis)
- [Formas de teste](#formas-de-teste)
- [Formatos de ROM suportados](#formatos-de-rom-suportados)
- [Estrutura do projeto](#estrutura-do-projeto)
- [Tecnologias utilizadas](#tecnologias-utilizadas)
- [Status atual e limitações](#status-atual-e-limitações)
- [Progresso](#progresso)
- [Planejamento futuro](#planejamento-futuro)
- [Como contribuir](#como-contribuir)
- [Licença e avisos legais](#licença-e-avisos-legais)
- [Créditos](#créditos)
- [Contato](#contato)

## Pré-requisitos

- PlayStation 2 ou emulador, para fins de teste.
- Ambiente de compilação PS2 configurado (`ps2sdk`, compilador `ee-gcc` e `make`).
- ROMs SNES compatíveis nos formatos `.sfc`, `.smc`, `.swc`, `.fig` ou `.zip`.

## Instalação

1. **Clonar o repositório**

   ```bash
   git clone https://github.com/ReyFxck/InfinityStation.git
   cd InfinityStation
   ```

2. **Compilar**

   Certifique-se de que `ps2sdk` e `make` estão instalados e configurados no `PATH`.

   ```bash
   make
   ```

   O build principal gera o ELF do projeto.

## Comandos úteis

```bash
make help
make clean
make rebuild
make V=1
make push
make push-android
make push-win WIN_OUT_DIR=/caminho
make iso
make iso ISO_ROMS_DIR=/pasta
make iso-push ISO_ROMS_DIR=/pasta
```

### Resumo rápido

- `make help` — mostra os comandos disponíveis.
- `make clean` — remove resíduos de builds anteriores.
- `make rebuild` — limpa e recompila o projeto.
- `make V=1` — compila com saída detalhada.
- `make push` / `make push-android` — copia o ELF para o diretório configurado no Android.
- `make push-win WIN_OUT_DIR=/caminho` — copia o ELF para um diretório no Windows.
- `make iso` — gera uma ISO de teste.
- `make iso ISO_ROMS_DIR=/pasta` — gera uma ISO incluindo ROMs da pasta informada.
- `make iso-push ISO_ROMS_DIR=/pasta` — gera a ISO e copia para o destino configurado.

## Formas de teste

### 1. Teste por ELF
- Compile o projeto.
- Copie o ELF gerado para um dispositivo USB ou cartão de memória.
- Execute pelo Free McBoot, LaunchELF ou outro gerenciador compatível.

### 2. Teste por ISO
- Gere uma ISO de teste com:
  ```bash
  make iso
  ```
- Para incluir ROMs na imagem:
  ```bash
  make iso ISO_ROMS_DIR=/pasta/com/roms
  ```

### 3. Teste em emulador
- O projeto pode ser testado em emuladores carregando o ELF diretamente.
- Quando aplicável, a ISO também pode ser usada para validação rápida do fluxo de boot.

## Formatos de ROM suportados

Os formatos atualmente considerados no fluxo do projeto são:

- `.sfc`
- `.smc`
- `.swc`
- `.fig`
- `.zip`

> **Observação:** o suporte prático pode variar conforme o estado atual do core, do loader e dos testes realizados.

## Estrutura do projeto

```text
.github/workflows/    # CI / build automatizado
libretro-common/      # utilitários compartilhados
ps2boot/              # frontend, integração PS2 e loader
source/               # código-base do core SNES
libretro.c            # ponte principal com o core libretro
Makefile              # build, push e geração de ISO
```

### Principais diretórios dentro de `ps2boot/`

- `ps2boot/app/`  
  Fluxo principal do app, boot, runtime, estado, transições, overlay e configuração do frontend.

- `ps2boot/ui/launcher/`  
  Launcher principal, browser, páginas, ações, fundo e fontes.

- `ps2boot/ui/select_menu/`  
  Menu in-game e páginas relacionadas.

- `ps2boot/platform/ps2/`  
  Código específico da plataforma PS2, incluindo módulos de áudio, debug, input, menu, storage e vídeo.

- `ps2boot/rom_loader/`  
  Loader de ROM e suporte a ZIP, incluindo integração com `miniz`.

## Tecnologias utilizadas

- **snes9x2005** — núcleo de emulação SNES adaptado para PS2.
- **miniz** — biblioteca para descompressão de arquivos ZIP.
- **ps2sdk** — SDK para desenvolvimento homebrew no PlayStation 2.
- **C / GCC** — linguagem e compilador principais do projeto.
- **GitHub Actions** — automação de build para validar compilação do projeto.

## Status atual e limitações

O projeto ainda possui vários bugs, mas já está em um estado jogável via emulação.

Limitações conhecidas no momento:

- o áudio ainda não está funcionando;
- podem ocorrer quedas de FPS mesmo com folga de EE;
- alguns menus ainda apresentam comportamentos incorretos, como confusão na abertura da ROM correta ou no carregamento por USB;
- algumas animações estão rápidas demais;
- o menu de configurações da launcher ainda não foi implementado;
- músicas no launcher seguem desativadas por causa dos problemas atuais com áudio.

## Progresso

### Menus

| Área | Otimização | Progresso / Adição | Bugs / Melhorias |
|---|---:|---:|---:|
| Menu da launcher inicial | 50% | 65% | 10% |
| Menu de configuração (launcher inicial) | 0% | 0% | 0% |
| Menu de créditos (launcher inicial) | 10% | 30% | 75% |
| Seleção de dispositivos | 60% | 10% | 95% |

### Game / Core / Sistema

| Área | Otimização | Progresso / Adição | Bugs / Status |
|---|---:|---:|---:|
| Áudio no game | 60% | 50% | 100% *(sem áudio)* |
| Otimizações gerais | 50% | 50% | 50% |
| GS | ? | ? | ? |
| Suporte NTSC/PAL | ? | ? | ? |
| Suporte a funções do core | 60% | 30% | 75% |
| Funções de tela (PS2) | 50% | 45% | 30% |

## Planejamento futuro

- Mais opções no launcher e no menu in-game.
- Melhor feedback visual e mensagens de erro.
- Mais testes de compatibilidade com ROMs.
- Polimento visual e aprimoramento da interface.
- Documentação mais completa e tradução.
- Mais testes em hardware real.

## Como contribuir

Contribuições são bem-vindas. Para colaborar:

1. Abra uma *issue* com sugestões, dúvidas ou problemas encontrados.
2. Faça um *fork* do repositório e crie uma *branch* para sua alteração.
3. Após implementar e testar, envie um *pull request* com uma descrição clara das mudanças.
4. Sinta-se à vontade para ajudar com documentação, tradução ou novos recursos.

## Licença e avisos legais

Este repositório reúne código próprio e componentes derivados ou integrados de projetos de terceiros.

Antes de redistribuir, reutilizar ou relicenciar qualquer parte do projeto, consulte:

- o arquivo `copyright`;
- os créditos do repositório;
- os cabeçalhos e avisos presentes nos arquivos herdados.

Isso ajuda a evitar confusão entre o código original do projeto e os componentes externos incorporados.

## Créditos

Agradecimentos especiais a:

- **Iaddis** (`@iaddis`) — ideias observadas nos arquivos do projeto Snesticle para implementação do carregamento de arquivos ZIP.
- **Rich Geldreich** (`@richgel999`) — por criar a biblioteca `miniz`, usada na descompactação de ZIP.
- **Comunidade do Snes9x e @libretro** — pelo núcleo `snes9x2005` e pelos trabalhos relacionados.
- **Contribuidores do `ps2sdk` / `ps2dev`** (`@ps2dev`) — pelo esforço em tornar o desenvolvimento para PS2 possível.

## Contato

Para dúvidas, sugestões ou colaboração, use o [GitHub Issues](https://github.com/ReyFxck/InfinityStation/issues) ou entre em contato com o mantenedor.