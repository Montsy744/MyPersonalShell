#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "./ligne-commande/ligne_commande.h"
#include <sys/wait.h>


#define MAX_HOSTNAME 256
#define MAX_PATH 256

// Affiche un prompt à l'utilisateur
void affiche_prompt();

// Lit une ligne depuis l'entrée standard et la traite de façon à
// produire une liste de commandes à exécuter.
//
// Une liste de commande est un tableau de commande.  Une commande et
// une table de chaîne de caractères dans une formation compatible avec
// le tableau argv d'un main.
//
// l'entier pointé par flag sera mis à jour avec une des valeur suivantes:
// - -1 : une erreur s'est produite
// - 0 : la ligne a été analysée correctement et elle ne se termine pas par le caractère &
// - 1 : la ligne a été analysée correctement et elle se termine par le caractère &
//
// l'entier pointé par nb sera mis à jour avec le nombre de
// commandes retournées, c'est à dire la taille du tableau de commandes
//
// En cas d'erreur, la fonction retourne NULL
//char ***ligne_commande(int *flag, int *nb);

void execute_ligne_commande(char ***ligneCommande, int nb, int flag);

int lance_commande(int in, int out, const char *com, char ** argv);

int main(void) {

    while(1) {
        affiche_prompt();
        int flag = 0;
        int nb = 0;
        char ***ligneCommande = ligne_commande(&flag, &nb);
        if(ligneCommande == NULL || strcmp(ligneCommande[0][0], "exit") == 0 ) {
            break;
        } else if (flag != -1) {
            execute_ligne_commande(ligneCommande, nb, flag);
        }
    }
    printf("\n");
}

void affiche_prompt() {
    char hostname[MAX_HOSTNAME];
    char cwd[MAX_PATH];
    char *username;
    
    // Récupérer le nom d'utilisateur
    username = getenv("USER");
    if (username == NULL) {
        username = "unknown";
    }
    // Récupérer le nom de la machine
    if (gethostname(hostname, MAX_HOSTNAME) != 0) {
        strcpy(hostname, "unknown");
    }
    // Récupérer le répertoire courant
    if (getcwd(cwd, MAX_PATH) == NULL) {
        strcpy(cwd, "unknown");
    }
    
    printf("%s@%s:%s> ", username, hostname, strrchr(cwd, '/'));
    fflush(stdout);
}

void execute_ligne_commande(char ***ligneCommande, int nb, int arriere_plan) {
    
    int in_precedent = 0;
    pid_t pids[nb];
    int nb_pids = 0; 
    int pid;

    for (int i = 0; i < nb; i++) {
        int out;
        int tube[2];
        
        if (i < nb - 1) { 
            if (pipe(tube) == -1) {
                perror("pipe");
                continue; 
            }
            out = tube[1];  
        } else {
            out = 1; 
        }

        pid = lance_commande(in_precedent, out, ligneCommande[i][0], ligneCommande[i]);

        if (pid != -1) {
            pids[nb_pids] = pid; 
            nb_pids++;
        }

        if(in_precedent != 0) {
            close(in_precedent);
        }
        if(out != 1) {
            close(out);
        }
        in_precedent = tube[0];
    }
    if (arriere_plan == 0) {
        for (int i = 0; i < nb_pids; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

int lance_commande(int in, int out, const char *com, char ** argv) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    } 
    else {
        if (pid == 0) { // enfant
            //redirection entrée
            if (in != 0) {
                dup2(in, 0);
                close(in);
            }
            //redirection sortie  
            if (out != 1) {
                dup2(out, 1);
                close(out);
            }

            execvp(com, argv);
            perror("execvp");
            exit(1);
        } else {
            return pid;
        }
    }
    return -1;
}

