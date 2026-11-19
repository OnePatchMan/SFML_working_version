#include "world/Arena.h"
#include "combat/CombatConfig.h"
#include "entities/Player.h"
#include "audio/SoundManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <random>

Arena* Arena::s_activeArena = nullptr;

Arena::Arena(int width, int height)
    : _width(width), _height(height), _floorY(std::max(2, height - 5)), _tileSize(TILE_SIZE)
{
    s_activeArena = this;
    float spawnX = 6.0f * _tileSize;
    float spawnY = static_cast<float>(_floorY) * _tileSize - 30.0f;
    _spawnPosition = {spawnX, spawnY};
    _enemySpawnPosition = {spawnX + 350.0f, spawnY + 2.0f};
    _enemySpawns = {{_enemySpawnPosition, 0}};
    _enemySpawnPositions = {_enemySpawnPosition};

    loadTileset(_tilesetPath);
    buildDefaultMap();
    loadRoom(1);
}

void Arena::syncEnemySpawnPositions() {
    _enemySpawnPositions.clear();
    for (const auto& sp : _enemySpawns) {
        _enemySpawnPositions.push_back(sp.position);
    }
    if (!_enemySpawnPositions.empty()) {
        _enemySpawnPosition = _enemySpawnPositions[0];
    }
}

void Arena::addEnemySpawn(sf::Vector2f pos, int type) {
    _enemySpawns.push_back({pos, type});
    syncEnemySpawnPositions();
}

void Arena::setEnemySpawnAt(int tx, int ty, int type) {
    float posX = static_cast<float>(tx * _tileSize);
    float posY = static_cast<float>((ty + 1) * _tileSize - 28.0f);
    for (auto& sp : _enemySpawns) {
        int stx = static_cast<int>(std::round(sp.position.x / _tileSize));
        int sty = static_cast<int>(std::round((sp.position.y + 28.0f) / _tileSize)) - 1;
        if (stx == tx && sty == ty) {
            sp.type = type;
            return;
        }
    }
    addEnemySpawn({posX, posY}, type);
}

bool Arena::removeEnemySpawnAt(float wx, float wy, float radius) {
    for (auto it = _enemySpawns.begin(); it != _enemySpawns.end(); ++it) {
        float dx = it->position.x - wx;
        float dy = it->position.y - wy;
        if (dx * dx + dy * dy <= radius * radius) {
            _enemySpawns.erase(it);
            syncEnemySpawnPositions();
            return true;
        }
    }
    return false;
}

bool Arena::removeEnemySpawnAtTile(int tx, int ty) {
    float checkX = static_cast<float>(tx) * _tileSize + _tileSize * 0.5f;
    float checkY = static_cast<float>((ty + 1) * _tileSize - 14.0f);
    return removeEnemySpawnAt(checkX, checkY, _tileSize * 0.9f);
}

void Arena::clearEnemySpawns() {
    _enemySpawns.clear();
    syncEnemySpawnPositions();
}

Arena::~Arena() {
    if (s_activeArena == this) {
        s_activeArena = nullptr;
    }
}

bool Arena::isBlockSolid(int tx, int ty) const {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return true; // Границы арены непроницаемы
    int t = _map[ty][tx];
    return (t == 1 || t == 2);
}

bool Arena::raycastSolid(sf::Vector2f start, sf::Vector2f target, sf::Vector2f& hitPoint, sf::Vector2i& hitTile, sf::Vector2f& hitNormal) const {
    float dx = target.x - start.x;
    float dy = target.y - start.y;
    float dist = std::hypot(dx, dy);
    if (dist < 0.001f) {
        int tx = static_cast<int>(std::floor(start.x / _tileSize));
        int ty = static_cast<int>(std::floor(start.y / _tileSize));
        if (isBlockSolid(tx, ty)) {
            hitPoint = start;
            hitTile = {tx, ty};
            hitNormal = {0.0f, -1.0f};
            return true;
        }
        return false;
    }

    // Точный шаг 3.0px для непрерывной детекции любых блоков и потолков
    float step = 3.0f;
    int numSteps = std::max(1, static_cast<int>(std::ceil(dist / step)));
    float stepX = dx / static_cast<float>(numSteps);
    float stepY = dy / static_cast<float>(numSteps);

    sf::Vector2f prev = start;
    sf::Vector2f cur = start;

    for (int i = 0; i <= numSteps; ++i) {
        if (i > 0) {
            cur.x += stepX;
            cur.y += stepY;
        }
        int tx = static_cast<int>(std::floor(cur.x / _tileSize));
        int ty = static_cast<int>(std::floor(cur.y / _tileSize));

        if (isBlockSolid(tx, ty)) {
            hitTile = {tx, ty};
            float tileLeft   = static_cast<float>(tx * _tileSize);
            float tileRight  = static_cast<float>((tx + 1) * _tileSize);
            float tileTop    = static_cast<float>(ty * _tileSize);
            float tileBottom = static_cast<float>((ty + 1) * _tileSize);

            if (prev.y <= tileTop) {
                hitNormal = {0.0f, -1.0f}; // Верхняя грань блока (пол)
                hitPoint = {std::clamp(cur.x, tileLeft, tileRight), tileTop};
            } else if (prev.y >= tileBottom) {
                hitNormal = {0.0f, 1.0f};  // Нижняя грань блока (потолок)
                hitPoint = {std::clamp(cur.x, tileLeft, tileRight), tileBottom};
            } else if (prev.x <= tileLeft) {
                hitNormal = {-1.0f, 0.0f}; // Левая грань блока (стена)
                hitPoint = {tileLeft, std::clamp(cur.y, tileTop, tileBottom)};
            } else if (prev.x >= tileRight) {
                hitNormal = {1.0f, 0.0f};  // Правая грань блока (стена)
                hitPoint = {tileRight, std::clamp(cur.y, tileTop, tileBottom)};
            } else {
                if (dy < 0.0f) {
                    hitNormal = {0.0f, 1.0f}; // Потолок
                    hitPoint = {std::clamp(cur.x, tileLeft, tileRight), tileBottom};
                } else {
                    hitNormal = {0.0f, -1.0f}; // Пол
                    hitPoint = {std::clamp(cur.x, tileLeft, tileRight), tileTop};
                }
            }
            return true;
        }
        prev = cur;
    }
    return false;
}

void Arena::spawnKillBlood(sf::Vector2f pos, sf::Vector2f knockDir, float maxHp) {
    static std::mt19937 rng(4242);
    std::uniform_real_distribution<float> angleDist(-0.65f, 0.65f);
    std::uniform_real_distribution<float> speedDist(90.0f, 230.0f); // Компактная дальность без огромных фонтанов
    std::uniform_real_distribution<float> sizeDist(1.4f, 2.8f);
    std::uniform_real_distribution<float> lifeDist(1.2f, 2.2f);

    float len = std::hypot(knockDir.x, knockDir.y);
    sf::Vector2f baseDir = (len > 0.001f) ? (knockDir / len) : sf::Vector2f(0.0f, -1.0f);

    // 1. Увеличенное в ~3 раза количество аккуратных капель крови при смерти (ранее 9 -> теперь 27)
    int particleCount = 27;
    for (int i = 0; i < particleCount; ++i) {
        BloodParticle p;
        p.position = pos + sf::Vector2f(angleDist(rng) * 4.0f, angleDist(rng) * 4.0f);

        float speed = speedDist(rng);
        float spreadAngle = angleDist(rng);
        float cosA = std::cos(spreadAngle);
        float sinA = std::sin(spreadAngle);
        sf::Vector2f dir = {
            baseDir.x * cosA - baseDir.y * sinA,
            baseDir.x * sinA + baseDir.y * cosA
        };
        // Мягкая дуга вверх только для горизонтальных ударов (чтобы удары вниз летели в пол, а апперкоты вверх)
        if (std::abs(baseDir.y) < 0.35f) {
            dir.y -= 0.30f;
        }
        float dLen = std::hypot(dir.x, dir.y);
        if (dLen > 0.001f) dir /= dLen;

        p.velocity = dir * speed;
        p.radius = sizeDist(rng);
        p.lifetime = lifeDist(rng);
        p.active = true;

        int cIdx = rng() % 4;
        if (cIdx == 0) p.color = sf::Color(155, 12, 18, 230);
        else if (cIdx == 1) p.color = sf::Color(185, 16, 24, 240);
        else if (cIdx == 2) p.color = sf::Color(120, 6, 10, 225);
        else p.color = sf::Color(165, 14, 20, 235);

        _bloodParticles.push_back(p);
    }

    // 2. Точечные капли на заднем фоне (ранее 4 -> теперь в 3 раза больше, 12 шт.)
    for (int i = 0; i < 12; ++i) {
        BackgroundBloodStreak streak;
        streak.isBlotch = true;
        streak.isSpray = true;
        float offsetAngle = static_cast<float>(rng() % 360) * 3.14159f / 180.0f;
        float offsetDist = 3.0f + static_cast<float>(rng() % 18);
        streak.startPos = pos + sf::Vector2f(std::cos(offsetAngle) * offsetDist, std::sin(offsetAngle) * offsetDist);
        streak.currentPos = streak.startPos;
        streak.length = 0.0f;
        streak.maxLength = 0.0f;
        streak.dripSpeed = 0.0f;
        streak.width = 1.4f + static_cast<float>(rng() % 2);
        streak.blotchH = 1.4f + static_cast<float>(rng() % 2);
        streak.active = false;

        int cIdx = rng() % 3;
        if (cIdx == 0) streak.color = sf::Color(145, 10, 16, 210);
        else if (cIdx == 1) streak.color = sf::Color(175, 15, 22, 220);
        else streak.color = sf::Color(115, 8, 12, 200);

        _bgBloodStreaks.push_back(streak);
    }
}

void Arena::spawnWallBounceBlood(sf::Vector2f pos, sf::Vector2f bounceDir) {
    static std::mt19937 rng(777);
    std::uniform_real_distribution<float> angleDist(-0.55f, 0.55f);
    std::uniform_real_distribution<float> speedDist(70.0f, 180.0f); // Компактный разлет от стены
    std::uniform_real_distribution<float> sizeDist(1.2f, 2.4f);
    std::uniform_real_distribution<float> lifeDist(1.0f, 1.8f);

    float len = std::hypot(bounceDir.x, bounceDir.y);
    sf::Vector2f baseDir = (len > 0.001f) ? (bounceDir / len) : sf::Vector2f(0.0f, -1.0f);

    // Чуть меньше, чем при обычной смерти (при смерти 27, здесь ~16 капель)
    int particleCount = 16;
    for (int i = 0; i < particleCount; ++i) {
        BloodParticle p;
        p.position = pos + sf::Vector2f(angleDist(rng) * 3.0f, angleDist(rng) * 3.0f);

        float speed = speedDist(rng);
        float spreadAngle = angleDist(rng);
        float cosA = std::cos(spreadAngle);
        float sinA = std::sin(spreadAngle);
        sf::Vector2f dir = {
            baseDir.x * cosA - baseDir.y * sinA,
            baseDir.x * sinA + baseDir.y * cosA
        };
        dir.y -= 0.15f;
        float dLen = std::hypot(dir.x, dir.y);
        if (dLen > 0.001f) dir /= dLen;

        p.velocity = dir * speed;
        p.radius = sizeDist(rng);
        p.lifetime = lifeDist(rng);
        p.active = true;

        int cIdx = rng() % 4;
        if (cIdx == 0) p.color = sf::Color(155, 12, 18, 230);
        else if (cIdx == 1) p.color = sf::Color(185, 16, 24, 240);
        else if (cIdx == 2) p.color = sf::Color(120, 6, 10, 225);
        else p.color = sf::Color(165, 14, 20, 235);

        _bloodParticles.push_back(p);
    }

    // Буквально 6-7 аккуратных статичных пятнышек на стене (чуть меньше 12)
    for (int i = 0; i < 7; ++i) {
        BackgroundBloodStreak streak;
        streak.isBlotch = true;
        streak.isSpray = true;
        float offsetAngle = static_cast<float>(rng() % 360) * 3.14159f / 180.0f;
        float offsetDist = 2.0f + static_cast<float>(rng() % 14);
        streak.startPos = pos + sf::Vector2f(std::cos(offsetAngle) * offsetDist, std::sin(offsetAngle) * offsetDist);
        streak.currentPos = streak.startPos;
        streak.length = 0.0f;
        streak.maxLength = 0.0f;
        streak.dripSpeed = 0.0f;
        streak.width = 1.2f + static_cast<float>(rng() % 2);
        streak.blotchH = 1.2f + static_cast<float>(rng() % 2);
        streak.active = false;

        int cIdx = rng() % 3;
        if (cIdx == 0) streak.color = sf::Color(145, 10, 16, 210);
        else if (cIdx == 1) streak.color = sf::Color(175, 15, 22, 220);
        else streak.color = sf::Color(115, 8, 12, 200);

        _bgBloodStreaks.push_back(streak);
    }
}

