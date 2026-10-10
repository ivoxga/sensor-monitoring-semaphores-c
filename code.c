// Cabeçalho necessário para utlizar pthreads no windows 
#define PTHREADS_STATIC_LIB_LOCAL_PATH "..\\packages\\aerospike-client-c-dependencies.1.0.2\\build\\native\\lib\\x64\\Release"
#pragma comment(lib, PTHREADS_STATIC_LIB_LOCAL_PATH"\\pthreadVC2.lib")
#pragma comment(lib, "winmm.lib")
#define HAVE_STRUCT_TIMESPEC
#define PTW32_STATIC_LIB
#include <pthread.h>
#include <windows.h>
#include <timeapi.h>

// Librarias necessárioas para o desenvolvimento do programa
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <process.h>

#define BUFFER_SIZE 15

// Buffers
float temp_buffer[BUFFER_SIZE];     // buffer para as temperaturas
float hum_buffer[BUFFER_SIZE];      // buffer para as humidades 
int temp_index = 0, hum_index = 0;  // indices para os buffers

// Semaforos
sem_t sem_tempBuffer, sem_humBuffer;    // sincronização dos buffers
sem_t sem_temp, sem_hum;                // sincronização dos controladores

// Contadores que indica quantos sensores terminaram
int hum_sensors_ok = 0;     // sensores de humidade
int temp_sensors_ok = 0;	// sensores de temperatura

// Funções para adicionar valores aos buffers
// 
// Adiciona um valor ao buffer de temperaturas
void add_to_temp_buffer(float temp) {
    sem_wait(&sem_tempBuffer);
    temp_buffer[temp_index % BUFFER_SIZE] = temp;
    temp_index++;
    sem_post(&sem_tempBuffer);
}
// Adiciona um valor ao buffer de humidades
void add_to_hum_buffer(float hum) {
    sem_wait(&sem_humBuffer);
    hum_buffer[hum_index % BUFFER_SIZE] = hum;
    hum_index++;
    sem_post(&sem_humBuffer);
}

// Funções para calcular a média dos valores dos buffers
// 
// Calcula a média das ultimas 5 temperaturas
float media_temp() {
    float soma = 0;
    int nElementos, start;
    float local_buffer[BUFFER_SIZE];
    int local_index;

    // Copiar valores
    sem_wait(&sem_tempBuffer);                              // bloqueia o semáforo
	memcpy(local_buffer, temp_buffer, sizeof(temp_buffer)); // copia o buffer para um buffer local
	local_index = temp_index;                               // copia o índice para um índice local
	sem_post(&sem_tempBuffer);                              // desbloqueia o semáforo


    // Define o limite e o índice inicial
    if (local_index < 5) {
        nElementos = local_index;   // usa apenas os elementos disponíveis
        start = 0;                  // começa no início do buffer
    }
    else {
        nElementos = 5;             // considera os últimos 5 valores
        start = local_index - 5;    // índice inicial para os últimos 5 valores
    }

    // Calcula a soma dos últimos valores
    for (int i = start; i < local_index; i++)
        soma += local_buffer[i % BUFFER_SIZE];

    // Retorna a média dos últimos valores
    if (local_index == 0)
        return 0;
    else
        return soma / nElementos;
}
// Calcula a média das ultimas 5 humidades
float media_hum() {
    float soma = 0;
    int nElementos, start;
    float local_buffer[BUFFER_SIZE];
    int local_index;

    // Copiar valores
    sem_wait(&sem_humBuffer);                               // bloqueia o semáforo
    memcpy(local_buffer, hum_buffer, sizeof(hum_buffer));   // copia o buffer para um buffer local
    local_index = hum_index;                                // copia o índice para um índice local
    sem_post(&sem_humBuffer);                               // desbloqueia o semáforo

    // Define o limite e o índice inicial
    if (local_index < 5) {
        nElementos = local_index;   // usa apenas os elementos disponíveis
        start = 0;                  // começa no início do buffer
    }
    else {
        nElementos = 5;             // considera os últimos 5 valores
        start = local_index - 5;    // índice inicial para os últimos 5 valores
    }

    // Calcula a soma dos últimos valores
    for (int i = start; i < local_index; i++)
        soma += local_buffer[i % BUFFER_SIZE];

    // Retorna a média dos últimos valores
    if (local_index == 0)
        return 0;
    else
        return soma / nElementos;
}

