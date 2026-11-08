-- =====================================================================
-- Module MATERIELS : contraintes de la table MATERIEL (SQL Developer, F5)
-- A executer UNE SEULE FOIS. Si une ligne donne ORA-00955 / ORA-02260 / ORA-02261 / ORA-02264
-- (objet deja existant), ignorez l'erreur et continuez.
-- Verifiez d'abord l'onglet "Contraintes" de la table : ne recreez pas ce qui existe deja.
-- =====================================================================

-- 1) Cle primaire (ignorer si la table en a deja une)
ALTER TABLE MATERIEL ADD CONSTRAINT PK_MATERIEL PRIMARY KEY (ID_MATERIEL);

-- 2) Reference unique
ALTER TABLE MATERIEL ADD CONSTRAINT UQ_MATERIEL_REFERENCE UNIQUE (REFERENCE);

-- 3) Quantites positives
ALTER TABLE MATERIEL ADD CONSTRAINT CK_MATERIEL_QUANTITE CHECK (QUANTITE_STOCK >= 0);
ALTER TABLE MATERIEL ADD CONSTRAINT CK_MATERIEL_SEUIL    CHECK (SEUIL_MINIMAL_STOCK >= 0);

-- 4) Verification
SELECT CONSTRAINT_NAME, CONSTRAINT_TYPE FROM USER_CONSTRAINTS WHERE TABLE_NAME = 'MATERIEL';