void Arena::spawnFinisherBlood(sf::Vector2f pos, sf::Vector2f burstDir, int count, float maxHp) {
    static std::mt19937 rng(1337);
    std::uniform_real_distribution<float> angleDist(-0.35f, 0.35f); // Сфокусированный направленный конус
    std::uniform_real_distribution<float> speedDist(650.0f, 1750.0f); // Повышенная скорость вылета: кровь летит далеко!
    std::uniform_real_distribution<float> sizeDist(2.5f, 5.5f);
    std::uniform_real_distribution<float> lifeDist(2.5f, 6.0f);

    float baseHp = CombatConfig::getFinisherBloodBaseHp();
    if (baseHp <= 0.0f) baseHp = 45.0f;

    // Множитель объема и плотности крови от максимального здоровья жертвы:
    // Чем больше maxHp (например, 110 у Танка vs 45 у Обычного), тем больше вылетает крови и тем массивнее потеки!
    float hpScale = std::clamp(maxHp / baseHp, 0.35f, 4.0f);
    float sizeScale = std::clamp(std::sqrt(hpScale), 0.75f, 1.55f);

    float len = std::hypot(burstDir.x, burstDir.y);
    sf::Vector2f baseDir = (len > 0.001f) ? (burstDir / len) : sf::Vector2f(0.0f, -1.0f);

    // 1. Летящие в направлении форсы частицы крови (капли и брызги, летящие на дальнюю дистанцию)
    int particleCount = std::max(15, static_cast<int>(std::round(count * hpScale)));
    for (int i = 0; i < particleCount; ++i) {
        BloodParticle p;
        p.position = pos + sf::Vector2f(angleDist(rng) * 8.0f, angleDist(rng) * 8.0f);

        float speed = speedDist(rng);
        float spreadAngle = angleDist(rng);
        float cosA = std::cos(spreadAngle);
        float sinA = std::sin(spreadAngle);
        sf::Vector2f dir = {
            baseDir.x * cosA - baseDir.y * sinA,
            baseDir.x * sinA + baseDir.y * cosA
        };
        // Летит строго по вектору форсы спецприема на высокую дистанцию
        p.velocity = dir * speed;
        p.radius = sizeDist(rng) * sizeScale;
        p.lifetime = lifeDist(rng);
        p.active = true;

        int cIdx = rng() % 4;
        if (cIdx == 0) p.color = sf::Color(160, 10, 16, 240);
        else if (cIdx == 1) p.color = sf::Color(195, 18, 26, 245);
        else if (cIdx == 2) p.color = sf::Color(125, 5, 10, 235);
        else p.color = sf::Color(175, 14, 22, 240);

        _bloodParticles.push_back(p);
    }

    // 2. Густая кровь на заднем фоне (основной шлейф в направлении удара + случайные брызги в других направлениях)
    sf::Vector2f normalDir(-baseDir.y, baseDir.x);

    // Вспомогательная лямбда размещения следа на заднем фоне с трассировкой луча:
    // если на пути есть твердый блок (потолок, стена, пол) — кровь ударяется о блок, задерживается на грани и стекает!
    auto placeBgStreak = [&](BackgroundBloodStreak& streak, sf::Vector2f target) {
        sf::Vector2f hitPoint;
        sf::Vector2i hitTile;
        sf::Vector2f hitNormal;
        if (raycastSolid(pos, target, hitPoint, hitTile, hitNormal)) {
            // Кровь ударилась о твердый блок: задерживается на грани и стекает!
            if (hitNormal.y > 0.5f) {
                // Удар в нижнюю кромку потолка
                streak.startPos = sf::Vector2f(hitPoint.x, static_cast<float>((hitTile.y + 1) * _tileSize));
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                if (streak.maxLength > 0.0f) {
                    streak.active = true;
                }
                // Задерживается на блоке: запекаем декаль на кромке потолка
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::HORIZONTAL;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                float tileLeft = static_cast<float>(hitTile.x * _tileSize);
                float tileRight = static_cast<float>((hitTile.x + 1) * _tileSize);
                decal.position = {std::clamp(hitPoint.x - stripLen * 0.5f, tileLeft, tileRight - stripLen), static_cast<float>((hitTile.y + 1) * _tileSize - 2.5f)};
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = 2.5f;
                _bloodDecals.push_back(decal);
            } else if (std::abs(hitNormal.x) > 0.5f) {
                // Удар в боковую грань стены
                float wallX = (hitNormal.x < 0.0f) ? static_cast<float>(hitTile.x * _tileSize)
                                                   : static_cast<float>((hitTile.x + 1) * _tileSize);
                streak.startPos = sf::Vector2f(wallX, hitPoint.y);
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                if (streak.maxLength > 0.0f) {
                    streak.active = true;
                }
                // Задерживается на блоке: запекаем декаль на кромке стены
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::VERTICAL;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                float tileTop = static_cast<float>(hitTile.y * _tileSize);
                float tileBottom = static_cast<float>((hitTile.y + 1) * _tileSize);
                float dX = (hitNormal.x < 0.0f) ? wallX : (wallX - 2.5f);
                decal.position = {dX, std::clamp(hitPoint.y - stripLen * 0.5f, tileTop, tileBottom - stripLen)};
                decal.length = stripLen;
                decal.radiusX = 2.5f;
                decal.radiusY = stripLen * 0.5f;
                _bloodDecals.push_back(decal);
            } else {
                // Удар в верхнюю кромку пола
                streak.startPos = sf::Vector2f(hitPoint.x, static_cast<float>(hitTile.y * _tileSize));
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                streak.active = false; // На полу не стекает вниз сквозь пол
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::HORIZONTAL;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                float tileLeft = static_cast<float>(hitTile.x * _tileSize);
                float tileRight = static_cast<float>((hitTile.x + 1) * _tileSize);
                decal.position = {std::clamp(hitPoint.x - stripLen * 0.5f, tileLeft, tileRight - stripLen), static_cast<float>(hitTile.y * _tileSize)};
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = 2.5f;
                _bloodDecals.push_back(decal);
            }
        } else {
            // Свободное пространство перед задней стеной
            streak.startPos = target;
            streak.currentPos = target;
            streak.length = 0.0f;
        }
    };

    // 2.1. ОСНОВНОЙ ШЛЕЙФ: Плотные перекрывающиеся сгустки крови в эпицентре удара (Dense Blood Mass)
    int blotchCount = std::max(10, static_cast<int>(std::round(28 * hpScale)));
    for (int i = 0; i < blotchCount; ++i) {
        BackgroundBloodStreak streak;
        streak.isBlotch = true;
        float fwd = -8.0f + static_cast<float>(rng() % 75);
        float lat = angleDist(rng) * 36.0f;
        sf::Vector2f target = pos + baseDir * fwd + normalDir * lat;

        streak.maxLength = 0.0f;
        streak.dripSpeed = 0.0f;
        streak.width = (8.0f + static_cast<float>(rng() % 16)) * sizeScale;    // Ширина сгустка
        streak.blotchH = (8.0f + static_cast<float>(rng() % 18)) * sizeScale;  // Высота сгустка
        streak.active = false;

        int cIdx = rng() % 4;
        if (cIdx == 0) streak.color = sf::Color(72, 4, 8, 250);   // Темно-бордовый сгусток
        else if (cIdx == 1) streak.color = sf::Color(98, 6, 12, 245);
        else if (cIdx == 2) streak.color = sf::Color(128, 8, 16, 240);
        else streak.color = sf::Color(155, 12, 20, 240);

        placeBgStreak(streak, target);
        _bgBloodStreaks.push_back(streak);
    }

    // 2.2. ОСНОВНОЙ ШЛЕЙФ: Толстые потоки (Heavy Dripping Streams)
    int heavyCount = std::max(6, static_cast<int>(std::round(16 * hpScale)));
    for (int i = 0; i < heavyCount; ++i) {
        BackgroundBloodStreak streak;
        float fwd = 8.0f + static_cast<float>(rng() % 220); // Дальний разлет вдоль форсы
        float lat = angleDist(rng) * 42.0f;
        sf::Vector2f target = pos + baseDir * fwd + normalDir * lat;

        streak.maxLength = (60.0f + static_cast<float>(rng() % 160)) * sizeScale; // Длина потека
        streak.dripSpeed = 32.0f + static_cast<float>(rng() % 40);  // Скорость 32..72 px/s
        streak.width = (5.5f + static_cast<float>(rng() % 5)) * sizeScale;        // Толщина ручья
        streak.active = true;

        int cIdx = rng() % 3;
        if (cIdx == 0) streak.color = sf::Color(105, 7, 12, 245);
        else if (cIdx == 1) streak.color = sf::Color(135, 10, 18, 240);
        else streak.color = sf::Color(165, 14, 22, 240);

        placeBgStreak(streak, target);
        _bgBloodStreaks.push_back(streak);
    }

    // 2.3. ОСНОВНОЙ ШЛЕЙФ: Средние ручьи (Medium Dripping Streams)
    int medCount = std::max(10, static_cast<int>(std::round(26 * hpScale)));
    for (int i = 0; i < medCount; ++i) {
        BackgroundBloodStreak streak;
        float fwd = 12.0f + static_cast<float>(rng() % 280);
        float lat = angleDist(rng) * 48.0f;
        sf::Vector2f target = pos + baseDir * fwd + normalDir * lat;

        streak.maxLength = (40.0f + static_cast<float>(rng() % 130)) * sizeScale;
        streak.dripSpeed = 25.0f + static_cast<float>(rng() % 32);
        streak.width = (3.2f + static_cast<float>(rng() % 3)) * sizeScale;
        streak.active = true;

        int cIdx = rng() % 3;
        if (cIdx == 0) streak.color = sf::Color(130, 9, 16, 235);
        else if (cIdx == 1) streak.color = sf::Color(170, 15, 24, 235);
        else streak.color = sf::Color(195, 20, 30, 240);

        placeBgStreak(streak, target);
        _bgBloodStreaks.push_back(streak);
    }

    // 2.4. ОСНОВНОЙ ШЛЕЙФ: Тонкие струйки (Fine Trickles)
    int fineCount = std::max(12, static_cast<int>(std::round(30 * hpScale)));
    for (int i = 0; i < fineCount; ++i) {
        BackgroundBloodStreak streak;
        float fwd = 15.0f + static_cast<float>(rng() % 350);
        float lat = angleDist(rng) * 55.0f;
        sf::Vector2f target = pos + baseDir * fwd + normalDir * lat;

        streak.maxLength = (25.0f + static_cast<float>(rng() % 95)) * sizeScale;
        streak.dripSpeed = 18.0f + static_cast<float>(rng() % 28);
        streak.width = (1.5f + static_cast<float>(rng() % 2)) * sizeScale;
        streak.active = true;

        int cIdx = rng() % 3;
        if (cIdx == 0) streak.color = sf::Color(140, 10, 18, 225);
        else if (cIdx == 1) streak.color = sf::Color(180, 18, 26, 230);
        else streak.color = sf::Color(205, 22, 32, 235);

        placeBgStreak(streak, target);
        _bgBloodStreaks.push_back(streak);
    }

    // 2.5. ОСНОВНОЙ ШЛЕЙФ: Мелкие крапинки (Spatter Specks)
    int speckCount = std::max(15, static_cast<int>(std::round(45 * hpScale)));
    for (int i = 0; i < speckCount; ++i) {
        BackgroundBloodStreak streak;
        float fwd = static_cast<float>(rng() % 380);
        float lat = angleDist(rng) * 65.0f;
        sf::Vector2f target = pos + baseDir * fwd + normalDir * lat;

        streak.maxLength = (2.0f + static_cast<float>(rng() % 6)) * sizeScale;
        streak.dripSpeed = 10.0f + static_cast<float>(rng() % 15);
        streak.width = (2.0f + static_cast<float>(rng() % 3)) * sizeScale;
        streak.active = true;

        int cIdx = rng() % 3;
        if (cIdx == 0) streak.color = sf::Color(120, 8, 14, 215);
        else if (cIdx == 1) streak.color = sf::Color(165, 14, 22, 220);
        else streak.color = sf::Color(190, 18, 28, 225);

        placeBgStreak(streak, target);
        _bgBloodStreaks.push_back(streak);
    }

    // 2.6. СЛУЧАЙНЫЕ БРЫЗГИ В ДРУГИХ НАПРАВЛЕНИЯХ (Randomized Secondary Splatters)
    // Разлетаются во всех направлениях случайным образом, создавая естественное хаотичное рассеяние
    int randSplashCount = std::max(14, static_cast<int>(std::round(38 * hpScale)));
    std::uniform_real_distribution<float> randAngleDist(-3.14159f, 3.14159f);
    for (int i = 0; i < randSplashCount; ++i) {
        BackgroundBloodStreak streak;
        float angle = randAngleDist(rng);
        sf::Vector2f rDir(std::cos(angle), std::sin(angle));
        float rDist = 15.0f + static_cast<float>(rng() % 150); // Меньший разлет, чем у основного шлейфа
        sf::Vector2f target = pos + rDir * rDist;

        streak.maxLength = (14.0f + static_cast<float>(rng() % 55)) * sizeScale;
        streak.dripSpeed = 15.0f + static_cast<float>(rng() % 25);
        streak.width = (1.3f + static_cast<float>(rng() % 3)) * sizeScale;   // Поменьше толщина ручейков
        streak.active = (rng() % 5 != 0);                      // Часть застывает как точечные капли

        int cIdx = rng() % 3;
        if (cIdx == 0) streak.color = sf::Color(135, 9, 16, 230);
        else if (cIdx == 1) streak.color = sf::Color(170, 14, 22, 235);
        else streak.color = sf::Color(200, 18, 28, 240);

        placeBgStreak(streak, target);
        _bgBloodStreaks.push_back(streak);
    }

    // 2.7. СПРЕЕВИДНЫЕ БРЫЗГИ НА БГ (Fine Static Mist/Spray Droplets)
    // Мелкие капли и крапинки тумана/спрея, оседающие на задней стене в широком конусе и не стекающие вниз
    int sprayCount = std::max(25, static_cast<int>(std::round(55 * hpScale)));
    std::uniform_real_distribution<float> sprayConeAngle(-0.85f, 0.85f); // Широкий спрей-конус
    for (int i = 0; i < sprayCount; ++i) {
        BackgroundBloodStreak streak;
        streak.isSpray = true;
        streak.isBlotch = false;
        streak.length = 0.0f;
        streak.maxLength = 0.0f;
        streak.dripSpeed = 0.0f;
        streak.active = false; // НЕ СТЕКАЕТ!

        float spreadA = sprayConeAngle(rng);
        float cosA = std::cos(spreadA);
        float sinA = std::sin(spreadA);
        sf::Vector2f sprayDir = {
            baseDir.x * cosA - baseDir.y * sinA,
            baseDir.x * sinA + baseDir.y * cosA
        };
        float dist = 10.0f + static_cast<float>(rng() % 360);
        sf::Vector2f target = pos + sprayDir * dist;

        // Мелкий размер пиксельных капель спрея (1..3.5px)
        streak.width = (1.0f + static_cast<float>(rng() % 3)) * sizeScale;
        streak.blotchH = (1.0f + static_cast<float>(rng() % 3)) * sizeScale;

        int cIdx = rng() % 4;
        if (cIdx == 0) streak.color = sf::Color(140, 10, 16, 210);
        else if (cIdx == 1) streak.color = sf::Color(175, 14, 22, 220);
        else if (cIdx == 2) streak.color = sf::Color(110, 8, 12, 200);
        else streak.color = sf::Color(195, 18, 26, 225);

        placeBgStreak(streak, target);
        _bgBloodStreaks.push_back(streak);
    }
}