// Funções de controlo simuladas
// 
// Função de controlo do aquecedor
void controlador_aquecedor(int estado, float* temperatura) {
	static int estadoAnterior = -1;    // estado anterior do aquecedor

    if (estado != estadoAnterior) {
        printf("\nAquecedor %s\n", estado == 0 ? "[OFF]" : "[ON]");
        estadoAnterior = estado;
    }

    if (estado == 1)
        *temperatura += 0.5f;
}
// Função de controlo do desumidificador
void controlador_desumidificador(int estado, int* humidade) {
	static int estadoAnterior = -1;    // estado anterior do desumidificador

    if (estado != estadoAnterior) {
        printf("\nDesumidificador %s\n", estado == 0 ? "[OFF]" : "[ON]");
        estadoAnterior = estado;
    }

    if (estado == 1)
        *humidade += 1;
}

// Threads do Processo Temperatura
// 
// Regista a temperatura do sensor 1 -> 3 em 3 segundos
void* temp_sensor1(void* arg) {
    for (int i = 0; i < 25; i++) {
        float temp = ((rand() % 1000) / 10.0f) + 1;
        add_to_temp_buffer(temp);
        printf("T1: Sensor 1 - Temperatura: %.1f\n", temp);
		sem_post(&sem_temp);  // notifica T3 para calcular a média
        Sleep(3000);
    }
	
	// Incrementa o contador de sensores terminados
    sem_wait(&sem_tempBuffer);
    temp_sensors_ok++;
	sem_post(&sem_tempBuffer);
    
	// Notifica T3 que o sensor acabou a leitura
    sem_post(&sem_temp);

    return NULL;
}
// Regista a temperatura do sensor 2 -> 5 em 5 segundos
void* temp_sensor2(void* arg) {
    for (int i = 0; i < 30; i++) {
        float temp = ((rand() % 1000) / 10.0f) + 1;
        add_to_temp_buffer(temp);
        printf("T2: Sensor 2 - Temperatura: %.1f\n", temp);
		sem_post(&sem_temp);    // notifica T3 para calcular a média
        Sleep(5000);
    }

	// Incrementa o contador de sensores terminados
	sem_wait(&sem_tempBuffer);
    temp_sensors_ok++;
	sem_post(&sem_tempBuffer);

	// Notifica T3 que o sensor acabou a leitura
	sem_post(&sem_temp);

    return NULL;
}
// Controla o aquecedor e mostrar a média das ultimas 5 temperaturas registadas
void* temp_controlador(void* arg) {
    while (1) {
		sem_wait(&sem_temp);    // bloqueia o semáforo para calcular a média

		// Verifica se os dois sensores terminaram
        sem_wait(&sem_tempBuffer);
        if (temp_sensors_ok == 2) {
            sem_post(&sem_tempBuffer);
            break;
        }
		sem_post(&sem_tempBuffer);

        float media = media_temp();
        printf("T3: Media das ultimas 5 temperaturas: %.1f\n", media);
        controlador_aquecedor(media < 18, &media);  // liga o aquecedor se a média for inferior a 18
        Sleep(1000);

    }
    return NULL;
}

