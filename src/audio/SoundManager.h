#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <iostream>

class SoundManager {
public:
    static SoundManager& get() {
        static SoundManager instance;
        return instance;
    }

    void init(const std::string& directory = "assets/soundeff");
    void reload();

    // Воспроизведение звука атаки с автоматическим поиском и fallback:
    // 1) Уникальные финишеры комбо со скобками: "(prefix)attack", например "(11)2", "(21)db1"
    // 2) Прямые звуки атак: "a1", "a2", "adw2", "1", "2"
    // 3) Fallback: не указанные атаки заменяются на просто 1 или 2 (или a1/a2/adw2 в воздухе)
    // Громкость уменьшена в 2 раза по требованию (42.5f вместо 85.0f)
    void playAttack(const std::string& attackType, const std::string& comboPrefix = "", bool inAir = false, float volume = 42.5f);

    // Прямое воспроизведение по имени ключа
    void play(const std::string& key, float volume = 42.5f);

    bool hasSound(const std::string& key) const;
    size_t getSoundCount() const { return _buffers.size(); }
    void setMasterVolume(float volume) { _masterVolume = volume; }
    float getMasterVolume() const { return _masterVolume; }

    std::string resolveAttackKey(const std::string& attackType, const std::string& comboPrefix = "", bool inAir = false) const;

private:
    SoundManager();
    ~SoundManager() = default;

    sf::SoundBuffer _dummyBuffer;
    std::unordered_map<std::string, sf::SoundBuffer> _buffers;
    std::vector<sf::Sound> _soundPool;
    size_t _nextSoundIndex = 0;
    float _masterVolume = 42.5f;
    std::string _soundDir = "assets/soundeff";
    bool _initialized = false;
};