void Arena::update(float dt) {
    // 1. Обновление летящих капель крови и их задержка на гранях ТВЕРДЫХ блоков без пролета насквозь
    for (auto& p : _bloodParticles) {
        if (!p.active) continue;

        p.lifetime -= dt;
        if (p.lifetime <= 0.0f) {
            p.active = false;
            continue;
        }

        p.velocity.y += 980.0f * dt;
        sf::Vector2f startPos = p.position;
        sf::Vector2f targetPos = p.position + p.velocity * dt;

        sf::Vector2f hitPoint;
        sf::Vector2i hitTile;
        sf::Vector2f hitNormal;
        if (raycastSolid(startPos, targetPos, hitPoint, hitTile, hitNormal)) {
            // Частица врезалась в твердый блок!
            // 1. Запекаем полосу-декаль строго на наружных 2.5px грани блока
            float thickness = 2.5f;
            float stripLen  = std::clamp(p.radius * 2.8f, 6.0f, 18.0f);

            BloodDecal decal;
            decal.color = p.color;
            decal.thickness = thickness;

            float tileLeft   = static_cast<float>(hitTile.x * _tileSize);
            float tileRight  = static_cast<float>((hitTile.x + 1) * _tileSize);
            float tileTop    = static_cast<float>(hitTile.y * _tileSize);
            float tileBottom = static_cast<float>((hitTile.y + 1) * _tileSize);

            if (hitNormal.y < -0.5f) {
                // Попадание в верхнюю грань блока (пол)
                decal.orientation = BloodDecalOrientation::HORIZONTAL;
                float startX = std::clamp(hitPoint.x - stripLen * 0.5f, tileLeft, tileRight - stripLen);
                decal.position = {startX, tileTop};
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = thickness;
                _bloodDecals.push_back(decal);
            } else if (hitNormal.y > 0.5f) {
                // Попадание в нижнюю грань блока (потолок)
                decal.orientation = BloodDecalOrientation::HORIZONTAL;
                float startX = std::clamp(hitPoint.x - stripLen * 0.5f, tileLeft, tileRight - stripLen);
                decal.position = {startX, tileBottom - thickness};
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = thickness;
                _bloodDecals.push_back(decal);

                // Кровь задерживается на блоке и стекает вниз!
                BackgroundBloodStreak streak;
                streak.isBlotch = false;
                streak.startPos = sf::Vector2f(hitPoint.x, tileBottom);
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                streak.maxLength = 18.0f + static_cast<float>(std::rand() % 45);
                streak.dripSpeed = 22.0f + static_cast<float>(std::rand() % 28);
                streak.width = std::clamp(p.radius * 0.9f, 2.0f, 4.0f);
                streak.color = p.color;
                streak.active = true;
                _bgBloodStreaks.push_back(streak);
            } else if (hitNormal.x < -0.5f) {
                // Попадание в левую грань блока (стена)
                decal.orientation = BloodDecalOrientation::VERTICAL;
                float startY = std::clamp(hitPoint.y - stripLen * 0.5f, tileTop, tileBottom - stripLen);
                decal.position = {tileLeft, startY};
                decal.length = stripLen;
                decal.radiusX = thickness;
                decal.radiusY = stripLen * 0.5f;
                _bloodDecals.push_back(decal);

                // Стекает вниз вдоль стены
                BackgroundBloodStreak streak;
                streak.isBlotch = false;
                streak.startPos = sf::Vector2f(tileLeft + 1.0f, hitPoint.y);
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                streak.maxLength = 16.0f + static_cast<float>(std::rand() % 40);
                streak.dripSpeed = 18.0f + static_cast<float>(std::rand() % 25);
                streak.width = std::clamp(p.radius * 0.8f, 1.8f, 3.5f);
                streak.color = p.color;
                streak.active = true;
                _bgBloodStreaks.push_back(streak);
            } else {
                // Попадание в правую грань блока (стена)
                decal.orientation = BloodDecalOrientation::VERTICAL;
                float startY = std::clamp(hitPoint.y - stripLen * 0.5f, tileTop, tileBottom - stripLen);
                decal.position = {tileRight - thickness, startY};
                decal.length = stripLen;
                decal.radiusX = thickness;
                decal.radiusY = stripLen * 0.5f;
                _bloodDecals.push_back(decal);

                // Стекает вниз вдоль стены
                BackgroundBloodStreak streak;
                streak.isBlotch = false;
                streak.startPos = sf::Vector2f(tileRight - 1.0f, hitPoint.y);
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                streak.maxLength = 16.0f + static_cast<float>(std::rand() % 40);
                streak.dripSpeed = 18.0f + static_cast<float>(std::rand() % 25);
                streak.width = std::clamp(p.radius * 0.8f, 1.8f, 3.5f);
                streak.color = p.color;
                streak.active = true;
                _bgBloodStreaks.push_back(streak);
            }

            p.position = hitPoint;
            p.active = false; // Поглощается блоком и не выходит наружу!
        } else {
            p.position = targetPos;
        }
    }

    _bloodParticles.erase(std::remove_if(_bloodParticles.begin(), _bloodParticles.end(),
        [](const BloodParticle& bp) { return !bp.active; }), _bloodParticles.end());

    if (_bloodDecals.size() > 2400) {
        _bloodDecals.erase(_bloodDecals.begin(), _bloodDecals.begin() + 200);
    }

    // 2. Обновление стекающих ручейков густой крови на заднем фоне (Background Blood Dripping)
    for (auto& streak : _bgBloodStreaks) {
        if (streak.isBlotch || streak.isSpray || !streak.active) continue;

        float dripDelta = streak.dripSpeed * dt;
        float nextLength = streak.length + dripDelta;
        float nextPosY = streak.currentPos.y + dripDelta;

        // Постепенное замедление стекания капли
        streak.dripSpeed = std::max(2.0f, streak.dripSpeed - 4.0f * dt);

        // Проверяем: не дотекла ли капля до твердого блока пола
        int curTx = static_cast<int>(std::floor(streak.startPos.x / _tileSize));
        int curTy = static_cast<int>(std::floor(nextPosY / _tileSize));
        if (isBlockSolid(curTx, curTy)) {
            // Капля дотекла до блока пола: фиксируем длину ровно до верхней кромки блока!
            float floorTop = static_cast<float>(curTy * _tileSize);
            streak.length = std::max(0.0f, floorTop - streak.startPos.y);
            streak.currentPos.y = streak.startPos.y + streak.length;
            streak.active = false; // Запеклась на блоке

            // Запекаем полосу крови на поверхности блока
            if (streak.width >= 2.0f) {
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::HORIZONTAL;
                float stripLen = std::clamp(streak.width * 2.0f, 6.0f, 16.0f);
                float tileLeft = static_cast<float>(curTx * _tileSize);
                float tileRight = static_cast<float>((curTx + 1) * _tileSize);
                decal.position = {std::clamp(streak.startPos.x - stripLen * 0.5f, tileLeft, tileRight - stripLen), floorTop};
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = 2.5f;
                _bloodDecals.push_back(decal);
            }
            continue;
        }

        streak.length = nextLength;
        streak.currentPos.y = nextPosY;

        if (streak.length >= streak.maxLength) {
            streak.active = false; // Достигла предельной длины потека
        }
    }

    if (_bgBloodStreaks.size() > 1600) {
        _bgBloodStreaks.erase(_bgBloodStreaks.begin(), _bgBloodStreaks.begin() + 150);
    }

    // 3. Обновление мерцания флуоресцентных ламп лаборатории
    for (auto& lamp : _lamps) {
        if (!lamp.enabled) {
            lamp.flickerDip = 0.0f;
            continue;
        }
        if (lamp.flickering) {
            lamp.flickerTimer -= dt;
            if (lamp.flickerTimer <= 0.0f) {
                lamp.flickerTimer = static_cast<float>(rand() % 16 + 4) / 100.0f;
                lamp.flickerDip = (rand() % 100 < 35) ? (static_cast<float>(rand() % 65 + 25) / 100.0f) : 0.0f;
            }
        } else {
            lamp.flickerDip = 0.0f;
        }
    }

    // 4. Обновление кулдаунов триггеров
    for (auto& tr : _triggers) {
        if (tr.currentCooldown > 0.0f) {
            tr.currentCooldown -= dt;
            if (tr.currentCooldown < 0.0f) tr.currentCooldown = 0.0f;
        }
    }

    // 5. Обновление таймера уведомлений
    if (_notificationTimer > 0.0f) {
        _notificationTimer -= dt;
        if (_notificationTimer <= 0.0f) {
            _notificationTimer = 0.0f;
            _notificationText.clear();
        }
    }
}

void Arena::loadTileset(const std::string& texturePath) {
    _tilesetPath = texturePath;
    _tileSprites.clear();

    std::string path = resolveAssetPath(texturePath);
    if (!_tileTexture.loadFromFile(path)) {
        std::cerr << "Предупреждение: Не удалось загрузить тайлсет (" << path << "). Создание процедурного тайлсета." << std::endl;
        sf::Image img({96, 32}, sf::Color::Transparent);
        for (unsigned int x = 32; x < 64; ++x) {
            for (unsigned int y = 0; y < 32; ++y) {
                if (x == 32 || x == 63 || y == 0 || y == 31) img.setPixel({x, y}, sf::Color(45, 55, 65));
                else if ((x + y) % 7 == 0) img.setPixel({x, y}, sf::Color(75, 85, 95));
                else img.setPixel({x, y}, sf::Color(60, 70, 80));
            }
        }
        for (unsigned int x = 64; x < 96; ++x) {
            for (unsigned int y = 0; y < 32; ++y) {
                if (y == 0 || y == 16 || y == 31 || 
                    ((y < 16) && (x == 64 || x == 80 || x == 95)) ||
                    ((y >= 16) && (x == 72 || x == 88))) {
                    img.setPixel({x, y}, sf::Color(80, 50, 40));
                } else {
                    img.setPixel({x, y}, sf::Color(130, 90, 70));
                }
            }
        }
        (void)_tileTexture.loadFromImage(img);
    }

    int count = static_cast<int>(_tileTexture.getSize().x / 32);
    if (count < 3) count = 3;
    _tileSprites.reserve(count);

    float scale = _tileSize / 32.0f;
    for (int i = 0; i < count; ++i) {
        sf::Sprite sp(_tileTexture);
        sp.setTextureRect(sf::IntRect({i * 32, 0}, {32, 32}));
        sp.scale({scale, scale});
        _tileSprites.push_back(sp);
    }

    // Загрузка спрайта двери
    std::string doorPath = resolveAssetPath("assets/tiles/03_door.png");
    if (!_doorTexture.loadFromFile(doorPath)) {
        doorPath = resolveAssetPath("assets/sprites/03_door.png");
        _doorTextureLoaded = _doorTexture.loadFromFile(doorPath);
    } else {
        _doorTextureLoaded = true;
    }

    // Загрузка текстур батута (jump_pad) и шипов (spikes)
    std::string jpPath = resolveAssetPath("assets/sprites/jump_pad.png");
    if (!_jumpPadTexture.loadFromFile(jpPath)) {
        jpPath = resolveAssetPath("assets/tiles/jump_pad.png");
        _jumpPadTextureLoaded = _jumpPadTexture.loadFromFile(jpPath);
    } else {
        _jumpPadTextureLoaded = true;
    }

    std::string spkPath = resolveAssetPath("assets/sprites/spikes.png");
    if (!_spikesTexture.loadFromFile(spkPath)) {
        spkPath = resolveAssetPath("assets/tiles/spikes.png");
        _spikesTextureLoaded = _spikesTexture.loadFromFile(spkPath);
    } else {
        _spikesTextureLoaded = true;
    }

    // Шрифт для отображения целевой комнаты на двери
    std::vector<std::string> fontCandidates = {
        resolveAssetPath("assets/fonts/font.ttf"),
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };
    for (const auto& fp : fontCandidates) {
        if (!fp.empty() && std::filesystem::exists(fp) && _doorFont.openFromFile(fp)) {
            _doorFontLoaded = true;
            _cachedDoorText.emplace(_doorFont, "", 13);
            _cachedDoorText->setFillColor(sf::Color(0, 255, 230));
            _cachedDoorText->setOutlineColor(sf::Color(0, 0, 0, 220));
            _cachedDoorText->setOutlineThickness(1.5f);
            break;
        }
    }
}

void Arena::buildDefaultMap() {
    _doors.clear();
    _tilemapDirty = true;
    // 0 = воздух, 1 = пол, 2 = стена/фундамент
    _map.assign(_height, std::vector<int>(_width, 0));

    // Сплошной пол и монолитный фундамент под ним
    for (int x = 0; x < _width; ++x) {
        _map[_floorY][x] = 1;
        for (int y = _floorY + 1; y < _height; ++y) {
            _map[y][x] = 2;
        }
    }

    // Потолок (y = 0) и боковые стены-ограничители
    for (int x = 0; x < _width; ++x) {
        _map[0][x] = 2;
    }
    for (int y = 0; y < _height; ++y) {
        _map[y][0] = 2;
        _map[y][_width - 1] = 2;
    }

    float spawnX = 6.0f * _tileSize;
    float spawnY = static_cast<float>(_floorY) * _tileSize - 30.0f;
    _spawnPosition = {spawnX, spawnY};
    _enemySpawnPosition = {spawnX + 350.0f, spawnY + 2.0f};
    _enemySpawns = {{_enemySpawnPosition, 0}};
    syncEnemySpawnPositions();
}

static std::vector<std::string> tokenizeKVLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::string cur;
    bool inQuote = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (!inQuote && c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            break; // Комментарий //
        }
        if (c == '"') {
            if (inQuote) {
                tokens.push_back(cur);
                cur.clear();
                inQuote = false;
            } else {
                if (!cur.empty()) {
                    tokens.push_back(cur);
                    cur.clear();
                }
                inQuote = true;
            }
        } else if (inQuote) {
            cur += c;
        } else if (c == '{' || c == '}') {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
            tokens.push_back(std::string(1, c));
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) {
        tokens.push_back(cur);
    }
    return tokens;
}

