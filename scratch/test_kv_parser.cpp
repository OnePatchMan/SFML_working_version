#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cassert>
#include <SFML/Graphics.hpp>
#include "core/Common.h"

struct AttackStats {
    float damage = 2.0f;
    float knockX = 160.0f;
    float knockY = -30.0f;
    float playerRecovery = 0.14f;
    float enemyStun = 0.20f;
    float activeTime = 0.12f;
    float lunge = 14.0f;
    std::string displayName;
    sf::Color color;
    float whiffStun = 0.0f;
};

struct ComboEntry {
    std::string id;
    std::vector<std::string> sequence;
    AttackStats stats;
    bool isUniqueFinisher = false;
    bool isFinisher = true;
};

inline std::vector<std::string> tokenizeLine(const std::string& line) {
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

inline std::vector<std::string> splitSequence(const std::string& seqStr) {
    std::vector<std::string> result;
    std::stringstream ss(seqStr);
    std::string item;
    while (ss >> item) {
        if (!item.empty() && item.back() == ',') item.pop_back();
        if (!item.empty()) result.push_back(item);
    }
    return result;
}

inline sf::Color parseColor(const std::string& str) {
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

int main() {
    std::string path = "assets/config/combat_balance.kv";
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        return 1;
    }

    std::unordered_map<std::string, AttackStats> attacks;
    std::vector<ComboEntry> combos;
    std::unordered_map<std::string, size_t> comboIndexMap;

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
                    t++; // consumed val

                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                    if (blockStack.size() >= 3) {
                        const std::string& section = blockStack[1];
                        const std::string& entryId = blockStack[2];

                        if (section == "Attacks") {
                            AttackStats& st = attacks[entryId];
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
                            auto it = comboIndexMap.find(entryId);
                            if (it == comboIndexMap.end()) {
                                ComboEntry ce;
                                ce.id = entryId;
                                combos.push_back(ce);
                                cIdx = combos.size() - 1;
                                comboIndexMap[entryId] = cIdx;
                            } else {
                                cIdx = it->second;
                            }

                            ComboEntry& ce = combos[cIdx];
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

    std::cout << "Parsed Attacks count: " << attacks.size() << std::endl;
    std::cout << "Parsed Combos count: " << combos.size() << std::endl;

    assert(attacks.find("1") != attacks.end());
    assert(attacks["1"].damage == 2.0f);
    assert(attacks["1"].knockX == 160.0f);
    assert(attacks["1"].whiffStun == 0.38f);
    assert(attacks["f2"].damage == 3.8f);
    assert(attacks["f2"].knockX == 1150.0f);

    assert(comboIndexMap.find("1f2d1") != comboIndexMap.end());
    size_t idx = comboIndexMap["1f2d1"];
    assert(combos[idx].stats.damage == 5.0f);
    assert(combos[idx].stats.knockY == -435.0f);
    assert(combos[idx].sequence.size() == 3);
    assert(combos[idx].sequence[0] == "1");
    assert(combos[idx].sequence[1] == "f2");
    assert(combos[idx].sequence[2] == "d1");
    assert(combos[idx].isUniqueFinisher == true);
    assert(combos[idx].isFinisher == true);

    std::cout << "All KV parser assertions PASSED!" << std::endl;
    return 0;
}

