-- ============================================================
--  Santa Aurora Roleplay — Migração 008
--  Fase 7: Crime e Organizações — Facções, Territórios
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- FACÇÕES
-- ============================================================
CREATE TABLE IF NOT EXISTS `factions` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `tag`           VARCHAR(8) NOT NULL,
    `type`          ENUM('legal','illegal','neutral') NOT NULL DEFAULT 'neutral',
    `description`   VARCHAR(256) DEFAULT NULL,
    `color`         INT NOT NULL DEFAULT -1,        -- RGBA para nome tag
    `cash`          INT NOT NULL DEFAULT 0,
    `reputation`    INT NOT NULL DEFAULT 0,
    `active`        TINYINT NOT NULL DEFAULT 1,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_tag` (`tag`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CARGOS DE FACÇÃO
-- ============================================================
CREATE TABLE IF NOT EXISTS `faction_ranks` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `faction_id`    SMALLINT UNSIGNED NOT NULL,
    `rank_level`    TINYINT NOT NULL DEFAULT 1,
    `name`          VARCHAR(32) NOT NULL,
    `can_recruit`   TINYINT NOT NULL DEFAULT 0,
    `can_manage`    TINYINT NOT NULL DEFAULT 0,
    `salary`        INT NOT NULL DEFAULT 500,
    PRIMARY KEY (`id`),
    KEY `idx_faction_id` (`faction_id`),
    CONSTRAINT `fk_frank_faction` FOREIGN KEY (`faction_id`)
        REFERENCES `factions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- MEMBROS DE FACÇÃO
-- ============================================================
CREATE TABLE IF NOT EXISTS `faction_members` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `faction_id`    SMALLINT UNSIGNED NOT NULL,
    `character_id`  INT UNSIGNED NOT NULL,
    `rank_level`    TINYINT NOT NULL DEFAULT 1,
    `contribution`  INT NOT NULL DEFAULT 0,
    `joined_at`     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `active`        TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_faction_char` (`faction_id`, `character_id`),
    KEY `idx_character_id` (`character_id`),
    CONSTRAINT `fk_fmem_faction` FOREIGN KEY (`faction_id`)
        REFERENCES `factions` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_fmem_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- COFRE DA FACÇÃO
-- ============================================================
CREATE TABLE IF NOT EXISTS `faction_storage` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `faction_id`    SMALLINT UNSIGNED NOT NULL,
    `item_id`       SMALLINT UNSIGNED NOT NULL,
    `quantity`      INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_faction_item` (`faction_id`, `item_id`),
    CONSTRAINT `fk_fstorage_faction` FOREIGN KEY (`faction_id`)
        REFERENCES `factions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- TERRITÓRIOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `territories` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `zone`          VARCHAR(32) DEFAULT NULL,
    `owner_faction` SMALLINT UNSIGNED DEFAULT NULL,
    `influence`     TINYINT NOT NULL DEFAULT 0,     -- 0-100%
    `pos_x`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_y`         FLOAT NOT NULL DEFAULT 0.0,
    `radius`        FLOAT NOT NULL DEFAULT 100.0,
    `reward_type`   ENUM('cash','reputation','resource','none') NOT NULL DEFAULT 'none',
    `reward_value`  INT NOT NULL DEFAULT 0,
    `last_captured` DATETIME DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_owner` (`owner_faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- VEÍCULOS DA FACÇÃO
-- ============================================================
CREATE TABLE IF NOT EXISTS `faction_vehicles` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `faction_id`    SMALLINT UNSIGNED NOT NULL,
    `vehicle_id`    INT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_faction_id` (`faction_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- SEED — FACÇÕES INICIAIS
-- ============================================================
INSERT IGNORE INTO `factions`
    (`id`, `name`, `tag`, `type`, `description`, `color`)
VALUES
-- Legais
(1, 'Polícia Militar de Santa Aurora', 'PMSA', 'legal',
 'Força policial da cidade de Santa Aurora.', -65536),   -- vermelho
(2, 'SAMU Santa Aurora',               'SAMU', 'legal',
 'Serviço de Atendimento Médico de Urgência.', -16711936), -- verde
(3, 'Prefeitura Municipal',            'GOV',  'legal',
 'Administração pública da cidade.', -16776961),           -- azul

-- Ilegais
(4, 'Los Santos Vaga Negra',           'LSVN', 'illegal',
 'Gang do lado oeste.', -65281),                          -- roxo
(5, 'Cartel do Norte',                 'CTNR', 'illegal',
 'Organização criminosa do norte.', -16776961),           -- azul escuro
(6, 'Máfia Italiana SA',               'MFSA', 'illegal',
 'Crime organizado tradicional.', -8388608),              -- marrom

-- Neutras
(7, 'Transportadores Autônomos',       'TRNS', 'neutral',
 'Cooperativa de transportadores.', -1);                  -- branco

-- Cargos da PMSA
INSERT IGNORE INTO `faction_ranks`
    (`faction_id`, `rank_level`, `name`, `can_recruit`, `can_manage`, `salary`)
VALUES
(1, 1, 'Soldado',         0, 0, 2000),
(1, 2, 'Cabo',            0, 0, 2500),
(1, 3, 'Sargento',        1, 0, 3000),
(1, 4, 'Tenente',         1, 1, 3500),
(1, 5, 'Capitão',         1, 1, 4000),
(1, 6, 'Delegado',        1, 1, 5000),
-- Cargos SAMU
(2, 1, 'Socorrista',      0, 0, 2000),
(2, 2, 'Técnico',         0, 0, 2500),
(2, 3, 'Enfermeiro',      1, 0, 3000),
(2, 4, 'Médico',          1, 1, 4000),
(2, 5, 'Diretor Médico',  1, 1, 5000),
-- Cargos Gang 1
(4, 1, 'Recruta',         0, 0,  500),
(4, 2, 'Membro',          0, 0,  700),
(4, 3, 'Soldado',         1, 0, 1000),
(4, 4, 'Veterano',        1, 1, 1500),
(4, 5, 'Líder',           1, 1, 2000);

-- Territórios de Santa Aurora
INSERT IGNORE INTO `territories`
    (`id`, `name`, `zone`, `pos_x`, `pos_y`, `radius`, `reward_type`, `reward_value`)
VALUES
(1,  'Centro Financeiro',    'Centro',           490.0,  -30.0,  150.0, 'cash',       2000),
(2,  'Zona Norte Alta',      'Zona Norte',        248.0, -179.0, 200.0, 'reputation',    50),
(3,  'Zona Sul Nobre',       'Zona Sul',          355.0, -121.0, 180.0, 'cash',       3000),
(4,  'Zona Industrial',      'Zona Industrial',   638.0,-1219.0, 250.0, 'resource',    100),
(5,  'Porto Logístico',      'Porto',            1017.0, -911.0, 200.0, 'cash',       2500),
(6,  'Zona Rural Oeste',     'Zona Rural',       1290.0,-1067.0, 300.0, 'resource',    150),
(7,  'Bairro Periférico',    'Periferia',         500.0, -400.0, 180.0, 'reputation',    30),
(8,  'Complexo Industrial',  'Zona Industrial',   900.0,-1100.0, 220.0, 'cash',       1500);

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('008', 'Fase 7 Crime e Organizações - factions, ranks, members, territories');
