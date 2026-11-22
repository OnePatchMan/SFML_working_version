#include "audio/SoundManager.h"
#include "core/Common.h"
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

SoundManager::SoundManager() {
    for (size_t i = 0; i < 16; ++i) {
        _soundPool.emplace_back(_dummyBuffer);
    }
}

void SoundManager::init(const std::string& directory) {
    _soundDir = directory;
    reload();
    _initialized = true;
}

void SoundManager::reload() {
    std::string resolvedDir = resolveAssetPath(_soundDir);
    std::cout << "[SoundManager] Scanning audio directory: " << resolvedDir << std::endl;

    _buffers.clear();
    _musicTracks.clear();

    if (!fs::exists(resolvedDir) || !fs::is_directory(resolvedDir)) {
        std::cerr << "[SoundManager] Directory not found: " << resolvedDir << std::endl;
        return;
    }

    size_t count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(resolvedDir)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac") {
            std::string relPath = fs::relative(entry.path(), resolvedDir).generic_string();
            std::string stem = entry.path().stem().string();

            // Проверяем: если файл лежит в папке OST — регистрируем как музыку (Soundtrack)
            if (relPath.find("OST") != std::string::npos || relPath.find("ost") != std::string::npos) {
                _musicTracks.push_back(entry.path().string());
                std::cout << "[SoundManager] Registered OST track: " << relPath << std::endl;
                continue;
            }

            sf::SoundBuffer buffer;
            bool loaded = buffer.loadFromFile(entry.path().string());
            if (!loaded) {
                // Попытка автоматической очистки обложек/тегов ID3 через ffmpeg
                std::string tmpClean = "/tmp/clean_audio_" + stem + ext;
                std::string cmd = "ffmpeg -y -v quiet -i \"" + entry.path().string() + "\" -vn -c:a copy -map_metadata -1 \"" + tmpClean + "\" > /dev/null 2>&1";
                if (std::system(cmd.c_str()) == 0 && fs::exists(tmpClean)) {
                    loaded = buffer.loadFromFile(tmpClean);
                    std::error_code ec;
                    fs::remove(tmpClean, ec);
                }
            }

            if (loaded) {
                _buffers[stem] = std::move(buffer);
                count++;

                std::string relKey = fs::path(relPath).replace_extension("").generic_string();
                if (relKey != stem) {
                    _buffers[relKey] = _buffers[stem];
                }
                std::cout << "[SoundManager] Loaded audio: '" << stem << "' from " << relPath << std::endl;
            } else {
                std::cerr << "[SoundManager] Failed to load audio: " << entry.path().string() << std::endl;
            }
        }
    }

    std::sort(_musicTracks.begin(), _musicTracks.end());
    std::cout << "[SoundManager] Total loaded sounds: " << count << ", OST tracks: " << _musicTracks.size() << std::endl;
}

bool SoundManager::hasSound(const std::string& key) const {
    return _buffers.find(key) != _buffers.end();
}

std::string SoundManager::resolveAttackKey(const std::string& attackType, const std::string& comboPrefix, bool inAir) const {
    // 1. Уникальные завершители комбо со скобками: "(prefix)attack" (например "(11)2", "(21)db1")
    if (!comboPrefix.empty()) {
        std::string comboKey = "(" + comboPrefix + ")" + attackType;
        if (hasSound(comboKey)) {
            return comboKey;
        }
    }

    // 2. Если атака в воздухе: сначала ищем воздушные звуки
    if (inAir) {
        if (!attackType.empty() && attackType[0] == 'a' && hasSound(attackType)) {
            return attackType;
        }
        std::string airKey = "a" + attackType;
        if (hasSound(airKey)) {
            return airKey;
        }
        if (hasSound(attackType)) {
            return attackType;
        }
    } else {
        // На земле: прямое точное совпадение ключа атаки (например "1", "2")
        if (hasSound(attackType)) {
            return attackType;
        }
    }

    // 4. Fallback-правило пользователя:
    // "остальные атаки которые не указаны пока что замени на просто 1 или 2 соотвественно"
    if (inAir) {
        // В воздухе: дайв-слэм -> adw2 (или 2), легкие -> a1 (или 1), тяжелые -> a2 (или 2)
        if (attackType.find("dw") != std::string::npos || attackType.find("slam") != std::string::npos) {
            if (hasSound("adw2")) return "adw2";
            if (hasSound("a2")) return "a2";
            if (hasSound("2")) return "2";
        }
        bool isLight = (!attackType.empty() && attackType.back() == '1') || attackType.find('1') != std::string::npos;
        if (isLight) {
            if (hasSound("a1")) return "a1";
            if (hasSound("1")) return "1";
        }
        if (hasSound("a2")) return "a2";
        if (hasSound("2")) return "2";
        if (hasSound("1")) return "1";
    } else {
        // На земле:
        // Если атака тяжелая (содержит 2) -> "2"
        if (attackType.find('2') != std::string::npos || (!attackType.empty() && attackType.back() == '2')) {
            if (hasSound("2")) return "2";
        }
        // Если атака легкая (содержит 1) -> "1"
        if (attackType.find('1') != std::string::npos || (!attackType.empty() && attackType.back() == '1')) {
            if (hasSound("1")) return "1";
        }
        // Дефолтный fallback
        if (hasSound("1")) return "1";
        if (hasSound("2")) return "2";
    }

    return "";
}

