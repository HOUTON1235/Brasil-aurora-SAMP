-- ============================================================
--  Santa Aurora Roleplay — Migração 007
--  Fase 6: Serviços Públicos — Polícia, Hospital, Governo
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- ANTECEDENTES CRIMINAIS
-- ============================================================
CREATE TABLE IF NOT EXISTS `criminal_records` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `officer_id`    INT UNSIGNED DEFAULT NULL,
    `crime`         VARCHAR(64) NOT NULL,
    `gravity`       TINYINT NOT NULL DEFAULT 1,     -- 1=leve 2=médio 3=grave
    `fine`          INT NOT NULL DEFAULT 0,
    `jail_time`     INT NOT NULL DEFAULT 0,          -- minutos
    `status`        ENUM('pending','serving','served','pardoned') NOT NULL DEFAULT 'pending',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PROCURADOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `wanted` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `level`         TINYINT NOT NULL DEFAULT 1,      -- 1-6
    `reason`        VARCHAR(128) NOT NULL,
    `officer_id`    INT UNSIGNED DEFAULT NULL,
    `active`        TINYINT NOT NULL DEFAULT 1,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_active` (`active`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- REGISTROS DE PRISÃO
-- ============================================================
CREATE TABLE IF NOT EXISTS `jail_records` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `officer_id`    INT UNSIGNED DEFAULT NULL,
    `reason`        VARCHAR(128) NOT NULL,
    `duration`      INT NOT NULL DEFAULT 5,         -- minutos
    `time_served`   INT NOT NULL DEFAULT 0,
    `status`        ENUM('serving','released','escaped') NOT NULL DEFAULT 'serving',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `released_at`   DATETIME DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CATÁLOGO DE CRIMES
-- ============================================================
CREATE TABLE IF NOT EXISTS `crime_catalog` (
    `id`            TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `display_name`  VARCHAR(48) NOT NULL,
    `gravity`       TINYINT NOT NULL DEFAULT 1,
    `fine`          INT NOT NULL DEFAULT 0,
    `jail_time`     INT NOT NULL DEFAULT 0,
    `wanted_level`  TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PRONTUÁRIOS MÉDICOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `medical_records` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `medic_id`      INT UNSIGNED DEFAULT NULL,
    `type`          ENUM('revive','treatment','hospitalization') NOT NULL DEFAULT 'treatment',
    `description`   VARCHAR(128) DEFAULT NULL,
    `cost`          INT NOT NULL DEFAULT 0,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CHAMADOS MÉDICOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `medical_calls` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `pos_x`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_y`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_z`         FLOAT NOT NULL DEFAULT 0.0,
    `status`        ENUM('pending','attending','closed') NOT NULL DEFAULT 'pending',
    `medic_id`      INT UNSIGNED DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- GOVERNO — ORÇAMENTO E AÇÕES
-- ============================================================
CREATE TABLE IF NOT EXISTS `government_budget` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `balance`       INT NOT NULL DEFAULT 500000,
    `tax_collected` INT NOT NULL DEFAULT 0,
    `expenses`      INT NOT NULL DEFAULT 0,
    `updated_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `government_actions` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `action_type`   VARCHAR(32) NOT NULL,
    `description`   VARCHAR(256) NOT NULL,
    `amount`        INT NOT NULL DEFAULT 0,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- SEED — CATÁLOGO DE CRIMES
-- ============================================================
INSERT IGNORE INTO `crime_catalog`
    (`id`, `name`, `display_name`, `gravity`, `fine`, `jail_time`, `wanted_level`)
VALUES
(1,  'CRIME_THEFT',         'Furto',                    1,  1000,  3,  1),
(2,  'CRIME_ROBBERY',       'Roubo',                    2,  3000,  8,  3),
(3,  'CRIME_ASSAULT',       'Agressão',                 1,  1500,  5,  1),
(4,  'CRIME_HOMICIDE',      'Homicídio',                3, 10000, 30,  6),
(5,  'CRIME_VANDALISM',     'Dano ao Patrimônio',       1,   800,  2,  1),
(6,  'CRIME_RECKLESS',      'Direção Perigosa',         1,  2000,  0,  1),
(7,  'CRIME_ILLEGAL_WEAPON','Porte Ilegal de Arma',     2,  4000, 10,  2),
(8,  'CRIME_DRUG',          'Tráfico de Drogas',        3,  8000, 20,  4),
(9,  'CRIME_CONTEMPT',      'Desacato',                 1,   500,  1,  1),
(10, 'CRIME_TRESPASSING',   'Invasão de Propriedade',   1,  1000,  3,  1),
(11, 'CRIME_FLEE',          'Fuga da Polícia',          2,  2500,  6,  2),
(12, 'CRIME_CORRUPTION',    'Corrupção',                2,  5000, 15,  3),
(13, 'CRIME_FRAUD',         'Fraude',                   2,  3000, 10,  2),
(14, 'CRIME_KIDNAPPING',    'Sequestro',                3, 15000, 40,  5),
(15, 'CRIME_CARJACKING',    'Roubo de Veículo',         2,  5000, 12,  3);

-- Orçamento inicial do governo
INSERT IGNORE INTO `government_budget` (`id`, `balance`) VALUES (1, 500000);

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('007', 'Fase 6 Serviços Públicos - polícia, hospital, governo, crimes, procurados');
