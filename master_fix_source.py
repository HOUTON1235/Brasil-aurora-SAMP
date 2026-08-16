import os
import re
import unicodedata

BASE = r'C:\Users\Felip\Downloads\Servidor 2'

FILE_MAP = [
    ("includes/admin/admin_core.inc",           "pawno/include/admin_core.inc"),
    ("includes/utilities/anticheat_core.inc",   "pawno/include/anticheat_core.inc"),
    ("includes/economy/bank_core.inc",          "pawno/include/bank_core.inc"),
    ("includes/businesses/business_core.inc",   "pawno/include/business_core.inc"),
    ("includes/database/db_core.inc",           "pawno/include/db_core.inc"),
    ("includes/database/db_queries.inc",        "pawno/include/db_queries.inc"),
    ("includes/core/defines.inc",               "pawno/include/defines.inc"),
    ("includes/core/events.inc",                "pawno/include/events.inc"),
    ("includes/factions/faction_core.inc",      "pawno/include/faction_core.inc"),
    ("includes/economy/fines_core.inc",         "pawno/include/fines_core.inc"),
    ("includes/houses/garage_core.inc",         "pawno/include/garage_core.inc"),
    ("includes/government/government_core.inc", "pawno/include/government_core.inc"),
    ("includes/hospital/hospital_core.inc",     "pawno/include/hospital_core.inc"),
    ("includes/inventory/inventory_core.inc",   "pawno/include/inventory_core.inc"),
    ("includes/jobs/jobs_core.inc",             "pawno/include/jobs_core.inc"),
    ("includes/logs/logger.inc",                "pawno/include/logger.inc"),
    ("includes/core/macros.inc",                "pawno/include/macros.inc"),
    ("includes/inventory/missions_core.inc",    "pawno/include/missions_core.inc"),
    ("includes/utilities/performance_core.inc", "pawno/include/performance_core.inc"),
    ("includes/core/permissions.inc",           "pawno/include/permissions.inc"),
    ("includes/phone/phone_core.inc",           "pawno/include/phone_core.inc"),
    ("includes/players/player_auth.inc",        "pawno/include/player_auth.inc"),
    ("includes/players/player_character.inc",   "pawno/include/player_character.inc"),
    ("includes/players/player_core.inc",        "pawno/include/player_core.inc"),
    ("includes/players/player_data.inc",        "pawno/include/player_data.inc"),
    ("includes/players/player_documents.inc",   "pawno/include/player_documents.inc"),
    ("includes/players/player_hud.inc",         "pawno/include/player_hud.inc"),
    ("includes/players/player_hunger.inc",      "pawno/include/player_hunger.inc"),
    ("includes/players/player_tutorial.inc",    "pawno/include/player_tutorial.inc"),
    ("includes/police/police_core.inc",         "pawno/include/police_core.inc"),
    ("includes/houses/property_core.inc",       "pawno/include/property_core.inc"),
    ("includes/core/scheduler.inc",             "pawno/include/scheduler.inc"),
    ("includes/core/states.inc",                "pawno/include/states.inc"),
    ("includes/economy/stores_core.inc",        "pawno/include/stores_core.inc"),
    ("includes/factions/territory_core.inc",    "pawno/include/territory_core.inc"),
    ("includes/government/traffic_core.inc",    "pawno/include/traffic_core.inc"),
    ("includes/ui/ui_core.inc",                 "pawno/include/ui_core.inc"),
    ("includes/utilities/utils.inc",            "pawno/include/utils.inc"),
    ("includes/vehicles/vehicle_core.inc",      "pawno/include/vehicle_core.inc"),
    ("includes/vehicles/vehicle_dealership.inc","pawno/include/vehicle_dealership.inc"),
    ("includes/vehicles/vehicle_fuel.inc",      "pawno/include/vehicle_fuel.inc"),
    ("includes/utilities/world_core.inc",       "pawno/include/world_core.inc"),
]


def to_ascii(text):
    """Convert UTF-8 non-ASCII chars to ASCII via NFD normalize + ignore."""
    normalized = unicodedata.normalize("NFD", text)
    return normalized.encode("ascii", "ignore").decode("ascii")


def read_file_raw(path):
    """Read file, strip BOM, decode UTF-8 or latin-1."""
    with open(path, "rb") as f:
        raw = f.read()
    if raw.startswith(b"\xef\xbb\xbf"):
        raw = raw[3:]
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError:
        return raw.decode("latin-1")


