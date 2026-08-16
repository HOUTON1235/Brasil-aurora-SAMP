-- ============================================================
--  Santa Aurora Roleplay — Migração 005
--  Fase 4: Empresas — businesses, funcionários, estoque, caixa
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- TIPOS DE EMPRESA
-- ============================================================
CREATE TABLE IF NOT EXISTS `business_types` (
    `id`            TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(32) NOT NULL,
    `display_name`  VARCHAR(48) NOT NULL,
    `category`      ENUM('food','service','transport','entertainment',
                         'retail','security','utility') NOT NULL DEFAULT 'retail',
    `base_price`    INT NOT NULL DEFAULT 100000,
    `daily_expenses` INT NOT NULL DEFAULT 500,
    `tax_rate`      FLOAT NOT NULL DEFAULT 0.08,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- EMPRESAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `businesses` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `owner_char_id` INT UNSIGNED DEFAULT NULL,
    `type_id`       TINYINT UNSIGNED NOT NULL,
    `name`          VARCHAR(48) NOT NULL,
    `cash`          INT NOT NULL DEFAULT 0,
    `level`         TINYINT NOT NULL DEFAULT 1,
    `pos_x`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_y`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_z`         FLOAT NOT NULL DEFAULT 0.0,
    `int_pos_x`     FLOAT NOT NULL DEFAULT 0.0,
    `int_pos_y`     FLOAT NOT NULL DEFAULT 0.0,
    `int_pos_z`     FLOAT NOT NULL DEFAULT 0.0,
    `int_world`     SMALLINT NOT NULL DEFAULT 0,
    `int_interior`  TINYINT NOT NULL DEFAULT 0,
    `open`          TINYINT NOT NULL DEFAULT 0,
    `for_sale`      TINYINT NOT NULL DEFAULT 1,
    `price`         INT NOT NULL DEFAULT 100000,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_owner` (`owner_char_id`),
    KEY `idx_type`  (`type_id`),
    CONSTRAINT `fk_biz_owner` FOREIGN KEY (`owner_char_id`)
        REFERENCES `characters` (`id`) ON DELETE SET NULL,
    CONSTRAINT `fk_biz_type` FOREIGN KEY (`type_id`)
        REFERENCES `business_types` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- FUNCIONÁRIOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `business_employees` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `business_id`   INT UNSIGNED NOT NULL,
    `character_id`  INT UNSIGNED NOT NULL,
    `rank`          VARCHAR(32) NOT NULL DEFAULT 'Funcionário',
    `salary`        INT NOT NULL DEFAULT 800,
    `hired_at`      DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `active`        TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_biz_char` (`business_id`, `character_id`),
    KEY `idx_business_id` (`business_id`),
    KEY `idx_character_id` (`character_id`),
    CONSTRAINT `fk_emp_biz` FOREIGN KEY (`business_id`)
        REFERENCES `businesses` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_emp_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- ESTOQUE DE EMPRESA
-- ============================================================
CREATE TABLE IF NOT EXISTS `business_inventory` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `business_id`   INT UNSIGNED NOT NULL,
    `item_id`       SMALLINT UNSIGNED NOT NULL,
    `quantity`      INT NOT NULL DEFAULT 0,
    `buy_price`     INT NOT NULL DEFAULT 0,    -- preço de custo
    `sell_price`    INT NOT NULL DEFAULT 0,    -- preço de venda ao público
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_biz_item` (`business_id`, `item_id`),
    KEY `idx_business_id` (`business_id`),
    CONSTRAINT `fk_bizinv_biz` FOREIGN KEY (`business_id`)
        REFERENCES `businesses` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- TRANSAÇÕES DE EMPRESA
-- ============================================================
CREATE TABLE IF NOT EXISTS `business_transactions` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `business_id`   INT UNSIGNED NOT NULL,
    `character_id`  INT UNSIGNED DEFAULT NULL,
    `type`          ENUM('sale','purchase','salary','tax','expense','deposit') NOT NULL,
    `amount`        INT NOT NULL,
    `description`   VARCHAR(128) DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_business_id` (`business_id`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- SEED — TIPOS DE EMPRESA
-- ============================================================
INSERT IGNORE INTO `business_types`
    (`id`, `name`, `display_name`, `category`, `base_price`, `daily_expenses`, `tax_rate`)
VALUES
(1,  'BIZ_RESTAURANT',  'Restaurante',         'food',          150000, 800,  0.08),
(2,  'BIZ_MARKET',      'Mercado',             'retail',        120000, 600,  0.07),
(3,  'BIZ_PHARMACY',    'Farmácia',            'retail',        100000, 500,  0.07),
(4,  'BIZ_MECHANIC',    'Oficina Mecânica',    'service',       200000, 1000, 0.09),
(5,  'BIZ_DEALERSHIP',  'Concessionária',      'service',       500000, 2000, 0.10),
(6,  'BIZ_FUEL',        'Posto de Combustível','service',       300000, 1500, 0.09),
(7,  'BIZ_TRANSPORT',   'Transportadora',      'transport',     250000, 1200, 0.08),
(8,  'BIZ_NIGHTCLUB',   'Boate',               'entertainment', 400000, 2000, 0.10),
(9,  'BIZ_BAR',         'Bar',                 'food',          120000, 600,  0.08),
(10, 'BIZ_SECURITY',    'Segurança Privada',   'security',      200000, 900,  0.09),
(11, 'BIZ_TAXI',        'Cooperativa de Táxi', 'transport',     150000, 700,  0.08),
(12, 'BIZ_CLOTHES',     'Loja de Roupas',      'retail',        100000, 500,  0.07);

-- ============================================================
-- SEED — EMPRESAS (10 negócios à venda)
-- ============================================================
INSERT IGNORE INTO `businesses`
    (`id`, `type_id`, `name`, `pos_x`, `pos_y`, `pos_z`,
     `int_pos_x`, `int_pos_y`, `int_pos_z`, `int_world`, `int_interior`,
     `price`, `for_sale`)
VALUES
(1,  1,  'Restaurante Sabor do Povo',   203.0, -24.0, 1001.8,  193.2, -15.1, 1001.2, 1,  5, 150000, 1),
(2,  2,  'Mercado Bom Preço',           490.0, -24.0, 1001.8,  480.2, -15.1, 1001.2, 2,  6, 120000, 1),
(3,  3,  'Farmácia Vida+',              362.0, -74.0, 1001.8,  352.2, -65.1, 1001.2, 3,  6, 100000, 1),
(4,  4,  'Oficina Speed',              2232.5, -1114.0, 26.4, 2220.0, -1114.0, 27.0, 4, 3, 200000, 1),
(5,  9,  'Bar do Zé',                   248.0, -179.0,   2.3,  240.0, -170.0, 1001.2, 5,  6, 120000, 1),
(6,  8,  'Club Noturno Aurora',         500.0,  -40.0, 1001.3,  490.0, -31.0, 1001.2, 6, 17, 400000, 1),
(7,  10, 'SA Security',                 362.9,  -74.4, 1001.8,  353.0, -65.0, 1001.2, 7,  6, 200000, 1),
(8,  11, 'Táxi Aurora Coop',            490.0,  -30.0, 1001.8,  480.0, -21.0, 1001.2, 8,  6, 150000, 1),
(9,  12, 'Moda Urbana',                 203.0,  -40.0, 1001.8,  193.0, -31.0, 1001.2, 9,  6, 100000, 1),
(10, 7,  'Transportes Santa Aurora',    638.1,-1219.4,   17.3,  630.0,-1210.0, 18.0, 10, 0, 250000, 1);

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('005', 'Fase 4 Empresas - businesses, tipos, funcionários, estoque, transações');
