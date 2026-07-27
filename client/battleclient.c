#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_MSG 128

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[MAX_MSG];

    // Criando socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    // Configurando endereço do servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertendo IP
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Endereço inválido/Não suportado");
        exit(1);
    }

    // Conectando ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Conexão falhou");
        exit(1);
    }

    printf("Conectado ao servidor.\n");

    // Loop de comunicação
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Erro no select");
            break;
        }

        // Se receber algo do servidor
        if (FD_ISSET(sock, &readfds)) {
            int n = recv(sock, buffer, MAX_MSG - 1, 0);
            if (n <= 0) {
                printf("Servidor desconectou.\n");
                break;
            }
            buffer[n] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }

        // Se o usuário digitar algo
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, MAX_MSG, stdin) == NULL) {
                break;
            }
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}