bool Arena::parseKVConfig(const std::string& filepath) {
    std::string resolved = resolveAssetPath(filepath);
    std::ifstream file(resolved);
    if (!file.is_open()) return false;

    std::vector<std::string> blockStack;
    std::string pendingBlock = "";
    std::string line;
    std::vector<std::pair<int, std::string>> layoutRows;

    bool hasCustomSpawns = false;
    float pX = _spawnPosition.x, pY = _spawnPosition.y;
    float eX = _enemySpawnPosition.x, eY = _enemySpawnPosition.y;

    while (std::getline(file, line)) {
        auto tokens = tokenizeKVLine(line);
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
                    t++;
                    std::string keyLower = key;
                    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);

                    if (blockStack.size() >= 2) {
                        const std::string& section = blockStack[1];
                        if (section == "Settings") {
                            if (keyLower == "layout_file") _layoutFilePath = val;
                            else if (keyLower == "tileset") _tilesetPath = val;
                            else if (keyLower == "tile_size") _tileSize = std::stof(val);
                        } else if (section == "Spawns") {
                            if (keyLower == "player_x") { pX = std::stof(val); hasCustomSpawns = true; }
                            else if (keyLower == "player_y") { pY = std::stof(val); hasCustomSpawns = true; }
                            else if (keyLower == "enemy_x") { eX = std::stof(val); hasCustomSpawns = true; }
                            else if (keyLower == "enemy_y") { eY = std::stof(val); hasCustomSpawns = true; }
                        } else if (section == "Layout") {
                            if (keyLower.find("row_") == 0) {
                                try {
                                    int rowIdx = std::stoi(keyLower.substr(4));
                                    layoutRows.emplace_back(rowIdx, val);
                                } catch (...) {}
                            }
                        }
                    }
                } else {
                    pendingBlock = tok;
                }
            }
        }
    }

    if (hasCustomSpawns) {
        _spawnPosition = {pX, pY};
        _enemySpawnPosition = {eX, eY};
        _enemySpawns = {{_enemySpawnPosition, 0}};
        syncEnemySpawnPositions();
    }

    // Если указан текстовый файл сетки (например assets/config/map.txt), загружаем его
    if (!_layoutFilePath.empty() && loadFromAsciiFile(_layoutFilePath)) {
        if (hasCustomSpawns) {
            _spawnPosition = {pX, pY};
            _enemySpawnPosition = {eX, eY};
            _enemySpawns = {{_enemySpawnPosition, 0}};
            syncEnemySpawnPositions();
        }
        return true;
    }

    // Иначе используем строки Layout из map.kv
    if (!layoutRows.empty()) {
        std::sort(layoutRows.begin(), layoutRows.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });
        std::vector<std::string> rows;
        for (const auto& pair : layoutRows) {
            rows.push_back(pair.second);
        }
        applyAsciiGrid(rows);
        if (hasCustomSpawns) {
            _spawnPosition = {pX, pY};
            _enemySpawnPosition = {eX, eY};
            _enemySpawns = {{_enemySpawnPosition, 0}};
            syncEnemySpawnPositions();
        }
        return true;
    }

    return true;
}

bool Arena::loadFromAsciiFile(const std::string& asciiPath) {
    std::string resolved = resolveAssetPath(asciiPath);
    std::ifstream file(resolved);
    if (!file.is_open()) return false;

    std::vector<std::string> rows;
    std::string line;
    while (std::getline(file, line)) {
        // Удаляем '\r' при Windows line endings
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/') {
            continue; // Пропуск комментариев
        }
        if (!line.empty()) {
            rows.push_back(line);
        }
    }

    if (rows.empty()) return false;
    return applyAsciiGrid(rows);
}

bool Arena::applyAsciiGrid(const std::vector<std::string>& rows) {
    if (rows.empty()) return false;

    int newHeight = static_cast<int>(rows.size());
    int newWidth = 0;
    for (const auto& r : rows) {
        newWidth = std::max(newWidth, static_cast<int>(r.size()));
    }
    if (newWidth <= 0) return false;

    _height = newHeight;
    _width = newWidth;
    _map.assign(_height, std::vector<int>(_width, 0));
    _doors.clear();
    _enemySpawns.clear();
    _enemySpawnPositions.clear();

    bool foundPlayerSpawn = false;
    int lowestFloorRow = -1;

    for (int y = 0; y < _height; ++y) {
        const std::string& row = rows[y];
        for (int x = 0; x < static_cast<int>(row.size()); ++x) {
            char c = row[x];
            if (c == '#' || c == 'B' || c == '2') {
                _map[y][x] = 2; // Стена / монолитный блок
            } else if (c == '=' || c == '-' || c == '1') {
                _map[y][x] = 1; // Верхний слой земли / платформа
                if (y > lowestFloorRow) lowestFloorRow = y;
            } else if (c == 'D' || c == '3') {
                _map[y][x] = 3; // Тайл 3 = Дверь / портал перехода между комнатами
                _doors.push_back({x, y, -1, -1, -1, ""});
            } else if (c == 'L' || c == '4') {
                _map[y][x] = 4; // Тайл 4 = Флуоресцентная лампа лаборатории
            } else if (c == 'P') {
                _map[y][x] = 0; // Воздух
                _spawnPosition = {static_cast<float>(x * _tileSize), 
                                  static_cast<float>((y + 1) * _tileSize - 30.0f)};
                foundPlayerSpawn = true;
            } else if (c == 'E') {
                _map[y][x] = 0; // Обычный враг
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 0});
            } else if (c == 'T') {
                _map[y][x] = 0; // Жирный враг (Tank)
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 1});
            } else if (c == 'U') {
                _map[y][x] = 0; // Дуэлянт (Duelist)
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 2});
            } else if (c == 'V') {
                _map[y][x] = 0; // Пассивный дуэлянт (Duelist Passive)
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 3});
            } else if (c == 'M') {
                _map[y][x] = 0; // Манекен (Dummy)
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 4});
            } else if (c == '^' || c == 'J') {
                _map[y][x] = 0;
                ArenaTrigger tr;
                tr.x = x;
                tr.y = y;
                tr.type = 0;
                tr.power = 1000.0f;
                _triggers.push_back(tr); // JumpPad
            } else if (c == 'S' || c == 'v') {
                _map[y][x] = 0;
                ArenaTrigger tr;
                tr.x = x;
                tr.y = y;
                tr.type = 1;
                tr.power = 20.0f;
                _triggers.push_back(tr); // Spikes
            } else {
                _map[y][x] = 0; // Воздух (., пробел и т.д.)
            }
        }
    }

    if (lowestFloorRow >= 0) {
        _floorY = lowestFloorRow;
    } else {
        _floorY = std::max(2, _height - 5);
    }

    if (!foundPlayerSpawn) {
        _spawnPosition = {6.0f * _tileSize, static_cast<float>(_floorY) * _tileSize - 30.0f};
    }
    if (_enemySpawns.empty()) {
        _enemySpawns.push_back({{_spawnPosition.x + 350.0f, _spawnPosition.y + 2.0f}, 0});
    }
    syncEnemySpawnPositions();
    _enemySpawnPosition = _enemySpawnPositions[0];
    _tilemapDirty = true;
    scanLamps();

    return true;
}

bool Arena::loadMapConfig(const std::string& configPath) {
    _configPath = configPath;
    bool parsed = parseKVConfig(configPath);
    if (!parsed) {
        std::string txtResolved = resolveAssetPath("assets/config/map.txt");
        if (std::filesystem::exists(txtResolved)) {
            loadFromAsciiFile(txtResolved);
        }
    }
    return true;
}

bool Arena::reload() {
    loadTileset(_tilesetPath);
    bool ok = loadRoom(_currentRoomIndex);
    std::cout << "[ROOM CONFIG] Карта арены (комната " << _currentRoomIndex << ") перезагружена! "
              << "(Размер: " << _width << "x" << _height 
              << ", Спавн игрока: (" << _spawnPosition.x << ", " << _spawnPosition.y << ")"
              << ", Врагов: " << _enemySpawnPositions.size() << ")" 
              << std::endl;
    return ok;
}

bool Arena::saveToAsciiFile(const std::string& asciiPath) const {
    std::vector<std::string> pathsToSave;
    std::string primary = resolveAssetPath(asciiPath);
    pathsToSave.push_back(primary);

    if (std::filesystem::exists("../" + asciiPath)) {
        std::string parentPath = "../" + asciiPath;
        if (parentPath != primary) pathsToSave.push_back(parentPath);
    }
    if (std::filesystem::exists(asciiPath)) {
        if (asciiPath != primary) pathsToSave.push_back(asciiPath);
    }

    int pTx = static_cast<int>(std::round(_spawnPosition.x / _tileSize));
    int pTy = static_cast<int>(std::round((_spawnPosition.y + 30.0f) / _tileSize)) - 1;

    struct EnemyTileInfo { int x; int y; int type; };
    std::vector<EnemyTileInfo> enemyTiles;
    for (const auto& esp : _enemySpawns) {
        int ex = static_cast<int>(std::round(esp.position.x / _tileSize));
        int ey = static_cast<int>(std::round((esp.position.y + 28.0f) / _tileSize)) - 1;
        enemyTiles.push_back({ex, ey, esp.type});
    }

    std::vector<std::string> rows;
    rows.reserve(_height);
    for (int y = 0; y < _height; ++y) {
        std::string row;
        row.reserve(_width);
        for (int x = 0; x < _width; ++x) {
            if (x == pTx && y == pTy) {
                row += 'P';
            } else {
                bool isEnemy = false;
                for (const auto& et : enemyTiles) {
                    if (x == et.x && y == et.y) {
                        char cType = (et.type == 1) ? 'T' : (et.type == 2) ? 'U' : (et.type == 3) ? 'V' : 'E';
                        row += cType;
                        isEnemy = true;
                        break;
                    }
                }
                if (!isEnemy) {
                    int tile = _map[y][x];
                    if (tile == 2) row += '#';
                    else if (tile == 1) row += '=';
                    else if (tile == 3) row += 'D';
                    else if (tile == 4) row += 'L';
                    else row += ' ';
                }
            }
        }
        rows.push_back(row);
    }

    bool success = false;
    for (const auto& path : pathsToSave) {
        std::ofstream file(path);
        if (file.is_open()) {
            for (const auto& row : rows) {
                file << row << "\n";
            }
            success = true;
        }
    }
    return success;
}

bool Arena::saveToImageFile(const std::string& imagePath) const {
    std::string resolved = resolveAssetPath(imagePath);
    sf::Image img({static_cast<unsigned int>(_width), static_cast<unsigned int>(_height)}, sf::Color::Transparent);

    int pTx = static_cast<int>(std::round(_spawnPosition.x / _tileSize));
    int pTy = static_cast<int>(std::round((_spawnPosition.y + 30.0f) / _tileSize)) - 1;

    struct EnemyImgInfo { int x; int y; int type; };
    std::vector<EnemyImgInfo> enemyTiles;
    for (const auto& esp : _enemySpawns) {
        int ex = static_cast<int>(std::round(esp.position.x / _tileSize));
        int ey = static_cast<int>(std::round((esp.position.y + 28.0f) / _tileSize)) - 1;
        enemyTiles.push_back({ex, ey, esp.type});
    }

    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            if (x == pTx && y == pTy) {
                img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(0, 128, 255)); // Blue = Player
            } else {
                bool isEnemy = false;
                for (const auto& et : enemyTiles) {
                    if (x == et.x && y == et.y) {
                        sf::Color col = (et.type == 1) ? sf::Color(255, 30, 30) : 
                                        (et.type == 2) ? sf::Color(170, 70, 230) : 
                                        (et.type == 3) ? sf::Color(45, 185, 245) : sf::Color(180, 180, 180);
                        img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, col);
                        isEnemy = true;
                        break;
                    }
                }
                if (!isEnemy) {
                    int tile = _map[y][x];
                    if (tile == 2) img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(0, 0, 0)); // Black = Wall
                    else if (tile == 1) img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(76, 175, 80)); // Green = Platform
                    else if (tile == 3) img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(0, 229, 255)); // Cyan = Door
                    else if (tile == 4) img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(255, 255, 150)); // Lamp
                    else img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(255, 255, 255)); // White = Air
                }
            }
        }
    }

    bool ok = img.saveToFile(resolved);
    if (std::filesystem::exists("../" + imagePath)) {
        (void)img.saveToFile("../" + imagePath);
    }
    return ok;
}

bool Arena::loadFromImageFile(const std::string& imagePath) {
    std::string resolved = resolveAssetPath(imagePath);
    sf::Image img;
    if (!img.loadFromFile(resolved)) return false;

    int newWidth = static_cast<int>(img.getSize().x);
    int newHeight = static_cast<int>(img.getSize().y);
    if (newWidth <= 0 || newHeight <= 0) return false;

    _width = newWidth;
    _height = newHeight;
    _map.assign(_height, std::vector<int>(_width, 0));
    _doors.clear();
    _enemySpawns.clear();
    _enemySpawnPositions.clear();

    bool foundPlayerSpawn = false;
    int lowestFloorRow = -1;

    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            sf::Color c = img.getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)});
            
            if (c.a < 50 || (c.r == 25 && c.g == 30 && c.b == 42)) {
                _map[y][x] = 0;
                continue;
            }

            // Синий / Лазурный / Ярко-зеленый = Спавн игрока (P)
            if ((c.b > 180 && c.r < 100 && c.g < 150) || (c.r == 0 && c.g == 230 && c.b == 118)) {
                _map[y][x] = 0;
                _spawnPosition = {static_cast<float>(x * _tileSize), 
                                  static_cast<float>((y + 1) * _tileSize - 30.0f)};
                foundPlayerSpawn = true;
            }
            // Черный / темно-серый / серо-синий (MapEditor Wall 120, 135, 160) = Стена (#)
            else if ((c.r < 60 && c.g < 60 && c.b < 60) ||
                     (c.r >= 100 && c.r <= 140 && c.g >= 120 && c.g <= 150 && c.b >= 140 && c.b <= 180)) {
                _map[y][x] = 2;
            }
            // Зеленый / травяной / коричневый = Платформа (=)
            else if ((c.g > 140 && c.g > c.r + 30 && c.g > c.b + 30) ||
                     (c.r > 100 && c.g > 60 && c.g < 130 && c.b < 90)) {
                _map[y][x] = 1;
                if (y > lowestFloorRow) lowestFloorRow = y;
            }
            // Голубой / Лазурный = Спавн пассивного дуэлянта (Duelist Passive)
            else if ((c.r == 45 && c.g == 185 && c.b == 245) ||
                     (c.r >= 30 && c.r <= 70 && c.g >= 160 && c.g <= 210 && c.b >= 225)) {
                _map[y][x] = 0;
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 3});
            }
            // Бирюзовый / Cyan = Дверь (D) (sf::Color(0, 229, 255))
            else if (c.g > 180 && c.b > 180 && c.r < 25) {
                _map[y][x] = 3;
                bool dfound = false;
                for (const auto& d : _doors) { if (d.x == x && d.y == y) { dfound = true; break; } }
                if (!dfound) _doors.push_back({x, y, -1, -1, -1, ""});
            }
            // Светло-желтый / неоновый = Флуоресцентная лампа (L)
            else if (c.r > 230 && c.g > 230 && c.b > 120 && c.b < 200) {
                _map[y][x] = 4;
            }
            // Пурпурный / Фиолетовый = Спавн дуэлянта (Duelist Aggressive)
            else if (c.r > 150 && c.g < 100 && c.b > 150) {
                _map[y][x] = 0;
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 2});
            }
            // Красный = Спавн жирного врага (Tank)
            else if (c.r > 200 && c.g < 100 && c.b < 100) {
                _map[y][x] = 0;
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 1});
            }
            // Серый = Спавн обычного врага (Normal)
            else if (c.r > 140 && c.r < 220 && c.g > 140 && c.g < 220 && c.b > 140 && c.b < 220) {
                _map[y][x] = 0;
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                _enemySpawns.push_back({pos, 0});
            }
            // Белый / Светло-серый = Воздух (.)
            else {
                _map[y][x] = 0;
            }
        }
    }

    if (lowestFloorRow >= 0) {
        _floorY = lowestFloorRow;
    } else {
        _floorY = std::max(2, _height - 5);
    }

    if (!foundPlayerSpawn) {
        _spawnPosition = {6.0f * _tileSize, static_cast<float>(_floorY) * _tileSize - 30.0f};
    }
    if (_enemySpawns.empty()) {
        _enemySpawns.push_back({{_spawnPosition.x + 350.0f, _spawnPosition.y + 2.0f}, 0});
    }
    syncEnemySpawnPositions();
    _tilemapDirty = true;
    scanLamps();

    return true;
}

