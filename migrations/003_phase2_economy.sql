-- ============================================================
--  Santa Aurora Roleplay — Migração 003
--  Fase 2: Economia — Banco, PIX, Empregos, Lojas, Impostos
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- CONTAS BANCÁRIAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `bank_accounts` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `account_number` VARCHAR(12) NOT NULL,
    `balance`       INT NOT NULL DEFAULT 0,
    `limit_pix`     INT NOT NULL DEFAULT 10000,     -- limite diário de PIX
    `limit_transfer` INT NOT NULL DEFAULT 50000,    -- limite diário de transferência
    `blocked`       TINYINT NOT NULL DEFAULT 0,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_char_id` (`character_id`),
    UNIQUE KEY `uq_account_number` (`account_number`),
    CONSTRAINT `fk_bank_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- TRANSAÇÕES BANCÁRIAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `bank_transactions` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `from_char_id`  INT UNSIGNED DEFAULT NULL,
    `to_char_id`    INT UNSIGNED DEFAULT NULL,
    `type`          ENUM('deposit','withdraw','transfer','pix','salary',
                         'tax','fine','loan_payment','purchase','refund',
                         'admin_give','admin_remove') NOT NULL,
    `amount`        INT NOT NULL,
    `balance_after` INT NOT NULL DEFAULT 0,
    `description`   VARCHAR(128) DEFAULT NULL,
    `reference_id`  VARCHAR(32) DEFAULT NULL,       -- idempotência
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_from_char` (`from_char_id`),
    KEY `idx_to_char` (`to_char_id`),
    KEY `idx_type` (`type`),
    KEY `idx_created_at` (`created_at`),
    KEY `idx_reference_id` (`reference_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CHAVES PIX
-- ============================================================
CREATE TABLE IF NOT EXISTS `pix_keys` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `key_type`      ENUM('cpf','phone','random') NOT NULL,
    `key_value`     VARCHAR(64) NOT NULL,
    `active`        TINYINT NOT NULL DEFAULT 1,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_key_value` (`key_value`),
    KEY `idx_character_id` (`character_id`),
    CONSTRAINT `fk_pix_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- EMPRÉSTIMOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `loans` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `principal`     INT NOT NULL,
    `interest_rate` FLOAT NOT NULL DEFAULT 0.05,
    `balance`       INT NOT NULL,
    `installments`  TINYINT NOT NULL DEFAULT 12,
    `installments_paid` TINYINT NOT NULL DEFAULT 0,
    `monthly_payment` INT NOT NULL,
    `status`        ENUM('active','paid','defaulted') NOT NULL DEFAULT 'active',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `due_date`      DATETIME DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_status` (`status`),
    CONSTRAINT `fk_loan_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- EMPREGOS (definições)
-- ============================================================
CREATE TABLE IF NOT EXISTS `jobs` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `display_name`  VARCHAR(48) NOT NULL,
    `category`      ENUM('starter','professional','illegal') NOT NULL DEFAULT 'starter',
    `min_level`     TINYINT NOT NULL DEFAULT 1,
    `base_salary`   INT NOT NULL DEFAULT 1000,
    `salary_per_level` INT NOT NULL DEFAULT 200,
    `xp_per_action` INT NOT NULL DEFAULT 10,
    `xp_per_level`  INT NOT NULL DEFAULT 500,
    `max_level`     TINYINT NOT NULL DEFAULT 10,
    `uniform_skin`  SMALLINT DEFAULT NULL,
    `description`   VARCHAR(128) DEFAULT NULL,
    `active`        TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- EMPREGOS DO PERSONAGEM
-- ============================================================
CREATE TABLE IF NOT EXISTS `character_jobs` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `job_id`        SMALLINT UNSIGNED NOT NULL,
    `job_level`     TINYINT NOT NULL DEFAULT 1,
    `job_xp`        INT NOT NULL DEFAULT 0,
    `total_actions` INT NOT NULL DEFAULT 0,
    `total_earned`  INT NOT NULL DEFAULT 0,
    `hired_at`      DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `last_worked`   DATETIME DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_job_id` (`job_id`),
    CONSTRAINT `fk_charjob_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_charjob_job` FOREIGN KEY (`job_id`)
        REFERENCES `jobs` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- LOJAS (definições)
-- ============================================================
CREATE TABLE IF NOT EXISTS `stores` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `type`          ENUM('market','pharmacy','clothes','electronics',
                         'tools','food','convenience') NOT NULL DEFAULT 'convenience',
    `pos_x`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_y`         FLOAT NOT NULL DEFAULT 0.0,
    `pos_z`         FLOAT NOT NULL DEFAULT 0.0,
    `interior`      TINYINT NOT NULL DEFAULT 0,
    `world`         TINYINT NOT NULL DEFAULT 0,
    `active`        TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- ESTOQUE DE LOJAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `store_stock` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `store_id`      SMALLINT UNSIGNED NOT NULL,
    `item_id`       SMALLINT UNSIGNED NOT NULL,
    `price`         INT NOT NULL,
    `stock`         INT NOT NULL DEFAULT -1,            -- -1 = ilimitado
    PRIMARY KEY (`id`),
    KEY `idx_store_id` (`store_id`),
    KEY `idx_item_id` (`item_id`),
    CONSTRAINT `fk_stock_store` FOREIGN KEY (`store_id`)
        REFERENCES `stores` (`id`),
    CONSTRAINT `fk_stock_item` FOREIGN KEY (`item_id`)
        REFERENCES `item_definitions` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- MULTAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `fines` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `officer_id`    INT UNSIGNED DEFAULT NULL,
    `type`          ENUM('traffic','police','administrative') NOT NULL DEFAULT 'traffic',
    `reason`        VARCHAR(128) NOT NULL,
    `amount`        INT NOT NULL,
    `status`        ENUM('pending','paid','contested','cancelled') NOT NULL DEFAULT 'pending',
    `due_date`      DATETIME DEFAULT NULL,
    `paid_at`       DATETIME DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- SEED — EMPREGOS INICIAIS
