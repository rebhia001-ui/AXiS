#include "materiel.h"
#include "connexion.h"

#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {

QSqlDatabase base() { return Connexion::oracle(); }

bool baseOuverte(QString *erreur)
{
    if (base().isValid() && base().isOpen())
        return true;
    if (erreur)
        *erreur = QStringLiteral("La connexion Oracle n'est pas ouverte.");
    return false;
}

void signaler(QString *erreur, const QString &message, const QSqlQuery &query)
{
    if (erreur)
        *erreur = message + QStringLiteral("\n") + query.lastError().text();
}

const QString COLONNES = QStringLiteral(
    "ID_MATERIEL, REFERENCE, DESIGNATION, CATEGORIE, QUANTITE_STOCK, SEUIL_MINIMAL_STOCK, "
    "ETAT, DISPONIBILITE, DATE_ACQUISITION, EMPLACEMENT_STOCKAGE");

Materiel depuisRequete(const QSqlQuery &q)
{
    Materiel m;
    m.id = q.value(0).toInt();
    m.reference = q.value(1).toString();
    m.designation = q.value(2).toString();
    m.categorie = q.value(3).toString();
    m.quantite = q.value(4).toInt();
    m.seuil = q.value(5).toInt();
    m.etat = q.value(6).toString();
    m.disponibilite = q.value(7).toString();
    m.dateAcquisition = q.value(8).toDate();
    m.emplacement = q.value(9).toString();
    return m;
}

} // namespace

// ---------- Valeurs calculées ----------

QString Materiel::idText() const
{
    return QStringLiteral("M%1").arg(id, 3, 10, QLatin1Char('0'));
}

// ---------- Contrôle de saisie ----------

