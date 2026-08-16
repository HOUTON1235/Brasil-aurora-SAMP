# Santa Aurora — Roleplay Brasileiro

Servidor SA-MP Roleplay brasileiro. Cidade fictícia **Santa Aurora**.

---

## Status das Fases

| Fase | Nome | Status |
|------|------|--------|
| 0  | Fundação (core, db, auth, admin) | ✅ |
| 1  | Jogador (HUD, docs, inventário, tutorial) | ✅ |
| 2  | Economia (banco, PIX, empregos, lojas, multas) | ✅ |
| 3  | Mundo (veículos, concessionária, combustível, propriedades, garagem) | ✅ |
| 4  | Empresas (funcionários, estoque, caixa, salários) | ✅ |
| 5  | Comunicação (celular, SMS, ligações, anúncios) | ✅ |
| 6  | Serviços Públicos (polícia, hospital, governo, trânsito) | ✅ |
| 7  | Crime e Organizações (facções, cargos, territórios) | ✅ |
| 8  | Conteúdo (missões, conquistas, rankings, clima, dia/noite) | ✅ |
| 9  | Segurança (anti-cheat, anti-flood, detecção) | ✅ |
| 10 | Otimização (performance monitor, profiling) | ✅ |

---

## Estatísticas do Projeto

- **52 arquivos** de código (`.pwn`, `.inc`, `.sql`)
- **~17.500 linhas** de código Pawn
- **9 migrações SQL** versionadas
- **200+ comandos** implementados

---

## Tecnologias

| Componente | Tecnologia |
|-----------|-----------|
| Linguagem | Pawn |
| Servidor | SA-MP 0.3.7-R2 |
| Banco | MySQL/MariaDB |
| Plugin MySQL | BlueG R41-4 / pBlueG |
| Streaming | Streamer Plugin |
| Comandos | ZCMD |
| Parsing | sscanf2 |
| Debug | CrashDetect |

---

## Setup Completo

### 1. Baixar SA-MP Server

https://sa-mp.mp/downloads/ → SA-MP 0.3.7 Server

Extrair tudo na pasta raiz (`Servidor 2/`).

### 2. Baixar Plugins

| Plugin | URL | Arquivo |
|--------|-----|---------|
| MySQL R41-4 | https://github.com/pBlueG/SA-MP-MySQL/releases | `mysql.dll` |
| Streamer | https://github.com/samp-incognito/samp-streamer-plugin/releases | `streamer.dll` |
| sscanf2 | https://github.com/Y-Less/sscanf/releases | `sscanf.dll` |
| CrashDetect | https://github.com/nicholasdgoodman/samp-plugin-crashdetect/releases | `crashdetect.dll` |
| ZCMD | https://github.com/Gamer080/zcmd (só `.inc`) | — |

**Colocar `.dll` em** `plugins/`
**Colocar `.inc` em** `pawno/include/`

### 3. Configurar MySQL

```sql
CREATE DATABASE santa_aurora CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'samp'@'localhost' IDENTIFIED BY 'samp_password';
GRANT ALL PRIVILEGES ON santa_aurora.* TO 'samp'@'localhost';
FLUSH PRIVILEGES;
```

### 4. Executar Migrações (na ordem)

```bash
mysql -u samp -p santa_aurora < migrations/001_initial.sql
mysql -u samp -p santa_aurora < migrations/002_phase1_player.sql
mysql -u samp -p santa_aurora < migrations/003_phase2_economy.sql
mysql -u samp -p santa_aurora < migrations/004_phase3_world.sql
mysql -u samp -p santa_aurora < migrations/005_phase4_businesses.sql
mysql -u samp -p santa_aurora < migrations/006_phase5_communication.sql
mysql -u samp -p santa_aurora < migrations/007_phase6_public_services.sql
mysql -u samp -p santa_aurora < migrations/008_phase7_factions.sql
mysql -u samp -p santa_aurora < migrations/009_phase8_content.sql
```

### 5. Configurar Conexão com Banco

Criar/editar `scriptfiles/db_config.ini`:
```ini
[database]
hostname=127.0.0.1
username=samp
password=samp_password
database=santa_aurora
port=3306
charset=utf8mb4
```

### 6. server.cfg (raiz)

```
echo Executing Server Config...
lanmode 0
rcon_password TROQUE_ESTA_SENHA
maxplayers 200
port 7777
hostname Santa Aurora - Roleplay Brasileiro
gamemode0 rp 1
announce 0
query 1
plugins mysql streamer sscanf crashdetect
```

> **Nota:** ZCMD é um `.inc` apenas (não é `.dll`), não precisa estar em `plugins`.

### 7. Compilar

Abrir **Pawno** (`pawno/pawno.exe`), carregar `gamemodes/rp.pwn`, compilar.