bool Arena::saveToKVCoordinates(const std::string& kvPath) const {
    std::vector<std::string> pathsToSave;
    std::string primary = resolveAssetPath(kvPath);
    pathsToSave.push_back(primary);

    if (std::filesystem::exists("../" + kvPath)) {
        std::string parentPath = "../" + kvPath;
        if (parentPath != primary) pathsToSave.push_back(parentPath);
    }
    if (std::filesystem::exists(kvPath)) {
        if (kvPath != primary) pathsToSave.push_back(kvPath);
    }

    int pTx = static_cast<int>(std::round(_spawnPosition.x / _tileSize));
    int pTy = static_cast<int>(std::round((_spawnPosition.y + 30.0f) / _tileSize)) - 1;

    int eTx = static_cast<int>(std::round(_enemySpawnPosition.x / _tileSize));
    int eTy = static_cast<int>(std::round((_enemySpawnPosition.y + 28.0f) / _tileSize)) - 1;

    std::ostringstream ss;
    ss << "// =========================================================================\n";
    ss << "// КООРДИНАТНЫЙ КОНФИГ КОМНАТЫ " << _currentRoomIndex << "\n";
    ss << "// =========================================================================\n\n";
    ss << "\"Room\"\n{\n";
    ss << "    \"width\"   \"" << _width << "\"\n";
    ss << "    \"height\"  \"" << _height << "\"\n\n";
    ss << "    \"player_spawn\"  \"" << pTx << " " << pTy << "\"\n";
    if (!_enemySpawnPositions.empty()) {
        int eTx = static_cast<int>(std::round(_enemySpawnPositions[0].x / _tileSize));
        int eTy = static_cast<int>(std::round((_enemySpawnPositions[0].y + 28.0f) / _tileSize)) - 1;
        ss << "    \"enemy_spawn\"   \"" << eTx << " " << eTy << "\"\n";
    }
    if (!_doors.empty()) {
        ss << "    \"door\"          \"" << _doors[0].x << " " << _doors[0].y << " " << _doors[0].targetRoom;
        if (_doors[0].targetDoorX >= 0 && _doors[0].targetDoorY >= 0) {
            ss << " " << _doors[0].targetDoorX << " " << _doors[0].targetDoorY;
        }
        ss << "\"\n";
        ss << "\n    // Список дверей и целевых комнат: [x  y  целевая_комната  целевая_дверь_x  целевая_дверь_y]\n";
        ss << "    \"Doors\"\n    {\n";
        for (size_t i = 0; i < _doors.size(); ++i) {
            std::string tgtStr = "-1";
            if (!_doors[i].targetRoomName.empty()) {
                tgtStr = _doors[i].targetRoomName;
            } else if (_doors[i].targetRoom > 0) {
                tgtStr = "room_" + std::to_string(_doors[i].targetRoom);
            }
            ss << "        \"door_" << i << "\"  \"" << _doors[i].x << " " << _doors[i].y << " " << tgtStr;
            if (_doors[i].targetDoorX >= 0 && _doors[i].targetDoorY >= 0) {
                ss << " " << _doors[i].targetDoorX << " " << _doors[i].targetDoorY;
            }
            ss << "\"\n";
        }
        ss << "    }\n";
    }

    ss << "\n    // Список врагов: [x  y  тип(0=обычный, 1=жирный, 2=дуэлянт)]\n";
    ss << "    \"Enemies\"\n    {\n";
    for (size_t i = 0; i < _enemySpawns.size(); ++i) {
        int ex = static_cast<int>(std::round(_enemySpawns[i].position.x / _tileSize));
        int ey = static_cast<int>(std::round((_enemySpawns[i].position.y + 28.0f) / _tileSize)) - 1;
        ss << "        \"enemy_" << i << "\"  \"" << ex << " " << ey << " " << _enemySpawns[i].type << "\"\n";
    }
    ss << "    }\n";

    ss << "\n    // Список платформ: [x  y  длина]\n";
    ss << "    \"Platforms\"\n    {\n";
    int platIdx = 0;
    for (int y = 0; y < _height; ++y) {
        int curStart = -1;
        for (int x = 0; x < _width; ++x) {
            if (_map[y][x] == 1) {
                if (curStart < 0) curStart = x;
            } else {
                if (curStart >= 0) {
                    int len = x - curStart;
                    std::string name = (y >= _height - 5) ? "floor" : ("plat_" + std::to_string(platIdx++));
                    ss << "        \"" << name << "\"  \"" << curStart << " " << y << " " << len << "\"\n";
                    curStart = -1;
                }
            }
        }
        if (curStart >= 0) {
            int len = _width - curStart;
            std::string name = (y >= _height - 5) ? "floor" : ("plat_" + std::to_string(platIdx++));
            ss << "        \"" << name << "\"  \"" << curStart << " " << y << " " << len << "\"\n";
        }
    }
    ss << "    }\n";
    ss << "}\n";

    std::string content = ss.str();
    bool success = false;
    for (const auto& path : pathsToSave) {
        std::ofstream file(path);
        if (file.is_open()) {
            file << content;
            success = true;
        }
    }
    return success;
}

bool Arena::loadFromKVCoordinates(const std::string& kvPath) {
    std::string resolved = resolveAssetPath(kvPath);
    std::ifstream file(resolved);
    if (!file.is_open()) return false;

    int newW = ARENA_WIDTH;
    int newH = ARENA_HEIGHT;
    int pX = 6, pY = 14;
    int eX = 50, eY = 14;
    bool hasP = false, hasE = false;
    std::vector<DoorBlock> doors;
    _enemySpawns.clear();
    _enemySpawnPositions.clear();

    struct Plat { int x, y, len; };
    struct Wall { int x, y, w, h; };
    std::vector<Plat> platforms;
    std::vector<Wall> walls;

    std::string line;
    std::string currentSection = "";

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto tokens = tokenizeKVLine(line);
        if (tokens.empty()) continue;

        if (tokens[0] == "Platforms" || tokens[0] == "platforms") {
            currentSection = "platforms";
            continue;
        }
        if (tokens[0] == "Walls" || tokens[0] == "walls") {
            currentSection = "walls";
            continue;
        }
        if (tokens[0] == "Enemies" || tokens[0] == "enemies") {
            currentSection = "enemies";
            continue;
        }
        if (tokens[0] == "Doors" || tokens[0] == "doors") {
            currentSection = "doors";
            continue;
        }
        if (tokens[0] == "{" || tokens[0] == "}") continue;

        if (tokens.size() >= 2) {
            std::string key = tokens[0];
            std::string val = tokens[1];
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            if (key == "width") {
                try { newW = std::stoi(val); } catch (...) {}
            } else if (key == "height") {
                try { newH = std::stoi(val); } catch (...) {}
            } else if (key == "player_spawn") {
                std::istringstream iss(val);
                iss >> pX >> pY;
                hasP = true;
            } else if (key == "enemy_spawn") {
                std::istringstream iss(val);
                iss >> eX >> eY;
                hasE = true;
            } else if (key == "door" || currentSection == "doors") {
                std::istringstream iss(val);
                int dx = 0, dy = 0;
                std::string tgtStr = "-1";
                int tdx = -1, tdy = -1;
                if (iss >> dx >> dy) {
                    if (iss >> tgtStr) {
                        iss >> tdx >> tdy;
                    }
                    int dtgt = -1;
                    std::string tgtName = "";
                    if (tgtStr != "-1" && !tgtStr.empty()) {
                        tgtName = tgtStr;
                        try {
                            dtgt = std::stoi(tgtStr);
                            if (dtgt > 0) tgtName = "room_" + std::to_string(dtgt);
                        } catch (...) {
                            if (tgtName.rfind("room_", 0) == 0) {
                                try { dtgt = std::stoi(tgtName.substr(5)); } catch (...) {}
                            }
                        }
                    }
                    doors.push_back({dx, dy, dtgt, tdx, tdy, tgtName});
                }
            } else if (currentSection == "enemies") {
                std::istringstream iss(val);
                int ex = 0, ey = 0;
                int etype = 0;
                if (iss >> ex >> ey) {
                    if (!(iss >> etype)) etype = 0;
                    _enemySpawns.push_back({{static_cast<float>(ex * _tileSize), 
                                             static_cast<float>((ey + 1) * _tileSize - 28.0f)}, etype});
                }
            } else if (currentSection == "platforms") {
                std::istringstream iss(val);
                int x = 0, y = 0, len = 0;
                if (iss >> x >> y >> len) platforms.push_back({x, y, len});
            } else if (currentSection == "walls") {
                std::istringstream iss(val);
                int x = 0, y = 0, w = 1, h = 1;
                if (iss >> x >> y >> w >> h) walls.push_back({x, y, w, h});
            }
        }
    }

    _width = std::max(20, newW);
    _height = std::max(10, newH);
    _map.assign(_height, std::vector<int>(_width, 0));
    _doors = doors;

    // Стены по краям по умолчанию
    for (int x = 0; x < _width; ++x) {
        _map[0][x] = 2;
        _map[_height - 1][x] = 2;
        _map[_height - 2][x] = 2;
        _map[_height - 3][x] = 2;
        _map[_height - 4][x] = 2;
    }
    for (int y = 0; y < _height; ++y) {
        _map[y][0] = 2;
        _map[y][_width - 1] = 2;
    }

    // Применяем пользовательские стены
    for (const auto& w : walls) {
        for (int dy = 0; dy < w.h; ++dy) {
            for (int dx = 0; dx < w.w; ++dx) {
                int cx = w.x + dx;
                int cy = w.y + dy;
                if (cx >= 0 && cx < _width && cy >= 0 && cy < _height) {
                    _map[cy][cx] = 2;
                }
            }
        }
    }

    // Применяем платформы
    int lowestFloor = -1;
    for (const auto& p : platforms) {
        for (int i = 0; i < p.len; ++i) {
            int cx = p.x + i;
            int cy = p.y;
            if (cx >= 0 && cx < _width && cy >= 0 && cy < _height) {
                _map[cy][cx] = 1;
                if (cy > lowestFloor) lowestFloor = cy;
            }
        }
    }

    // Применяем двери
    for (const auto& d : doors) {
        if (d.x >= 0 && d.x < _width && d.y >= 0 && d.y < _height) {
            _map[d.y][d.x] = 3;
        }
    }

    _floorY = (lowestFloor >= 0) ? lowestFloor : (_height - 5);

    if (hasP) {
        _spawnPosition = { static_cast<float>(pX * _tileSize), static_cast<float>((pY + 1) * _tileSize - 30.0f) };
    } else {
        _spawnPosition = { 6.0f * _tileSize, static_cast<float>(_floorY) * _tileSize - 30.0f };
    }

    if (hasE && _enemySpawns.empty()) {
        _enemySpawns.push_back({{ static_cast<float>(eX * _tileSize), static_cast<float>((eY + 1) * _tileSize - 28.0f) }, 0});
    }
    if (_enemySpawns.empty()) {
        _enemySpawns.push_back({{ _spawnPosition.x + 350.0f, _spawnPosition.y + 2.0f }, 0});
    }
    syncEnemySpawnPositions();
    _tilemapDirty = true;
    scanLamps();

    return true;
}

bool Arena::saveCurrentRoom() const {
    std::string base = "assets/maps/" + _currentRoomName;
    bool okTxt = saveToAsciiFile(base + ".txt");
    bool okPng = saveToImageFile(base + ".png");
    bool okKv  = saveToKVCoordinates(base + ".kv");

    if (_currentRoomName == "room_1") {
        saveToAsciiFile("assets/config/map.txt");
        saveToImageFile("assets/config/map.png");
    }
    return okTxt || okPng || okKv;
}

