#include <stdio.h>
#include "couche_transport.h"
#include "services_reseau.h"
#include "application.h"

/* ************************************************************************** */
/* *************** Fonctions utilitaires couche transport ******************* */
/* ************************************************************************** */

// RAJOUTER VOS FONCTIONS DANS CE FICHIER...



/*--------------------------------------*/
/* Fonction d'inclusion dans la fenetre */
/*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille) {

    unsigned int sup = (inf+taille-1) % SEQ_NUM_SIZE;

    return
        /* inf <= pointeur <= sup */
        ( inf <= sup && pointeur >= inf && pointeur <= sup ) ||
        /* sup < inf <= pointeur */
        ( sup < inf && pointeur >= inf) ||
        /* pointeur <= sup < inf */
        ( sup < inf && pointeur <= sup);
}

uint8_t generer_controle(paquet_t paquet) {
    //on initialise le champ somme par un XOR sur tous les autres
    paquet.somme_ctrl = paquet.num_seq ^ paquet.lg_info ^ paquet.type;

    // XOR sur chaque octet du message
    for (int i = 0; i < paquet.lg_info; i++) {
        paquet.somme_ctrl ^= paquet.info[i];
    }

    // pas un pointeur donc je ne peux pas modifier directement le champ somme_ctrl

    return paquet.somme_ctrl;
}


int verifier_controle(paquet_t paquet) {
    
    //fonction considéré comme un booléen donc 0 = faux
    int est_correct = 0;
    uint8_t controle = generer_controle(paquet);
    
    // merci à l'encadrant de TP ! (pour la ligne ci-dessous)
    est_correct = paquet.somme_ctrl == controle;
    // je me suis embrouillé sur le booléen + le résultat ne doit pas être égal à 111111...

    return est_correct;
}

// fonction reprise au format de celle utilisée en cours-TD
int inc(int num, int modulo) {
    return (num+1) % modulo;
}

int taille_fenetre_correcte(int x) {
    int w = SEQ_NUM_SIZE - 1;
    return x < w;
}