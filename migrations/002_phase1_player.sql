-- ============================================================
--  Santa Aurora Roleplay — Migração 002
--  Fase 1: Jogador — Documentos, Inventário, Tutorial
-- ============================================================

USE `santa_aurora`;

-- ============================================================
-- DOCUMENTOS DO PERSONAGEM
-- ============================================================
CREATE TABLE IF NOT EXISTS `character_documents` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `type`          ENUM('rg','cpf','cnh','gun_license','work_license',
                         'vehicle_doc','property_doc') NOT NULL,
    `number`        VARCHAR(20) NOT NULL,
    `issued_at`     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `expires_at`    DATETIME DEFAULT NULL,
    `status`        ENUM('valid','expired','revoked','suspended') NOT NULL DEFAULT 'valid',
    `issued_by`     VARCHAR(32) DEFAULT 'Prefeitura',
    -- CNH específico
    `cnh_category`  VARCHAR(5) DEFAULT NULL,            -- A, B, C, D, E
    `cnh_points`    TINYINT NOT NULL DEFAULT 0,
    `cnh_suspended` TINYINT NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`),
    KEY `idx_char_id` (`character_id`),
    KEY `idx_type` (`type`),
    KEY `idx_status` (`status`),
    CONSTRAINT `fk_doc_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- DEFINIÇÃO DE ITENS (item registry central)
