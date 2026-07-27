# BattleShip - Jogo de Batalha Naval em C (TCP)

## Descricao
Projeto de um jogo multiplayer de Batalha Naval desenvolvido em linguagem C, utilizando comunicacao via sockets TCP (modelo cliente-servidor). O servidor gerencia a logica do jogo, e dois jogadores se conectam como clientes, posicionam seus navios e realizam ataques alternadamente.


## Como compilar

Execute no terminal, na raiz do projeto:

```bash
make
```

Isso gerara os binarios:
- `server/battleserver`
- `client/battleclient`


## Como executar

### Inicie o servidor:

```bash
./server/battleserver
```

### Em seguida, abra **dois terminais diferentes** e execute o cliente em ambos:

```bash
./client/battleclient
```


## Comandos do Protocolo

| Comando | Descricao                                      |
|--------|-------------------------------------------------|
| JOIN   | Informa o nome do jogador (JOIN <nome>)         |
| POS    | Posiciona um navio (`POS <tipo> <x> <y> <H/V>`) |
| READY  | Indica que esta pronto                          |
| FIRE   | Ataca uma posicao (`FIRE <x> <y>`)              |
| HIT    | Informa que o tiro acertou                      |
| MISS   | Informa que o tiro errou                        |
| WIN    | Vitoria do jogador                              |
| LOSE   | Derrota do jogador                              |


## Estrutura do Projeto

```
battleship/
├── Makefile
├── protocol.h
├── README.md
├── server/
│   └── battleserver.c
├── client/
│   └── battleclient.c
```

## Requisitos

- Sistema Linux
- Compilador GCC
- Biblioteca POSIX sockets (ja inclusa no Linux)


## Exemplo de uso
- Jogador 1 envia: `JOIN Alice`
- Jogador 2 envia: `JOIN Bob`
- Cada jogador posiciona seus navios com o comando POS.
- Apos ambos digitarem READY, a partida inicia.
- No seu turno, dispare com `FIRE x y`.
- Vence quem afundar todos os navios do oponente.