// Threads do Processo Humidade
//
// Regista a humidade do sensor 1 -> 8 em 8 segundos
void* hum_sensor1(void* arg) {
    for (int i = 0; i < 15; i++) {
        float hum = ((rand() % 1000) / 10.0f) + 1;
        add_to_hum_buffer(hum);
        printf("H1: Sensor 1 - Humidade: %.1f\n", hum);
		sem_post(&sem_hum);     // notifica H3 para calcular a média
        Sleep(8000);
    }
	
	// Incrementa o contador de sensores terminados
    sem_wait(&sem_humBuffer);
    hum_sensors_ok++;
	sem_post(&sem_humBuffer);

	// Notifica H3 que o sensor acabou a leitura
	sem_post(&sem_hum);

    return NULL;
}
// Regista a humidade do sensor 2 -> 8 em 8 segundos
void* hum_sensor2(void* arg) {
    for (int i = 0; i < 25; i++) {
        float hum = ((rand() % 1000) / 10.0f) + 1;
        add_to_hum_buffer(hum);
        printf("H2: Sensor 2 - Humidade: %.1f\n", hum);
		sem_post(&sem_hum);     // notifica H3 para calcular a média
        Sleep(8000);
    }

	// Incrementa o contador de sensores terminados
	sem_wait(&sem_humBuffer);
    hum_sensors_ok++;
	sem_post(&sem_humBuffer);

	// Notifica H3 que o sensor acabou a leitura
	sem_post(&sem_hum);
    
    return NULL;
}
// Controla o desumidificador e mostrar a média das ultimas 5 humidades registadas
void* hum_controlador(void* arg) {
    while (1) {
		sem_wait(&sem_hum); // bloqueia o semáforo para calcular a média

		// Verifica se os dois sensores terminaram
        sem_wait(&sem_humBuffer);
        if (hum_sensors_ok == 2) {
            sem_post(&sem_humBuffer);
            break;
        }
        sem_post(&sem_humBuffer);

        float media = media_hum();
        printf("H3: Media das ultimas 5 humidades: %.1f\n", media);
        controlador_desumidificador(media > 60, &media);    // liga o desumidificador se a média for superior a 60
        Sleep(1000);
    }
    return NULL;
}

// Processos
// 
// Temperatura
void* processo_temperatura(void* arg) {
	pthread_t t1, t2, t3;
	sem_init(&sem_temp, 0, 0);
	pthread_create(&t1, NULL, temp_sensor1, NULL);      // cria thread para o sensor 1
	pthread_create(&t2, NULL, temp_sensor2, NULL);      // cria thread para o sensor 2
	pthread_create(&t3, NULL, temp_controlador, NULL);  // cria thread para o controlador
	pthread_join(t1, NULL);                             // espera que a thread 1 termine
	pthread_join(t2, NULL);                             // espera que a thread 2 termine
	pthread_join(t3, NULL);                             // espera que a thread 3 termine
    return NULL;
}
// Humidade
void* processo_humidade(void* arg) {
	pthread_t h1, h2, h3;
	sem_init(&sem_hum, 0, 0);
	pthread_create(&h1, NULL, hum_sensor1, NULL);       // cria thread para o sensor 1
	pthread_create(&h2, NULL, hum_sensor2, NULL);       // cria thread para o sensor 2
	pthread_create(&h3, NULL, hum_controlador, NULL);   // cria thread para o controlador
	pthread_join(h1, NULL);                             // espera que a thread 1 termine
	pthread_join(h2, NULL);                             // espera que a thread 2 termine
	pthread_join(h3, NULL);                             // espera que a thread 3 termine
    return NULL;
}

// Processo Principal
int main() {
    // Inicializa o gerador de números aleatórios
	srand((unsigned int)time(NULL)); 

	// Inicializa os semáforos
    sem_init(&sem_tempBuffer, 0, 1);
    sem_init(&sem_humBuffer, 0, 1);

	// Inicializa os processos
    pthread_t temp_thread, hum_thread;
    pthread_create(&temp_thread, NULL, processo_temperatura, NULL);
    pthread_create(&hum_thread, NULL, processo_humidade, NULL);

	// Espera que os processos terminem
    pthread_join(temp_thread, NULL);
    pthread_join(hum_thread, NULL);

    printf("Execucao Finalizada.\n");

	// Destroi os semáforos
    sem_destroy(&sem_tempBuffer);
    sem_destroy(&sem_humBuffer);

    return 0;
}