void Arena::loadKVOverlays(const std::string& kvPath) {
    std::string resolved = resolveAssetPath(kvPath);
    std::ifstream file(resolved);
    if (!file.is_open()) return;

    std::string line;
    std::string currentSection = "";
    std::vector<DoorBlock> kvDoors;
    std::vector<EnemySpawnConfig> kvEnemies;
    std::vector<ArenaTrigger> kvTriggers;
    bool hasPlayerSpawn = false;
    sf::Vector2f playerSpawnPos = _spawnPosition;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto tokens = tokenizeKVLine(line);
        if (tokens.empty()) continue;

        if (tokens[0] == "Doors" || tokens[0] == "doors") { currentSection = "doors"; continue; }
        if (tokens[0] == "Enemies" || tokens[0] == "enemies") { currentSection = "enemies"; continue; }
        if (tokens[0] == "Triggers" || tokens[0] == "triggers") { currentSection = "triggers"; continue; }
        if (tokens[0] == "Platforms" || tokens[0] == "platforms") { currentSection = "platforms"; continue; }
        if (tokens[0] == "Walls" || tokens[0] == "walls") { currentSection = "walls"; continue; }
        if (tokens[0] == "{" || tokens[0] == "}") { if (tokens[0] == "}") currentSection = ""; continue; }

        if (tokens.size() >= 2) {
            std::string key = tokens[0];
            std::string val = tokens[1];
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            if (key == "player_spawn") {
                std::istringstream iss(val);
                int px = 0, py = 0;
                if (iss >> px >> py) {
                    playerSpawnPos = { static_cast<float>(px * _tileSize), static_cast<float>((py + 1) * _tileSize - 30.0f) };
                    hasPlayerSpawn = true;
                }
            } else if (key == "door" || currentSection == "doors") {
                std::istringstream iss(val);
                int dx = 0, dy = 0;
                std::string tgtStr = "-1";
                int tdx = -1, tdy = -1;
                if (iss >> dx >> dy) {
                    if (iss >> tgtStr) {
                        iss >> tdx >> tdy;
                    }
                    int dtgt = -1;
                    std::string tgtName = "";
                    if (tgtStr != "-1" && !tgtStr.empty()) {
                        tgtName = tgtStr;
                        try {
                            dtgt = std::stoi(tgtStr);
                            if (dtgt > 0) tgtName = "room_" + std::to_string(dtgt);
                        } catch (...) {
                            if (tgtName.rfind("room_", 0) == 0) {
                                try { dtgt = std::stoi(tgtName.substr(5)); } catch (...) {}
                            }
                        }
                    }
                    kvDoors.push_back({dx, dy, dtgt, tdx, tdy, tgtName});
                }
            } else if (key.rfind("enemy", 0) == 0 || currentSection == "enemies") {
                std::istringstream iss(val);
                int ex = 0, ey = 0, etype = 0;
                if (iss >> ex >> ey) {
                    if (iss >> etype) {}
                    kvEnemies.push_back({{ static_cast<float>(ex * _tileSize),
                                           static_cast<float>((ey + 1) * _tileSize - 28.0f) }, etype});
                }
            } else if (key.rfind("trigger", 0) == 0 || currentSection == "triggers") {
                std::istringstream iss(val);
                int tx = 0, ty = 0, ttype = 0;
                float tpow = 1000.0f;
                if (iss >> tx >> ty >> ttype) {
                    if (iss >> tpow) {}
                    ArenaTrigger tr;
                    tr.x = tx;
                    tr.y = ty;
                    tr.type = ttype;
                    tr.power = tpow;
                    std::string token;
                    while (iss >> token) {
                        if (token.rfind("tag:", 0) == 0) tr.tag = token.substr(4);
                        else if (token.rfind("target:", 0) == 0) tr.targetTag = token.substr(7);
                        else if (token.rfind("action:", 0) == 0) tr.action = token.substr(7);
                        else if (token.rfind("cond:", 0) == 0) {
                            try { tr.condition = std::stoi(token.substr(5)); } catch (...) {}
                        }
                        else if (token.rfind("once:", 0) == 0) tr.triggerOnce = (token.substr(5) == "1");
                        else if (token.rfind("cd:", 0) == 0) {
                            try { tr.cooldown = std::stof(token.substr(3)); } catch (...) {}
                        }
                    }
                    kvTriggers.push_back(tr);
                }
            } else if (currentSection == "lamps" || key.rfind("lamp", 0) == 0) {
                std::istringstream iss(val);
                int lx = 0, ly = 0;
                if (iss >> lx >> ly) {
                    if (lx >= 0 && lx < _width && ly >= 0 && ly < _height) {
                        _map[ly][lx] = 4;
                    }
                }
            } else if (currentSection == "platforms") {
                std::istringstream iss(val);
                int px = 0, py = 0, plen = 0;
                if (iss >> px >> py >> plen) {
                    for (int i = 0; i < plen; ++i) {
                        int cx = px + i;
                        int cy = py;
                        if (cx >= 0 && cx < _width && cy >= 0 && cy < _height) {
                            _map[cy][cx] = 1;
                        }
                    }
                    _tilemapDirty = true;
                }
            }
        }
    }

    if (hasPlayerSpawn) {
        _spawnPosition = playerSpawnPos;
    }

    if (!kvDoors.empty()) {
        _doors = kvDoors;
        for (const auto& d : _doors) {
            if (d.y >= 0 && d.y < _height && d.x >= 0 && d.x < _width) {
                _map[d.y][d.x] = 3;
            }
        }
    }

    if (!kvEnemies.empty()) {
        _enemySpawns = kvEnemies;
        syncEnemySpawnPositions();
    }

    if (!kvTriggers.empty()) {
        _triggers = kvTriggers;
    }
}

bool Arena::loadRoomByName(const std::string& name) {
    _currentRoomName = name;
    if (name.rfind("room_", 0) == 0) {
        try { _currentRoomIndex = std::stoi(name.substr(5)); } catch (...) {}
    }
    _doors.clear();
    _triggers.clear();

    std::string base = "assets/maps/" + name;
    bool loadedBase = false;

    // 1. Проверяем PNG картинку-карту
    std::string pngPath = resolveAssetPath(base + ".png");
    if (std::filesystem::exists(pngPath)) {
        if (loadFromImageFile(pngPath)) {
            std::cout << "[ROOM] Загружена PNG-карта комнаты " << name 
                      << " (" << pngPath << ", " << _width << "x" << _height << ")" << std::endl;
            loadedBase = true;
        }
    }

    // 2. Проверяем блочный координатный KV файл
    std::string kvPath = resolveAssetPath(base + ".kv");
    if (!loadedBase && std::filesystem::exists(kvPath)) {
        if (loadFromKVCoordinates(kvPath)) {
            std::cout << "[ROOM] Загружен координатный KV-файл комнаты " << name 
                      << " (" << kvPath << ", " << _width << "x" << _height << ")" << std::endl;
            loadedBase = true;
        }
    }

    // 3. Fallback: текстовый ASCII-файл
    if (!loadedBase) {
        std::string txtPath = resolveAssetPath(base + ".txt");
        if (!std::filesystem::exists(txtPath)) {
            txtPath = resolveAssetPath("assets/config/map.txt");
        }

        _layoutFilePath = txtPath;
        loadedBase = loadFromAsciiFile(txtPath);
        std::cout << "[ROOM] Загружен текстовый файл комнаты " << name 
                  << " (" << txtPath << ", " << _width << "x" << _height << ")" << std::endl;
    }

    // Накладываем KV оверлеи (двери, связи комнат, спавны врагов, триггеры)
    if (std::filesystem::exists(kvPath)) {
        loadKVOverlays(kvPath);
    }

    scanLamps();

    return loadedBase;
}

bool Arena::loadRoom(int roomIndex) {
    if (roomIndex < 1) roomIndex = _totalRooms;
    if (roomIndex > _totalRooms) roomIndex = 1;
    _currentRoomIndex = roomIndex;
    return loadRoomByName("room_" + std::to_string(roomIndex));
}

bool Arena::nextRoom() {
    return loadRoom(_currentRoomIndex + 1);
}

bool Arena::prevRoom() {
    return loadRoom(_currentRoomIndex - 1);
}

std::string Arena::getCurrentRoomPath() const {
    return "assets/maps/room_" + std::to_string(_currentRoomIndex) + ".txt";
}

void Arena::resize(int newWidth, int newHeight) {
    if (newWidth < 20) newWidth = 20;
    if (newHeight < 10) newHeight = 10;
    if (newWidth > 300) newWidth = 300;
    if (newHeight > 100) newHeight = 100;

    std::vector<std::vector<int>> newMap(newHeight, std::vector<int>(newWidth, 0));
    int copyH = std::min(_height, newHeight);
    int copyW = std::min(_width, newWidth);
    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            newMap[y][x] = _map[y][x];
        }
    }
    _width = newWidth;
    _height = newHeight;
    _map = std::move(newMap);
    _tilemapDirty = true;
}

void Arena::clearRoom(int fillTile) {
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            _map[y][x] = fillTile;
        }
    }
    _doors.clear();
    _enemySpawns.clear();
    syncEnemySpawnPositions();
    _tilemapDirty = true;
}

bool Arena::duplicateRoom(int targetRoom) {
    if (targetRoom < 1 || targetRoom > _totalRooms || targetRoom == _currentRoomIndex) return false;
    std::string base = "assets/maps/room_" + std::to_string(targetRoom);
    saveToAsciiFile(base + ".txt");
    saveToImageFile(base + ".png");
    saveToKVCoordinates(base + ".kv");
    return true;
}

void Arena::restoreFromSnapshot(int w, int h, const std::vector<int>& tiles,
                                const std::vector<DoorBlock>& doors,
                                const std::vector<EnemySpawnConfig>& enemySpawns,
                                sf::Vector2f playerSpawn) {
    _width = w;
    _height = h;
    _map.assign(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            _map[y][x] = tiles[y * w + x];
        }
    }
    _doors = doors;
    for (const auto& d : _doors) {
        if (d.y >= 0 && d.y < _height && d.x >= 0 && d.x < _width) {
            _map[d.y][d.x] = 3;
        }
    }
    _enemySpawns = enemySpawns;
    syncEnemySpawnPositions();
    _spawnPosition = playerSpawn;
    _tilemapDirty = true;
}

bool Arena::isDoor(int tx, int ty) const {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return false;
    if (_map[ty][tx] == 3) return true;
    for (const auto& d : _doors) {
        if (d.x == tx && d.y == ty) return true;
    }
    return false;
}

int Arena::getDoorTargetRoom(int tx, int ty) const {
    for (const auto& d : _doors) {
        if (d.x == tx && d.y == ty) return d.targetRoom;
    }
    return (_currentRoomIndex % _totalRooms) + 1;
}

void Arena::setDoor(int tx, int ty, int targetRoom, int targetDoorX, int targetDoorY) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return;
    _map[ty][tx] = 3;
    std::string tgtName = (targetRoom > 0) ? ("room_" + std::to_string(targetRoom)) : "";
    for (auto& d : _doors) {
        if (d.x == tx && d.y == ty) {
            d.targetRoom = targetRoom;
            d.targetRoomName = tgtName;
            d.targetDoorX = targetDoorX;
            d.targetDoorY = targetDoorY;
            return;
        }
    }
    _doors.push_back({tx, ty, targetRoom, targetDoorX, targetDoorY, tgtName});
}

void Arena::removeDoor(int tx, int ty) {
    _doors.erase(std::remove_if(_doors.begin(), _doors.end(),
        [tx, ty](const DoorBlock& d) { return d.x == tx && d.y == ty; }), _doors.end());
    if (tx >= 0 && tx < _width && ty >= 0 && ty < _height && _map[ty][tx] == 3) {
        _map[ty][tx] = 0;
    }
}

bool Arena::checkPlayerDoorOverlap(float px, float py, float halfW, float topOffset, float bottomOffset) const {
    return getOverlappingDoorTargetRoom(px, py, halfW, topOffset, bottomOffset).has_value();
}

std::optional<int> Arena::getOverlappingDoorTargetRoom(float px, float py, float halfW, float topOffset, float bottomOffset) const {
    auto d = getOverlappingDoor(px, py, halfW, topOffset, bottomOffset);
    if (d.has_value()) return d->targetRoom;
    return std::nullopt;
}

std::optional<DoorBlock> Arena::getOverlappingDoor(float px, float py, float halfW, float topOffset, float bottomOffset) const {
    float left = px - halfW;
    float right = px + halfW - 0.001f;
    float top = py - topOffset;
    float bottom = py + bottomOffset - 0.001f;

    int minTx = static_cast<int>(std::floor(left / _tileSize));
    int maxTx = static_cast<int>(std::floor(right / _tileSize));
    int minTy = static_cast<int>(std::floor(top / _tileSize));
    int maxTy = static_cast<int>(std::floor(bottom / _tileSize));

    for (int ty = minTy; ty <= maxTy; ++ty) {
        for (int tx = minTx; tx <= maxTx; ++tx) {
            for (const auto& d : _doors) {
                if (d.x == tx && d.y == ty) return d;
            }
            if (isDoor(tx, ty)) {
                return DoorBlock{tx, ty, (_currentRoomIndex % _totalRooms) + 1, -1, -1, ""};
            }
        }
    }
    return std::nullopt;
}

void Arena::drawBlackoutMask(sf::RenderWindow& window) const {
    float roomW = static_cast<float>(_width * _tileSize);
    float roomH = static_cast<float>(_height * _tileSize);
    float bigM = 300000.0f;

    auto drawRect = [&](float x, float y, float w, float h) {
        sf::RectangleShape r({w, h});
        r.setPosition({x, y});
        r.setFillColor(sf::Color::Black);
        window.draw(r);
    };

    // Верхняя черная область
    drawRect(-bigM, -bigM, bigM * 2.0f + roomW, bigM);
    // Нижняя черная область
    drawRect(-bigM, roomH, bigM * 2.0f + roomW, bigM);
    // Левая черная область
    drawRect(-bigM, 0.0f, bigM, roomH);
    // Правая черная область
    drawRect(roomW, 0.0f, bigM, roomH);
}

bool Arena::isSolid(int tx, int ty) const {
    if (ty < 0 || ty >= _height) return true;
    if (tx < 0 || tx >= _width) {
        // Открытый проход на краях карты (на высоте ходьбы персонажа)
        if (ty >= _floorY - 4 && ty < _floorY) return false;
        return true;
    }
    // Открытый проход у самого левого и правого края внутри карты
    if (ty >= _floorY - 4 && ty < _floorY) {
        if ((tx <= 1 || tx >= _width - 2) && (_map[ty][tx] == 0 || _map[ty][tx] == 3)) {
            return false;
        }
    }
    return _map[ty][tx] == 1 || _map[ty][tx] == 2;
}

bool Arena::overlapsSolid(float px, float py, float halfW, float topOffset, float bottomOffset) const {
    float left = px - halfW;
    float right = px + halfW - 0.001f;
    float top = py - topOffset;
    float bottom = py + bottomOffset - 0.001f;

    int minTx = static_cast<int>(std::floor(left / _tileSize));
    int maxTx = static_cast<int>(std::floor(right / _tileSize));
    int minTy = static_cast<int>(std::floor(top / _tileSize));
    int maxTy = static_cast<int>(std::floor(bottom / _tileSize));

    for (int ty = minTy; ty <= maxTy; ++ty) {
        for (int tx = minTx; tx <= maxTx; ++tx) {
            if (isSolid(tx, ty)) return true;
        }
    }
    return false;
}

