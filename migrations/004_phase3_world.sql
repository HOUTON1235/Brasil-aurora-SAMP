-- ============================================================
--  Santa Aurora Roleplay — Migração 004
--  Fase 3: Mundo — Veículos, Propriedades, Garagem, Postos
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- VEÍCULOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `vehicles` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `owner_char_id` INT UNSIGNED DEFAULT NULL,
    `model`         SMALLINT NOT NULL,
    `plate`         VARCHAR(8) NOT NULL,
    `color1`        TINYINT NOT NULL DEFAULT 0,
    `color2`        TINYINT NOT NULL DEFAULT 1,
    `fuel`          FLOAT NOT NULL DEFAULT 100.0,
    `health`        FLOAT NOT NULL DEFAULT 1000.0,
    `engine_health` FLOAT NOT NULL DEFAULT 1000.0,
    `mileage`       INT NOT NULL DEFAULT 0,
    `garage_id`     INT UNSIGNED DEFAULT NULL,     -- NULL = no mundo
    `pos_x`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_y`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_z`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_a`         FLOAT NOT NULL DEFAULT 0.0,
    `interior`      TINYINT NOT NULL DEFAULT 0,
    `world`         TINYINT NOT NULL DEFAULT 0,
    `spawned`       TINYINT NOT NULL DEFAULT 0,
    `locked`        TINYINT NOT NULL DEFAULT 1,
    `insurance`     TINYINT NOT NULL DEFAULT 0,
    `insurance_exp` DATETIME DEFAULT NULL,
    `impounded`     TINYINT NOT NULL DEFAULT 0,
    `impound_reason` VARCHAR(128) DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_plate` (`plate`),
    KEY `idx_owner` (`owner_char_id`),
    KEY `idx_spawned` (`spawned`),
    CONSTRAINT `fk_veh_owner` FOREIGN KEY (`owner_char_id`)
        REFERENCES `characters` (`id`) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- FINANCIAMENTO DE VEÍCULOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `vehicle_financing` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `vehicle_id`    INT UNSIGNED NOT NULL,
    `character_id`  INT UNSIGNED NOT NULL,
    `total_price`   INT NOT NULL,
    `down_payment`  INT NOT NULL DEFAULT 0,
    `balance`       INT NOT NULL,
    `installments`  TINYINT NOT NULL DEFAULT 12,
    `installments_paid` TINYINT NOT NULL DEFAULT 0,
    `monthly_payment` INT NOT NULL,
    `status`        ENUM('active','paid','repossessed') NOT NULL DEFAULT 'active',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_vehicle_id` (`vehicle_id`),
    KEY `idx_character_id` (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- MULTAS DE VEÍCULO (vinculadas)
-- ============================================================
CREATE TABLE IF NOT EXISTS `vehicle_fines` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `vehicle_id`    INT UNSIGNED NOT NULL,
    `character_id`  INT UNSIGNED NOT NULL,
    `reason`        VARCHAR(128) NOT NULL,
    `amount`        INT NOT NULL,
    `status`        ENUM('pending','paid') NOT NULL DEFAULT 'pending',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_vehicle_id` (`vehicle_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- LOGS DE VEÍCULO
-- ============================================================
CREATE TABLE IF NOT EXISTS `vehicle_logs` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `vehicle_id`    INT UNSIGNED NOT NULL,
    `character_id`  INT UNSIGNED DEFAULT NULL,
    `action`        VARCHAR(32) NOT NULL,
    `details`       VARCHAR(128) DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_vehicle_id` (`vehicle_id`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CONCESSIONÁRIAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `dealerships` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `category`      ENUM('car','moto','utility','truck') NOT NULL DEFAULT 'car',
    `pos_x`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_y`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_z`         FLOAT NOT NULL DEFAULT 0.0,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CATÁLOGO DE VEÍCULOS À VENDA
-- ============================================================
CREATE TABLE IF NOT EXISTS `dealership_stock` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `dealership_id` SMALLINT UNSIGNED NOT NULL,
    `model`         SMALLINT NOT NULL,
    `name`          VARCHAR(32) NOT NULL,
    `price`         INT NOT NULL,
    `color1`        TINYINT NOT NULL DEFAULT 0,
    `color2`        TINYINT NOT NULL DEFAULT 1,
    `fuel_type`     ENUM('gasoline','diesel','electric') NOT NULL DEFAULT 'gasoline',
    PRIMARY KEY (`id`),
    KEY `idx_dealership_id` (`dealership_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- POSTOS DE COMBUSTÍVEL
-- ============================================================
CREATE TABLE IF NOT EXISTS `fuel_stations` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `pos_x`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_y`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_z`         FLOAT NOT NULL DEFAULT 0.0,
    `price_gasoline` INT NOT NULL DEFAULT 5,    -- por litro/unidade
    `price_diesel`  INT NOT NULL DEFAULT 4,
    `active`        TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PROPRIEDADES (casas, aptos, terrenos, garagens públicas)
-- ============================================================
CREATE TABLE IF NOT EXISTS `properties` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `owner_char_id` INT UNSIGNED DEFAULT NULL,
    `type`          ENUM('house','apartment','mansion','terrain',
                         'garage','commercial') NOT NULL DEFAULT 'house',
    `name`          VARCHAR(48) NOT NULL,
    `price`         INT NOT NULL DEFAULT 50000,
    `rent_price`    INT NOT NULL DEFAULT 0,       -- 0 = não aluga
    `interior_id`   TINYINT NOT NULL DEFAULT 1,
    `ext_pos_x`     FLOAT NOT NULL DEFAULT 0.0,   -- porta de entrada
    `ext_pos_y`     FLOAT NOT NULL DEFAULT 0.0,
    `ext_pos_z`     FLOAT NOT NULL DEFAULT 0.0,
    `ext_pos_a`     FLOAT NOT NULL DEFAULT 0.0,
    `int_pos_x`     FLOAT NOT NULL DEFAULT 0.0,   -- spawn interior
    `int_pos_y`     FLOAT NOT NULL DEFAULT 0.0,
    `int_pos_z`     FLOAT NOT NULL DEFAULT 0.0,
    `int_world`     SMALLINT NOT NULL DEFAULT 0,
    `int_interior`  TINYINT NOT NULL DEFAULT 1,
    `locked`        TINYINT NOT NULL DEFAULT 1,
    `garage_slots`  TINYINT NOT NULL DEFAULT 1,
    `safe_money`    INT NOT NULL DEFAULT 0,
    `for_sale`      TINYINT NOT NULL DEFAULT 1,
    `for_rent`      TINYINT NOT NULL DEFAULT 0,
    `zone`          VARCHAR(32) DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_owner` (`owner_char_id`),
    KEY `idx_type` (`type`),
    KEY `idx_for_sale` (`for_sale`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PERMISSÕES DE PROPRIEDADE (co-proprietários, inquilinos)
-- ============================================================
CREATE TABLE IF NOT EXISTS `property_permissions` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `property_id`   INT UNSIGNED NOT NULL,
    `character_id`  INT UNSIGNED NOT NULL,
    `permission`    ENUM('key','tenant','co_owner') NOT NULL DEFAULT 'key',
    `granted_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_property_id` (`property_id`),
    KEY `idx_character_id` (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- SEED — CONCESSIONÁRIAS
-- ============================================================
INSERT IGNORE INTO `dealerships` (`id`, `name`, `category`, `pos_x`, `pos_y`, `pos_z`) VALUES
(1, 'Auto Center Norte',  'car',     114.8, -629.5, 16.3),
(2, 'Motos & Cia',        'moto',    490.4, -1741.3, 10.1),
(3, 'Utilitários SA',     'utility', 638.1, -1219.4, 17.3),
(4, 'Caminhões do Brasil','truck',   1017.3, -911.5, 42.3);

-- Catálogo de veículos por concessionária
INSERT IGNORE INTO `dealership_stock`
    (`dealership_id`, `model`, `name`, `price`, `color1`, `color2`, `fuel_type`)
VALUES
-- Auto Center (carros)
(1, 411, 'Infernus',    85000,  0,  0, 'gasoline'),
(1, 415, 'Cheetah',     72000,  3,  3, 'gasoline'),
(1, 420, 'Mesa',        28000,  6,  1, 'gasoline'),
(1, 421, 'Rancher',     35000,  7,  1, 'gasoline'),
(1, 432, 'Rhino',       95000,  2,  2, 'diesel'),
(1, 436, 'Previon',     22000, 10,  1, 'gasoline'),
(1, 445, 'Admiral',     45000,  1,  1, 'gasoline'),
(1, 451, 'Turismo',     78000,  0,  5, 'gasoline'),
(1, 466, 'Romero',      18000,  4,  4, 'gasoline'),
(1, 467, 'Sentinel',    32000,  6,  1, 'gasoline'),
(1, 468, 'Greenwood',   25000,  9,  2, 'gasoline'),
(1, 475, 'Sabre',       38000,  2,  8, 'gasoline'),
(1, 489, 'Rancher',     38000,  5,  3, 'gasoline'),
(1, 491, 'Virgo',       29000,  7,  7, 'gasoline'),
(1, 492, 'Stallion',    31000,  3,  1, 'gasoline'),
(1, 496, 'Blista Compact', 19000, 11, 1, 'gasoline'),
(1, 500, 'Mesa',        27000,  8,  2, 'gasoline'),
(1, 516, 'Nebula',      35000,  1,  4, 'gasoline'),
(1, 517, 'Majestic',    34000,  6,  6, 'gasoline'),
(1, 518, 'Buccaneer',   28000,  0,  3, 'gasoline'),
-- Motos
(2, 461, 'PCJ-600',     12000,  0,  0, 'gasoline'),
(2, 462, 'Faggio',       5000,  4,  4, 'gasoline'),
(2, 463, 'Freeway',      8500,  1,  1, 'gasoline'),
(2, 468, 'Sanchez',      9000,  6,  6, 'gasoline'),
(2, 471, 'Quad',         7000,  2,  2, 'gasoline'),
(2, 481, 'BMX',          1500,  0,  0, 'gasoline'),
(2, 509, 'Bf-400',      10500,  3,  3, 'gasoline'),
(2, 510, 'HPV-1000',    14000,  5,  5, 'gasoline'),
-- Utilitários
(3, 412, 'Moonbeam',    15000,  4,  1, 'gasoline'),
(3, 416, 'Ambulance',   30000,  0,  0, 'diesel'),
(3, 417, 'Firetruck',   35000,  0,  0, 'diesel'),
(3, 422, 'Bobcat',      20000,  7,  1, 'gasoline'),
(3, 426, 'FBI Truck',   28000,  2,  2, 'diesel'),
(3, 431, 'Bus',         25000,  5,  1, 'diesel'),
(3, 437, 'Coach',       22000,  6,  1, 'diesel'),
(3, 440, 'Swat',        40000,  2,  2, 'diesel'),
(3, 443, 'Packer',      32000,  3,  3, 'diesel'),
-- Caminhões
(4, 403, 'Linerunner',  55000,  4,  1, 'diesel'),
(4, 414, 'Tanker',      65000,  3,  3, 'diesel'),
(4, 423, 'Mr Whoopee',  18000,  0,  0, 'gasoline'),
(4, 455, 'Flatbed',     48000,  7,  1, 'diesel'),
(4, 470, 'Patriot',     42000,  2,  2, 'diesel');

-- ============================================================
-- SEED — POSTOS DE COMBUSTÍVEL
-- ============================================================
INSERT IGNORE INTO `fuel_stations`
    (`id`, `name`, `pos_x`, `pos_y`, `pos_z`, `price_gasoline`, `price_diesel`)
VALUES
(1, 'Posto Central',       2021.0, -1415.0, 17.1, 6, 5),
(2, 'Posto Norte',         -85.0,  -283.0,  1.6,  5, 4),
(3, 'Posto Industrial',    1026.0, -1027.0, 32.1, 4, 3),
(4, 'Posto Sul',           490.0, -1765.0,  10.1, 6, 5),
(5, 'Posto Aeroporto',     1782.0, -2418.0, 13.5, 7, 6);

-- ============================================================
-- SEED — PROPRIEDADES (20 imóveis iniciais)
-- ============================================================
INSERT IGNORE INTO `properties`
    (`id`, `type`, `name`, `price`, `rent_price`,
     `ext_pos_x`, `ext_pos_y`, `ext_pos_z`, `ext_pos_a`,
     `int_pos_x`, `int_pos_y`, `int_pos_z`,
     `int_world`, `int_interior`, `garage_slots`, `for_sale`, `zone`)
VALUES
-- Casas
(1,  'house',     'Casa Rua das Flores 1',    80000,  2000, 2233.6, -1117.8, 26.4,  90.0, 2259.0, -1135.8, 1025.0, 1,  3, 1, 1, 'Centro'),
(2,  'house',     'Casa Rua das Flores 2',    85000,  2100, 2233.6, -1123.0, 26.4,  90.0, 2259.0, -1135.8, 1025.0, 2,  3, 1, 1, 'Centro'),
(3,  'house',     'Casa Zona Norte 1',        60000,  1500, 248.7,  -179.3,  2.3,   90.0, 225.1,  -177.8,  1024.3, 3,  3, 1, 1, 'Zona Norte'),
(4,  'house',     'Casa Zona Norte 2',        62000,  1550, 253.0,  -179.3,  2.3,   90.0, 225.1,  -177.8,  1024.3, 4,  3, 1, 1, 'Zona Norte'),
(5,  'house',     'Casa Zona Sul 1',         120000,  3000, 355.7,  -121.3,  1.8,   90.0, 372.0,  -133.5,  1025.0, 5,  3, 2, 1, 'Zona Sul'),
(6,  'house',     'Casa Zona Sul 2',         125000,  3100, 362.0,  -121.3,  1.8,   90.0, 372.0,  -133.5,  1025.0, 6,  3, 2, 1, 'Zona Sul'),
-- Apartamentos
(7,  'apartment', 'Apto Centro 101',          45000,  1200, 485.2,  -23.1,   1001.3, 90.0, 501.5,  -26.3,   1001.5, 7,  8, 0, 1, 'Centro'),
(8,  'apartment', 'Apto Centro 102',          46000,  1250, 485.2,  -26.0,   1001.3, 90.0, 501.5,  -26.3,   1001.5, 8,  8, 0, 1, 'Centro'),
(9,  'apartment', 'Apto Norte 201',           38000,  980,  500.3,  -40.2,   1001.3, 90.0, 502.0,  -41.5,   1001.5, 9,  8, 0, 1, 'Zona Norte'),
(10, 'apartment', 'Apto Norte 202',           39000,  990,  500.3,  -43.2,   1001.3, 90.0, 502.0,  -41.5,   1001.5, 10, 8, 0, 1, 'Zona Norte'),
-- Mansões
(11, 'mansion',   'Mansão Vista Mar',        450000, 10000, 286.8,  -35.8,   2.0,   90.0, 225.1,  -177.8,  1024.3, 11, 10, 3, 1, 'Zona Sul'),
(12, 'mansion',   'Mansão Colina Verde',     380000,  8500, 293.0,  -35.8,   2.0,   90.0, 225.1,  -177.8,  1024.3, 12, 10, 3, 1, 'Zona Sul'),
-- Garagens públicas
(13, 'garage',    'Garagem Centro',           25000,   600, 2100.0, -1800.0, 13.5,  90.0, 2106.5, -1800.5, 1023.5, 13, 0, 3, 1, 'Centro'),
(14, 'garage',    'Garagem Norte',            20000,   500, -80.0,  -310.0,  1.8,   90.0, -79.5,  -308.5,  1023.5, 14, 0, 2, 1, 'Zona Norte'),
(15, 'garage',    'Garagem Industrial',       18000,   450, 1020.0, -1010.0, 32.1,  90.0, 1019.5, -1010.5, 1023.5, 15, 0, 4, 1, 'Zona Industrial'),
-- Comerciais
(16, 'commercial','Ponto Comercial Centro 1', 200000, 5000, 490.0,  -30.0,   1001.8, 90.0, 492.0,  -32.0,   1001.8, 16, 5, 0, 1, 'Centro'),
(17, 'commercial','Ponto Comercial Centro 2', 210000, 5200, 490.0,  -36.0,   1001.8, 90.0, 492.0,  -32.0,   1001.8, 17, 5, 0, 1, 'Centro'),
(18, 'house',     'Casa Rural 1',             40000,  1000, 1290.0, -1067.0, 14.1,  90.0, 225.1,  -177.8,  1024.3, 18, 3, 1, 1, 'Zona Rural'),
(19, 'house',     'Casa Rural 2',             42000,  1050, 1295.0, -1067.0, 14.1,  90.0, 225.1,  -177.8,  1024.3, 19, 3, 1, 1, 'Zona Rural'),
(20, 'apartment', 'Apto Industrial 301',      30000,   800, 1022.0, -910.0,  42.3,  90.0, 502.0,  -41.5,   1001.5, 20, 8, 0, 1, 'Zona Industrial');

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('004', 'Fase 3 Mundo - Veículos, financiamento, concessionárias, postos, propriedades');

-- ============================================================
-- FIM DA MIGRAÇÃO 004
-- ============================================================