def apply_common_transforms(content):
    # 1. Normalize line endings
    content = content.replace("\r\n", "\n").replace("\r", "\n")

    # 2. Convert non-ASCII chars to ASCII
    content = to_ascii(content)

    # 3. Remove relative #include "..." lines
    content = re.sub(r'^#include\s+"[^"]*"\s*$',
                     '// removed relative include', content, flags=re.MULTILINE)

    # 4. stock bool: -> stock
    content = re.sub(r'\bstock\s+bool:', 'stock ', content)

    # 5. Remove const from array params: const param[] -> param[]
    content = re.sub(r'\bconst\s+(\w+\s*\[\])', r'\1', content)

    # 6. Remove E_PLAYER_STATE: tag
    content = re.sub(r'\bE_PLAYER_STATE:', '', content)

    # 7. Remove bool: from params with default value
    content = re.sub(r'\bbool:(\w+\s*=\s*(?:true|false))', r'\1', content)

    # 8. strequal( -> !strcmp(
    content = re.sub(r'\bstrequal\(', '!strcmp(', content)

    # 9. Remove entire lines containing gpci(
    content = re.sub(r'^[^\n]*\bgpci\([^\n]*\n?', '', content, flags=re.MULTILINE)

    # 10. Replace ___(4) with empty string
    content = content.replace('___(4)', '')

    # 11. mysql_error(g_DBHandle, ... -> mysql_error(
    content = re.sub(r'\bmysql_error\s*\(\s*g_DBHandle\s*,\s*', 'mysql_error(', content)

    # 12. forward bool: -> forward
    content = re.sub(r'\bforward\s+bool:', 'forward ', content)

    return content


def fix_states_inc(content):
    """Replace stock State_GetName(...){...} with a #define macro."""
    start_marker = 'stock State_GetName('
    idx = content.find(start_marker)
    if idx != -1:
        brace_count = 0
        found_open = False
        end_idx = idx
        for i in range(idx, len(content)):
            if content[i] == '{':
                brace_count += 1
                found_open = True
            elif content[i] == '}':
                brace_count -= 1
                if found_open and brace_count == 0:
                    end_idx = i + 1
                    break
        macro = '#define State_GetName(%0,%1,%2) format(%1,%2,"Estado_%0")'
        content = content[:idx] + macro + '\n' + content[end_idx:]
    return content


def fix_db_queries_inc(content):
    """Keep only the include guard header, remove all function bodies."""
    lines = content.split('\n')
    guard_lines = []
    in_guard = False
    for line in lines:
        stripped = line.strip()
        if stripped.startswith('#if defined _SA_DB_QUERIES_INC'):
            in_guard = True
            guard_lines.append(line)
        elif in_guard and stripped.startswith('#define _SA_DB_QUERIES_INC'):
            guard_lines.append(line)
            guard_lines.append('')
            guard_lines.append('// db_queries.inc: functions moved to db_core.inc')
            guard_lines.append('')
            break
    if not guard_lines:
        return '// db_queries.inc: emptied - functions in db_core.inc\n'
    return '\n'.join(guard_lines) + '\n'


def fix_permissions_inc(content):
    """Ensure bool: stripped from forward declarations."""
    content = re.sub(r'forward\s+bool:Player_HasPermission',
                     'forward Player_HasPermission', content)
    content = re.sub(r'forward\s+bool:Player_IsAdmin',
                     'forward Player_IsAdmin', content)
    return content


def fix_player_core_inc(content):
    """Fix _:state -> state in SetPVarInt calls."""
    content = re.sub(r'\b_:state\b', 'state', content)
    return content


def write_ascii(path, content):
    """Write content as plain ASCII, no BOM, Unix line endings."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='ascii', errors='replace', newline='\n') as f:
        f.write(content)


def process_file(src_rel, dst_rel, src_basename):
    src_path = os.path.join(BASE, src_rel.replace('/', os.sep))
    dst_path = os.path.join(BASE, dst_rel.replace('/', os.sep))

    if not os.path.exists(src_path):
        print("  SKIP (not found): " + src_rel)
        return

    content = read_file_raw(src_path)
    content = apply_common_transforms(content)

    if src_basename == 'states.inc':
        content = fix_states_inc(content)
    elif src_basename == 'db_queries.inc':
        content = fix_db_queries_inc(content)
    elif src_basename == 'permissions.inc':
        content = fix_permissions_inc(content)
    elif src_basename == 'player_core.inc':
        content = fix_player_core_inc(content)

    write_ascii(dst_path, content)
    print("  OK: " + src_rel + " -> " + dst_rel)


def process_rp_pwn():
    src = os.path.join(BASE, 'gamemodes', 'rp.pwn')
    if not os.path.exists(src):
        print("  SKIP: gamemodes/rp.pwn not found")
        return
    content = read_file_raw(src)
    content = content.replace('\r\n', '\n').replace('\r', '\n')
    content = to_ascii(content)
    write_ascii(src, content)
    print("  OK: gamemodes/rp.pwn (BOM removed, ASCII)")


def main():
    print("=" * 60)
    print("master_fix.py - Santa Aurora RP Include Fixer")
    print("=" * 60)
    for src_rel, dst_rel in FILE_MAP:
        src_basename = os.path.basename(src_rel)
        process_file(src_rel, dst_rel, src_basename)
    print("")
    print("Processing gamemodes/rp.pwn ...")
    process_rp_pwn()
    print("")
    print("Done.")


if __name__ == '__main__':
    main()
