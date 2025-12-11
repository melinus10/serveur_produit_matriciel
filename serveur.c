#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>
#include <syslog.h>

// Définition des structures
typedef struct {
    pid_t pid;         // PID du client
    int n;             // Nombre de lignes de la matrice A
    int m;             // Nombre de colonnes de la matrice A et lignes de B
    int p;             // Nombre de colonnes de la matrice B
    int borne;         // Borne supérieure des éléments des matrices
} Requete;

typedef struct {
    int n, m, p;       // Dimensions des matrices
    int i, j;          // Indices (i, j) pour C[i][j]
    int* A;            // Matrice A
    int* B;            // Matrice B
    int* C;            // Matrice C
} ThreadArgs;

// Constantes
#define MAIN_TUBE "main_tube"
#define SHARED_MEMORY_NAME "/matrix_shared_memory_"
#define SEM_NAME "/matrix_sync_sem_"

// Gestion des signaux
void gestion_zombies(int sig) {
    (void)sig;
    while (wait(NULL) > 0);
}

void terminer_serveur(int sig) {
    (void)sig;
    printf("\nTerminaison du serveur...\n");
    unlink(MAIN_TUBE);
    exit(EXIT_FAILURE);
}

// Fonction pour générer une matrice aléatoire
void random_mat(int row, int col, int borne, int* mat) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            mat[i * col + j] = rand() % (borne + 1);
        }
    }
}

// Fonction exécutée par les threads pour calculer un élément de C
void* calculer_element(void* args) {
    ThreadArgs* data = (ThreadArgs*)args;
    int sum = 0;
    for (int k = 0; k < data->m; k++) {
        sum += data->A[data->i * data->m + k] * data->B[k * data->p + data->j];
    }
    data->C[data->i * data->p + data->j] = sum;
    pthread_exit(NULL);
}

// Fonction pour écrire une matrice dans un tube
void ecrire_matrice(int fd, int *matrice, int lignes, int colonnes) {
    if (write(fd, &lignes, sizeof(int)) != sizeof(int)) {
        perror("Erreur d'écriture : lignes");
        exit(EXIT_FAILURE);
    }
    if (write(fd, &colonnes, sizeof(int)) != sizeof(int)) {
        perror("Erreur d'écriture : colonnes");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < lignes; i++) {
        for (int j = 0; j < colonnes; j++) {
            if (write(fd, &matrice[i * colonnes + j], sizeof(int)) != sizeof(int)) {
                perror("Erreur d'écriture dans le fichier");
                close(fd);
                exit(EXIT_FAILURE);
            }
        }
    }
}


// Fonction pour transformer le processus en démon
void demon() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("Erreur lors du fork pour le démon");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("Erreur lors de la création de la session");
        exit(EXIT_FAILURE);
    }

    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) {
        perror("Erreur lors du second fork pour le démon");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY); 
    open("/dev/null", O_WRONLY); 
    open("/dev/null", O_RDWR);   

    openlog("serveur_matrices", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Démon démarré avec succès");
}


