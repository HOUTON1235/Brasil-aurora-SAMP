# 🚀 SETUP GUIA COMPLETO - Santa Aurora RP

**Tempo estimado: 30-45 minutos**

---

## ✅ PASSO 1: Instalar SA-MP Server (5 min)

### Windows
1. Baixe: https://www.sa-mp.com/download.php
2. Extraia em: `C:\samp\`
3. Abra `server.cfg` e configure:

```ini
# server.cfg
hostname Santa Aurora RP
gamemode0 rp
port 7777
maxplayers 200
language PT-BR
```

### Linux
```bash
cd /opt/samp
wget https://www.sa-mp.com/download.php?version=linux
unzip samp03z-svn5325-linux.zip
chmod +x samp-npc samp-server
```

---

## ✅ PASSO 2: Instalar MySQL (10 min)

### Windows
1. Baixe MySQL: https://dev.mysql.com/downloads/mysql/
2. Execute o installer
3. Configure:
   - Root password: `sua_senha_segura`
   - Port: `3306`

### Linux
```bash
sudo apt-get install mysql-server
sudo mysql_secure_installation
```

### macOS
```bash
brew install mysql@8.0
brew services start mysql@8.0
mysql_secure_installation
```

---

## ✅ PASSO 3: Criar Banco de Dados (5 min)

Abra MySQL CLI:
```bash
mysql -u root -p
```

Cole este SQL completo (veja arquivo `database_schema.sql` abaixo)

---

## ✅ PASSO 4: Instalar Pawn Compiler (5 min)

### Windows
1. Baixe: https://github.com/pawn-lang/compiler/releases
2. Extraia em: `C:\pawn-compiler\`
3. Adicione ao PATH (Windows)

### Linux/macOS
```bash
wget https://github.com/pawn-lang/compiler/releases/download/v3.10.10/pawncc
chmod +x pawncc
sudo mv pawncc /usr/local/bin/
```

Teste:
```bash
pawncc -v
# Output: Pawn compiler 3.10.10
```

---

## ✅ PASSO 5: Plugins SA-MP (5 min)

Copie para `samp/plugins/`:
- `mysql.so` (Linux) ou `mysql.dll` (Windows)
- `sscanf.so` ou `sscanf.dll`
- `streamer.so` ou `streamer.dll`

Download: https://github.com/pbluesky/SA-MP-MySQL

---

## ✅ PASSO 6: Compilar Gamemode (5 min)

```bash
cd Brasil-aurora-SAMP
pawncc gamemodes/rp.pwn -o gamemodes/rp.amx
```

**Output esperado:**
```
Pawn compiler 3.10.10
Header size:      600 bytes
Code size:        250000 bytes
Data size:        100000 bytes
Stack/heap size:  32000 bytes
Total size:       382600 bytes
0 Errors. 0 Warnings.
```

---

## ✅ PASSO 7: Configurar sa-mp para MySQL (5 min)

Crie `scriptfiles/db_config.ini`:
```ini
[MySQL]
host=localhost
user=samp_user
password=senha123
database=samp_aurora
port=3306
```

---

## ✅ PASSO 8: Iniciar Servidor (2 min)

### Windows
```bash
cd C:\samp\
samp-server.exe
```

### Linux
```bash
cd /opt/samp
./samp-server
```

**Esperado:**
```
SA-MP Server: 0.3.7
Started server on port 7777
Loading gamemode 'rp.amx'...
Database connected successfully
Server online!
```

---

## ✅ PASSO 9: Conectar no Jogo (2 min)

1. Abra SA-MP Client
2. Add Server: `127.0.0.1:7777`
3. Connect
4. Vá para "Register" (criar conta)
5. Depois "Login"

---

## 🐛 TROUBLESHOOTING

### "Cannot connect to server"
- Firewall bloqueando porta 7777?
- `netstat -an | grep 7777` (Linux/macOS)
- `netstat -an | findstr 7777` (Windows)

### "Database connection failed"
- MySQL running? `mysql -u root -p` (teste)
- Credenciais corretas em `db_config.ini`?
- Database `samp_aurora` existe?

### "Compile errors"
- Pawncc instalado? `pawncc -v`
- Includes faltando? Verifique `#include` paths

### "Gamemode won't load"
- `.amx` file corruption? Recompile
- Plugin crash? Verifique logs `samp-server-log.txt`

---

## 📦 Arquivos Necessários

```
Brasil-aurora-SAMP/
├── gamemodes/
│   └── rp.pwn              ✅ Main gamemode
├── includes/
│   ├── core/               ✅ Core includes
│   └── players/            ✅ Player system
├── scriptfiles/
│   ├── db_config.ini       ⚠️ CRIAR
│   └── logs/               ⚠️ CRIAR (pasta)
├── database_schema.sql     ✅ SQL schema
└── SETUP_GUIDE.md         ✅ Este arquivo
```

---

## 🎮 Primeiros Testes

1. **Login funciona?**
   - Registre conta
   - Faça login
   - Spawn no mapa

2. **Dinheiro funciona?**
   - `/money` - mostra saldo
   - `/bank` - mostra banco

3. **Admin funciona?**
   - `/admin` - abre menu admin
   - `/givemoney <id> <amount>` - dar dinheiro

---

## ✅ Checklist Final

- [ ] SA-MP Server instalado
- [ ] MySQL rodando
- [ ] Banco de dados criado
- [ ] Pawn compiler funciona
- [ ] `rp.pwn` compilado sem erros
- [ ] `db_config.ini` configurado
- [ ] Servidor inicia sem crashes
- [ ] Cliente conecta com sucesso
- [ ] Login/Register funciona
- [ ] Spawnpoint aparece
- [ ] Dinheiro mostra corretamente

---

**Próximo Passo**: Se tudo ok, execute os testes da Fase 1!

Dúvidas? Verifique `database_schema.sql` e `includes/core/defines.inc`