Ou via linha de comando:
```
pawno\pawncc.exe gamemodes\rp.pwn -i pawno\include -o gamemodes\rp.amf
```

### 8. Iniciar

```
samp-server.exe
```

---

## Primeiro Login

1. Conectar no servidor (IP:7777)
2. Dialog de login aparece
3. Se for primeira vez → clica "Registrar"
4. Criar conta → fluxo de criação de personagem → tutorial

### Criar Admin

Para definir admin no primeiro acesso, execute no MySQL:
```sql
UPDATE accounts SET admin_level=7 WHERE username='SeuNome';
```

---

## Arquitetura de Módulos

```
gamemodes/rp.pwn                ← orquestrador central
includes/
  core/           defines, macros, states, events, permissions, scheduler
  database/       conexão MySQL, query helpers
  players/        auth, data, hud, hunger, documents, character, tutorial
  inventory/      inventário, itens, missões/conquistas
  economy/        banco, lojas, multas
  jobs/           empregos, progressão, salário
  vehicles/       veículos, concessionária, combustível
  houses/         propriedades, garagem
  businesses/     empresas, funcionários, estoque
  phone/          celular, SMS, ligações, anúncios
  police/         sistema policial completo
  hospital/       SAMU, revive, tratamento
  government/     governo, trânsito, orçamento
  factions/       facções, cargos, territórios
  admin/          painel admin
  ui/             componentes de interface
  logs/           logger por módulo
  utilities/      utils, world, anti-cheat, performance
```

---

## Comandos Principais

### Jogador
`/status` `/documentos` `/inventario` `/banco` `/celular` `/emprego` `/missoes` `/conquistas`

### Veículos / Propriedades
`/veiculo` `/trancar` `/abastecer` `/concessionaria` `/casa` `/imoveis` `/garagem`

### RP / Social
`/me` `/do` `/b` `/pm` `/report` `/radio` `/faccao` `/territorios`

### Serviços Públicos
`/hospital` `/samu` `/prefeitura` `/detran` `/abordar` `/prender`

### Admin
`/atp` `/akick` `/aban` `/agm` `/ainfo` `/serverstatus` `/healthcheck` `/acstatus`

---

## Logs

```
scriptfiles/logs/
  system/     inicialização, erros críticos
  login/      logins, registros, tentativas
  economy/    transações financeiras
  admin/      ações administrativas
  security/   detecções do anti-cheat
  punishments/ punições
  chat/       logs de chat e RP
  police/     ações policiais
  vehicles/   logs de veículos
```

---

## Segurança

- Senhas com **Whirlpool** (64 chars)
- Proteção brute force (5 tentativas → kick automático)
- Anti-cheat: velocidade, teleporte, dinheiro, armas ilegais
- Anti-flood: comandos e chat com rate limiter
- Dirty flags para evitar gravações desnecessárias
- Transações atômicas com referência única (anti-duplicação)
- Escape SQL em todas as queries

---

## Banco de Dados — Tabelas

| Tabela | Descrição |
|--------|-----------|
| accounts | Contas dos jogadores |
| characters | Personagens |
| character_stats | Estatísticas |
| character_documents | Documentos (RG, CPF, CNH) |
| inventory | Inventário |
| item_definitions | Catálogo de itens |
| vehicles | Veículos |
| vehicle_financing | Financiamentos |
| properties | Propriedades |
| property_permissions | Permissões de acesso |
| businesses | Empresas |
| business_employees | Funcionários |
| business_inventory | Estoque de empresa |
| bank_accounts | Contas bancárias |
| bank_transactions | Extrato bancário |
| pix_keys | Chaves PIX |
| loans | Empréstimos |
| jobs | Empregos |
| character_jobs | Empregos dos personagens |
| factions | Facções |
| faction_ranks | Cargos |
| faction_members | Membros |
| territories | Territórios |
| criminal_records | Antecedentes criminais |
| wanted | Procurados |
| jail_records | Registros de prisão |
| crime_catalog | Catálogo de crimes |
| medical_records | Prontuários |
| medical_calls | Chamados SAMU |
| government_budget | Orçamento |
| phones | Celulares |
| messages | SMS |
| calls | Ligações |
| announcements | Anúncios |
| mission_definitions | Definições de missões |
| character_missions | Missões dos personagens |
| achievement_definitions | Conquistas |
| character_achievements | Conquistas desbloqueadas |
| fines | Multas |
| punishments | Punições |
| admin_logs | Logs admin |
| security_logs | Logs de segurança |
| server_config | Configurações |
| schema_migrations | Controle de versão SQL |

---

## Notas de Produção

- Altere `rcon_password` no `server.cfg`
- Configure backups automáticos do MySQL
- Monitore `scriptfiles/logs/errors/` diariamente
- Use `/healthcheck` para ver estado em tempo real
- O anti-cheat usa kick automático, não ban — bans são manuais para evitar falsos positivos
