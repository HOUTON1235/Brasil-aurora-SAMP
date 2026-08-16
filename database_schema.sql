-- ============================================================
--  Santa Aurora Roleplay - Database Schema
--  Version: 1.0
--  Last Updated: 2026-08-16
-- ============================================================

-- ============================================================
-- CREATE DATABASE
-- ============================================================
CREATE DATABASE IF NOT EXISTS samp_aurora;
USE samp_aurora;

-- ============================================================
-- ACCOUNTS TABLE
-- ============================================================
CREATE TABLE IF NOT EXISTS `accounts` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `username` VARCHAR(24) UNIQUE NOT NULL,
    `password` VARCHAR(255) NOT NULL,
    `email` VARCHAR(64) NOT NULL,
    `admin_level` INT DEFAULT 0,
    `ip_address` VARCHAR(16),
    `gpci` VARCHAR(41),
    `last_login` DATETIME,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_username (username),
    INDEX idx_email (email),
    INDEX idx_admin_level (admin_level),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- CHARACTERS TABLE
-- ============================================================
CREATE TABLE IF NOT EXISTS `characters` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `account_id` INT NOT NULL,
    `name` VARCHAR(32) UNIQUE NOT NULL,
    `sex` INT DEFAULT 0,
    `age` INT DEFAULT 18,
    `skin` INT DEFAULT 0,
    
    -- Vitals
    `health` FLOAT DEFAULT 100.0,
    `armour` FLOAT DEFAULT 0.0,
    `hunger` INT DEFAULT 100,
    `thirst` INT DEFAULT 100,
    
    -- Economy
    `money` INT DEFAULT 500,
    `bank` INT DEFAULT 1000,
    
    -- Progression
    `level` INT DEFAULT 1,
    `experience` INT DEFAULT 0,
    
    -- Job/Faction
    `job_id` INT DEFAULT 0,
    `faction_id` INT DEFAULT 0,
    `faction_rank` INT DEFAULT 0,
    
    -- Position
    `pos_x` FLOAT DEFAULT 1545.5,
    `pos_y` FLOAT DEFAULT -1675.7,
    `pos_z` FLOAT DEFAULT 13.5,
    `pos_a` FLOAT DEFAULT 90.0,
    `interior` INT DEFAULT 0,
    `world` INT DEFAULT 0,
    
    -- Session
    `total_playtime` INT DEFAULT 0,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    INDEX idx_account_id (account_id),
    INDEX idx_name (name),
    INDEX idx_level (level),
    INDEX idx_job_id (job_id),
    INDEX idx_faction_id (faction_id),
    INDEX idx_updated_at (updated_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- SESSIONS TABLE (Para rastrear logins)
-- ============================================================
CREATE TABLE IF NOT EXISTS `sessions` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `account_id` INT NOT NULL,
    `character_id` INT NOT NULL,
    `player_id` INT DEFAULT 0,
    `ip_address` VARCHAR(16),
    `gpci` VARCHAR(41),
    `login_time` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `logout_time` DATETIME NULL,
    `playtime_seconds` INT DEFAULT 0,
    
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    INDEX idx_account_id (account_id),
    INDEX idx_character_id (character_id),
    INDEX idx_login_time (login_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- ECONOMY: BANK ACCOUNTS
-- ============================================================
CREATE TABLE IF NOT EXISTS `bank_accounts` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `character_id` INT NOT NULL UNIQUE,
    `balance` INT DEFAULT 0,
    `account_number` VARCHAR(20) UNIQUE,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    INDEX idx_account_number (account_number)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- ECONOMY: TRANSACTIONS LOG
-- ============================================================
CREATE TABLE IF NOT EXISTS `transactions` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `character_id` INT NOT NULL,
    `type` VARCHAR(32),
    `amount` INT NOT NULL,
    `description` VARCHAR(128),
    `timestamp` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    INDEX idx_character_id (character_id),
    INDEX idx_type (type),
    INDEX idx_timestamp (timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- JOBS
-- ============================================================
CREATE TABLE IF NOT EXISTS `jobs` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `name` VARCHAR(32) NOT NULL,
    `description` VARCHAR(128),
    `salary_per_shift` INT DEFAULT 0,
    `shift_duration_minutes` INT DEFAULT 60,
    `spawn_x` FLOAT,
    `spawn_y` FLOAT,
    `spawn_z` FLOAT,
    `spawn_a` FLOAT,
    
    UNIQUE KEY (name),
    INDEX idx_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- JOB SHIFTS (Turnos trabalhados)
-- ============================================================
CREATE TABLE IF NOT EXISTS `job_shifts` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `character_id` INT NOT NULL,
    `job_id` INT NOT NULL,
    `start_time` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `end_time` DATETIME NULL,
    `payment_received` INT DEFAULT 0,
    
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE,
    INDEX idx_character_id (character_id),
    INDEX idx_job_id (job_id),
    INDEX idx_start_time (start_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- FACTIONS
-- ============================================================
CREATE TABLE IF NOT EXISTS `factions` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `name` VARCHAR(32) NOT NULL UNIQUE,
    `description` VARCHAR(256),
    `type` VARCHAR(32),
    `color` VARCHAR(8),
    `treasury` INT DEFAULT 0,
    `leader_id` INT,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_name (name),
    INDEX idx_leader_id (leader_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- FACTION MEMBERS
-- ============================================================
CREATE TABLE IF NOT EXISTS `faction_members` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `faction_id` INT NOT NULL,
    `character_id` INT NOT NULL,
    `rank` INT DEFAULT 0,
    `rank_name` VARCHAR(32),
    `joined_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE KEY (faction_id, character_id),
    FOREIGN KEY (faction_id) REFERENCES factions(id) ON DELETE CASCADE,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    INDEX idx_faction_id (faction_id),
    INDEX idx_character_id (character_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- VEHICLES
-- ============================================================
CREATE TABLE IF NOT EXISTS `vehicles` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `model_id` INT NOT NULL,
    `owner_id` INT,
    `color1` INT DEFAULT 0,
    `color2` INT DEFAULT 0,
    `fuel` FLOAT DEFAULT 100.0,
    `health` FLOAT DEFAULT 1000.0,
    `mileage` INT DEFAULT 0,
    `spawn_x` FLOAT,
    `spawn_y` FLOAT,
    `spawn_z` FLOAT,
    `spawn_a` FLOAT,
    `plate` VARCHAR(8),
    `locked` INT DEFAULT 0,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (owner_id) REFERENCES characters(id) ON DELETE SET NULL,
    INDEX idx_owner_id (owner_id),
    INDEX idx_model_id (model_id),
    INDEX idx_plate (plate)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- PROPERTIES
-- ============================================================
CREATE TABLE IF NOT EXISTS `properties` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `owner_id` INT,
    `address` VARCHAR(64),
    `type` VARCHAR(32),
    `price` INT DEFAULT 0,
    `furniture_count` INT DEFAULT 0,
    `safe_money` INT DEFAULT 0,
    `entrance_x` FLOAT,
    `entrance_y` FLOAT,
    `entrance_z` FLOAT,
    `entrance_a` FLOAT,
    `interior_id` INT DEFAULT 0,
    `world` INT DEFAULT 0,
    `locked` INT DEFAULT 1,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (owner_id) REFERENCES characters(id) ON DELETE SET NULL,
    INDEX idx_owner_id (owner_id),
    INDEX idx_type (type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- BUSINESSES
-- ============================================================
CREATE TABLE IF NOT EXISTS `businesses` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `name` VARCHAR(32) NOT NULL,
    `owner_id` INT,
    `type` VARCHAR(32),
    `location_x` FLOAT,
    `location_y` FLOAT,
    `location_z` FLOAT,
    `treasury` INT DEFAULT 0,
    `level` INT DEFAULT 1,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE KEY (name),
    FOREIGN KEY (owner_id) REFERENCES characters(id) ON DELETE SET NULL,
    INDEX idx_owner_id (owner_id),
    INDEX idx_type (type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- BUSINESS EMPLOYEES
-- ============================================================
CREATE TABLE IF NOT EXISTS `business_employees` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `business_id` INT NOT NULL,
    `character_id` INT NOT NULL,
    `salary` INT DEFAULT 0,
    `position` VARCHAR(32),
    `joined_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE KEY (business_id, character_id),
    FOREIGN KEY (business_id) REFERENCES businesses(id) ON DELETE CASCADE,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    INDEX idx_business_id (business_id),
    INDEX idx_character_id (character_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- INVENTORY ITEMS
-- ============================================================
CREATE TABLE IF NOT EXISTS `inventory_items` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `item_name` VARCHAR(32) NOT NULL UNIQUE,
    `item_model` INT NOT NULL,
    `item_type` INT DEFAULT 0,
    `description` VARCHAR(128),
    `weight` FLOAT DEFAULT 1.0,
    
    INDEX idx_item_name (item_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- PLAYER INVENTORY
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_inventory` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `character_id` INT NOT NULL,
    `item_id` INT NOT NULL,
    `quantity` INT DEFAULT 1,
    `added_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE KEY (character_id, item_id),
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    FOREIGN KEY (item_id) REFERENCES inventory_items(id) ON DELETE CASCADE,
    INDEX idx_character_id (character_id),
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- BANS
-- ============================================================
CREATE TABLE IF NOT EXISTS `bans` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `account_id` INT,
    `character_id` INT,
    `ip_address` VARCHAR(16),
    `reason` VARCHAR(128),
    `banned_by` INT,
    `ban_time` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `unban_time` DATETIME NULL,
    `permanent` INT DEFAULT 0,
    
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    FOREIGN KEY (banned_by) REFERENCES accounts(id) ON DELETE SET NULL,
    INDEX idx_account_id (account_id),
    INDEX idx_ip_address (ip_address),
    INDEX idx_permanent (permanent)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- WARNINGS/INFRACTIONS
-- ============================================================
CREATE TABLE IF NOT EXISTS `warnings` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `character_id` INT NOT NULL,
    `reason` VARCHAR(128),
    `warned_by` INT,
    `warning_level` INT DEFAULT 1,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    `expires_at` DATETIME NULL,
    
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
    FOREIGN KEY (warned_by) REFERENCES accounts(id) ON DELETE SET NULL,
    INDEX idx_character_id (character_id),
    INDEX idx_warning_level (warning_level)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- ADMIN LOGS
-- ============================================================
CREATE TABLE IF NOT EXISTS `admin_logs` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `admin_id` INT NOT NULL,
    `action` VARCHAR(64),
    `target_id` INT,
    `details` VARCHAR(256),
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (admin_id) REFERENCES accounts(id) ON DELETE CASCADE,
    INDEX idx_admin_id (admin_id),
    INDEX idx_action (action),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================
-- DEFAULT JOBS
-- ============================================================
INSERT INTO `jobs` (name, description, salary_per_shift, spawn_x, spawn_y, spawn_z, spawn_a) VALUES
    ('Trucker', 'Trabalhar como caminhoneiro', 5000, 1545.5, -1675.7, 13.5, 90.0),
    ('Mechanic', 'Trabalhar como mecânico', 4500, 1545.5, -1675.7, 13.5, 90.0),
    ('Taxi Driver', 'Trabalhar como taxista', 3000, 1545.5, -1675.7, 13.5, 90.0),
    ('Police', 'Trabalhar na polícia', 6000, 1545.5, -1675.7, 13.5, 90.0),
    ('EMT', 'Trabalhar como paramédico', 5500, 1545.5, -1675.7, 13.5, 90.0);

-- ============================================================
-- DEFAULT FACTIONS
-- ============================================================
INSERT INTO `factions` (name, type, description, color, treasury) VALUES
    ('LSPD', 'Law Enforcement', 'Los Santos Police Department', '#0000FF', 100000),
    ('SAFD', 'Emergency Services', 'San Andreas Fire Department', '#FF0000', 50000),
    ('Families', 'Gang', 'Gang da região', '#00FF00', 25000),
    ('Ballas', 'Gang', 'Gang da região', '#9900FF', 25000);

-- ============================================================
-- DEFAULT INVENTORY ITEMS
-- ============================================================
INSERT INTO `inventory_items` (item_name, item_model, item_type, description, weight) VALUES
    ('Wallet', 1575, 1, 'Carteira com documentos', 0.5),
    ('Phone', 18631, 1, 'Telefone celular', 1.0),
    ('Keys', 325, 1, 'Chaves de carro', 0.1),
    ('License', 2702, 1, 'Carteira de motorista', 0.1);

-- ============================================================
-- VIEWS para queries mais fáceis
-- ============================================================

-- View: Account Login Info
CREATE OR REPLACE VIEW vw_account_info AS
SELECT 
    a.id,
    a.username,
    a.email,
    a.admin_level,
    COUNT(DISTINCT c.id) as character_count,
    a.last_login,
    a.created_at
FROM accounts a
LEFT JOIN characters c ON a.id = c.account_id
GROUP BY a.id;

-- View: Character Full Info
CREATE OR REPLACE VIEW vw_character_info AS
SELECT 
    c.id,
    c.name,
    c.sex,
    c.age,
    c.level,
    c.money,
    c.bank,
    c.health,
    c.armour,
    c.job_id,
    c.faction_id,
    a.username,
    a.admin_level,
    c.updated_at
FROM characters c
LEFT JOIN accounts a ON c.account_id = a.id;

-- ============================================================
-- INDEXES para Performance
-- ============================================================
CREATE INDEX idx_account_password ON accounts(username, password);
CREATE INDEX idx_character_account ON characters(account_id, updated_at);
CREATE INDEX idx_vehicle_owner ON vehicles(owner_id, created_at);
CREATE INDEX idx_property_owner ON properties(owner_id, created_at);
CREATE INDEX idx_business_owner ON businesses(owner_id, created_at);

-- ============================================================
-- Database Setup Complete
-- ============================================================
COMMIT;