bool Arena::hasLineOfSight(sf::Vector2f from, sf::Vector2f to) const {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float dist = std::hypot(dx, dy);
    if (dist < 1.0f) return true;

    // Шаг проверки около 1/3 размера тайла для высокой точности без пропусков
    float step = _tileSize * 0.35f;
    int numSteps = static_cast<int>(std::ceil(dist / step));
    if (numSteps <= 1) return true;

    float stepX = dx / static_cast<float>(numSteps);
    float stepY = dy / static_cast<float>(numSteps);

    float curX = from.x;
    float curY = from.y;

    for (int i = 1; i < numSteps; ++i) {
        curX += stepX;
        curY += stepY;
        int tx = static_cast<int>(std::floor(curX / _tileSize));
        int ty = static_cast<int>(std::floor(curY / _tileSize));
        if (isSolid(tx, ty)) {
            return false; // Препятствие блокирует прямую видимость!
        }
    }
    return true;
}

int Arena::getTile(int tx, int ty) const {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return 2;
    return _map[ty][tx];
}

void Arena::setTile(int tx, int ty, int tile) {
    if (tx >= 0 && tx < _width && ty >= 0 && ty < _height) {
        int oldTile = _map[ty][tx];
        _map[ty][tx] = tile;
        _tilemapDirty = true;
        if (tile == 3) {
            bool found = false;
            for (auto& d : _doors) {
                if (d.x == tx && d.y == ty) { found = true; break; }
            }
            if (!found) {
                _doors.push_back({tx, ty, -1, -1, -1, ""});
            }
        } else {
            removeDoor(tx, ty);
        }

        if (oldTile == 4 && tile != 4) {
            removeLamp(tx, ty);
        } else if (oldTile != 4 && tile == 4) {
            addLamp(tx, ty);
        }
    }
}

namespace {
inline void addQuadColored(sf::VertexArray& va, float x, float y, float w, float h, const sf::Color& c) {
    va.append({{x, y}, c, {0.0f, 0.0f}});
    va.append({{x + w, y}, c, {0.0f, 0.0f}});
    va.append({{x + w, y + h}, c, {0.0f, 0.0f}});

    va.append({{x, y}, c, {0.0f, 0.0f}});
    va.append({{x + w, y + h}, c, {0.0f, 0.0f}});
    va.append({{x, y + h}, c, {0.0f, 0.0f}});
}
}

void Arena::rebuildTilemapVertices() {
    _tilemapVertices.clear();
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            int tile = _map[y][x];
            if (tile == 0 || tile == 3 || tile == 4) continue; // 0 = воздух, 3 = дверь, 4 = флуоресцентная лампа
            if (tile < 0 || static_cast<size_t>(tile) >= _tileSprites.size()) continue;

            float px = static_cast<float>(x * _tileSize);
            float py = static_cast<float>(y * _tileSize);
            float tx = static_cast<float>(tile * 32);
            float ty = 0.0f;
            float tw = 32.0f;
            float th = 32.0f;

            _tilemapVertices.append({{px, py}, sf::Color::White, {tx, ty}});
            _tilemapVertices.append({{px + _tileSize, py}, sf::Color::White, {tx + tw, ty}});
            _tilemapVertices.append({{px + _tileSize, py + _tileSize}, sf::Color::White, {tx + tw, ty + th}});

            _tilemapVertices.append({{px, py}, sf::Color::White, {tx, ty}});
            _tilemapVertices.append({{px + _tileSize, py + _tileSize}, sf::Color::White, {tx + tw, ty + th}});
            _tilemapVertices.append({{px, py + _tileSize}, sf::Color::White, {tx, ty + th}});
        }
    }
    _tilemapDirty = false;
}

void Arena::draw(sf::RenderWindow& window) {
    sf::View view = window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    int startX = std::max(0, static_cast<int>((center.x - size.x / 2.0f) / _tileSize) - 1);
    int endX = std::min(_width, static_cast<int>((center.x + size.x / 2.0f) / _tileSize) + 2);
    int startY = std::max(0, static_cast<int>((center.y - size.y / 2.0f) / _tileSize) - 1);
    int endY = std::min(_height, static_cast<int>((center.y + size.y / 2.0f) / _tileSize) + 2);

    // 0. Заливка фона текущей локации (пустые блоки внутри локации сохраняют привычный цвет)
    float roomW = static_cast<float>(_width * _tileSize);
    float roomH = static_cast<float>(_height * _tileSize);
    sf::RectangleShape roomBg({roomW, roomH});
    roomBg.setPosition({0.0f, 0.0f});
    roomBg.setFillColor(sf::Color(25, 28, 35));
    window.draw(roomBg);

    // 1. Пакетная отрисовка стекающей крови на заднем фоне (Batch VertexArray в 1 Draw Call)
    if (!_bgBloodStreaks.empty()) {
        sf::VertexArray bgBloodVA(sf::PrimitiveType::Triangles);
        for (const auto& streak : _bgBloodStreaks) {
            if (streak.isSpray) {
                float h = (streak.blotchH > 0.0f) ? streak.blotchH : streak.width;
                float px = std::floor(streak.startPos.x - streak.width * 0.5f);
                float py = std::floor(streak.startPos.y - (streak.blotchH > 0.0f ? streak.blotchH * 0.5f : streak.width * 0.5f));
                addQuadColored(bgBloodVA, px, py, streak.width, h, streak.color);
            } else if (streak.isBlotch) {
                float px = std::floor(streak.startPos.x - streak.width * 0.5f);
                float py = std::floor(streak.startPos.y - streak.blotchH * 0.5f);
                addQuadColored(bgBloodVA, px, py, streak.width, streak.blotchH, streak.color);
            } else if (streak.length <= 1.0f) {
                float px = std::floor(streak.startPos.x - streak.width * 0.5f);
                float py = std::floor(streak.startPos.y);
                addQuadColored(bgBloodVA, px, py, streak.width, std::max(2.0f, streak.width * 0.8f), streak.color);
            } else {
                float anchorW = streak.width * 1.35f;
                float anchorH = std::max(3.5f, streak.width * 0.85f);
                addQuadColored(bgBloodVA, std::floor(streak.startPos.x - anchorW * 0.5f), std::floor(streak.startPos.y), anchorW, anchorH, streak.color);

                addQuadColored(bgBloodVA, std::floor(streak.startPos.x - streak.width * 0.5f), std::floor(streak.startPos.y), streak.width, streak.length, streak.color);

                float headW = streak.width * 1.35f;
                float headH = std::max(4.0f, streak.width * 1.5f);
                addQuadColored(bgBloodVA, std::floor(streak.currentPos.x - headW * 0.5f), std::floor(streak.currentPos.y), headW, headH, streak.color);
            }
        }
        if (bgBloodVA.getVertexCount() > 0) {
            window.draw(bgBloodVA);
        }
    }

    // 2. Пакетная отрисовка тайлов карты в 1 Draw Call через кешированный VertexArray
    if (_tilemapDirty) {
        rebuildTilemapVertices();
    }
    sf::RenderStates tileStates(&_tileTexture);
    window.draw(_tilemapVertices, tileStates);

    // 3. Пакетная отрисовка наружных декалей крови и летящих частиц в воздухе (Batch VertexArray в 1 Draw Call)
    if (!_bloodDecals.empty() || !_bloodParticles.empty()) {
        sf::VertexArray fgBloodVA(sf::PrimitiveType::Triangles);
        for (const auto& decal : _bloodDecals) {
            float len = (decal.length > 0.0f) ? decal.length : (decal.radiusX * 2.0f);
            float thick = (decal.thickness > 0.0f) ? decal.thickness : 2.5f;
            float w = (decal.orientation == BloodDecalOrientation::HORIZONTAL) ? std::max(4.0f, len) : thick;
            float h = (decal.orientation == BloodDecalOrientation::HORIZONTAL) ? thick : std::max(4.0f, len);
            addQuadColored(fgBloodVA, std::floor(decal.position.x), std::floor(decal.position.y), w, h, decal.color);
        }

        for (const auto& bp : _bloodParticles) {
            if (!bp.active) continue;
            float pSize = std::max(2.5f, std::floor(bp.radius * 1.1f));
            addQuadColored(fgBloodVA, std::floor(bp.position.x - pSize * 0.5f), std::floor(bp.position.y - pSize * 0.5f), pSize, pSize, bp.color);
        }

        if (fgBloodVA.getVertexCount() > 0) {
            window.draw(fgBloodVA);
        }
    }

    // 4. Отрисовка настраиваемых блоков дверей
    for (const auto& d : _doors) {
        if (d.x < startX - 1 || d.x > endX + 1 || d.y < startY - 1 || d.y > endY + 1) continue;
        float dx = static_cast<float>(d.x * _tileSize);
        float dy = static_cast<float>(d.y * _tileSize);

        if (_doorTextureLoaded) {
            sf::Sprite doorSp(_doorTexture);
            float scale = _tileSize / 32.0f;
            doorSp.setScale({scale, scale});
            doorSp.setPosition({dx, dy});
            window.draw(doorSp);

            // Неоновое свечение и маркер целевой комнаты над дверью
            if (_doorFontLoaded && _cachedDoorText.has_value()) {
                std::string targetDisplay = !d.targetRoomName.empty() ? d.targetRoomName : (d.targetRoom > 0 ? ("Rm " + std::to_string(d.targetRoom)) : "?");
                _cachedDoorText->setString("-> " + targetDisplay);
                _cachedDoorText->setPosition({dx - 4.0f, dy - 18.0f});
                window.draw(*_cachedDoorText);
            }
        }
    }

    // 4.1. Отрисовка флуоресцентных лабораторных ламп
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            if (_map[y][x] == 4) {
                float lx = static_cast<float>(x * _tileSize);
                float ly = static_cast<float>(y * _tileSize);

                bool isEnabled = true;
                float flickerDip = 0.0f;
                for (const auto& l : _lamps) {
                    if (l.tileX == x && l.tileY == y) {
                        isEnabled = l.enabled;
                        flickerDip = l.flickerDip;
                        break;
                    }
                }

                sf::RectangleShape fixture({_tileSize - 6.0f, 14.0f});
                fixture.setPosition({lx + 3.0f, ly + 2.0f});
                fixture.setFillColor(sf::Color(38, 42, 50));
                fixture.setOutlineColor(sf::Color(70, 78, 92));
                fixture.setOutlineThickness(1.0f);
                window.draw(fixture);

                sf::RectangleShape mountL({4.0f, 6.0f});
                mountL.setPosition({lx + 6.0f, ly});
                mountL.setFillColor(sf::Color(80, 88, 102));
                window.draw(mountL);

                sf::RectangleShape mountR({4.0f, 6.0f});
                mountR.setPosition({lx + _tileSize - 10.0f, ly});
                mountR.setFillColor(sf::Color(80, 88, 102));
                window.draw(mountR);

                sf::RectangleShape tube({_tileSize - 18.0f, 7.0f});
                tube.setPosition({lx + 9.0f, ly + 6.0f});

                if (isEnabled) {
                    float factor = std::clamp(1.0f - flickerDip * 0.85f, 0.15f, 1.0f);
                    sf::Color tubeColor(
                        static_cast<uint8_t>(240 * factor),
                        static_cast<uint8_t>(252 * factor),
                        static_cast<uint8_t>(255 * factor)
                    );
                    tube.setFillColor(tubeColor);
                    window.draw(tube);

                    sf::RectangleShape glow({_tileSize - 14.0f, 11.0f});
                    glow.setPosition({lx + 7.0f, ly + 4.0f});
                    glow.setFillColor(sf::Color(120, 225, 255, static_cast<uint8_t>(75 * factor)));
                    window.draw(glow);
                } else {
                    tube.setFillColor(sf::Color(55, 60, 70));
                    window.draw(tube);
                }
            }
        }
    }

    // 5. Отрисовка интерактивных триггеров (батуты и шипы с текстурами, переключатели)
    for (const auto& tr : _triggers) {
        if (tr.x < startX - 1 || tr.x > endX + 1 || tr.y < startY - 1 || tr.y > endY + 1) continue;
        float tx = static_cast<float>(tr.x * _tileSize);
        float ty = static_cast<float>(tr.y * _tileSize);

        if (tr.type == 0 && _jumpPadTextureLoaded && _jumpPadTexture.getSize().x > 0) {
            sf::Sprite spr(_jumpPadTexture);
            spr.setPosition({tx, ty});
            float sx = _tileSize / static_cast<float>(_jumpPadTexture.getSize().x);
            float sy = _tileSize / static_cast<float>(_jumpPadTexture.getSize().y);
            spr.setScale({sx, sy});
            window.draw(spr);
        } else if (tr.type == 1 && _spikesTextureLoaded && _spikesTexture.getSize().x > 0) {
            sf::Sprite spr(_spikesTexture);
            spr.setPosition({tx, ty});
            float sx = _tileSize / static_cast<float>(_spikesTexture.getSize().x);
            float sy = _tileSize / static_cast<float>(_spikesTexture.getSize().y);
            spr.setScale({sx, sy});
            window.draw(spr);
        } else if (tr.type == 3) {
            sf::RectangleShape panel({32.0f, 44.0f});
            panel.setPosition({tx + 16.0f, ty + 10.0f});
            panel.setFillColor(sf::Color(22, 25, 32));
            panel.setOutlineColor(sf::Color(0, 220, 255));
            panel.setOutlineThickness(1.5f);
            window.draw(panel);

            sf::RectangleShape swBtn({16.0f, 12.0f});
            swBtn.setPosition({tx + 24.0f, ty + 24.0f});
            swBtn.setFillColor(sf::Color(45, 52, 65));
            swBtn.setOutlineColor(sf::Color(100, 200, 255));
            swBtn.setOutlineThickness(1.0f);
            window.draw(swBtn);

            sf::CircleShape led(3.0f);
            led.setPosition({tx + 29.0f, ty + 14.0f});
            led.setFillColor(sf::Color(0, 255, 180));
            window.draw(led);
        } else {
            sf::RectangleShape tBox({_tileSize, _tileSize});
            tBox.setPosition({tx, ty});
            if (tr.type == 0) {
                tBox.setFillColor(sf::Color(255, 170, 0, 160));
                tBox.setOutlineColor(sf::Color(255, 200, 50, 240));
            } else if (tr.type == 1) {
                tBox.setFillColor(sf::Color(230, 40, 40, 160));
                tBox.setOutlineColor(sf::Color(255, 80, 80, 240));
            } else {
                tBox.setFillColor(sf::Color(160, 60, 255, 120));
                tBox.setOutlineColor(sf::Color(190, 100, 255, 240));
            }
            tBox.setOutlineThickness(1.5f);
            window.draw(tBox);
        }
    }
}

