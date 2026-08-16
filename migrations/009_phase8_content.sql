-- ============================================================
--  Santa Aurora Roleplay — Migração 009
--  Fase 8: Conteúdo — Missões, Conquistas, Rankings, Clima
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- DEFINIÇÃO DE MISSÕES
-- ============================================================
CREATE TABLE IF NOT EXISTS `mission_definitions` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `description`   VARCHAR(128) NOT NULL,
    `type`          ENUM('daily','weekly','job','event','story') NOT NULL DEFAULT 'daily',
    `category`      ENUM('work','drive','buy','sell','explore','social','faction') NOT NULL DEFAULT 'work',
    `target_value`  INT NOT NULL DEFAULT 1,
    `reward_money`  INT NOT NULL DEFAULT 0,
    `reward_xp`     INT NOT NULL DEFAULT 100,
    `reward_item`   SMALLINT UNSIGNED DEFAULT NULL,
    `reward_item_qty` TINYINT NOT NULL DEFAULT 0,
    `min_level`     TINYINT NOT NULL DEFAULT 1,
    `active`        TINYINT NOT NULL DEFAULT 1,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PROGRESSO DE MISSÕES DO PERSONAGEM
-- ============================================================
CREATE TABLE IF NOT EXISTS `character_missions` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `mission_id`    SMALLINT UNSIGNED NOT NULL,
    `progress`      INT NOT NULL DEFAULT 0,
    `completed`     TINYINT NOT NULL DEFAULT 0,
    `completed_at`  DATETIME DEFAULT NULL,
    `assigned_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `expires_at`    DATETIME DEFAULT NULL,
    PRIMARY KEY (`id`),
    KEY `idx_character_id` (`character_id`),
    KEY `idx_mission_id` (`mission_id`),
    KEY `idx_completed` (`completed`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- DEFINIÇÃO DE CONQUISTAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `achievement_definitions` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `description`   VARCHAR(128) NOT NULL,
    `icon`          VARCHAR(32) DEFAULT NULL,
    `reward_xp`     INT NOT NULL DEFAULT 200,
    `reward_money`  INT NOT NULL DEFAULT 0,
    `hidden`        TINYINT NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- SEED — MISSÕES DIÁRIAS
-- ============================================================
INSERT IGNORE INTO `mission_definitions`
    (`id`, `name`, `description`, `type`, `category`, `target_value`,
     `reward_money`, `reward_xp`, `min_level`)
VALUES
-- Trabalho
(1,  'MISSION_WORK_5',     'Realizar 5 ações de trabalho',       'daily', 'work',    5,   500, 100, 1),
(2,  'MISSION_WORK_15',    'Realizar 15 ações de trabalho',      'daily', 'work',   15,  1500, 250, 5),
(3,  'MISSION_EARN_2K',    'Ganhar $2.000 trabalhando',          'daily', 'work',  2000, 1000, 150, 1),
(4,  'MISSION_EARN_5K',    'Ganhar $5.000 trabalhando',          'daily', 'work',  5000, 2000, 300, 3),
-- Dirigir
(5,  'MISSION_DRIVE_10KM', 'Dirigir 10km sem infração',         'daily', 'drive',   10000, 800,  120, 1),
(6,  'MISSION_DRIVE_25KM', 'Dirigir 25km',                      'daily', 'drive',   25000, 1500, 200, 3),
(7,  'MISSION_REFUEL',     'Abastecer um veículo',              'daily', 'drive',   1,    200,   50, 1),
-- Comprar/Vender
(8,  'MISSION_BUY_FOOD',   'Comprar 3 itens de comida/bebida',  'daily', 'buy',     3,    300,   80, 1),
(9,  'MISSION_SPEND_500',  'Gastar $500 em lojas',              'daily', 'buy',   500,    500,  100, 1),
-- Social / Explorar
(10, 'MISSION_CHAT_10',    'Enviar 10 mensagens locais',        'daily', 'social',  10,   200,   60, 1),
(11, 'MISSION_VISIT_3',    'Visitar 3 zonas diferentes',        'daily', 'explore', 3,    600,  120, 1),
(12, 'MISSION_REPORT',     'Enviar um report à equipe',         'daily', 'social',  1,    100,   30, 1),
-- Semanais
(20, 'MISSION_WEEKLY_WORK','Realizar 50 ações de trabalho',    'weekly','work',   50,   5000,  500, 1),
(21, 'MISSION_WEEKLY_EARN','Ganhar $20.000 trabalhando',       'weekly','work', 20000, 10000,  800, 3),
(22, 'MISSION_WEEKLY_DRIVE','Dirigir 100km',                   'weekly','drive',100000, 3000,  400, 1);

-- ============================================================
-- SEED — CONQUISTAS
-- ============================================================
INSERT IGNORE INTO `achievement_definitions`
    (`id`, `name`, `description`, `reward_xp`, `reward_money`)
VALUES
(1,  'ACH_FIRST_JOB',      'Primeiro Emprego: conseguir um emprego',              200,   0),
(2,  'ACH_FIRST_VEHICLE',  'Primeiro Veículo: comprar um veículo',                300, 500),
(3,  'ACH_FIRST_PROPERTY', 'Primeiro Imóvel: comprar uma propriedade',            500,   0),
(4,  'ACH_100KM',          'Viajante: dirigir 100km no total',                    300,   0),
(5,  'ACH_100_ACTIONS',    'Trabalhador: realizar 100 ações de trabalho',         400, 1000),
(6,  'ACH_BUSINESS_OWNER', 'Empresário: comprar uma empresa',                     500, 2000),
(7,  'ACH_VETERAN',        'Veterano: atingir nível 10',                         1000, 5000),
(8,  'ACH_RICH',           'Rico: ter $100.000 no banco',                        1000,    0),
(9,  'ACH_FACTION_JOIN',   'Pertencer a uma facção',                              200,   0),
(10, 'ACH_FIRST_MISSION',  'Completar a primeira missão diária',                  200, 500),
(11, 'ACH_10_MISSIONS',    'Completar 10 missões diárias',                        500, 2000),
(12, 'ACH_TUTORIAL_DONE',  'Completar o tutorial',                                300, 0),
(13, 'ACH_PHONE_OWNER',    'Ter um celular ativo',                                100, 0),
(14, 'ACH_LEVEL_5',        'Atingir nível 5',                                     300, 1000),
(15, 'ACH_LEVEL_20',       'Atingir nível 20',                                    800, 5000),
(16, 'ACH_BANK_ACCOUNT',   'Criar uma conta bancária',                            100, 0),
(17, 'ACH_DOCS_COMPLETE',  'Ter RG, CPF e CNH',                                   400, 0),
(18, 'ACH_50_DELIVERIES',  'Realizar 50 entregas',                                500, 2000),
(19, 'ACH_CRIMINAL',       'Acumular 3 fichas criminais',                         200, 0),
(20, 'ACH_TERRITORY',      'Capturar um território para a facção',                600, 0);

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('009', 'Fase 8 Conteúdo - missões diárias/semanais, conquistas, definições');
