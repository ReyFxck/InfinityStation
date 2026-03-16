# PS2 SNES Concept

[English](README.md)

Um frontend/boot para PlayStation 2 voltado para executar um core de SNES, com launcher personalizado, menu in-game, suporte a ROM ZIP e código específico para PS2.

## Status

**W.I.P**

Este projeto ainda está em construção e em melhoria contínua.

O trabalho recente foi focado em:
- dividir arquivos grandes em módulos menores
- melhorar a manutenção do projeto
- melhorar a usabilidade no lado do PS2
- preparar a base de código para futuras correções e adições

## Inspirado Em

Este projeto é fortemente inspirado em:
- **PGEN**
- **SNESticle**
- **SNESStation**

Não como cópia direta de apenas um deles, mas como parte da ideia geral, do estilo e da direção para criar uma experiência mais usável de emulação e launcher no PlayStation 2.

## Recursos Atuais

- Opção de iniciar ROM embutida
- Navegador/launcher por USB
- Menu in-game
- Ajuste de posição da imagem
- Opções de proporção de tela
- Overlay de FPS
- Opções de limite de frames
- Suporte para carregar ROMs ZIP
- Código modularizado para app, launcher, menu e vídeo do PS2

## Progresso

Os trabalhos recentes incluem:
- divisão de arquivos grandes do launcher em módulos menores
- divisão das ações e da renderização das páginas do menu
- divisão do código de vídeo do PS2 em partes separadas
- divisão das responsabilidades de inicialização, runtime e callbacks do app
- adição de carregamento modular de ROM
- adição de suporte a ZIP com `miniz`
- limpeza e consolidação do `Makefile`

## Melhorias Planejadas

- mais opções no launcher
- melhor feedback visual e mensagens de erro
- mais testes de compatibilidade de ROMs
- mais polimento visual
- mais documentação
- mais testes em hardware real

## Créditos

- **Iaddis** (**@iaddis**)  
  Algumas ideias sobre como implementar a abertura de arquivos ZIP.

- **Rich Geldreich** (**@richgel999**)  
  `miniz`, usado na parte de descompactação e suporte a ZIP.

- **Snes9x e comunidade**, junto com **@libretro**  
  Pelo core `snes9x2005` e por trabalhos relacionados usados neste projeto.

- **Contribuidores do ps2sdk / ps2dev** (**@ps2dev**)  
  Obrigado pelo esforço para tornar mais fácil compilar aplicações para o PlayStation 2.

## Observações

Isto não representa uma versão final e totalmente polida.

Atualmente o projeto está focado em:
- limpeza de código
- modularização
- melhorias de usabilidade no PS2
- facilitar o trabalho futuro

## Licença

Veja o arquivo `LICENSE` do repositório.
