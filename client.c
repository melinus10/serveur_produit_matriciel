#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct requete {
    pid_t pid;
    int n;
    int m;
    int p;
    int borne_supp;
};

#define MAIN "main_tube"

int main(int argc, char **argv) {
     struct requete req; // Déclaration de la requête
    int f;

     // Vérification du nombre d'arguments
    if (argc != 5) {
        fprintf(stderr, "Usage : %s <n> <m> <p> <borne_supp>\n", argv[0]);
        fprintf(stderr, "  <n> : Nombre de lignes de A \n");
        fprintf(stderr, "  <m> : Nombre de colonnes de A et lignes de B \n");
        fprintf(stderr, "  <p> : Nombre de colonnes de B \n");
        fprintf(stderr, "  <borne_supp> : Borne supérieure des éléments des matrices \n");
        exit(EXIT_FAILURE);
    }

    req.n = atoi(argv[1]);
    req.m = atoi(argv[2]);
    req.p = atoi(argv[3]);
    req.borne_supp = atoi(argv[4]);

    if (req.n <= 0) {
        fprintf(stderr, "Erreur : <n> doit être un entier positif.\n");
        exit(EXIT_FAILURE);
    }

    if (req.m <= 0) {
        fprintf(stderr, "Erreur : <m> doit être un entier positif.\n");
        exit(EXIT_FAILURE);
    }

    if (req.p <= 0) {
        fprintf(stderr, "Erreur : <p> doit être un entier positif.\n");
        exit(EXIT_FAILURE);
    }

    if (req.borne_supp < 0) {
        fprintf(stderr, "Erreur : <borne_supp> doit être un entier non négatif.\n");
        exit(EXIT_FAILURE);
    }
    

    req.pid = getpid(); // Obtenir le PID du processus actuel

    // Créer un tube réponse unique pour ce client 
    char tube_reponse[50];
    sprintf(tube_reponse, "reponse_%d", req.pid);
    unlink(tube_reponse); // Supprimer le tube existant, s'il existe
    if (mkfifo(tube_reponse, 0666) == -1) { // crée le tube réponse 
        perror("Erreur création tube réponse");
        exit(EXIT_FAILURE);
    }

    // Ouvrir le tube principal en écriture
    f = open(MAIN, O_WRONLY);
    if (f == -1) {
        perror("Erreur ouverture tube principal");
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

    // Écrire la structure dans le tube principal
    if (write(f, &req, sizeof(req)) == -1) {
        perror("Erreur écriture dans le tube principal");
        close(f);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }
    close(f); // Fermer le tube principal

    // Ouvrir le tube de réponse en lecture
    int f1 = open(tube_reponse, O_RDONLY);
    if (f1 == -1) {
        perror("Erreur ouverture tube réponse");
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

    int n , m ;
 
    if (read(f1, &n, sizeof(int)) != sizeof(int)) {
        perror("Erreur lecture dimension n");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

     if (read(f1, &m, sizeof(int)) != sizeof(int)) {
        perror("Erreur lecture dimension n");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }
    
 // Lecture et affichage de la matrice A
    printf("Résultats reçus : Matrice A (%dx%d) :\n", n, m);
    int *A = malloc((size_t)n * (size_t)m * sizeof(int)); // Allocation dynamique pour la matrice C
        if (!A) {
        perror("Erreur allocation mémoire pour la matrice A");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (read(f1, &A[i * m + j], sizeof(int)) != sizeof(int)) {
                perror("Erreur lecture des éléments de A");
                free(A);
                close(f1);
                unlink(tube_reponse);
                exit(EXIT_FAILURE);
            }
            printf("%d\t", A[i * m + j]); // Affichage des éléments
        }
        printf("\n");
    }

int p ; 
if (read(f1, &m, sizeof(int)) != sizeof(int)) {
        perror("Erreur lecture dimension n");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

     if (read(f1, &p, sizeof(int)) != sizeof(int)) {
        perror("Erreur lecture dimension n");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }
    
 // Lecture et affichage de la matrice B
    printf("Résultats reçus : Matrice B (%dx%d) :\n", m, p);
    int *B = malloc((size_t)m * (size_t)p * sizeof(int)); // Allocation dynamique pour la matrice B
        if (!B) {
		free(A);
        perror("Erreur allocation mémoire pour la matrice A");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            if (read(f1, &B[i * p + j], sizeof(int)) != sizeof(int)) {
                perror("Erreur lecture des éléments de ");
                free(A);
                free(B);
                close(f1);
                unlink(tube_reponse);
                exit(EXIT_FAILURE);
            }
            printf("%d\t", B[i * p + j]); // Affichage des éléments
        }
        printf("\n");
    }

    if (read(f1, &n, sizeof(int)) != sizeof(int)) {
        perror("Erreur lecture dimension n");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }
    if (read(f1, &p, sizeof(int)) != sizeof(int)) {
        perror("Erreur lecture dimension p");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

    // Lecture et affichage de la matrice C
    printf("Résultats reçus : Matrice C (%dx%d) :\n", n, p);
    int *C = malloc((size_t)n * (size_t)p * sizeof(int)); // Allocation dynamique pour la matrice C
        if (!C) {
        perror("Erreur allocation mémoire pour la matrice C");
        close(f1);
        unlink(tube_reponse);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            if (read(f1, &C[i * p + j], sizeof(int)) != sizeof(int)) {
                perror("Erreur lecture des éléments de C");
                free(C);
                close(f1);
                unlink(tube_reponse);
                exit(EXIT_FAILURE);
            }
            printf("%d\t", C[i * p + j]); // Affichage des éléments
        }
        printf("\n");
    }

    printf(" Dimension n reçue : %d\n", n);
    printf(" Dimension p reçue : %d\n", p);
    printf(" Dimension m reçue : %d\n", m);
    printf("----------------------------------------\n ");
    
    // Libérer la mémoire et fermer les tubes
    free(A);
    free(B);
    free(C);
    close(f1);
    unlink(tube_reponse);

    return 0;
}
