#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define TAB_SIZE 8
#define MAX_MSG 128
#define TOTAL_NAVIOS 4
#define PORT 8080

// Tabuleiros: 0 = vazio, 1..4 = navio, -1 = atingido, 9 = água atacada
int tab1[TAB_SIZE][TAB_SIZE];
int tab2[TAB_SIZE][TAB_SIZE];

typedef struct {
    int tamanho;
    int partes_afundadas;
} Navio;

Navio navios_p1[TOTAL_NAVIOS];
Navio navios_p2[TOTAL_NAVIOS];

// Verifica se o navio cabe na posição com a orientação dada
int cabe_navio(int x, int y, int tam, char orient) {
    if (orient == 'H') {
        if (x + tam > TAB_SIZE) return 0;
    } else if (orient == 'V') {
        if (y + tam > TAB_SIZE) return 0;
    } else {
        return 0;
    }
    return 1;
}

// Verifica se a posição já está ocupada por outro navio
int pos_ocupada(int tab[TAB_SIZE][TAB_SIZE], int x, int y, int tam, char orient) {
    int i;
    if (orient == 'H') {
        for (i = 0; i < tam; i++) {
            if (tab[y][x+i] != 0) return 1;
        }
    } else {
        for (i = 0; i < tam; i++) {
            if (tab[y+i][x] != 0) return 1;
        }
    }
    return 0;
}

// Marca as posições do navio no tabuleiro
void posicionar_navio(int tab[TAB_SIZE][TAB_SIZE], int navio_id, int x, int y, int tam, char orient) {
    int i;
    if (orient == 'H') {
        for (i = 0; i < tam; i++) {
            tab[y][x+i] = navio_id; // id do navio (1..4)
        }
    } else {
        for (i = 0; i < tam; i++) {
            tab[y+i][x] = navio_id;
        }
    }
}

// Verifica se todos os navios foram afundados
int todos_afundados(Navio navios[]) {
    int i;
    for (i = 0; i < TOTAL_NAVIOS; i++) {
        if (navios[i].partes_afundadas < navios[i].tamanho)
            return 0; // ainda tem parte viva
    }
    return 1; // todos afundados
}

// Processa um tiro no tabuleiro do adversário
// Retorna: 0=MISS, 1=HIT, 2=SUNK
int processar_tiro(int tab[TAB_SIZE][TAB_SIZE], Navio navios[], int x, int y) {
    if (x < 0 || x >= TAB_SIZE || y < 0 || y >= TAB_SIZE) return 0;
    if (tab[y][x] == 0) return 0; // água = miss

    if (tab[y][x] > 0) {
        int navio_id = tab[y][x] - 1;
        tab[y][x] = -1; // marca como atingido
        navios[navio_id].partes_afundadas++;

        if (navios[navio_id].partes_afundadas == navios[navio_id].tamanho)
            return 2; // sunk
        else
            return 1; // hit
    }
    return 0;
}

