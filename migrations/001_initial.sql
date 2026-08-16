-- ============================================================
--  Santa Aurora Roleplay — Migração 001
--  Fase 0: Fundação — Tabelas base
--  Executar: mysql -u samp -p santa_aurora < 001_initial.sql
-- ============================================================

-- Criar banco se não existir
CREATE DATABASE IF NOT EXISTS `santa_aurora`
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE `santa_aurora`;

-- Controle de migrações executadas
CREATE TABLE IF NOT EXISTS `schema_migrations` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `version`       VARCHAR(20) NOT NULL,
    `description`   VARCHAR(128) NOT NULL,
    `applied_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_version` (`version`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CONTAS
-- ============================================================
CREATE TABLE IF NOT EXISTS `accounts` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `username`      VARCHAR(24) NOT NULL,
    `password_hash` VARCHAR(64) NOT NULL,               -- Whirlpool hash
    `email`         VARCHAR(64) DEFAULT NULL,
    `status`        TINYINT NOT NULL DEFAULT 1,         -- 0=desativado, 1=ativo, 2=banido
    `admin_level`   TINYINT NOT NULL DEFAULT 0,
    `admin_name`    VARCHAR(24) DEFAULT NULL,            -- nome exibido ao usar admin
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `last_login`    DATETIME DEFAULT NULL,
    `last_ip`       VARCHAR(16) DEFAULT NULL,
    `serial`        VARCHAR(40) DEFAULT NULL,            -- GPCI
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_username` (`username`),
    KEY `idx_status` (`status`),
    KEY `idx_last_ip` (`last_ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PERSONAGENS
-- ============================================================
CREATE TABLE IF NOT EXISTS `characters` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `account_id`    INT UNSIGNED NOT NULL,
    `name`          VARCHAR(24) NOT NULL,
    `sex`           TINYINT NOT NULL DEFAULT 0,          -- 0=M 1=F
    `age`           TINYINT NOT NULL DEFAULT 18,
    `skin_id`       SMALLINT NOT NULL DEFAULT 0,
    `health`        FLOAT NOT NULL DEFAULT 100.0,
    `armour`        FLOAT NOT NULL DEFAULT 0.0,
    `hunger`        TINYINT NOT NULL DEFAULT 100,
    `thirst`        TINYINT NOT NULL DEFAULT 100,
    `money`         INT NOT NULL DEFAULT 500,
    `bank`          INT NOT NULL DEFAULT 1000,
    `level`         SMALLINT NOT NULL DEFAULT 1,
    `experience`    INT NOT NULL DEFAULT 0,
    `job_id`        SMALLINT NOT NULL DEFAULT 0,
    `faction_id`    SMALLINT NOT NULL DEFAULT 0,
    `faction_rank`  TINYINT NOT NULL DEFAULT 0,
    `pos_x`         FLOAT NOT NULL DEFAULT 1545.5,
    `pos_y`         FLOAT NOT NULL DEFAULT -1675.7,
    `pos_z`         FLOAT NOT NULL DEFAULT 13.5,
    `pos_a`         FLOAT NOT NULL DEFAULT 90.0,
    `interior`      TINYINT NOT NULL DEFAULT 0,
    `world`         TINYINT NOT NULL DEFAULT 0,
    `state`         TINYINT NOT NULL DEFAULT 1,          -- 1=ativo, 99=deletado
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_account_id` (`account_id`),
    KEY `idx_name` (`name`),
    CONSTRAINT `fk_char_account` FOREIGN KEY (`account_id`)
        REFERENCES `accounts` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- STATS DO PERSONAGEM
-- ============================================================
CREATE TABLE IF NOT EXISTS `character_stats` (
    `id`                INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `character_id`      INT UNSIGNED NOT NULL,
    `total_playtime`    INT NOT NULL DEFAULT 0,          -- segundos
    `distance_driven`   FLOAT NOT NULL DEFAULT 0.0,
    `deaths`            INT NOT NULL DEFAULT 0,
    `kills`             INT NOT NULL DEFAULT 0,
    `arrests`           INT NOT NULL DEFAULT 0,
    `created_at`        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_character_id` (`character_id`),
    CONSTRAINT `fk_stats_char` FOREIGN KEY (`character_id`)
        REFERENCES `characters` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- PUNIÇÕES
-- ============================================================
CREATE TABLE IF NOT EXISTS `punishments` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `player_id`     INT UNSIGNED NOT NULL,               -- account.id
    `admin_id`      INT UNSIGNED NOT NULL DEFAULT 0,     -- account.id do admin (0=console)
    `type`          ENUM('warn','mute','kick','jail','ban_temp','ban_perm','ban')
                    NOT NULL DEFAULT 'warn',
    `reason`        VARCHAR(256) NOT NULL,
    `duration`      INT NOT NULL DEFAULT 0,              -- minutos (0=permanente)
    `expires_at`    DATETIME DEFAULT NULL,
    `active`        TINYINT NOT NULL DEFAULT 1,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_player_id` (`player_id`),
    KEY `idx_type_active` (`type`, `active`),
    KEY `idx_expires_at` (`expires_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- LOGS ADMINISTRATIVOS
-- ============================================================
CREATE TABLE IF NOT EXISTS `admin_logs` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `admin_id`      INT UNSIGNED NOT NULL DEFAULT 0,
    `target_id`     INT UNSIGNED NOT NULL DEFAULT 0,
    `action`        VARCHAR(64) NOT NULL,
    `details`       TEXT DEFAULT NULL,
    `ip`            VARCHAR(16) DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_admin_id` (`admin_id`),
    KEY `idx_target_id` (`target_id`),
    KEY `idx_action` (`action`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- LOGS DE SEGURANÇA
-- ============================================================
CREATE TABLE IF NOT EXISTS `security_logs` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `player_id`     INT UNSIGNED NOT NULL DEFAULT 0,
    `player_name`   VARCHAR(24) DEFAULT NULL,
    `event_type`    VARCHAR(64) NOT NULL,
    `details`       TEXT DEFAULT NULL,
    `ip`            VARCHAR(16) DEFAULT NULL,
    `serial`        VARCHAR(40) DEFAULT NULL,
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_player_id` (`player_id`),
    KEY `idx_event_type` (`event_type`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- CONFIGURAÇÃO DO SERVIDOR
-- ============================================================
CREATE TABLE IF NOT EXISTS `server_config` (
    `id`            INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `config_key`    VARCHAR(64) NOT NULL,
    `config_value`  TEXT NOT NULL,
    `description`   VARCHAR(256) DEFAULT NULL,
    `updated_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_config_key` (`config_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- DADOS INICIAIS (SEED)
-- ============================================================

-- Configurações padrão do servidor
INSERT IGNORE INTO `server_config` (`config_key`, `config_value`, `description`) VALUES
('economy.starting_money',    '500',      'Dinheiro inicial para novos personagens'),
('economy.starting_bank',     '1000',     'Saldo bancário inicial para novos personagens'),
('economy.tax_rate',          '0.05',     'Taxa de imposto padrão (5%)'),
('economy.max_money',         '2000000000', 'Limite máximo de dinheiro físico'),
('gameplay.afk_timeout',      '300',      'Segundos para considerar jogador AFK'),
('gameplay.save_interval',    '300',      'Intervalo de save automático em segundos'),
('gameplay.login_max_attempts','5',       'Máximo de tentativas de login antes do kick'),
('server.max_players',        '200',      'Máximo de jogadores simultâneos'),
('server.language',           'pt-BR',    'Idioma do servidor'),
('anticheat.enabled',         '1',        'Anti-cheat ativado (1=sim, 0=não)');

-- Registrar migração
INSERT IGNORE INTO `schema_migrations` (`version`, `description`)
VALUES ('001', 'Fase 0 Fundação - Tabelas base: accounts, characters, stats, punishments, logs, config');

-- ============================================================
-- FIM DA MIGRAÇÃO 001
-- ============================================================

