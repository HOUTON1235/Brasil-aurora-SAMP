-- ============================================================
--  Santa Aurora Roleplay — Migração 006
--  Fase 5: Comunicação — Celular, SMS, Ligações, Anúncios
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- CELULARES
-- ============================================================
CREATE TABLE IF NOT EXISTS `phones` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `number`        VARCHAR(12) NOT NULL,
    `model`         VARCHAR(32) NOT NULL DEFAULT 'Basic Phone',
    `battery`       TINYINT NOT NULL DEFAULT 100,
    `blocked`       TINYINT NOT NULL DEFAULT 0,
    `stolen`        TINYINT NOT NULL DEFAULT 0,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_number` (`number`),
    KEY `idx_character_id` (`character_id`),
    CONSTRAINT `fk_phone_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CONTATOS DO CELULAR
-- ============================================================
CREATE TABLE IF NOT EXISTS `phone_contacts` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `contact_name`  VARCHAR(32) NOT NULL,
    `number`        VARCHAR(12) NOT NULL,
    `blocked`       TINYINT NOT NULL DEFAULT 0,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    CONSTRAINT `fk_contact_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- MENSAGENS (SMS)
-- ============================================================
CREATE TABLE IF NOT EXISTS `messages` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `from_char_id`  INT UNSIGNED NOT NULL,
    `to_char_id`    INT UNSIGNED NOT NULL,
    `from_number`   VARCHAR(12) NOT NULL,
    `to_number`     VARCHAR(12) NOT NULL,
    `message`       VARCHAR(256) NOT NULL,
    `read`          TINYINT NOT NULL DEFAULT 0,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_to_char` (`to_char_id`),
    KEY `idx_from_char` (`from_char_id`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CHAMADAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `calls` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `from_char_id`  INT UNSIGNED NOT NULL,
    `to_char_id`    INT UNSIGNED NOT NULL,
    `from_number`   VARCHAR(12) NOT NULL,
    `to_number`     VARCHAR(12) NOT NULL,
    `duration`      INT NOT NULL DEFAULT 0,   -- segundos
    `status`        ENUM('missed','answered','rejected') NOT NULL DEFAULT 'missed',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_from_char` (`from_char_id`),
    KEY `idx_to_char` (`to_char_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- ANÚNCIOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `announcements` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `type`          ENUM('sale','buy','job','service','event','other') NOT NULL DEFAULT 'other',
    `title`         VARCHAR(48) NOT NULL,
    `body`          VARCHAR(256) NOT NULL,
    `price`         INT NOT NULL DEFAULT 0,
    `contact`       VARCHAR(16) NOT NULL,
    `active`        TINYINT NOT NULL DEFAULT 1,
    `expires_at`    DATETIME DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_type` (`type`),
    KEY `idx_active` (`active`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('006', 'Fase 5 Comunicação - phones, contacts, messages, calls, announcements');
