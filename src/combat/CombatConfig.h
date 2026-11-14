#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include "core/Common.h"

// =========================================================================
// ТАБЛИЦА БАЛАНСА БОЕВОЙ СИСТЕМЫ И ПАРАМЕТРОВ КОМБО (КВ ТАБЛИЦА / KEY-VALUES)
// Здесь собраны все значения урона, форсы отталкивания, стан-лока игрока
// (playerRecovery), стан-лока врага (enemyStun) и штрафного стана (whiffStun).
// Таблица загружается из внешнего файла assets/config/combat_balance.kv
// и поддерживает мгновенную горячую перезагрузку в реальном времени ([F5]).
// =========================================================================

struct AttackStats {
    float damage = 2.0f;          // Урон удара
    float knockX = 160.0f;        // Горизонтальный импульс (форса по X)
    float knockY = -30.0f;        // Вертикальный импульс (форса по Y)
    float playerRecovery = 0.14f; // Стан-лок / время восстановления персонажа при попадании (cooldown)
    float enemyStun = 0.20f;      // Стан-лок противника (hurtTimer)
    float activeTime = 0.12f;     // Длительность активной фазы хитбокса
    float lunge = 14.0f;          // Смещение игрока вперед при атаке
    std::string displayName;      // Отображаемое имя приема
    sf::Color color = sf::Color::White; // Цвет спецэффектов и вспышек
    float whiffStun = 0.0f;       // Длинный стан-лок персонажа при промахе по врагу (если 0, рассчитывается автоматически)
};

struct ComboEntry {
    std::string id;                         // Уникальный идентификатор
    std::vector<std::string> sequence;      // Цепочка ввода (например, {"1", "f2", "d1"})
    AttackStats stats;                      // Параметры завершающего удара комбо
    bool isUniqueFinisher = false;          // Является ли уникальной вариацией
    bool isFinisher = true;                 // Завершающий удар (false для базовых промежуточных комбо)
};

class CombatConfig {
public:
    // =====================================================================
    // ПУБЛИЧНЫЕ МЕТОДЫ ДОСТУПА К КВ ТАБЛИЦЕ (KEY-VALUE GETTERS)
    // =====================================================================

    // Получение параметров базовой атаки по ключу ("1", "f2", "dw1", "d1", "a1" и т.д.)
    static AttackStats getBaseAttack(const std::string& type) {
        ensureInitialized();
        auto it = _attacks.find(type);
        if (it != _attacks.end()) {
            return it->second;
        }

        // Поддержка алиасов направлений
        if (type == "df1") return getBaseAttack("d1");
        if (type == "df2") return getBaseAttack("d2");
        if (type == "af1") return getBaseAttack("a1");
        if (type == "af2") return getBaseAttack("a2");
        if (type == "f1")  return getBaseAttack("1");

        // Безопасное значение по умолчанию
        return {2.0f, 160.0f, -25.0f, 0.14f, 0.20f, 0.12f, 14.0f, 
                "ATTACK: " + type, sf::Color::White, 0.38f};
    }

    // Синоним getBaseAttack для чистоты KV API
    static AttackStats getAttack(const std::string& id) {
        return getBaseAttack(id);
    }

    // Проверка наличия атаки в КВ реестре
    static bool hasAttack(const std::string& id) {
        ensureInitialized();
        return _attacks.find(id) != _attacks.end();
    }

    // Полный реестр цепочек комбо (уникальные финишеры идут первыми)
    static const std::vector<ComboEntry>& getCombos() {
        ensureInitialized();
        return _combos;
    }

    // Поиск конкретного комбо по его уникальному ID ("1f2d1", "11b2", "1f2" и т.д.)
    static const ComboEntry* getCombo(const std::string& id) {
        ensureInitialized();
        auto it = _comboIndexMap.find(id);
        if (it != _comboIndexMap.end() && it->second < _combos.size()) {
            return &_combos[it->second];
        }
        return nullptr;
    }

    // Проверка наличия комбо в КВ реестре
    static bool hasCombo(const std::string& id) {
        ensureInitialized();
        return _comboIndexMap.find(id) != _comboIndexMap.end();
    }

    // Получение всей таблицы атак
    static const std::unordered_map<std::string, AttackStats>& getAllAttacks() {
        ensureInitialized();
        return _attacks;
    }

    // Модификация параметров атаки в рантайме
    static void setAttack(const std::string& id, const AttackStats& stats) {
        ensureInitialized();
        _attacks[id] = stats;
    }