void SoundManager::playAttack(const std::string& attackType, const std::string& comboPrefix, bool inAir, float volume) {
    std::string key = resolveAttackKey(attackType, comboPrefix, inAir);
    if (!key.empty()) {
        play(key, volume);
    }
}

void SoundManager::play(const std::string& key, float volume) {
    auto it = _buffers.find(key);
    if (it == _buffers.end()) return;

    if (_soundPool.empty()) {
        for (size_t i = 0; i < 16; ++i) {
            _soundPool.emplace_back(_dummyBuffer);
        }
    }

    // Ищем остановленный звук в пуле
    sf::Sound* soundToUse = nullptr;
    for (size_t i = 0; i < _soundPool.size(); ++i) {
        size_t idx = (_nextSoundIndex + i) % _soundPool.size();
        if (_soundPool[idx].getStatus() == sf::Sound::Status::Stopped) {
            soundToUse = &_soundPool[idx];
            _nextSoundIndex = (idx + 1) % _soundPool.size();
            break;
        }
    }

    // Если все играют — переиспользуем старейший (round-robin)
    if (!soundToUse) {
        soundToUse = &_soundPool[_nextSoundIndex];
        _nextSoundIndex = (_nextSoundIndex + 1) % _soundPool.size();
    }

    soundToUse->setBuffer(it->second);
    float finalVol = std::clamp((volume * _masterVolume) / 100.0f, 0.0f, 100.0f);
    soundToUse->setVolume(finalVol);
    soundToUse->play();
}

void SoundManager::playMenuMusic(float volume) {
    float vol = (volume >= 0.0f) ? volume : _musicBaseVolume;
    playMusic("menu", true, vol);
}

void SoundManager::playGameMusic(float volume) {
    float vol = (volume >= 0.0f) ? volume : _musicBaseVolume;
    playMusic("", true, vol);
}

void SoundManager::playBossMusic(const std::string& bossTrack, float volume) {
    float vol = (volume >= 0.0f) ? volume : _musicBaseVolume;
    std::string key = bossTrack.empty() ? "boss" : bossTrack;
    playMusic(key, true, vol);
}

void SoundManager::playMusic(const std::string& trackName, bool loop, float volume) {
    if (_musicTracks.empty()) {
        std::string ostDir = resolveAssetPath(_soundDir + "/OST");
        if (fs::exists(ostDir) && fs::is_directory(ostDir)) {
            for (const auto& entry : fs::recursive_directory_iterator(ostDir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac") {
                        _musicTracks.push_back(entry.path().string());
                    }
                }
            }
            std::sort(_musicTracks.begin(), _musicTracks.end());
        }
    }

    if (_musicTracks.empty()) {
        std::cerr << "[SoundManager] No music tracks found in OST directory" << std::endl;
        return;
    }

    std::string targetTrack = "";
    if (!trackName.empty()) {
        for (const auto& t : _musicTracks) {
            if (t.find(trackName) != std::string::npos) {
                targetTrack = t;
                break;
            }
        }
    }
    if (targetTrack.empty()) {
        // Для игры выбираем трек, не входящий в подпапку menu (например Sublevel_Four.mp3)
        for (const auto& t : _musicTracks) {
            if (t.find("/menu/") == std::string::npos && t.find("\\menu\\") == std::string::npos) {
                targetTrack = t;
                break;
            }
        }
        if (targetTrack.empty() && !_musicTracks.empty()) {
            targetTrack = _musicTracks.front();
        }
    }

    if (!_music) {
        _music = std::make_unique<sf::Music>();
    }

    if (_currentTrackPath == targetTrack && _music->getStatus() == sf::Music::Status::Playing) {
        return;
    }

    if (!_music->openFromFile(targetTrack)) {
        std::cerr << "[SoundManager] Failed to load music: " << targetTrack << std::endl;
        return;
    }

    _currentTrackPath = targetTrack;
    _musicBaseVolume = volume;
    _music->setLooping(loop);
    float finalVol = std::clamp((_musicBaseVolume * _masterVolume) / 100.0f, 0.0f, 100.0f);
    _music->setVolume(finalVol);
    _music->play();
    std::cout << "[SoundManager] Playing music: " << fs::path(targetTrack).filename().string() << std::endl;
}

void SoundManager::stopMusic() {
    if (_music) {
        _music->stop();
        _currentTrackPath = "";
    }
}

void SoundManager::pauseMusic() {
    if (_music && _music->getStatus() == sf::Music::Status::Playing) {
        _music->pause();
    }
}

void SoundManager::resumeMusic() {
    if (_music && _music->getStatus() == sf::Music::Status::Paused) {
        _music->play();
    }
}

bool SoundManager::isMusicPlaying() const {
    return _music && (_music->getStatus() == sf::Music::Status::Playing);
}

void SoundManager::setMusicVolume(float volume) {
    _musicBaseVolume = volume;
    if (_music) {
        float finalVol = std::clamp((_musicBaseVolume * _masterVolume) / 100.0f, 0.0f, 100.0f);
        _music->setVolume(finalVol);
    }
}

void SoundManager::setMasterVolume(float volume) {
    _masterVolume = volume;
    if (_music) {
        float finalVol = std::clamp((_musicBaseVolume * _masterVolume) / 100.0f, 0.0f, 100.0f);
        _music->setVolume(finalVol);
    }
}