int main() {
	
	demon();
	    
   struct sigaction sa;

    // Gestion de SIGCHLD 
    sa.sa_handler = gestion_zombies;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    // Gestion de SIGINT 
    sa.sa_handler = terminer_serveur;
    sa.sa_flags = 0; 
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Gestion de SIGTERM 
    sa.sa_handler = terminer_serveur;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    
    // Création du tube principal
    unlink(MAIN_TUBE);
    if (mkfifo(MAIN_TUBE, 0666) == -1) {
        perror("Erreur création du tube principal");
        exit(EXIT_FAILURE);
    }


    int fd_tube_principal = open(MAIN_TUBE, O_RDONLY);
    if (fd_tube_principal == -1) {
        perror("Erreur ouverture du tube principal");
        unlink(MAIN_TUBE);
        exit(EXIT_FAILURE);
    }

    while (1) {
        Requete req;
        ssize_t bytes_read = read(fd_tube_principal, &req, sizeof(req));
        if (bytes_read > 0) {

            pid_t pid_requete = fork();
            if (pid_requete == -1) {
                perror("Erreur fork gestion requête");
                continue; // Ne pas bloquer le serveur
            }

            if (pid_requete == 0) { // Processus fils pour cette requête
                char tube_reponse[50];
                sprintf(tube_reponse, "reponse_%d", req.pid);

                // Création d'un nom unique pour la mémoire partagée et le sémaphore
                char shm_name[100], sem_name[100];
                sprintf(shm_name, "%s%d", SHARED_MEMORY_NAME , req.pid);
                sprintf(sem_name, "%s%d", SEM_NAME , req.pid);

                size_t taille_mem = sizeof(int) * ((size_t)req.n * (size_t)req.m + (size_t)req.m * (size_t)req.p + (size_t)req.n * (size_t)req.p);
                int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
                if (shm_fd == -1) {
                    perror("Erreur création segment mémoire partagée");
                    exit(EXIT_FAILURE);
                }
                if (ftruncate(shm_fd, (off_t)taille_mem) == -1) {
                    perror("Erreur redimensionnement mémoire partagée");
                    shm_unlink(shm_name);
                    exit(EXIT_FAILURE);
                }
                void* shared_mem = mmap(NULL, taille_mem, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if (shared_mem == MAP_FAILED) {
                    perror("Erreur mapping mémoire partagée");
                    shm_unlink(shm_name);
                    exit(EXIT_FAILURE);
                }

                int* A = (int*)shared_mem;

                // Générer la matrice A
                random_mat(req.n, req.m, req.borne, A);

                sem_t* sem = sem_open(sem_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
                if (sem == SEM_FAILED) {
                    perror("Erreur création sémaphore");
                    shm_unlink(shm_name);
                    exit(EXIT_FAILURE);
                }

                pid_t pidB = fork();
                if (pidB == -1) {
                    perror("Erreur fork WorkerB");
                    exit(EXIT_FAILURE);
                }

                if (pidB == 0) { // WorkerB
                    sem_wait(sem); // attendre workerA

                    int* B = A + req.n * req.m;
                    int* C = B + req.m * req.p;

                    random_mat(req.m, req.p, req.borne, B);

                    pthread_t threads[req.n * req.p];
                    ThreadArgs args[req.n * req.p];
                    int thread_index = 0;

                    for (int i = 0; i < req.n; i++) {
                        for (int j = 0; j < req.p; j++) {
                            args[thread_index] = (ThreadArgs){req.n, req.m, req.p, i, j, A, B, C};
                            pthread_create(&threads[thread_index], NULL, calculer_element, &args[thread_index]);
                            thread_index++;
                        }
                    }

                    for (int i = 0; i < req.n * req.p; i++) {
                        pthread_join(threads[i], NULL);
                    }

                    sem_close(sem);
                    munmap(shared_mem, taille_mem);
                    exit(0);
                }

                sem_post(sem); // signaler que A à fini  
                waitpid(pidB, NULL, 0); // attendre le processus B et avoir le résultat

                int fd_tube_reponse = open(tube_reponse, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                if (fd_tube_reponse == -1) {
                    perror("Erreur ouverture du tube réponse");
                    munmap(shared_mem, taille_mem);
                    shm_unlink(shm_name);
                    exit(EXIT_FAILURE);
                }

                int* C = A + req.n * req.m + req.m * req.p;
                int* B = A + req.n * req.m;

                ecrire_matrice(fd_tube_reponse, A, req.n, req.m); // Matrice A
                ecrire_matrice(fd_tube_reponse, B, req.m, req.p); // Matrice B
                ecrire_matrice(fd_tube_reponse, C, req.n, req.p); // Matrice C

                close(fd_tube_reponse);

                sem_close(sem);
                munmap(shared_mem, taille_mem);
                shm_unlink(shm_name);
                exit(0);
            }

            waitpid(pid_requete, NULL, 0);
        }
    }

    return 0;
}