    // Модификация комбо в рантайме
    static void setCombo(const std::string& id, const ComboEntry& entry) {
        ensureInitialized();
        auto it = _comboIndexMap.find(id);
        if (it != _comboIndexMap.end()) {
            _combos[it->second] = entry;
        } else {
            _combos.push_back(entry);
            _comboIndexMap[id] = _combos.size() - 1;
        }
    }

    // =====================================================================
    // ГОРАЧАЯ ПЕРЕЗАГРУЗКА И РАБОТА С КВ ФАЙЛОМ (KEY-VALUES I/O)
    // =====================================================================

    // Горячая перезагрузка КВ таблицы из файла combat_balance.kv
    static bool reload() {
        _initialized = false;
        ensureInitialized();
        std::cout << "[KV CONFIG] Таблица баланса комбо и атак (combat_balance.kv) перезагружена! (" 
                  << _attacks.size() << " атак, " << _combos.size() << " комбо)" << std::endl;
        return true;
    }

    // Загрузка таблицы из указанного KV файла
    static bool loadKVFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        std::vector<std::string> blockStack;
        std::string pendingBlock = "";

        std::string line;
        while (std::getline(file, line)) {
            auto tokens = tokenizeLine(line);
            if (tokens.empty()) continue;

            for (size_t t = 0; t < tokens.size(); ++t) {
                const std::string& tok = tokens[t];
                if (tok == "{") {
                    blockStack.push_back(pendingBlock);
                    pendingBlock.clear();
                } else if (tok == "}") {
                    if (!blockStack.empty()) blockStack.pop_back();
                } else {
                    if (t + 1 < tokens.size() && (tokens[t+1] != "{" && tokens[t+1] != "}")) {
                        std::string key = tok;
                        std::string val = tokens[t+1];
                        t++; // пропуск значения

                        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                        if (blockStack.size() >= 3) {
                            const std::string& section = blockStack[1];
                            const std::string& entryId = blockStack[2];

                            if (section == "Attacks") {
                                AttackStats& st = _attacks[entryId];
                                if (key == "damage") st.damage = std::stof(val);
                                else if (key == "knock_x" || key == "knockx") st.knockX = std::stof(val);
                                else if (key == "knock_y" || key == "knocky") st.knockY = std::stof(val);
                                else if (key == "player_recovery" || key == "playerrecovery") st.playerRecovery = std::stof(val);
                                else if (key == "enemy_stun" || key == "enemystun") st.enemyStun = std::stof(val);
                                else if (key == "whiff_stun" || key == "whiffstun") st.whiffStun = std::stof(val);
                                else if (key == "active_time" || key == "activetime") st.activeTime = std::stof(val);
                                else if (key == "lunge") st.lunge = std::stof(val);
                                else if (key == "name" || key == "displayname") st.displayName = val;
                                else if (key == "color") st.color = parseColor(val);
                            } else if (section == "Combos") {
                                size_t cIdx = 0;
                                auto it = _comboIndexMap.find(entryId);
                                if (it == _comboIndexMap.end()) {
                                    ComboEntry ce;
                                    ce.id = entryId;
                                    _combos.push_back(ce);
                                    cIdx = _combos.size() - 1;
                                    _comboIndexMap[entryId] = cIdx;
                                } else {
                                    cIdx = it->second;
                                }

                                ComboEntry& ce = _combos[cIdx];
                                if (key == "sequence") ce.sequence = splitSequence(val);
                                else if (key == "is_unique" || key == "isunique") ce.isUniqueFinisher = (val == "1" || val == "true");
                                else if (key == "is_finisher" || key == "isfinisher") ce.isFinisher = (val == "1" || val == "true");
                                else if (key == "damage") ce.stats.damage = std::stof(val);
                                else if (key == "knock_x" || key == "knockx") ce.stats.knockX = std::stof(val);
                                else if (key == "knock_y" || key == "knocky") ce.stats.knockY = std::stof(val);
                                else if (key == "player_recovery" || key == "playerrecovery") ce.stats.playerRecovery = std::stof(val);
                                else if (key == "enemy_stun" || key == "enemystun") ce.stats.enemyStun = std::stof(val);
                                else if (key == "whiff_stun" || key == "whiffstun") ce.stats.whiffStun = std::stof(val);
                                else if (key == "active_time" || key == "activetime") ce.stats.activeTime = std::stof(val);
                                else if (key == "lunge") ce.stats.lunge = std::stof(val);
                                else if (key == "name" || key == "displayname") ce.stats.displayName = val;
                                else if (key == "color") ce.stats.color = parseColor(val);
                            }
                        }
                    } else {
                        pendingBlock = tok;
                    }
                }
            }
        }
        return true;
    }

