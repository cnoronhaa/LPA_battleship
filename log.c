#include <stdio.h>
#include <time.h>
#include <stdarg.h>

FILE *log_file;

// Funcao que escreve no log com timestamp
void log_event(const char *format, ...) {
    if (!log_file) return;

    // Adiciona timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log_file, "[%02d/%02d/%04d %02d:%02d:%02d] ",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec);

    // Escreve mensagem formatada
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file); // gravar no arquivo imediatamente
}

int main() {
    log_file = fopen("server.log", "a");
    if (!log_file) {
        perror("Erro abrindo arquivo de log");
        return 1;
    }

    log_event("Servidor iniciado na porta 8080");

    // Quando alguem conectar:
    log_event("Jogador 1 conectado.");

    // Quando alguem disparar:
    log_event("Jogador 1 disparou em (%d,%d)", x, y);

    // Quando o jogo terminar:
    log_event("Jogador 2 venceu a partida.");

    // No final do programa:
    fclose(log_file);

    return 0;
}