-- ============================================================
CREATE TABLE IF NOT EXISTS `item_definitions` (
    `id`            SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name`          VARCHAR(48) NOT NULL,
    `display_name`  VARCHAR(48) NOT NULL,
    `category`      ENUM('food','drink','document','tool','material',
                         'medicine','job_item','special','weapon','ammo',
                         'clothing','misc') NOT NULL DEFAULT 'misc',
    `weight`        FLOAT NOT NULL DEFAULT 0.1,          -- kg
    `max_stack`     SMALLINT NOT NULL DEFAULT 99,
    `base_value`    INT NOT NULL DEFAULT 0,
    `usable`        TINYINT NOT NULL DEFAULT 0,
    `stackable`     TINYINT NOT NULL DEFAULT 1,
    `description`   VARCHAR(128) DEFAULT NULL,
    `on_use_effect` VARCHAR(64) DEFAULT NULL,            -- identificador do efeito
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_name` (`name`),
    KEY `idx_category` (`category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- INVENTÁRIO DO PERSONAGEM
-- ============================================================
CREATE TABLE IF NOT EXISTS `inventory` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `item_id`       SMALLINT UNSIGNED NOT NULL,
    `quantity`      SMALLINT NOT NULL DEFAULT 1,
    `slot`          TINYINT NOT NULL DEFAULT 0,
    `data`          VARCHAR(128) DEFAULT NULL,           -- dados extras (ex: durabilidade)
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_char_id` (`character_id`),
    KEY `idx_item_id` (`item_id`),
    CONSTRAINT `fk_inv_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_inv_item` FOREIGN KEY (`item_id`)
        REFERENCES `item_definitions` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PROGRESSO DO TUTORIAL
-- ============================================================
CREATE TABLE IF NOT EXISTS `tutorial_progress` (
    `character_id`  INT UNSIGNED NOT NULL,
    `step`          TINYINT NOT NULL DEFAULT 0,
    `completed`     TINYINT NOT NULL DEFAULT 0,
    `completed_at`  DATETIME DEFAULT NULL,
    PRIMARY KEY (`character_id`),
    CONSTRAINT `fk_tut_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CONQUISTAS DO PERSONAGEM
-- ============================================================
CREATE TABLE IF NOT EXISTS `character_achievements` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`  INT UNSIGNED NOT NULL,
    `achievement_id` SMALLINT UNSIGNED NOT NULL,
    `unlocked_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_char_ach` (`character_id`, `achievement_id`),
    KEY `idx_char_id` (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- SEED — ITENS BASE
-- ============================================================
INSERT IGNORE INTO `item_definitions`
    (`id`, `name`, `display_name`, `category`, `weight`, `max_stack`,
     `base_value`, `usable`, `stackable`, `description`, `on_use_effect`)
VALUES
-- Comida
(1,  'ITEM_BREAD',       'Pão',              'food',     0.3, 10,   50,  1, 1, 'Pão simples. Restaura um pouco de fome.',     'hunger+20'),
(2,  'ITEM_SANDWICH',    'Sanduíche',        'food',     0.3, 10,  120,  1, 1, 'Sanduíche. Restaura boa quantidade de fome.', 'hunger+40'),
(3,  'ITEM_BURGER',      'Hambúrguer',       'food',     0.4,  5,  250,  1, 1, 'Hambúrguer completo.',                        'hunger+60'),
(4,  'ITEM_PIZZA',       'Pizza',            'food',     0.5,  5,  400,  1, 1, 'Uma fatia de pizza.',                         'hunger+50'),
(5,  'ITEM_RICE_BEANS',  'Arroz e Feijão',   'food',     0.4,  5,  200,  1, 1, 'Prato completo brasileiro.',                  'hunger+70'),

-- Bebida
(10, 'ITEM_WATER',       'Água',             'drink',    0.5, 10,   30,  1, 1, 'Garrafa de água.',                            'thirst+30'),
(11, 'ITEM_SODA',        'Refrigerante',     'drink',    0.5, 10,   80,  1, 1, 'Lata de refrigerante.',                       'thirst+40'),
(12, 'ITEM_JUICE',       'Suco',             'drink',    0.4, 10,  100,  1, 1, 'Copo de suco natural.',                       'thirst+50'),
(13, 'ITEM_COFFEE',      'Café',             'drink',    0.2, 10,   60,  1, 1, 'Xícara de café.',                             'thirst+20'),
(14, 'ITEM_ENERGY',      'Energético',       'drink',    0.3, 10,  150,  1, 1, 'Bebida energética.',                          'thirst+35'),

-- Medicamentos
(20, 'ITEM_BANDAGE',     'Curativo',         'medicine', 0.1, 20,  150,  1, 1, 'Curativo básico. Restaura pouca vida.',        'health+10'),
(21, 'ITEM_MEDKIT',      'Kit Médico',       'medicine', 0.5,  5,  500,  1, 1, 'Kit de primeiros socorros.',                  'health+50'),
(22, 'ITEM_PILLS',       'Comprimidos',      'medicine', 0.1, 10,   80,  1, 1, 'Analgésico.',                                 'health+20'),
(23, 'ITEM_MORPHINE',    'Morfina',          'medicine', 0.1,  5, 1000,  1, 1, 'Analgésico forte (uso hospitalar).',          'health+80'),

-- Ferramentas
(30, 'ITEM_PHONE',       'Celular',          'misc',     0.2,  1,  500,  1, 0, 'Celular para ligações e mensagens.',          'open_phone'),
(31, 'ITEM_REPAIR_KIT',  'Kit de Reparo',    'tool',     1.0,  3,  300,  1, 1, 'Para reparar veículos levemente.',            'repair_vehicle'),
(32, 'ITEM_FUEL_CAN',    'Galão de Gasolina','tool',     2.0,  3,  200,  1, 1, 'Gasolina para veículos.',                     'refuel_vehicle'),
(33, 'ITEM_WRENCH',      'Chave de Fenda',   'tool',     0.5,  1,  150,  0, 0, 'Ferramenta mecânica.',                        NULL),
(34, 'ITEM_LOCKPICK',    'Gazua',            'tool',     0.1,  5,  500,  1, 1, 'Para abrir fechaduras.',                      'use_lockpick'),
(35, 'ITEM_FLASHLIGHT',  'Lanterna',         'tool',     0.3,  1,  100,  1, 0, 'Iluminação portátil.',                        'toggle_light'),

-- Documentos
(40, 'ITEM_DOC_RG',      'RG',               'document', 0.0,  1,    0,  1, 0, 'Registro Geral.',                             'show_document'),
(41, 'ITEM_DOC_CPF',     'CPF',              'document', 0.0,  1,    0,  1, 0, 'CPF do personagem.',                          'show_document'),
(42, 'ITEM_DOC_CNH',     'CNH',              'document', 0.0,  1,    0,  1, 0, 'Carteira Nacional de Habilitação.',            'show_document'),

-- Materiais
(50, 'ITEM_WOOD',        'Madeira',          'material', 2.0, 50,   80,  0, 1, 'Tábua de madeira.',                           NULL),
(51, 'ITEM_METAL',       'Metal',            'material', 3.0, 50,  120,  0, 1, 'Peça metálica.',                              NULL),
(52, 'ITEM_FABRIC',      'Tecido',           'material', 0.5, 50,   60,  0, 1, 'Rolo de tecido.',                             NULL),
(53, 'ITEM_PLASTIC',     'Plástico',         'material', 0.5, 50,   40,  0, 1, 'Peça de plástico.',                           NULL),

-- Itens especiais
(60, 'ITEM_BRIEFCASE',   'Maleta',           'special',  1.5,  1, 2000,  1, 0, 'Maleta misteriosa.',                          NULL),
(61, 'ITEM_KEY_HOUSE',   'Chave de Casa',    'special',  0.1,  5,  100,  1, 1, 'Chave de uma propriedade.',                   'use_key'),
(62, 'ITEM_KEY_VEHICLE', 'Chave de Veículo', 'special',  0.1,  5,  100,  1, 1, 'Chave de veículo.',                           'use_key');

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('002', 'Fase 1 Jogador - Documentos, inventário, tutorial, conquistas, itens seed');

-- ============================================================
-- FIM DA MIGRAÇÃO 002
-- ============================================================