private:
    inline static std::unordered_map<std::string, AttackStats> _attacks;
    inline static std::vector<ComboEntry> _combos;
    inline static std::unordered_map<std::string, size_t> _comboIndexMap;
    inline static bool _initialized = false;

    // Внутренняя инициализация значений по умолчанию и загрузка KV файла
    static void ensureInitialized() {
        if (_initialized) return;
        _initialized = true;

        _attacks.clear();
        _combos.clear();
        _comboIndexMap.clear();

        // 1. Инициализация базовых атак по умолчанию в КВ таблице
        initDefaultAttacks();

        // 2. Инициализация комбо по умолчанию в КВ таблице
        initDefaultCombos();

        // 3. Попытка загрузить внешний KV файл конфигурации
        std::string kvPath = resolveAssetPath("assets/config/combat_balance.kv");
        if (std::filesystem::exists(kvPath)) {
            loadKVFile(kvPath);
        }
    }

    static void initDefaultAttacks() {
        // Легкие атаки на земле
        _attacks["1"]   = {2.0f, 160.0f, -25.0f, 0.14f, 0.18f, 0.11f, 14.0f, "ATTACK: 1 (Light Jab)", sf::Color(255, 235, 90), 0.38f};
        _attacks["f1"]  = {2.2f, 220.0f, -30.0f, 0.15f, 0.20f, 0.12f, 18.0f, "ATTACK: f1 (Forward Thrust)", sf::Color(255, 240, 110), 0.38f};
        _attacks["b1"]  = {2.0f, -140.0f, -30.0f, 0.14f, 0.18f, 0.11f, 0.0f, "ATTACK: b1 (Turn Jab)", sf::Color(245, 220, 90), 0.38f};
        _attacks["u1"]  = {2.2f, 80.0f, -435.0f, 0.16f, 0.40f, 0.12f, 8.0f, "ATTACK: u1 (Anti-Air Poke)", sf::Color(255, 245, 120), 0.40f};
        _attacks["dw1"] = {2.0f, 180.0f, -20.0f, 0.14f, 0.20f, 0.11f, 10.0f, "ATTACK: dw1 (Low Kick)", sf::Color(240, 210, 80), 0.38f};

        // Тяжелые атаки на земле
        _attacks["2"]   = {3.5f, 350.0f, -40.0f, 0.24f, 0.26f, 0.15f, 16.0f, "ATTACK: 2 (Heavy Strike)", sf::Color(255, 110, 60), 0.55f};
        _attacks["f2"]  = {3.8f, 1150.0f, -45.0f, 0.22f, 0.35f, 0.14f, 18.0f, "ATTACK: f2 (Forward Smash -> Push)", sf::Color(255, 130, 50), 0.52f};
        _attacks["b2"]  = {3.5f, -350.0f, -50.0f, 0.24f, 0.28f, 0.15f, 0.0f, "ATTACK: b2 (Back Swing)", sf::Color(255, 100, 50), 0.55f};
        _attacks["u2"]  = {4.0f, 120.0f, -435.0f, 0.26f, 0.42f, 0.16f, 10.0f, "ATTACK: u2 (Uppercut)", sf::Color(255, 95, 70), 0.60f};
        _attacks["dw2"] = {3.6f, 260.0f, -30.0f, 0.25f, 0.32f, 0.15f, 12.0f, "ATTACK: dw2 (Low Sweep)", sf::Color(255, 120, 60), 0.58f};

        // Атаки в дэше
        _attacks["d1"]  = {2.5f, 600.0f, -40.0f, 0.16f, 0.22f, 0.12f, 18.0f, "ATTACK: d1 (Dash Thrust)", sf::Color(90, 220, 255), 0.40f};
        _attacks["df1"] = _attacks["d1"];
        _attacks["db1"] = {3.0f, -220.0f, -435.0f, 0.18f, 0.40f, 0.14f, 0.0f, "ATTACK: db1 (Reverse Dash Sweep)", sf::Color(100, 240, 220), 0.42f};
        _attacks["d2"]  = {4.0f, 1150.0f, -55.0f, 0.26f, 0.32f, 0.16f, 22.0f, "ATTACK: d2 (Heavy Dash Smash)", sf::Color(255, 80, 110), 0.60f};
        _attacks["df2"] = _attacks["d2"];
        _attacks["db2"] = {4.0f, -550.0f, -435.0f, 0.28f, 0.42f, 0.16f, 0.0f, "ATTACK: db2 (Reverse Dash Launcher)", sf::Color(255, 90, 140), 0.65f};

        // Воздушные атаки
        _attacks["a1"]   = {2.0f, 70.0f, -40.0f, 0.16f, 0.35f, 0.13f, 6.0f, "AIR ATTACK: a1 (Air Juggle)", sf::Color(80, 220, 255), 0.40f};
        _attacks["af1"]  = _attacks["a1"];
        _attacks["a2"]   = {3.5f, 1250.0f, 0.0f, 0.24f, 0.48f, 0.15f, 0.0f, "AIR ATTACK: a2 (Horizontal Air Push)", sf::Color(240, 130, 255), 0.55f};
        _attacks["af2"]  = _attacks["a2"];
        _attacks["adw2"] = {4.0f, 440.0f, 550.0f, 0.28f, 0.40f, 0.16f, 8.0f, "AIR ATTACK: adw2 (Air Down Slam)", sf::Color(255, 90, 30), 0.60f};
        _attacks["ad1"]  = {2.5f, 160.0f, -60.0f, 0.17f, 0.25f, 0.13f, 12.0f, "AIR DASH ATTACK: ad1", sf::Color(100, 240, 220), 0.40f};
        _attacks["ad2"]  = {4.0f, 220.0f, 320.0f, 0.28f, 0.28f, 0.16f, 15.0f, "AIR DASH ATTACK: ad2", sf::Color(255, 90, 160), 0.60f};
    }

    static void addCombo(const ComboEntry& ce) {
        _combos.push_back(ce);
        _comboIndexMap[ce.id] = _combos.size() - 1;
    }

    static void initDefaultCombos() {
        // --- 1. Уникальные вариации комбо (финишеры) ---
        addCombo({
            "1f2d1",
            {"1", "f2", "d1"},
            {5.0f, 460.0f, -435.0f, 0.26f, 0.42f, 0.15f, 15.0f, "COMBO 1-f2-d1: DIAGONAL LAUNCHER!", sf::Color(255, 170, 40), 0.60f},
            true, true
        });

        addCombo({
            "11b2",
            {"1", "1", "b2"},
            {4.5f, -540.0f, -435.0f, 0.30f, 0.45f, 0.16f, 12.0f, "COMBO 1-1-b2: GRAB & BACK THROW!", sf::Color(255, 80, 80), 0.65f},
            true, true
        });

        addCombo({
            "112",
            {"1", "1", "2"},
            {4.5f, 180.0f, -435.0f, 0.28f, 0.42f, 0.16f, 18.0f, "COMBO 1-1-2: RAPID COMBO LAUNCHER!", sf::Color(255, 60, 60), 0.60f},
            true, true
        });

        addCombo({
            "111",
            {"1", "1", "1"},
            {2.5f, 500.0f, -40.0f, 0.15f, 0.22f, 0.12f, 16.0f, "COMBO 1-1-1: TRIPLE LIGHT STRIKE", sf::Color(255, 240, 90), 0.38f},
            false, true
        });

        addCombo({
            "222",
            {"2", "2", "2"},
            {4.0f, 1200.0f, -60.0f, 0.28f, 0.30f, 0.16f, 25.0f, "COMBO 2-2-2: TRIPLE HEAVY STRIKE", sf::Color(255, 95, 45), 0.65f},
            false, true
        });

        addCombo({
            "21db1",
            {"2", "1", "db1"},
            {4.8f, 220.0f, -435.0f, 0.26f, 0.42f, 0.15f, 0.0f, "COMBO 2-1-db1: REVERSE SWEEP LAUNCHER!", sf::Color(255, 140, 80), 0.60f},
            true, true
        });

        addCombo({
            "a1a1adw2",
            {"a1", "a1", "adw2"},
            {5.5f, 1150.0f, -280.0f, 0.32f, 0.52f, 0.18f, 10.0f, "COMBO a1-a1-adw2: DUAL AIR DIVE SLAM!", sf::Color(255, 75, 25), 0.65f},
            true, true
        });

        addCombo({
            "a1adw2",
            {"a1", "adw2"},
            {4.5f, 440.0f, 550.0f, 0.28f, 0.40f, 0.16f, 8.0f, "COMBO a1-adw2: GROUND BOUNCE SLAM!", sf::Color(255, 90, 30), 0.60f},
            true, true
        });

        addCombo({
            "a1a2",
            {"a1", "a2"},
            {3.5f, 1250.0f, 0.0f, 0.24f, 0.45f, 0.15f, 0.0f, "COMBO a1-a2: HORIZONTAL AIR PUSH!", sf::Color(240, 130, 255), 0.55f},
            false, true
        });

        addCombo({
            "a2ad1",
            {"a2", "ad1"},
            {4.5f, 460.0f, -435.0f, 0.28f, 0.40f, 0.16f, 12.0f, "COMBO a2-ad1: AIR DIAGONAL LAUNCH!", sf::Color(190, 85, 255), 0.60f},
            true, true
        });

        // --- 2. Базовые комбо (общее начало для дальнейших уникальных вариаций) ---
        addCombo({
            "1f2",
            {"1", "f2"},
            {3.5f, 1150.0f, -45.0f, 0.22f, 0.35f, 0.14f, 15.0f, "COMBO 1-f2: PUSH! -> PRESS [SHIFT]", sf::Color(255, 140, 50), 0.52f},
            false, false
        });

        addCombo({
            "11",
            {"1", "1"},
            {2.0f, 160.0f, -25.0f, 0.14f, 0.18f, 0.11f, 14.0f, "COMBO 1-1: Light Jab 2", sf::Color(255, 215, 60), 0.38f},
            false, false
        });

        addCombo({
            "21",
            {"2", "1"},
            {3.0f, 850.0f, -35.0f, 0.20f, 0.30f, 0.13f, 16.0f, "COMBO 2-1: SETUP FOR DASH BEHIND", sf::Color(255, 160, 60), 0.48f},
            false, false
        });

        addCombo({
            "22",
            {"2", "2"},
            {3.5f, 260.0f, -35.0f, 0.26f, 0.26f, 0.15f, 18.0f, "COMBO 2-2: Heavy Strike 2", sf::Color(255, 120, 50), 0.58f},
            false, false
        });

        addCombo({
            "a1a1",
            {"a1", "a1"},
            {2.2f, 80.0f, -35.0f, 0.16f, 0.35f, 0.13f, 8.0f, "COMBO a1-a1: Dual Air Juggle", sf::Color(90, 230, 255), 0.40f},
            false, false
        });
    }

    // Вспомогательные парсеры для KV формата
    static std::vector<std::string> tokenizeLine(const std::string& line) {
        std::vector<std::string> tokens;
        size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n' || line[i] == '=' || line[i] == ':')) {
                i++;
            }
            if (i >= line.size()) break;

            if (line[i] == '/' && i + 1 < line.size() && line[i+1] == '/') break;
            if (line[i] == '#' || line[i] == ';') break;

            if (line[i] == '"') {
                i++;
                size_t start = i;
                while (i < line.size() && line[i] != '"') {
                    i++;
                }
                tokens.push_back(line.substr(start, i - start));
                if (i < line.size() && line[i] == '"') i++;
            } else if (line[i] == '{' || line[i] == '}') {
                tokens.push_back(std::string(1, line[i]));
                i++;
            } else {
                size_t start = i;
                while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && 
                       line[i] != '\n' && line[i] != '=' && line[i] != ':' && line[i] != '{' && line[i] != '}' && line[i] != '"') {
                    i++;
                }
                tokens.push_back(line.substr(start, i - start));
            }
        }
        return tokens;
    }

    static std::vector<std::string> splitSequence(const std::string& seqStr) {
        std::vector<std::string> result;
        std::stringstream ss(seqStr);
        std::string item;
        while (ss >> item) {
            if (!item.empty() && item.back() == ',') item.pop_back();
            if (!item.empty()) result.push_back(item);
        }
        return result;
    }

    static sf::Color parseColor(const std::string& str) {
        std::stringstream ss(str);
        int r = 255, g = 255, b = 255, a = 255;
        char comma;
        if (str.find(',') != std::string::npos) {
            ss >> r >> comma >> g >> comma >> b;
            if (ss >> comma >> a) {}
        } else {
            ss >> r >> g >> b;
            if (ss >> a) {}
        }
        return sf::Color(r, g, b, a);
    }
};