bool Materiel::valider(QString *erreur) const
{
    QStringList e;

    const QString ref = reference.trimmed();
    if (ref.isEmpty())
        e << QStringLiteral("La référence est obligatoire.");
    else if (ref.size() > 50)
        e << QStringLiteral("La référence ne doit pas dépasser 50 caractères.");
    else if (!QRegularExpression(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9_-]*$")).match(ref).hasMatch())
        e << QStringLiteral("La référence ne peut contenir que des lettres, chiffres, '-' ou '_' (ex. MAT-0001).");

    const QString des = designation.trimmed();
    if (des.isEmpty())
        e << QStringLiteral("La désignation est obligatoire (nom du matériel).");
    else if (des.size() > 100)
        e << QStringLiteral("La désignation ne doit pas dépasser 100 caractères.");
    else if (!des.contains(QRegularExpression(QStringLiteral("[A-Za-zÀ-ÿ]"))))
        e << QStringLiteral("La désignation doit être un nom de matériel (ex. Extincteur CO2), pas un nombre.");

    if (categorie.trimmed().isEmpty())
        e << QStringLiteral("La catégorie est obligatoire.");
    else if (categorie.size() > 50)
        e << QStringLiteral("La catégorie ne doit pas dépasser 50 caractères.");

    if (quantite < 0) e << QStringLiteral("La quantité en stock doit être un entier ≥ 0.");
    if (seuil < 0)    e << QStringLiteral("Le seuil minimal doit être un entier ≥ 0.");

    if (etat.trimmed().isEmpty() || etat.size() > 30)
        e << QStringLiteral("L'état est obligatoire (30 caractères max).");
    if (disponibilite.trimmed().isEmpty() || disponibilite.size() > 20)
        e << QStringLiteral("La disponibilité est obligatoire (20 caractères max).");

    if (!dateAcquisition.isValid())
        e << QStringLiteral("Date d'acquisition invalide.");
    else if (dateAcquisition > QDate::currentDate())
        e << QStringLiteral("La date d'acquisition ne peut pas être dans le futur.");
    else if (dateAcquisition.year() < 1990)
        e << QStringLiteral("La date d'acquisition est trop ancienne (année ≥ 1990).");

    const QString emp = emplacement.trimmed();
    if (emp.isEmpty())
        e << QStringLiteral("L'emplacement de stockage est obligatoire.");
    else if (emp.size() > 100)
        e << QStringLiteral("L'emplacement ne doit pas dépasser 100 caractères.");

    if (e.isEmpty())
        return true;
    if (erreur)
        *erreur = e.join(QLatin1Char('\n'));
    return false;
}

// ---------- CRUD ----------

bool Materiel::ajouter(QString *erreur)
{
    if (!valider(erreur) || !baseOuverte(erreur))
        return false;

    if (referenceExiste(reference, 0)) {
        if (erreur) *erreur = QStringLiteral("Cette référence existe déjà.");
        return false;
    }

    // Prochain identifiant
    QSqlQuery suivant(base());
    suivant.prepare(QStringLiteral("SELECT COALESCE(MAX(ID_MATERIEL), 0) + 1 FROM MATERIEL"));
    if (!suivant.exec() || !suivant.next()) {
        signaler(erreur, QStringLiteral("Ajout impossible : identifiant non attribué."), suivant);
        return false;
    }
    const int nouvelId = suivant.value(0).toInt();

    QSqlQuery query(base());
    if (!query.prepare(QStringLiteral(
            "INSERT INTO MATERIEL (" ) + COLONNES + QStringLiteral(
            ") VALUES (:id, :reference, :designation, :categorie, :quantite, :seuil, "
            ":etat, :dispo, :date_acq, :emplacement)"))) {
        signaler(erreur, QStringLiteral("Ajout impossible : requête invalide."), query);
        return false;
    }
    query.bindValue(QStringLiteral(":id"), nouvelId);
    query.bindValue(QStringLiteral(":reference"), reference.trimmed().toUpper());
    query.bindValue(QStringLiteral(":designation"), designation.trimmed());
    query.bindValue(QStringLiteral(":categorie"), categorie.trimmed());
    query.bindValue(QStringLiteral(":quantite"), quantite);
    query.bindValue(QStringLiteral(":seuil"), seuil);
    query.bindValue(QStringLiteral(":etat"), etat.trimmed());
    query.bindValue(QStringLiteral(":dispo"), disponibilite.trimmed());
    query.bindValue(QStringLiteral(":date_acq"), dateAcquisition);
    query.bindValue(QStringLiteral(":emplacement"), emplacement.trimmed());
    if (!query.exec()) {
        signaler(erreur, QStringLiteral("Ajout impossible."), query);
        return false;
    }
    // Les connexions ODBC sont en validation automatique : l'ajout est déjà enregistré.
    const_cast<Materiel *>(this)->id = nouvelId;
    return true;
}

bool Materiel::afficher(int id, Materiel &resultat, QString *erreur)
{
    if (!baseOuverte(erreur))
        return false;
    QSqlQuery query(base());
    query.prepare(QStringLiteral("SELECT ") + COLONNES + QStringLiteral(" FROM MATERIEL WHERE ID_MATERIEL = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        signaler(erreur, QStringLiteral("Lecture impossible."), query);
        return false;
    }
    if (!query.next()) {
        if (erreur) *erreur = QStringLiteral("Matériel introuvable.");
        return false;
    }
    resultat = depuisRequete(query);
    return true;
}

bool Materiel::modifier(QString *erreur) const
{
    if (!valider(erreur) || !baseOuverte(erreur))
        return false;

    if (referenceExiste(reference, id)) {
        if (erreur) *erreur = QStringLiteral("Cette référence existe déjà.");
        return false;
    }

    QSqlQuery query(base());
    if (!query.prepare(QStringLiteral(
            "UPDATE MATERIEL SET REFERENCE = :reference, DESIGNATION = :designation, CATEGORIE = :categorie, "
            "QUANTITE_STOCK = :quantite, SEUIL_MINIMAL_STOCK = :seuil, ETAT = :etat, "
            "DISPONIBILITE = :dispo, DATE_ACQUISITION = :date_acq, EMPLACEMENT_STOCKAGE = :emplacement "
            "WHERE ID_MATERIEL = :id"))) {
        signaler(erreur, QStringLiteral("Modification impossible : requête invalide."), query);
        return false;
    }
    query.bindValue(QStringLiteral(":reference"), reference.trimmed().toUpper());
    query.bindValue(QStringLiteral(":designation"), designation.trimmed());
    query.bindValue(QStringLiteral(":categorie"), categorie.trimmed());
    query.bindValue(QStringLiteral(":quantite"), quantite);
    query.bindValue(QStringLiteral(":seuil"), seuil);
    query.bindValue(QStringLiteral(":etat"), etat.trimmed());
    query.bindValue(QStringLiteral(":dispo"), disponibilite.trimmed());
    query.bindValue(QStringLiteral(":date_acq"), dateAcquisition);
    query.bindValue(QStringLiteral(":emplacement"), emplacement.trimmed());
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        signaler(erreur, QStringLiteral("Modification impossible."), query);
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Modification impossible : matériel introuvable.");
        return false;
    }
    return true;
}

bool Materiel::supprimer(int id, QString *erreur)
{
    if (!baseOuverte(erreur))
        return false;
    QSqlQuery query(base());
    query.prepare(QStringLiteral("DELETE FROM MATERIEL WHERE ID_MATERIEL = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        // Ex. ORA-02292 : matériel déjà utilisé dans une autre table (clé étrangère)
        signaler(erreur, QStringLiteral("Suppression impossible (matériel peut-être déjà utilisé)."), query);
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Suppression impossible : matériel introuvable.");
        return false;
    }
    return true;
}

QVector<Materiel> Materiel::lister(const QString &recherche, const QString &tri, QString *erreur)
{
    QVector<Materiel> liste;
    if (!baseOuverte(erreur))
        return liste;

    // Liste blanche : le texte de l'utilisateur n'est jamais placé dans l'ORDER BY
    QString ordre;
    if (tri == QLatin1String("etat"))
        ordre = QStringLiteral("CASE ETAT WHEN 'Bon' THEN 1 WHEN 'Moyen' THEN 2 WHEN 'Mauvais' THEN 3 ELSE 4 END, UPPER(REFERENCE)");
    else if (tri == QLatin1String("designation"))      ordre = QStringLiteral("UPPER(DESIGNATION)");
    else if (tri == QLatin1String("categorie"))        ordre = QStringLiteral("UPPER(CATEGORIE), UPPER(REFERENCE)");
    else if (tri == QLatin1String("quantite"))         ordre = QStringLiteral("QUANTITE_STOCK");
    else if (tri == QLatin1String("date_acquisition")) ordre = QStringLiteral("DATE_ACQUISITION");
    else                                               ordre = QStringLiteral("UPPER(REFERENCE)");

    QSqlQuery query(base());
    query.prepare(QStringLiteral("SELECT ") + COLONNES + QStringLiteral(
                      " FROM MATERIEL WHERE UPPER(REFERENCE) LIKE UPPER(:s1) OR UPPER(DESIGNATION) LIKE UPPER(:s2)"
                      " ORDER BY ") + ordre);
    const QString motif = QLatin1Char('%') + recherche.trimmed() + QLatin1Char('%');
    query.bindValue(QStringLiteral(":s1"), motif);
    query.bindValue(QStringLiteral(":s2"), motif);
    if (!query.exec()) {
        signaler(erreur, QStringLiteral("Lecture impossible."), query);
        return liste;
    }
    while (query.next())
        liste.append(depuisRequete(query));
    return liste;
}

bool Materiel::referenceExiste(const QString &reference, int ignoreId)
{
    if (!baseOuverte(nullptr))
        return false;
    QSqlQuery query(base());
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM MATERIEL WHERE UPPER(REFERENCE) = UPPER(:ref) AND ID_MATERIEL <> :id"));
    query.bindValue(QStringLiteral(":ref"), reference.trimmed());
    query.bindValue(QStringLiteral(":id"), ignoreId);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}