-- ============================================================
INSERT IGNORE INTO `jobs`
    (`id`, `name`, `display_name`, `category`, `min_level`, `base_salary`,
     `salary_per_level`, `xp_per_action`, `xp_per_level`, `max_level`, `description`)
VALUES
(1,  'JOB_DELIVERY',       'Entregador',               'starter', 1,  800,  100, 10, 300, 10, 'Faça entregas de pacotes pela cidade.'),
(2,  'JOB_BUS_DRIVER',     'Motorista de Ônibus',       'starter', 1, 1000,  150, 12, 400, 10, 'Transporte passageiros pelas linhas da cidade.'),
(3,  'JOB_TRUCKER',        'Caminhoneiro',              'starter', 3, 1500,  200, 15, 500, 10, 'Transporte cargas entre depósitos.'),
(4,  'JOB_TAXI',           'Taxista',                   'starter', 1,  700,   80, 8,  250, 10, 'Leve passageiros a seus destinos.'),
(5,  'JOB_MECHANIC',       'Mecânico',                  'starter', 2, 1200,  150, 12, 400, 10, 'Repare veículos na oficina.'),
(6,  'JOB_FISHERMAN',      'Pescador',                  'starter', 1,  600,   80, 8,  250, 10, 'Pesque no rio ou no litoral.'),
(7,  'JOB_FARMER',         'Agricultor',                'starter', 1,  700,   90, 8,  250, 10, 'Cultive na zona rural.'),
(8,  'JOB_GARBAGEMAN',     'Coletor de Lixo',           'starter', 1,  900,  100, 10, 300, 10, 'Colete o lixo pelos bairros.'),
(9,  'JOB_MINER',          'Minerador',                 'starter', 2, 1100,  150, 12, 400, 10, 'Extraia minérios na zona industrial.'),
(10, 'JOB_FOOD_DELIVERY',  'Entregador de Comida',      'starter', 1,  750,   90, 8,  250, 10, 'Entregue pedidos de restaurantes.'),
(11, 'JOB_RIDESHARE',      'Motorista de Aplicativo',   'starter', 1,  800,  100, 10, 300, 10, 'Transporte pessoas pelo app RP.'),
(12, 'JOB_POLICE',         'Policial',                  'professional', 5, 2000, 300, 20, 800, 15, 'Mantenha a ordem em Santa Aurora.'),
(13, 'JOB_MEDIC',          'Médico/Paramédico',         'professional', 5, 2000, 300, 20, 800, 15, 'Salve vidas pela cidade.'),
(14, 'JOB_LAWYER',         'Advogado',                  'professional', 8, 3000, 400, 25, 1000, 15, 'Defenda ou acuse no fórum.'),
(15, 'JOB_JUDGE',          'Juiz',                      'professional', 10, 4000, 500, 30, 1200, 15, 'Julgue casos no tribunal.'),
(16, 'JOB_CIVIL_SERVANT',  'Funcionário Público',       'professional', 5, 1800, 250, 18, 700, 15, 'Trabalhe na prefeitura.');

-- ============================================================
-- SEED — LOJAS
-- ============================================================
INSERT IGNORE INTO `stores`
    (`id`, `name`, `type`, `pos_x`, `pos_y`, `pos_z`)
VALUES
(1, 'Mercado Central',    'market',      203.8,  -24.6,  1001.8),
(2, 'Farmácia São Lucas', 'pharmacy',    490.1,  -30.2,  1001.8),
(3, 'Loja 24/7 Norte',    'convenience', -25.9, -185.7,  1003.6),
(4, 'Loja 24/7 Sul',      'convenience', 362.9, -74.4,   1001.8),
(5, 'Ferragens Cidade',   'tools',       298.7, -112.3,  1001.8);

-- Estoque dos mercados/lojas
INSERT IGNORE INTO `store_stock` (`store_id`, `item_id`, `price`, `stock`) VALUES
-- Mercado Central (id=1)
(1, 1, 80,   -1),   -- Pão
(1, 2, 180,  -1),   -- Sanduíche
(1, 3, 300,  -1),   -- Hambúrguer
(1, 5, 250,  -1),   -- Arroz e Feijão
(1, 10, 50,  -1),   -- Água
(1, 11, 100, -1),   -- Refrigerante
(1, 12, 120, -1),   -- Suco
-- Farmácia (id=2)
(2, 20, 200, -1),   -- Curativo
(2, 21, 600, -1),   -- Kit Médico
(2, 22, 100, -1),   -- Comprimidos
(2, 10, 60,  -1),   -- Água
-- Loja 24/7 Norte (id=3)
(3, 1, 90,   -1),   -- Pão
(3, 10, 60,  -1),   -- Água
(3, 11, 110, -1),   -- Refrigerante
(3, 13, 80,  -1),   -- Café
(3, 14, 180, -1),   -- Energético
-- Loja 24/7 Sul (id=4)
(4, 1, 90,   -1),
(4, 2, 200,  -1),
(4, 10, 60,  -1),
(4, 11, 110, -1),
-- Ferragens (id=5)
(5, 31, 350, -1),   -- Kit de Reparo
(5, 32, 250, -1),   -- Galão de Gasolina
(5, 33, 200, -1),   -- Chave de Fenda
(5, 35, 130, -1);   -- Lanterna

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('003', 'Fase 2 Economia - Banco, PIX, empréstimos, empregos, lojas, multas');

-- ============================================================
-- FIM DA MIGRAÇÃO 003
-- ============================================================