bool Arena::removeTriggerAtTile(int tx, int ty) {
    auto it = std::remove_if(_triggers.begin(), _triggers.end(), [tx, ty](const ArenaTrigger& tr) {
        return tr.x == tx && tr.y == ty;
    });
    if (it != _triggers.end()) {
        _triggers.erase(it, _triggers.end());
        return true;
    }
    return false;
}

void Arena::updateTriggers(float dt, Player& player) {
    float px = player.position.x;
    float py = player.position.y;
    float halfW = 16.0f;
    float topOffset = 30.0f;
    float bottomOffset = 30.0f;
    sf::FloatRect playerBox({px - halfW, py - topOffset}, {halfW * 2.0f, topOffset + bottomOffset});

    for (auto& tr : _triggers) {
        sf::FloatRect trBox({tr.x * _tileSize, tr.y * _tileSize}, 
                            {std::max(1, tr.width) * _tileSize, std::max(1, tr.height) * _tileSize});

        bool overlap = playerBox.findIntersection(trBox).has_value();

        if (tr.type == 0 && overlap) {
            // JumpPad
            player.velocity.y = -tr.power;
            SoundManager::get().play("jump");
        } else if (tr.type == 1 && overlap) {
            // Spikes
            if (tr.currentCooldown <= 0.0f) {
                player.takeDamage(tr.power, 0.0f, -200.0f, 0.3f);
                tr.currentCooldown = 0.8f;
            }
        } else if (tr.type == 3) {
            // LightSwitch
            float dist = std::hypot(px - (tr.x + 0.5f) * _tileSize, py - (tr.y + 0.5f) * _tileSize);
            if ((overlap || dist <= 56.0f) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
                if (tr.currentCooldown <= 0.0f) {
                    std::string act = !tr.action.empty() ? tr.action : ("light:toggle " + (tr.targetTag.empty() ? "all" : tr.targetTag));
                    executeTriggerAction(act, &player, &tr);
                    tr.currentCooldown = tr.cooldown > 0.0f ? tr.cooldown : 0.5f;
                    SoundManager::get().play("hit");
                }
            }
        } else if (tr.type == 2 || tr.type == 4) {
            // TriggerZone / ScriptZone
            if (tr.condition == 0) { // ON_ENTER
                if (overlap && (!tr.triggerOnce || !tr.hasTriggered) && tr.currentCooldown <= 0.0f) {
                    executeTriggerAction(tr.action, &player, &tr);
                    tr.hasTriggered = true;
                    tr.currentCooldown = tr.cooldown > 0.0f ? tr.cooldown : 1.0f;
                }
            } else if (tr.condition == 1) { // ON_INTERACT (E)
                float dist = std::hypot(px - (tr.x + 0.5f) * _tileSize, py - (tr.y + 0.5f) * _tileSize);
                if ((overlap || dist <= 56.0f) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E) && tr.currentCooldown <= 0.0f) {
                    if (!tr.triggerOnce || !tr.hasTriggered) {
                        executeTriggerAction(tr.action, &player, &tr);
                        tr.hasTriggered = true;
                        tr.currentCooldown = tr.cooldown > 0.0f ? tr.cooldown : 0.5f;
                    }
                }
            }
        }
    }
}

void Arena::executeTriggerAction(const std::string& action, Player* player, const ArenaTrigger* tr) {
    if (action.empty()) return;

    std::istringstream stream(action);
    std::string cmd;
    while (std::getline(stream, cmd, ';')) {
        // Trim whitespace
        cmd.erase(0, cmd.find_first_not_of(" \t\r\n"));
        cmd.erase(cmd.find_last_not_of(" \t\r\n") + 1);
        if (cmd.empty()) continue;

        std::istringstream cmdStream(cmd);
        std::string verb;
        cmdStream >> verb;

        if (verb == "light:toggle") {
            std::string tag = "all";
            if (cmdStream >> tag) {}
            toggleLamp(tag);
        } else if (verb == "light:on") {
            std::string tag = "all";
            if (cmdStream >> tag) {}
            setLampState(tag, true);
        } else if (verb == "light:off") {
            std::string tag = "all";
            if (cmdStream >> tag) {}
            setLampState(tag, false);
        } else if (verb == "light:flicker") {
            std::string tag = "all";
            std::string val = "1";
            if (cmdStream >> tag >> val) {
                setLampFlicker(tag, val != "0" && val != "false");
            }
        } else if (verb == "ambient:set") {
            int r = 26, g = 30, b = 40;
            if (cmdStream >> r >> g >> b) {
                _ambientColor = sf::Color(r, g, b);
            }
        } else if (verb == "msg:show") {
            std::string rest;
            std::getline(cmdStream, rest);
            rest.erase(0, rest.find_first_not_of(" \t"));
            std::string msgText = "";
            float duration = 3.0f;
            int r = 100, g = 255, b = 180;

            if (!rest.empty() && (rest.front() == '"' || rest.front() == '\'')) {
                char quote = rest.front();
                size_t endQ = rest.find(quote, 1);
                if (endQ != std::string::npos) {
                    msgText = rest.substr(1, endQ - 1);
                    std::string after = rest.substr(endQ + 1);
                    std::istringstream extraStream(after);
                    if (extraStream >> duration) {
                        extraStream >> r >> g >> b;
                    }
                } else {
                    msgText = rest.substr(1);
                }
            } else {
                std::istringstream restStream(rest);
                restStream >> msgText >> duration >> r >> g >> b;
            }
            showNotification(msgText, duration, sf::Color(r, g, b));
        } else if (verb == "sound:play") {
            std::string snd;
            if (cmdStream >> snd) {
                SoundManager::get().play(snd);
            }
        } else if (verb == "player:heal") {
            float amount = 25.0f;
            if (cmdStream >> amount) {}
            if (player) {
                player->hp = std::min(player->maxHp, player->hp + amount);
                showNotification("+" + std::to_string(static_cast<int>(amount)) + " HP ВОССТАНОВЛЕНО", 1.5f, sf::Color(80, 255, 120));
            }
        } else if (verb == "player:damage") {
            float amount = 10.0f;
            if (cmdStream >> amount) {}
            if (player) {
                player->takeDamage(amount, 0.0f, -150.0f, 0.2f);
            }
        } else if (verb == "door:teleport") {
            std::string roomTarget;
            if (cmdStream >> roomTarget) {
                loadRoomByName(roomTarget);
            }
        }
    }
}

void Arena::scanLamps() {
    std::unordered_map<std::string, LampLight> oldProps;
    for (const auto& l : _lamps) {
        std::string key = std::to_string(l.tileX) + "_" + std::to_string(l.tileY);
        oldProps[key] = l;
    }

    _lamps.clear();
    int lampIndex = 0;
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            if (_map[y][x] == 4) {
                LampLight lamp;
                lamp.tileX = x;
                lamp.tileY = y;
                lamp.worldPos = {(x + 0.5f) * _tileSize, (y + 0.5f) * _tileSize};
                lamp.radius = 850.0f;
                lamp.intensity = 1.0f;
                lamp.color = sf::Color(230, 245, 255);
                lamp.enabled = true;
                lamp.flickering = false;
                lamp.tag = "lamp_" + std::to_string(lampIndex++);

                std::string key = std::to_string(x) + "_" + std::to_string(y);
                auto it = oldProps.find(key);
                if (it != oldProps.end()) {
                    lamp.enabled = it->second.enabled;
                    lamp.flickering = it->second.flickering;
                    if (!it->second.tag.empty()) lamp.tag = it->second.tag;
                    lamp.color = it->second.color;
                    lamp.radius = it->second.radius;
                }
                _lamps.push_back(lamp);
            }
        }
    }
}

void Arena::addLamp(int tx, int ty, const std::string& tag, bool flickering) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return;
    _map[ty][tx] = 4;
    _tilemapDirty = true;
    LampLight lamp;
    lamp.tileX = tx;
    lamp.tileY = ty;
    lamp.worldPos = {(tx + 0.5f) * _tileSize, (ty + 0.5f) * _tileSize};
    lamp.radius = 850.0f;
    lamp.intensity = 1.0f;
    lamp.color = sf::Color(230, 245, 255);
    lamp.enabled = true;
    lamp.flickering = flickering;
    lamp.tag = tag.empty() ? ("lamp_" + std::to_string(_lamps.size())) : tag;
    _lamps.push_back(lamp);
}

void Arena::removeLamp(int tx, int ty) {
    if (tx >= 0 && tx < _width && ty >= 0 && ty < _height && _map[ty][tx] == 4) {
        _map[ty][tx] = 0;
        _tilemapDirty = true;
    }
    _lamps.erase(std::remove_if(_lamps.begin(), _lamps.end(), [tx, ty](const LampLight& l) {
        return l.tileX == tx && l.tileY == ty;
    }), _lamps.end());
}

void Arena::setLampState(const std::string& tag, bool enabled) {
    for (auto& l : _lamps) {
        if (tag == "all" || l.tag == tag) {
            l.enabled = enabled;
        }
    }
}

void Arena::toggleLamp(const std::string& tag) {
    for (auto& l : _lamps) {
        if (tag == "all" || l.tag == tag) {
            l.enabled = !l.enabled;
        }
    }
}

void Arena::setLampFlicker(const std::string& tag, bool flicker) {
    for (auto& l : _lamps) {
        if (tag == "all" || l.tag == tag) {
            l.flickering = flicker;
        }
    }
}

void Arena::renderLighting(sf::RenderWindow& window, const sf::View& camera, sf::Vector2f playerPos, bool playerArmed) {
    if (!_lightingEnabled) return;

    sf::Vector2u winSize = window.getSize();
    if (winSize.x == 0 || winSize.y == 0) return;

    if (!_lightmapInitialized || _lightmap.getSize() != winSize) {
        if (!_lightmap.resize(winSize)) {
            return;
        }
        _lightmapInitialized = true;
    }

    _lightmap.setView(camera);
    _lightmap.clear(_ambientColor);

    sf::RenderStates addStates;
    addStates.blendMode = sf::BlendAdd;

    // 1. Освещение от активных флуоресцентных ламп
    for (const auto& lamp : _lamps) {
        if (!lamp.enabled) continue;

        float effectiveIntensity = lamp.intensity * std::max(0.05f, 1.0f - lamp.flickerDip);
        if (effectiveIntensity <= 0.01f) continue;

        sf::Vector2f sourcePos = {lamp.worldPos.x, lamp.worldPos.y + 6.0f};
        float radX = lamp.radius * 1.0f;
        float radY = lamp.radius * 1.55f;

        const int segments = 32;
        sf::VertexArray lightFan(sf::PrimitiveType::TriangleFan);

        sf::Color centerCol(
            static_cast<uint8_t>(std::clamp(lamp.color.r * effectiveIntensity, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(lamp.color.g * effectiveIntensity, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(lamp.color.b * effectiveIntensity, 0.0f, 255.0f)),
            255
        );
        lightFan.append({sourcePos, centerCol});

        for (int i = 0; i <= segments; ++i) {
            float angle = static_cast<float>(i) * (2.0f * 3.14159265f / segments);
            float dirY = std::sin(angle);
            float currentRadY = (dirY > 0.0f) ? radY : (radY * 0.25f);
            float lx = sourcePos.x + std::cos(angle) * radX;
            float ly = sourcePos.y + dirY * currentRadY;
            lightFan.append({{lx, ly}, sf::Color(0, 0, 0, 255)});
        }
        _lightmap.draw(lightFan, addStates);

        // Яркий точечный фокус вокруг лампы
        sf::VertexArray coreFan(sf::PrimitiveType::TriangleFan);
        coreFan.append({sourcePos, sf::Color(255, 255, 255, 255)});
        float coreRad = 85.0f * effectiveIntensity;
        for (int i = 0; i <= 16; ++i) {
            float angle = static_cast<float>(i) * (2.0f * 3.14159265f / 16);
            coreFan.append({{sourcePos.x + std::cos(angle) * coreRad * 2.2f, 
                             sourcePos.y + std::sin(angle) * coreRad * 0.8f}, sf::Color(0, 0, 0, 255)});
        }
        _lightmap.draw(coreFan, addStates);
    }

    // 2. Персональная аура игрока удалена (свет излучается строго лампами)

    _lightmap.display();

    // 3. Умножение лайтмапа на мир
    sf::RenderStates mulStates;
    mulStates.blendMode = sf::BlendMultiply;
    sf::Sprite lightSprite(_lightmap.getTexture());
    window.setView(window.getDefaultView());
    window.draw(lightSprite, mulStates);
    window.setView(camera);
}

void Arena::showNotification(const std::string& text, float duration, sf::Color col) {
    _notificationText = text;
    _notificationDuration = duration;
    _notificationTimer = duration;
    _notificationColor = col;
}

void Arena::drawNotificationUI(sf::RenderWindow& window, const sf::Font* font) {
    if (_notificationTimer <= 0.0f || _notificationText.empty() || !font) return;

    sf::Text notifText(*font, toSfString(_notificationText), 14);
    notifText.setFillColor(_notificationColor);
    notifText.setStyle(sf::Text::Bold);
    sf::FloatRect bounds = notifText.getLocalBounds();

    float padX = 24.0f;
    float padY = 8.0f;
    float boxW = bounds.size.x + padX * 2.0f;
    float boxH = bounds.size.y + padY * 2.0f;
    float winW = static_cast<float>(window.getSize().x);
    float boxX = (winW - boxW) * 0.5f;
    float boxY = 45.0f;

    sf::RectangleShape box({boxW, boxH});
    box.setPosition({boxX, boxY});
    box.setFillColor(sf::Color(8, 10, 14, 240));
    box.setOutlineColor(_notificationColor);
    box.setOutlineThickness(1.5f);
    window.draw(box);

    // Верхняя неоновая полоса
    sf::RectangleShape topStrip({boxW, 2.5f});
    topStrip.setPosition({boxX, boxY});
    topStrip.setFillColor(_notificationColor);
    window.draw(topStrip);

    notifText.setPosition({boxX + padX - bounds.position.x, boxY + padY - bounds.position.y});
    window.draw(notifText);
}