int main() {
    int server_fd, player1_fd, player2_fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char buffer[MAX_MSG];
    int n;

    // Inicializa tabuleiros e navios
    memset(tab1, 0, sizeof(tab1));
    memset(tab2, 0, sizeof(tab2));

    Navio navios_tipo[TOTAL_NAVIOS] = {
        {1,0}, // Submarino
        {2,0}, // Fragata 1
        {2,0}, // Fragata 2
        {3,0}  // Destroyer
    };

    memcpy(navios_p1, navios_tipo, sizeof(navios_tipo));
    memcpy(navios_p2, navios_tipo, sizeof(navios_tipo));

    // Cria socket servidor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 2) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Aguardando jogadores...\n");

    // Aceita jogador 1
    player1_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
    if (player1_fd < 0) {
        perror("accept");
        exit(1);
    }
    send(player1_fd, "Digite seu nome (JOIN <nome>):\n", 31, 0);
    n = recv(player1_fd, buffer, MAX_MSG-1, 0);
    buffer[n] = '\0';

    send(player1_fd, "AGUARDE JOGADOR\n", 16, 0);

    // Aceita jogador 2
    player2_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
    if (player2_fd < 0) {
        perror("accept");
        exit(1);
    }
    send(player2_fd, "Digite seu nome (JOIN <nome>):\n", 31, 0);
    n = recv(player2_fd, buffer, MAX_MSG-1, 0);
    buffer[n] = '\0';

    // Informa início do jogo
    send(player1_fd, "JOGO INICIADO\nVocê é o Jogador 1\n", 33, 0);
    send(player2_fd, "JOGO INICIADO\nVocê é o Jogador 2\n", 33, 0);

    // Navios para posicionar
    const char *navios[] = {"SUBMARINO", "FRAGATA", "FRAGATA", "DESTROYER"};
    int navios_tamanho[] = {1, 2, 2, 3};

    // ******** POSICIONAMENTO JOGADOR 1 ********
    int navios_posicionados = 0;
    while (navios_posicionados < TOTAL_NAVIOS) {
        snprintf(buffer, sizeof(buffer), "POSICIONE: POS %s <x> <y> <H/V>\n", navios[navios_posicionados]);
        send(player1_fd, buffer, strlen(buffer), 0);

        n = recv(player1_fd, buffer, MAX_MSG-1, 0);
        if (n <= 0) { close(player1_fd); close(player2_fd); return 1; }
        buffer[n] = '\0';

        char cmd[8], tipo[16], orient;
        int x, y;

        if(sscanf(buffer, "%7s %15s %d %d %c", cmd, tipo, &x, &y, &orient) != 5 ||
           strcmp(cmd, "POS") != 0 ||
           strcmp(tipo, navios[navios_posicionados]) != 0) {
            send(player1_fd, "Formato inválido! Use POS <tipo> <x> <y> <H/V>\n", 44, 0);
            continue;
        }

        int tam = navios_tamanho[navios_posicionados];

        if(orient != 'H' && orient != 'V') {
            send(player1_fd, "Orientação inválida.\n", 21, 0);
            continue;
        }
        if(x < 0 || x >= TAB_SIZE || y < 0 || y >= TAB_SIZE) {
            send(player1_fd, "Coordenadas inválidas.\n", 23, 0);
            continue;
        }
        if(!cabe_navio(x, y, tam, orient)) {
            send(player1_fd, "Navio não cabe na posição.\n", 27, 0);
            continue;
        }
        if(pos_ocupada(tab1, x, y, tam, orient)) {
            send(player1_fd, "Posição já ocupada.\n", 20, 0);
            continue;
        }

        posicionar_navio(tab1, navios_posicionados + 1, x, y, tam, orient);
        navios_posicionados++;
        send(player1_fd, "Navio posicionado com sucesso!\n", 30, 0);
    }

    send(player1_fd, "Aguardando o jogador 2 para começar...\n", 39, 0);

    // ******** POSICIONAMENTO JOGADOR 2 ********
    navios_posicionados = 0;
    while (navios_posicionados < TOTAL_NAVIOS) {
        snprintf(buffer, sizeof(buffer), "POSICIONE: POS %s <x> <y> <H/V>\n", navios[navios_posicionados]);
        send(player2_fd, buffer, strlen(buffer), 0);

        n = recv(player2_fd, buffer, MAX_MSG-1, 0);
        if (n <= 0) { close(player1_fd); close(player2_fd); return 1; }
        buffer[n] = '\0';

        char cmd[8], tipo[16], orient;
        int x, y;

        if(sscanf(buffer, "%7s %15s %d %d %c", cmd, tipo, &x, &y, &orient) != 5 ||
           strcmp(cmd, "POS") != 0 ||
           strcmp(tipo, navios[navios_posicionados]) != 0) {
            send(player2_fd, "Formato inválido! Use POS <tipo> <x> <y> <H/V>\n", 44, 0);
            continue;
        }

        int tam = navios_tamanho[navios_posicionados];

        if(orient != 'H' && orient != 'V') {
            send(player2_fd, "Orientação inválida.\n", 21, 0);
            continue;
        }
        if(x < 0 || x >= TAB_SIZE || y < 0 || y >= TAB_SIZE) {
            send(player2_fd, "Coordenadas inválidas.\n", 23, 0);
            continue;
        }
        if(!cabe_navio(x, y, tam, orient)) {
            send(player2_fd, "Navio não cabe na posição.\n", 27, 0);
            continue;
        }
        if(pos_ocupada(tab2, x, y, tam, orient)) {
            send(player2_fd, "Posição já ocupada.\n", 20, 0);
            continue;
        }

        posicionar_navio(tab2, navios_posicionados + 1, x, y, tam, orient);
        navios_posicionados++;
        send(player2_fd, "Navio posicionado com sucesso!\n", 30, 0);
    }

    send(player2_fd, "Aguardando o jogador 1 para começar...\n", 39, 0);

    // Ambos enviam READY para iniciar o jogo
    send(player1_fd, "Envie READY para iniciar o jogo.\n", 32, 0);
    send(player2_fd, "Envie READY para iniciar o jogo.\n", 32, 0);

    while(1) {
        n = recv(player1_fd, buffer, MAX_MSG-1, 0);
        if(n <= 0) { close(player1_fd); close(player2_fd); return 1; }
        buffer[n] = '\0';
        if(strcmp(buffer, "READY\n") == 0 || strcmp(buffer, "READY") == 0)
            break;
        send(player1_fd, "Por favor, envie READY para continuar.\n", 40, 0);
    }

    while(1) {
        n = recv(player2_fd, buffer, MAX_MSG-1, 0);
        if(n <= 0) { close(player1_fd); close(player2_fd); return 1; }
        buffer[n] = '\0';
        if(strcmp(buffer, "READY\n") == 0 || strcmp(buffer, "READY") == 0)
            break;
        send(player2_fd, "Por favor, envie READY para continuar.\n", 40, 0);
    }

    // INÍCIO DO JOGO
    send(player1_fd, "INÍCIO DO JOGO\nVocê começa!\n", 29, 0);
    send(player2_fd, "INÍCIO DO JOGO\nAguarde seu turno...\n", 31, 0);

    int vez = 1;
    int x, y;
    int resultado;

    while(1) {
        int current_fd = (vez == 1) ? player1_fd : player2_fd;
        int other_fd = (vez == 1) ? player2_fd : player1_fd;
        int (*tab_jogador)[TAB_SIZE] = (vez == 1) ? tab2 : tab1; // jogador ataca o tab do adversário
        Navio *navios_adversario = (vez == 1) ? navios_p2 : navios_p1;

        send(current_fd, "SEU TURNO: FIRE x y\n", 19, 0);

        while(1) {
            n = recv(current_fd, buffer, MAX_MSG-1, 0);
            if(n <= 0) { close(player1_fd); close(player2_fd); return 1; }
            buffer[n] = '\0';

            if(sscanf(buffer, "FIRE %d %d", &x, &y) != 2) {
                send(current_fd, "Comando inválido. Use FIRE x y\n", 31, 0);
                continue;
            }

            if(x < 0 || x >= TAB_SIZE || y < 0 || y >= TAB_SIZE) {
                send(current_fd, "Coordenadas inválidas.\n", 23, 0);
                continue;
            }

            if(tab_jogador[y][x] == -1 || tab_jogador[y][x] == 9) {
                send(current_fd, "JA ATACOU\n", 10, 0);
                continue;
            }

            resultado = processar_tiro(tab_jogador, navios_adversario, x, y);

            if(resultado == 0) {
                tab_jogador[y][x] = 9; // marca água atacada
                send(current_fd, "MISS\n", 5, 0);
                send(other_fd, "AGUARDE TURNO...\n", 16, 0);
                vez = (vez == 1) ? 2 : 1;
                break;
            }
            else if(resultado == 1) {
                send(current_fd, "HIT\n", 4, 0);
                send(other_fd, "HIT\n", 4, 0);
            }
            else if(resultado == 2) {
                send(current_fd, "SUNK\n", 5, 0);
                send(other_fd, "SUNK\n", 5, 0);
            }

            if(todos_afundados(navios_adversario)) {
                send(current_fd, "WIN\n", 4, 0);
                send(other_fd, "LOSE\n", 5, 0);
                send(current_fd, "END\n", 4, 0);
                send(other_fd, "END\n", 4, 0);
                close(player1_fd);
                close(player2_fd);
                close(server_fd);
                return 0;
            }
        }
    }

    return 0;
}