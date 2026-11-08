#ifndef MATERIEL_H
#define MATERIEL_H

#include <QDate>
#include <QString>
#include <QVector>

// Classe métier du module Matériels (table Oracle MATERIEL).
// Toutes les requêtes SQL du module sont écrites ici (requêtes préparées) ; l'interface ne contient aucun SQL.
// La connexion Oracle est la connexion commune du projet : Connexion::oracle().
class Materiel
{
public:
    // Attributs (colonnes de la table MATERIEL)
    int id = 0;                 // ID_MATERIEL
    QString reference;          // REFERENCE (obligatoire, unique, 50 caractères max)
    QString designation;        // DESIGNATION (obligatoire, 100 caractères max)
    QString categorie;          // CATEGORIE (50 caractères max)
    int quantite = 0;           // QUANTITE_STOCK
    int seuil = 0;              // SEUIL_MINIMAL_STOCK
    QString etat;               // ETAT (30 caractères max)
    QString disponibilite;      // DISPONIBILITE (20 caractères max)
    QDate dateAcquisition;      // DATE_ACQUISITION
    QString emplacement;        // EMPLACEMENT_STOCKAGE (100 caractères max)

    // Valeurs calculées
    QString idText() const;                              // "M001"
    bool critique() const { return quantite < seuil; }   // stock sous le seuil minimal

    // Contrôle de saisie : renvoie false et remplit « erreur » (toutes les erreurs, une par ligne)
    bool valider(QString *erreur) const;

    // Opérations CRUD
    bool ajouter(QString *erreur = nullptr);
    static bool afficher(int id, Materiel &resultat, QString *erreur = nullptr);
    bool modifier(QString *erreur = nullptr) const;
    static bool supprimer(int id, QString *erreur = nullptr);

    // Liste complète ou filtrée (recherche sur référence/désignation) et triée.
    // tri : "reference" (défaut), "designation", "categorie", "quantite", "date_acquisition", "etat"
    static QVector<Materiel> lister(const QString &recherche = QString(), const QString &tri = QString(),
                                    QString *erreur = nullptr);

    // Vrai si une autre fiche (id différent de ignoreId) porte déjà cette référence.
    static bool referenceExiste(const QString &reference, int ignoreId = 0);
};

#endif // MATERIEL_H
