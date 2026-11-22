#include "world/Arena.h"
#include "combat/CombatConfig.h"
#include "combat/CombatSystem.h"
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
    _map.assign(_height, std::vector<int>(_width, 0));
    _bgMap.assign(_height, std::vector<int>(_width, 0));
    _decorMap.assign(_height, std::vector<int>(_width, 0));

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
    if (t < 0) return true;
    if (static_cast<size_t>(t) < _tileSolidFlags.size()) {
        return _tileSolidFlags[t];
    }
    // Твердые блоки: платформы, стены, колонны, шипы, невидимый барьер и блоки > 8
    // Воздух (0), Дверь (3), Лампа (4), Окно (5), Фоновый блок без хитбоксов (8) — нематериальны!
    return (t == TileId::PLATFORM || t == TileId::WALL || t == TileId::PILLAR || t == TileId::HAZARD || t == TileId::BARRIER || (t > 8));
}

bool Arena::raycastSolid(sf::Vector2f start, sf::Vector2f target, sf::Vector2f& hitPoint, sf::Vector2i& hitTile, sf::Vector2f& hitNormal) const {
    float dx = target.x - start.x;
    float dy = target.y - start.y;
    float dist = std::hypot(dx, dy);
    if (dist < 0.001f) {
        int tx = static_cast<int>(std::floor(start.x / _tileSize));
        int ty = static_cast<int>(std::floor(start.y / _tileSize));
        if (isPointSolid(start.x, start.y)) {
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

    constexpr float kInvSqrt2 = 0.70710678f;

    for (int i = 0; i <= numSteps; ++i) {
        if (i > 0) {
            cur.x += stepX;
            cur.y += stepY;
        }
        int tx = static_cast<int>(std::floor(cur.x / _tileSize));
        int ty = static_cast<int>(std::floor(cur.y / _tileSize));

        if (isPointSolid(cur.x, cur.y)) {
            hitTile = {tx, ty};
            TileShape shape = getTileShape(tx, ty);
            float tileLeft   = static_cast<float>(tx * _tileSize);
            float tileRight  = static_cast<float>((tx + 1) * _tileSize);
            float tileTop    = static_cast<float>(ty * _tileSize);
            float tileBottom = static_cast<float>((ty + 1) * _tileSize);

            if (shape == TileShape::SLOPE_DOWN_RIGHT) {
                // Твердая часть: y >= tileTop + (x - tileLeft)
                // Наружная нормаль пандуса, обращенная в воздух: (kInvSqrt2, -kInvSqrt2)
                if (prev.x <= tileLeft) {
                    hitNormal = {-1.0f, 0.0f};
                    hitPoint = {tileLeft, std::clamp(cur.y, tileTop, tileBottom)};
                } else if (prev.y >= tileBottom) {
                    hitNormal = {0.0f, 1.0f};
                    hitPoint = {std::clamp(cur.x, tileLeft, tileRight), tileBottom};
                } else {
                    hitNormal = {kInvSqrt2, -kInvSqrt2};
                    float dPrev = (prev.x - tileLeft) - (prev.y - tileTop);
                    float dCur  = (cur.x - tileLeft) - (cur.y - tileTop);
                    float t = 0.5f;
                    if (std::abs(dPrev - dCur) > 0.0001f) {
                        t = std::clamp(dPrev / (dPrev - dCur), 0.0f, 1.0f);
                    }
                    hitPoint = prev + t * (cur - prev);
                    hitPoint.x = std::clamp(hitPoint.x, tileLeft, tileRight);
                    hitPoint.y = tileTop + (hitPoint.x - tileLeft);
                }
                return true;
            } else if (shape == TileShape::SLOPE_UP_RIGHT) {
                // Твердая часть: y >= tileTop + (_tileSize - (x - tileLeft))
                // Наружная нормаль пандуса, обращенная в воздух: (-kInvSqrt2, -kInvSqrt2)
                if (prev.x >= tileRight) {
                    hitNormal = {1.0f, 0.0f};
                    hitPoint = {tileRight, std::clamp(cur.y, tileTop, tileBottom)};
                } else if (prev.y >= tileBottom) {
                    hitNormal = {0.0f, 1.0f};
                    hitPoint = {std::clamp(cur.x, tileLeft, tileRight), tileBottom};
                } else {
                    hitNormal = {-kInvSqrt2, -kInvSqrt2};
                    float dPrev = (prev.y - tileTop) - (_tileSize - (prev.x - tileLeft));
                    float dCur  = (cur.y - tileTop) - (_tileSize - (cur.x - tileLeft));
                    float t = 0.5f;
                    if (std::abs(dCur - dPrev) > 0.0001f) {
                        t = std::clamp(-dPrev / (dCur - dPrev), 0.0f, 1.0f);
                    }
                    hitPoint = prev + t * (cur - prev);
                    hitPoint.x = std::clamp(hitPoint.x, tileLeft, tileRight);
                    hitPoint.y = tileTop + (_tileSize - (hitPoint.x - tileLeft));
                }
                return true;
            } else if (shape == TileShape::HALF_BOTTOM) {
                float halfTop = tileTop + _tileSize * 0.5f;
                if (prev.y <= halfTop) {
                    hitNormal = {0.0f, -1.0f};
                    hitPoint = {std::clamp(cur.x, tileLeft, tileRight), halfTop};
                } else if (prev.y >= tileBottom) {
                    hitNormal = {0.0f, 1.0f};
                    hitPoint = {std::clamp(cur.x, tileLeft, tileRight), tileBottom};
                } else if (prev.x <= tileLeft) {
                    hitNormal = {-1.0f, 0.0f};
                    hitPoint = {tileLeft, std::clamp(cur.y, halfTop, tileBottom)};
                } else {
                    hitNormal = {1.0f, 0.0f};
                    hitPoint = {tileRight, std::clamp(cur.y, halfTop, tileBottom)};
                }
                return true;
            } else {
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
            TileShape shape = getTileShape(hitTile.x, hitTile.y);
            float tileLeft = static_cast<float>(hitTile.x * _tileSize);
            float tileRight = static_cast<float>((hitTile.x + 1) * _tileSize);
            float tileTop = static_cast<float>(hitTile.y * _tileSize);
            float tileBottom = static_cast<float>((hitTile.y + 1) * _tileSize);
            if (shape == TileShape::HALF_BOTTOM) {
                tileTop += _tileSize * 0.5f;
            }

            if (shape == TileShape::SLOPE_DOWN_RIGHT) {
                // Удар в наклонную грань лестницы (вниз-вправо)
                float lx = std::clamp(hitPoint.x - tileLeft, 0.0f, _tileSize);
                sf::Vector2f slopePoint(tileLeft + lx, tileTop + lx);
                streak.startPos = slopePoint;
                streak.currentPos = slopePoint;
                streak.length = 0.0f;
                streak.active = false;
                if (streak.isBlotch) {
                    streak.width = 0.0f;
                    streak.blotchH = 0.0f;
                }
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::SLOPE_DOWN_RIGHT;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                decal.position = slopePoint;
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = 2.5f;
                _bloodDecals.push_back(decal);
            } else if (shape == TileShape::SLOPE_UP_RIGHT) {
                // Удар в наклонную грань лестницы (вверх-вправо)
                float lx = std::clamp(hitPoint.x - tileLeft, 0.0f, _tileSize);
                sf::Vector2f slopePoint(tileLeft + lx, tileTop + (_tileSize - lx));
                streak.startPos = slopePoint;
                streak.currentPos = slopePoint;
                streak.length = 0.0f;
                streak.active = false;
                if (streak.isBlotch) {
                    streak.width = 0.0f;
                    streak.blotchH = 0.0f;
                }
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::SLOPE_UP_RIGHT;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                decal.position = slopePoint;
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = 2.5f;
                _bloodDecals.push_back(decal);
            } else if (hitNormal.y > 0.5f) {
                // Удар в нижнюю кромку потолка
                streak.startPos = sf::Vector2f(hitPoint.x, tileBottom);
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                if (streak.maxLength > 0.0f) {
                    streak.active = true;
                }
                if (streak.isBlotch) {
                    streak.width = 0.0f;
                    streak.blotchH = 0.0f;
                }
                // Задерживается на блоке: запекаем декаль на кромке потолка
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::HORIZONTAL;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                decal.position = {std::clamp(hitPoint.x - stripLen * 0.5f, tileLeft, tileRight - stripLen), tileBottom - 2.5f};
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = 2.5f;
                _bloodDecals.push_back(decal);
            } else if (std::abs(hitNormal.x) > 0.5f) {
                // Удар в боковую грань стены
                float wallX = (hitNormal.x < 0.0f) ? tileLeft : tileRight;
                streak.startPos = sf::Vector2f(wallX, hitPoint.y);
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                if (streak.maxLength > 0.0f) {
                    streak.active = true;
                }
                if (streak.isBlotch) {
                    streak.width = 0.0f;
                    streak.blotchH = 0.0f;
                }
                // Задерживается на блоке: запекаем декаль на кромке стены
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::VERTICAL;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                float dX = (hitNormal.x < 0.0f) ? wallX : (wallX - 2.5f);
                decal.position = {dX, std::clamp(hitPoint.y - stripLen * 0.5f, tileTop, tileBottom - stripLen)};
                decal.length = stripLen;
                decal.radiusX = 2.5f;
                decal.radiusY = stripLen * 0.5f;
                _bloodDecals.push_back(decal);
            } else {
                // Удар в верхнюю кромку пола
                streak.startPos = sf::Vector2f(hitPoint.x, tileTop);
                streak.currentPos = streak.startPos;
                streak.length = 0.0f;
                streak.active = false; // На полу не стекает вниз сквозь пол
                if (streak.isBlotch) {
                    streak.width = 0.0f;
                    streak.blotchH = 0.0f;
                }
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                decal.orientation = BloodDecalOrientation::HORIZONTAL;
                float stripLen = std::clamp(streak.width * 2.2f, 6.0f, 20.0f);
                decal.position = {std::clamp(hitPoint.x - stripLen * 0.5f, tileLeft, tileRight - stripLen), tileTop};
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
    updateInteractiveDoors(dt, nullptr);
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

            TileShape shape = getTileShape(hitTile.x, hitTile.y);
            if (shape == TileShape::SLOPE_DOWN_RIGHT) {
                float lx = std::clamp(hitPoint.x - tileLeft, 0.0f, _tileSize);
                sf::Vector2f slopePoint(tileLeft + lx, tileTop + lx);
                decal.orientation = BloodDecalOrientation::SLOPE_DOWN_RIGHT;
                decal.position = slopePoint;
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = thickness;
                _bloodDecals.push_back(decal);
                p.position = slopePoint;
                p.active = false;
                continue;
            } else if (shape == TileShape::SLOPE_UP_RIGHT) {
                float lx = std::clamp(hitPoint.x - tileLeft, 0.0f, _tileSize);
                sf::Vector2f slopePoint(tileLeft + lx, tileTop + (_tileSize - lx));
                decal.orientation = BloodDecalOrientation::SLOPE_UP_RIGHT;
                decal.position = slopePoint;
                decal.length = stripLen;
                decal.radiusX = stripLen * 0.5f;
                decal.radiusY = thickness;
                _bloodDecals.push_back(decal);
                p.position = slopePoint;
                p.active = false;
                continue;
            }

            if (shape == TileShape::HALF_BOTTOM) {
                tileTop += _tileSize * 0.5f;
            }

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

        // Проверяем: не дотекла ли капля до твердого блока пола или наклонной лестницы
        int curTx = static_cast<int>(std::floor(streak.startPos.x / _tileSize));
        int curTy = static_cast<int>(std::floor(nextPosY / _tileSize));
        if (isPointSolid(streak.startPos.x, nextPosY)) {
            // Капля дотекла до поверхности пола или лестницы!
            float floorTop = static_cast<float>(curTy * _tileSize);
            TileShape shape = getTileShape(curTx, curTy);
            if (shape == TileShape::SLOPE_DOWN_RIGHT) {
                float lx = streak.startPos.x - static_cast<float>(curTx * _tileSize);
                floorTop += std::clamp(lx, 0.0f, _tileSize);
            } else if (shape == TileShape::SLOPE_UP_RIGHT) {
                float lx = streak.startPos.x - static_cast<float>(curTx * _tileSize);
                floorTop += std::clamp(_tileSize - lx, 0.0f, _tileSize);
            } else if (shape == TileShape::HALF_BOTTOM) {
                floorTop += _tileSize * 0.5f;
            }

            streak.length = std::max(0.0f, floorTop - streak.startPos.y);
            streak.currentPos.y = floorTop;
            streak.active = false; // Запеклась на блоке

            // Запекаем полосу крови на поверхности блока
            if (streak.width >= 2.0f) {
                BloodDecal decal;
                decal.color = streak.color;
                decal.thickness = 2.5f;
                float stripLen = std::clamp(streak.width * 2.0f, 6.0f, 16.0f);
                if (shape == TileShape::SLOPE_DOWN_RIGHT) {
                    decal.orientation = BloodDecalOrientation::SLOPE_DOWN_RIGHT;
                    decal.position = {streak.startPos.x, floorTop};
                } else if (shape == TileShape::SLOPE_UP_RIGHT) {
                    decal.orientation = BloodDecalOrientation::SLOPE_UP_RIGHT;
                    decal.position = {streak.startPos.x, floorTop};
                } else {
                    decal.orientation = BloodDecalOrientation::HORIZONTAL;
                    float tileLeft = static_cast<float>(curTx * _tileSize);
                    float tileRight = static_cast<float>((curTx + 1) * _tileSize);
                    decal.position = {std::clamp(streak.startPos.x - stripLen * 0.5f, tileLeft, tileRight - stripLen), floorTop};
                }
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

    // 3. Обновление мерцания флуоресцентных ламп лаборатории (редкое естественное моргание раз в 5-7 секунд)
    for (auto& lamp : _lamps) {
        if (!lamp.enabled) {
            lamp.flickerDip = 0.0f;
            lamp.flickerEventTimer = 0.0f;
            continue;
        }

        // Если включен режим непрерывного аварийного/сломанного мерцания
        if (lamp.flickering) {
            lamp.flickerTimer -= dt;
            if (lamp.flickerTimer <= 0.0f) {
                lamp.flickerTimer = static_cast<float>(rand() % 16 + 4) / 100.0f;
                lamp.flickerDip = (rand() % 100 < 35) ? (static_cast<float>(rand() % 65 + 25) / 100.0f) : 0.0f;
            }
            continue;
        }

        // Естественное редкое моргание (раз в 5-7 секунд, каждая лампа в свое случайное время)
        if (lamp.flickerEventTimer > 0.0f) {
            lamp.flickerEventTimer -= dt;
            if (lamp.flickerEventTimer <= 0.0f) {
                // Моргание завершилось — свет снова стабилен
                lamp.flickerDip = 0.0f;
                lamp.flickerEventTimer = 0.0f;
                // Назначаем следующее моргание через 5.0 - 7.5 секунд (случайный разброс)
                lamp.nextFlickerTimer = 5.0f + static_cast<float>(rand() % 251) / 100.0f;
            } else {
                float progress = 1.0f - (lamp.flickerEventTimer / lamp.flickerDuration);
                // Аутентичный люминесцентный микро-спад напряжения
                if (lamp.flickerPattern == 0) {
                    // Одиночный микро-провал (синусная полуволна)
                    float pulse = std::sin(progress * 3.14159265f);
                    lamp.flickerDip = pulse * 0.35f; // легкое притухание на ~35%
                } else if (lamp.flickerPattern == 1) {
                    // Двойной быстрый щелчок
                    float pulse1 = std::sin(std::clamp(progress * 2.0f, 0.0f, 1.0f) * 3.14159265f);
                    float pulse2 = std::sin(std::clamp((progress - 0.5f) * 2.0f, 0.0f, 1.0f) * 3.14159265f);
                    lamp.flickerDip = std::max(pulse1 * 0.40f, pulse2 * 0.28f);
                } else {
                    // Короткий трепет (два спада подряд)
                    float twitch = (progress > 0.1f && progress < 0.45f) ? 0.38f :
                                   ((progress > 0.55f && progress < 0.85f) ? 0.22f : 0.0f);
                    lamp.flickerDip = twitch;
                }
            }
        } else {
            lamp.flickerDip = 0.0f;
            lamp.nextFlickerTimer -= dt;
            if (lamp.nextFlickerTimer <= 0.0f) {
                // Начинаем новое короткое моргание (~0.12 - 0.22 сек)
                lamp.flickerDuration = 0.12f + static_cast<float>(rand() % 11) / 100.0f;
                lamp.flickerEventTimer = lamp.flickerDuration;
                lamp.flickerPattern = rand() % 3;
            }
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

    // 1. Поиск всех спрайтшитов блоков в папке assets/tiles/blocks/
    std::vector<std::string> blockSheetPaths;
    std::string blocksDir = resolveAssetPath("assets/tiles/blocks");
    if (std::filesystem::exists(blocksDir) && std::filesystem::is_directory(blocksDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(blocksDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                blockSheetPaths.push_back(entry.path().string());
            }
        }
    }

    // Базовый лист BlocksTest.png должен идти первым для фиксации тайла 1 (платформа) и 2 (стена)
    std::sort(blockSheetPaths.begin(), blockSheetPaths.end(), [](const std::string& a, const std::string& b) {
        bool aIsBase = (a.find("BlocksTest") != std::string::npos || a.find("01_") != std::string::npos);
        bool bIsBase = (b.find("BlocksTest") != std::string::npos || b.find("01_") != std::string::npos);
        if (aIsBase && !bIsBase) return true;
        if (!aIsBase && bIsBase) return false;
        return a < b;
    });

    if (blockSheetPaths.empty()) {
        std::string fallbackPath = resolveAssetPath(texturePath);
        if (std::filesystem::exists(fallbackPath)) {
            blockSheetPaths.push_back(fallbackPath);
        }
    }

    // 2. Автоматическое разделение по 32 пикселя по X и Y для каждого найденного спрайтшита
    struct TileSlice {
        sf::Image imgSlice;
        bool isSolid = true;
    };
    std::vector<TileSlice> allSlices;

    auto load32Img = [](const std::string& path, sf::Color fallbackColor) -> sf::Image {
        sf::Image img;
        if (std::filesystem::exists(path)) {
            (void)img.loadFromFile(path);
        }
        if (img.getSize().x == 0 || img.getSize().y == 0) {
            img.resize({32, 32}, fallbackColor);
        }
        return img;
    };

    // Срез 0: Пустота / Воздух (Air) - прозрачный 32x32
    {
        TileSlice airSlice;
        airSlice.imgSlice.resize({32, 32}, sf::Color::Transparent);
        airSlice.isSolid = false;
        allSlices.push_back(std::move(airSlice));
    }
    // Срез 1: Платформа (01_platform.png)
    {
        std::string p = resolveAssetPath("assets/tiles/blocks/01_platform.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/01_platform.png");
        TileSlice s1;
        s1.imgSlice = load32Img(p, sf::Color(60, 70, 80));
        s1.isSolid = true;
        allSlices.push_back(std::move(s1));
    }
    // Срез 2: Стена (02_wall.png)
    {
        std::string p = resolveAssetPath("assets/tiles/blocks/02_wall.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/02_wall.png");
        TileSlice s2;
        s2.imgSlice = load32Img(p, sf::Color(130, 90, 70));
        s2.isSolid = true;
        allSlices.push_back(std::move(s2));
    }
    // Срез 3: Дверь (03_door.png)
    {
        std::string p = resolveAssetPath("assets/tiles/tools/03_door.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/03_door.png");
        TileSlice s3;
        s3.imgSlice = load32Img(p, sf::Color(0, 200, 255));
        s3.isSolid = false;
        allSlices.push_back(std::move(s3));
    }
    // Срез 4: Лампа (04_lamp.png)
    {
        std::string p = resolveAssetPath("assets/tiles/decor/04_lamp.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/blocks/04_lamp.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/04_lamp.png");
        TileSlice s4;
        s4.imgSlice = load32Img(p, sf::Color(255, 255, 180));
        s4.isSolid = false;
        allSlices.push_back(std::move(s4));
    }
    // Срез 5: Окно (05_window.png)
    {
        std::string p = resolveAssetPath("assets/tiles/decor/05_window.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/05_window.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/blocks/05_window.png");
        TileSlice s5;
        s5.imgSlice = load32Img(p, sf::Color(100, 180, 240));
        s5.isSolid = false;
        allSlices.push_back(std::move(s5));
    }
    // Срез 6: Колонна (04_pillar.png)
    {
        std::string p = resolveAssetPath("assets/tiles/decor/04_pillar.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/blocks/04_pillar.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/04_pillar.png");
        TileSlice s6;
        s6.imgSlice = load32Img(p, sf::Color(130, 135, 150));
        s6.isSolid = true;
        allSlices.push_back(std::move(s6));
    }
    // Срез 7: Лава / Опасность (05_hazard.png)
    {
        std::string p = resolveAssetPath("assets/tiles/decor/05_hazard.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/blocks/05_hazard.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/05_hazard.png");
        TileSlice s7;
        s7.imgSlice = load32Img(p, sf::Color(210, 60, 20));
        s7.isSolid = true;
        allSlices.push_back(std::move(s7));
    }
    // Срез 8: Фоновый блок без хитбоксов (08_bg.png)
    {
        std::string p = resolveAssetPath("assets/tiles/background/08_bg.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/decor/08_bg.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/blocks/08_bg.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/08_bg.png");
        TileSlice s8;
        s8.imgSlice = load32Img(p, sf::Color(35, 40, 52));
        s8.isSolid = false;
        allSlices.push_back(std::move(s8));
    }

    struct DirConfig {
        std::string path;
        bool isSolid;
        bool isStair;
    };
    std::vector<DirConfig> extraDirs = {
        {resolveAssetPath("assets/tiles/blocks"), true, false},
        {resolveAssetPath("assets/tiles/stairs"), true, true},
        {resolveAssetPath("assets/tiles/decor"), false, false},
        {resolveAssetPath("assets/tiles/background"), false, false}
    };
    for (const auto& dcfg : extraDirs) {
        const auto& dirPath = dcfg.path;
        if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) continue;
        std::vector<std::string> curFiles;
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                curFiles.push_back(entry.path().string());
            }
        }
        std::sort(curFiles.begin(), curFiles.end());
        for (const auto& sheetPath : curFiles) {
            std::string fname = std::filesystem::path(sheetPath).filename().string();
            if (fname == "01_platform.png" || fname == "02_wall.png" || fname == "04_lamp.png" ||
                fname == "05_window.png" || fname == "04_pillar.png" || fname == "05_hazard.png" ||
                fname == "08_bg.png" || fname == "03_door.png" || fname == "jump_pad.png" || fname == "spikes.png" ||
                fname == "player_spawn.png" || fname.find("enemy_") == 0) {
                continue;
            }

            sf::Image sheetImg;
            if (!sheetImg.loadFromFile(sheetPath)) continue;

            unsigned int sheetW = sheetImg.getSize().x;
            unsigned int sheetH = sheetImg.getSize().y;
            if (sheetW == 0 || sheetH == 0) continue;

            int cols = static_cast<int>(sheetW / 32);
            int rows = static_cast<int>(sheetH / 32);
            if (cols <= 0) cols = 1;
            if (rows <= 0) rows = 1;

            bool isBlocksTest = (fname.find("BlocksTest") != std::string::npos);

            int sliceIdx = 0;
            for (int ry = 0; ry < rows; ++ry) {
                for (int rx = 0; rx < cols; ++rx) {
                    if (isBlocksTest && sliceIdx < 3) {
                        // В BlocksTest первые 3 среза (воздух, платформа, стена) уже в базовых
                        sliceIdx++;
                        continue;
                    }
                    sliceIdx++;

                    unsigned int startX = rx * 32;
                    unsigned int startY = ry * 32;

                    // Пропускаем полностью пустые/прозрачные ячейки тайлсета
                    bool isSliceEmpty = true;
                    for (unsigned int py = 0; py < 32 && (startY + py) < sheetH; ++py) {
                        for (unsigned int px = 0; px < 32 && (startX + px) < sheetW; ++px) {
                            if (sheetImg.getPixel({startX + px, startY + py}).a > 5) {
                                isSliceEmpty = false;
                                break;
                            }
                        }
                        if (!isSliceEmpty) break;
                    }
                    if (isSliceEmpty) {
                        continue;
                    }

                    TileSlice slice;
                    slice.imgSlice.resize({32, 32}, sf::Color::Transparent);
                    slice.isSolid = dcfg.isSolid;
                    for (unsigned int py = 0; py < 32 && (startY + py) < sheetH; ++py) {
                        for (unsigned int px = 0; px < 32 && (startX + px) < sheetW; ++px) {
                            slice.imgSlice.setPixel({px, py}, sheetImg.getPixel({startX + px, startY + py}));
                        }
                    }

                    int sliceCode = static_cast<int>(allSlices.size());
                    if (dcfg.isStair) {
                        if (rows >= 2) {
                            if (ry == 0) {
                                if (rx == 0) _tileShapes[sliceCode] = TileShape::SLOPE_UP_RIGHT;
                                else if (rx == 1) _tileShapes[sliceCode] = TileShape::SLOPE_DOWN_RIGHT;
                                else _tileShapes[sliceCode] = TileShape::HALF_BOTTOM;
                            } else {
                                _tileShapes[sliceCode] = TileShape::FULL;
                            }
                        } else {
                            if (rx == 0) _tileShapes[sliceCode] = TileShape::SLOPE_UP_RIGHT;
                            else _tileShapes[sliceCode] = TileShape::SLOPE_DOWN_RIGHT;
                        }
                    }

                    allSlices.push_back(std::move(slice));
                }
            }
        }
    }

    // Процедурный fallback если ни один лист не найден
    if (allSlices.size() <= 1) {
        TileSlice plat;
        plat.imgSlice.resize({32, 32}, sf::Color(60, 70, 80));
        allSlices.push_back(std::move(plat));

        TileSlice wall;
        wall.imgSlice.resize({32, 32}, sf::Color(130, 90, 70));
        allSlices.push_back(std::move(wall));
    }

    // 3. Формируем единый горизонтальный атлас тайлов (все срезы по 32px в ряд)
    unsigned int totalCount = static_cast<unsigned int>(allSlices.size());
    sf::Image combinedImg({totalCount * 32, 32}, sf::Color::Transparent);
    for (unsigned int i = 0; i < totalCount; ++i) {
        for (unsigned int py = 0; py < 32; ++py) {
            for (unsigned int px = 0; px < 32; ++px) {
                combinedImg.setPixel({i * 32 + px, py}, allSlices[i].imgSlice.getPixel({px, py}));
            }
        }
    }

    (void)_tileTexture.loadFromImage(combinedImg);

    float scale = _tileSize / 32.0f;
    _tileSolidFlags.clear();
    _tileSolidFlags.reserve(totalCount);
    _tileSprites.reserve(totalCount);
    for (unsigned int i = 0; i < totalCount; ++i) {
        _tileSolidFlags.push_back(allSlices[i].isSolid);
        sf::Sprite sp(_tileTexture);
        sp.setTextureRect(sf::IntRect({static_cast<int>(i * 32), 0}, {32, 32}));
        sp.scale({scale, scale});
        _tileSprites.push_back(sp);
    }

    // Загрузка спрайта двери
    std::string doorPath = resolveAssetPath("assets/tiles/tools/03_door.png");
    if (!std::filesystem::exists(doorPath)) doorPath = resolveAssetPath("assets/tiles/03_door.png");
    if (!_doorTexture.loadFromFile(doorPath)) {
        doorPath = resolveAssetPath("assets/sprites/03_door.png");
        _doorTextureLoaded = _doorTexture.loadFromFile(doorPath);
    } else {
        _doorTextureLoaded = true;
    }

    // Загрузка текстур батута (jump_pad) и шипов (spikes)
    std::string jpPath = resolveAssetPath("assets/tiles/tools/jump_pad.png");
    if (!std::filesystem::exists(jpPath)) jpPath = resolveAssetPath("assets/tiles/jump_pad.png");
    if (!std::filesystem::exists(jpPath)) jpPath = resolveAssetPath("assets/sprites/jump_pad.png");
    _jumpPadTextureLoaded = _jumpPadTexture.loadFromFile(jpPath);

    std::string spkPath = resolveAssetPath("assets/tiles/tools/spikes.png");
    if (!std::filesystem::exists(spkPath)) spkPath = resolveAssetPath("assets/tiles/spikes.png");
    if (!std::filesystem::exists(spkPath)) spkPath = resolveAssetPath("assets/sprites/spikes.png");
    _spikesTextureLoaded = _spikesTexture.loadFromFile(spkPath);

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
    _bgMap.assign(_height, std::vector<int>(_width, 0));
    _decorMap.assign(_height, std::vector<int>(_width, 0));
    _doors.clear();
    _enemySpawns.clear();
    _enemySpawnPositions.clear();

    bool foundPlayerSpawn = false;
    int lowestFloorRow = -1;

    struct CharTileBinding {
        int tileId;
    };

    static const auto s_asciiTiles = []() {
        std::unordered_map<char, int> m = {
            {'#', TileId::WALL}, {'2', TileId::WALL},
            {'=', TileId::PLATFORM}, {'-', TileId::PLATFORM}, {'1', TileId::PLATFORM},
            {'D', TileId::DOOR}, {'3', TileId::DOOR},
            {'L', TileId::LAMP}, {'4', TileId::LAMP},
            {'W', TileId::WINDOW}, {'w', TileId::WINDOW}, {'5', TileId::WINDOW},
            {'C', TileId::PILLAR}, {'c', TileId::PILLAR}, {'6', TileId::PILLAR},
            {'H', TileId::HAZARD}, {'h', TileId::HAZARD}, {'7', TileId::HAZARD},
            {'B', TileId::BG_DECOR}, {'b', TileId::BG_DECOR}, {'8', TileId::BG_DECOR},
            {'9', 9},
            {'/', 39}, {'\\', 40}
        };
        for (char ch = 'a'; ch <= 'z'; ++ch) {
            m[ch] = 10 + (ch - 'a');
        }
        return m;
    }();

    static const std::unordered_map<char, int> s_asciiEnemies = {
        {'E', 0}, // Обычный враг
        {'N', 0}, // Обычный враг
        {'T', 1}, // Жирный враг (Tank)
        {'U', 2}, // Дуэлянт (Duelist)
        {'V', 3}, // Пассивный дуэлянт (Duelist Passive)
        {'M', 4}, // Манекен (Dummy)
        {'K', 5}  // БОСС 1 (Boss 1)
    };

    for (int y = 0; y < _height; ++y) {
        const std::string& row = rows[y];
        for (int x = 0; x < static_cast<int>(row.size()); ++x) {
            char c = row[x];

            auto itTile = s_asciiTiles.find(c);
            if (itTile != s_asciiTiles.end()) {
                int tid = itTile->second;
                if (tid == TileId::BG_DECOR) {
                    _bgMap[y][x] = 8;
                    _map[y][x] = 0;
                } else if (tid == TileId::PILLAR || tid == TileId::HAZARD) {
                    _decorMap[y][x] = tid;
                    _map[y][x] = 0;
                } else {
                    _map[y][x] = tid;
                }
                if (tid == TileId::PLATFORM && y > lowestFloorRow) {
                    lowestFloorRow = y;
                } else if (tid == TileId::DOOR) {
                    _doors.push_back({x, y, -1, -1, -1, ""});
                }
                continue;
            }

            if (c >= '8' && c <= '9') {
                _map[y][x] = (c - '0');
                continue;
            }
            if (c >= 'a' && c <= 'z') {
                _map[y][x] = 10 + (c - 'a');
                continue;
            }

            auto itEnemy = s_asciiEnemies.find(c);
            if (itEnemy != s_asciiEnemies.end()) {
                _map[y][x] = TileId::AIR;
                sf::Vector2f pos = {static_cast<float>(x * _tileSize), 
                                    static_cast<float>((y + 1) * _tileSize - 28.0f)};
                std::string eid = (itEnemy->second == 5) ? "boss_1" : "";
                _enemySpawns.push_back({pos, itEnemy->second, eid});
                continue;
            }

            if (c == 'P') {
                _map[y][x] = TileId::AIR;
                _spawnPosition = {static_cast<float>(x * _tileSize), 
                                  static_cast<float>((y + 1) * _tileSize - 30.0f)};
                foundPlayerSpawn = true;
            } else if (c == '^' || c == 'J') {
                _map[y][x] = TileId::AIR;
                ArenaTrigger tr;
                tr.x = x;
                tr.y = y;
                tr.type = 0;
                tr.power = 1000.0f;
                _triggers.push_back(tr); // JumpPad
            } else if (c == 'S' || c == 'v') {
                _map[y][x] = TileId::AIR;
                ArenaTrigger tr;
                tr.x = x;
                tr.y = y;
                tr.type = 1;
                tr.power = 20.0f;
                _triggers.push_back(tr); // Spikes
            } else {
                _map[y][x] = TileId::AIR; // Воздух (., пробел и т.д.)
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
    scanWindows();

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
                    else if (tile == 5) row += 'W';
                    else if (tile == 6) row += 'C';
                    else if (tile == 7) row += 'H';
                    else if (tile >= 8 && tile <= 9) row += static_cast<char>('0' + tile);
                    else if (tile >= 10 && tile <= 35) row += static_cast<char>('a' + (tile - 10));
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

    std::ostringstream ss;
    ss << "// =========================================================================\n";
    ss << "// КООРДИНАТНЫЙ КОНФИГ КОМНАТЫ " << _currentRoomIndex << "\n";
    ss << "// =========================================================================\n\n";
    ss << "\"Room\"\n{\n";
    ss << "    \"floor_type\"   \"" << (_floorType == FloorType::BASEMENT ? "basement" : "normal") << "\"\n";
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

    if (!_interactiveDoors.empty()) {
        ss << "\n    // Интерактивные физические двери: [x  y  isOpen  isBroken]\n";
        ss << "    \"InteractiveDoors\"\n    {\n";
        for (size_t i = 0; i < _interactiveDoors.size(); ++i) {
            ss << "        \"door_" << i << "\"  \"" << _interactiveDoors[i].x << " " << _interactiveDoors[i].y
               << " " << (_interactiveDoors[i].isOpen ? 1 : 0)
               << " " << (_interactiveDoors[i].isBroken ? 1 : 0) << "\"\n";
        }
        ss << "    }\n";
    }

    ss << "\n    // Список врагов: [x  y  тип(0=обычный, 1=жирный, 2=дуэлянт)  id]\n";
    ss << "    \"Enemies\"\n    {\n";
    for (size_t i = 0; i < _enemySpawns.size(); ++i) {
        int ex = static_cast<int>(std::round(_enemySpawns[i].position.x / _tileSize));
        int ey = static_cast<int>(std::round((_enemySpawns[i].position.y + 28.0f) / _tileSize)) - 1;
        ss << "        \"enemy_" << i << "\"  \"" << ex << " " << ey << " " << _enemySpawns[i].type;
        if (!_enemySpawns[i].enemyId.empty()) {
            ss << " " << _enemySpawns[i].enemyId;
        }
        ss << "\"\n";
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
    ss << "\n    // Список люминесцентных ламп: [x  y  r  g  b  intensity  radius  coneAngle  enabled]\n";
    ss << "    \"Lamps\"\n    {\n";
    int lampIdx = 0;
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            if (_map[y][x] == 4) {
                const LampLight* l = findLamp(x, y);
                if (l) {
                    ss << "        \"lamp_" << lampIdx++ << "\"  \"" << x << " " << y << " "
                       << static_cast<int>(l->color.r) << " "
                       << static_cast<int>(l->color.g) << " "
                       << static_cast<int>(l->color.b) << " "
                       << l->intensity << " " << l->radius << " " << l->coneAngle << " "
                       << (l->enabled ? 1 : 0) << "\"\n";
                } else {
                    ss << "        \"lamp_" << lampIdx++ << "\"  \"" << x << " " << y << "\"\n";
                }
            }
        }
    }
    ss << "    }\n";

    // Сохраняем слой фона (BackgroundLayer)
    bool hasBg = false;
    for (int y = 0; y < _height && !hasBg; ++y) {
        for (int x = 0; x < _width && !hasBg; ++x) {
            if (y < static_cast<int>(_bgMap.size()) && x < static_cast<int>(_bgMap[y].size()) && _bgMap[y][x] > 0) hasBg = true;
        }
    }
    if (hasBg) {
        ss << "\n    \"BackgroundLayer\"\n    {\n";
        for (int y = 0; y < _height; ++y) {
            ss << "        \"row_" << y << "\"  \"";
            for (int x = 0; x < _width; ++x) {
                int c = (y < static_cast<int>(_bgMap.size()) && x < static_cast<int>(_bgMap[y].size())) ? _bgMap[y][x] : 0;
                ss << c << (x + 1 < _width ? " " : "");
            }
            ss << "\"\n";
        }
        ss << "    }\n";
    }

    // Сохраняем слой декора (DecorLayer)
    bool hasDecor = false;
    for (int y = 0; y < _height && !hasDecor; ++y) {
        for (int x = 0; x < _width && !hasDecor; ++x) {
            if (y < static_cast<int>(_decorMap.size()) && x < static_cast<int>(_decorMap[y].size()) && _decorMap[y][x] > 0) hasDecor = true;
        }
    }
    if (hasDecor) {
        ss << "\n    \"DecorLayer\"\n    {\n";
        for (int y = 0; y < _height; ++y) {
            ss << "        \"row_" << y << "\"  \"";
            for (int x = 0; x < _width; ++x) {
                int c = (y < static_cast<int>(_decorMap.size()) && x < static_cast<int>(_decorMap[y].size())) ? _decorMap[y][x] : 0;
                ss << c << (x + 1 < _width ? " " : "");
            }
            ss << "\"\n";
        }
        ss << "    }\n";
    }

    ss << "\n    \"TileLayer\"\n    {\n";
    for (int y = 0; y < _height; ++y) {
        ss << "        \"row_" << y << "\"  \"";
        for (int x = 0; x < _width; ++x) {
            ss << _map[y][x] << (x + 1 < _width ? " " : "");
        }
        ss << "\"\n";
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
    struct LampCoord { int x, y; };
    std::vector<Plat> platforms;
    std::vector<Wall> walls;
    std::vector<LampCoord> lamps;

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
                std::string eid = "";
                if (iss >> ex >> ey) {
                    if (!(iss >> etype)) etype = 0;
                    if (iss >> eid) {}
                    _enemySpawns.push_back({{static_cast<float>(ex * _tileSize), 
                                             static_cast<float>((ey + 1) * _tileSize - 28.0f)}, etype, eid});
                }
            } else if (currentSection == "platforms") {
                std::istringstream iss(val);
                int x = 0, y = 0, len = 0;
                if (iss >> x >> y >> len) platforms.push_back({x, y, len});
            } else if (currentSection == "walls") {
                std::istringstream iss(val);
                int x = 0, y = 0, w = 1, h = 1;
                if (iss >> x >> y >> w >> h) walls.push_back({x, y, w, h});
            } else if (currentSection == "lamps" || key.rfind("lamp", 0) == 0) {
                std::istringstream iss(val);
                int lx = 0, ly = 0;
                if (iss >> lx >> ly) lamps.push_back({lx, ly});
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

    // Применяем люминесцентные лампы
    for (const auto& l : lamps) {
        if (l.x >= 0 && l.x < _width && l.y >= 0 && l.y < _height) {
            _map[l.y][l.x] = 4;
        }
    }

    _floorY = (lowestFloor >= 0) ? lowestFloor : (_height - 5);

    if (hasP) {
        int groundTileY = pY + 1;
        while (groundTileY < _height && !isSolid(pX, groundTileY)) {
            groundTileY++;
        }
        if (groundTileY < _height && isSolid(pX, groundTileY)) {
            _spawnPosition = { static_cast<float>(pX * _tileSize), static_cast<float>(groundTileY * _tileSize - 30.0f) };
        } else {
            _spawnPosition = { static_cast<float>(pX * _tileSize), static_cast<float>((pY + 1) * _tileSize - 30.0f) };
        }
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
        if (tokens[0] == "InteractiveDoors" || tokens[0] == "interactivedoors") { currentSection = "interactivedoors"; continue; }
        if (tokens[0] == "Enemies" || tokens[0] == "enemies") { currentSection = "enemies"; continue; }
        if (tokens[0] == "Triggers" || tokens[0] == "triggers") { currentSection = "triggers"; continue; }
        if (tokens[0] == "Platforms" || tokens[0] == "platforms") { currentSection = "platforms"; continue; }
        if (tokens[0] == "Walls" || tokens[0] == "walls") { currentSection = "walls"; continue; }
        if (tokens[0] == "BackgroundLayer" || tokens[0] == "backgroundlayer" || tokens[0] == "Background" || tokens[0] == "background") { currentSection = "background"; continue; }
        if (tokens[0] == "DecorLayer" || tokens[0] == "decorlayer" || tokens[0] == "Decor" || tokens[0] == "decor") { currentSection = "decor"; continue; }
        if (tokens[0] == "TileLayer" || tokens[0] == "tilelayer" || tokens[0] == "Tiles" || tokens[0] == "tiles") { currentSection = "tiles"; continue; }
        if (tokens[0] == "{" || tokens[0] == "}") { if (tokens[0] == "}") currentSection = ""; continue; }

        if (tokens.size() >= 2) {
            std::string key = tokens[0];
            std::string val = tokens[1];
            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);

            if (currentSection == "tiles" || currentSection == "background" || currentSection == "decor") {
                if (_bgMap.size() != static_cast<size_t>(_height)) _bgMap.assign(_height, std::vector<int>(_width, 0));
                if (_decorMap.size() != static_cast<size_t>(_height)) _decorMap.assign(_height, std::vector<int>(_width, 0));
                int rowIndex = -1;
                if (keyLower.rfind("row_", 0) == 0) {
                    try { rowIndex = std::stoi(keyLower.substr(4)); } catch (...) {}
                } else if (keyLower.rfind("r", 0) == 0) {
                    try { rowIndex = std::stoi(keyLower.substr(1)); } catch (...) {}
                }
                if (rowIndex >= 0 && rowIndex < _height) {
                    std::istringstream iss(val);
                    int col = 0, tCode = 0;
                    while (iss >> tCode && col < _width) {
                        if (currentSection == "background") _bgMap[rowIndex][col] = tCode;
                        else if (currentSection == "decor") _decorMap[rowIndex][col] = tCode;
                        else _map[rowIndex][col] = tCode;
                        col++;
                    }
                }
                continue;
            }

            if (key == "floor_type") {
                if (val == "basement") _floorType = FloorType::BASEMENT;
                else _floorType = FloorType::NORMAL;
            } else if (key == "player_spawn") {
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
                std::string eid = "";
                if (iss >> ex >> ey) {
                    if (iss >> etype) {}
                    if (iss >> eid) {}
                    kvEnemies.push_back({{ static_cast<float>(ex * _tileSize),
                                           static_cast<float>((ey + 1) * _tileSize - 28.0f) }, etype, eid});
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
                        else if (token.rfind("boss:", 0) == 0) tr.isBossFight = (token.substr(5) == "1");
                        else if (token.rfind("bossname:", 0) == 0) tr.bossName = token.substr(9);
                        else if (token.rfind("camlock:", 0) == 0) tr.lockCamera = (token.substr(8) == "1");
                        else if (token.rfind("cammode:", 0) == 0) {
                            try { tr.cameraLockMode = std::stoi(token.substr(8)); } catch (...) {}
                        }
                        else if (token.rfind("campos:", 0) == 0) {
                            auto s = token.substr(7);
                            auto comma = s.find(',');
                            if (comma != std::string::npos) {
                                tr.cameraLockPos.x = std::stof(s.substr(0, comma));
                                tr.cameraLockPos.y = std::stof(s.substr(comma + 1));
                            }
                        }
                        else if (token.rfind("camsize:", 0) == 0) {
                            auto s = token.substr(8);
                            auto comma = s.find(',');
                            if (comma != std::string::npos) {
                                tr.cameraSize.x = std::stof(s.substr(0, comma));
                                tr.cameraSize.y = std::stof(s.substr(comma + 1));
                            }
                        }
                        else if (token.rfind("camentag:", 0) == 0) tr.cameraTargetEnemyTag = token.substr(9);
                        else if (token.rfind("camenidx:", 0) == 0) {
                            try { tr.cameraTargetEnemyIndex = std::stoi(token.substr(9)); } catch (...) {}
                        }
                        else if (token.rfind("spawnb:", 0) == 0) {
                            // Format: spawnb:x,y,t;x,y,t
                            std::stringstream ss(token.substr(7));
                            std::string item;
                            while (std::getline(ss, item, ';')) {
                                int bx = 0, by = 0, bt = 9;
                                char sep1 = 0, sep2 = 0;
                                std::stringstream itemSs(item);
                                if (itemSs >> bx >> sep1 >> by >> sep2 >> bt) {
                                    tr.spawnBlocks.push_back({bx, by, bt});
                                }
                            }
                        }
                        else if (token.rfind("spawne:", 0) == 0) {
                            // Format: spawne:x,y,type,id;...
                            std::stringstream ss(token.substr(7));
                            std::string item;
                            while (std::getline(ss, item, ';')) {
                                int ex = 0, ey = 0, etype = 0;
                                std::string eid = "";
                                char sep1 = 0, sep2 = 0;
                                std::stringstream itemSs(item);
                                if (itemSs >> ex >> sep1 >> ey >> sep2 >> etype) {
                                    char sep3 = 0;
                                    if (itemSs >> sep3) std::getline(itemSs, eid);
                                    sf::Vector2f pos(ex * _tileSize + _tileSize * 0.5f, (ey + 1) * _tileSize - 14.0f);
                                    tr.spawnEnemies.push_back({pos, etype, eid});
                                }
                            }
                        }
                    }
                    kvTriggers.push_back(tr);
                }
            } else if (currentSection == "interactivedoors") {
                std::istringstream iss(val);
                int idx = 0, idy = 0, iOpen = 0, iBroken = 0;
                if (iss >> idx >> idy) {
                    if (iss >> iOpen) {}
                    if (iss >> iBroken) {}
                    _interactiveDoors.push_back({idx, idy, iOpen != 0, iBroken != 0, 0.0f, 0.0f});
                }
            } else if (currentSection == "lamps" || key.rfind("lamp", 0) == 0) {
                std::istringstream iss(val);
                int lx = 0, ly = 0;
                if (iss >> lx >> ly) {
                    if (lx >= 0 && lx < _width && ly >= 0 && ly < _height) {
                        _map[ly][lx] = 4;
                        int r = 240, g = 250, b = 255;
                        float intens = 1.0f, rad = 980.0f, cone = 130.0f;
                        if (iss >> r >> g >> b) {
                            if (iss >> intens) {}
                            if (iss >> rad) {}
                            if (iss >> cone) {}
                            int en = 1;
                            if (iss >> en) {}
                            LampLight* ex = findLamp(lx, ly);
                            if (ex) {
                                ex->color = sf::Color(r, g, b);
                                ex->intensity = intens;
                                ex->radius = rad;
                                ex->coneAngle = cone;
                                ex->enabled = (en != 0);
                            } else {
                                LampLight nl;
                                nl.tileX = lx;
                                nl.tileY = ly;
                                nl.worldPos = {(lx + 0.5f) * _tileSize, (ly + 0.5f) * _tileSize};
                                nl.color = sf::Color(r, g, b);
                                nl.intensity = intens;
                                nl.radius = rad;
                                nl.coneAngle = cone;
                                nl.enabled = (en != 0);
                                _lamps.push_back(nl);
                            }
                        }
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
        int spTx = static_cast<int>(std::round(playerSpawnPos.x / _tileSize));
        int spTy = static_cast<int>(std::round((playerSpawnPos.y + 30.0f) / _tileSize)) - 1;
        int checkY = spTy + 1;
        while (checkY < _height && !isSolid(spTx, checkY)) {
            checkY++;
        }
        if (checkY < _height && isSolid(spTx, checkY)) {
            playerSpawnPos.y = static_cast<float>(checkY * _tileSize - 30.0f);
        }
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

    // Синхронизация и миграция слоев (фон сзади всех всегда):
    if (_bgMap.size() != static_cast<size_t>(_height)) _bgMap.assign(_height, std::vector<int>(_width, 0));
    if (_decorMap.size() != static_cast<size_t>(_height)) _decorMap.assign(_height, std::vector<int>(_width, 0));

    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            if (_map[y][x] == 8) {
                if (_bgMap[y][x] == 0) _bgMap[y][x] = 8;
                _map[y][x] = 0;
            }
        }
    }
    // Если на клетке светильник, а соседние клетки в строке имеют фон 8 — заполняем фон под лампой!
    for (const auto& l : _lamps) {
        int lx = l.tileX;
        int ly = l.tileY;
        if (lx >= 0 && lx < _width && ly >= 0 && ly < _height) {
            if (_bgMap[ly][lx] == 0) {
                if ((lx > 0 && _bgMap[ly][lx - 1] == 8) || (lx + 1 < _width && _bgMap[ly][lx + 1] == 8)) {
                    _bgMap[ly][lx] = 8;
                }
            }
        }
    }
    _tilemapDirty = true;
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

    // 1. Приоритет: текстовый ASCII-файл или координатный KV-файл (authoritative source)
    std::string txtPath = resolveAssetPath(base + ".txt");
    std::string kvPath  = resolveAssetPath(base + ".kv");
    std::string pngPath = resolveAssetPath(base + ".png");

    if (std::filesystem::exists(txtPath)) {
        _layoutFilePath = txtPath;
        loadedBase = loadFromAsciiFile(txtPath);
        std::cout << "[ROOM] Загружен текстовый файл комнаты " << name 
                  << " (" << txtPath << ", " << _width << "x" << _height << ")" << std::endl;
    } else if (std::filesystem::exists(kvPath)) {
        loadedBase = loadFromKVCoordinates(kvPath);
        std::cout << "[ROOM] Загружен координатный KV-файл комнаты " << name 
                  << " (" << kvPath << ", " << _width << "x" << _height << ")" << std::endl;
    } else if (std::filesystem::exists(pngPath)) {
        loadedBase = loadFromImageFile(pngPath);
        std::cout << "[ROOM] Загружена PNG-карта комнаты " << name 
                  << " (" << pngPath << ", " << _width << "x" << _height << ")" << std::endl;
    } else {
        std::string cfgMap = resolveAssetPath("assets/config/map.txt");
        _layoutFilePath = cfgMap;
        loadedBase = loadFromAsciiFile(cfgMap);
        std::cout << "[ROOM] Загружен дефолтный текстовый файл карты " << cfgMap << std::endl;
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
    _interactiveDoors.clear();
    _doorSplinters.clear();
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
    scanLamps();
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

void Arena::addInteractiveDoor(int tx, int ty, bool isOpen) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return;
    for (auto& d : _interactiveDoors) {
        if (d.x == tx && d.y == ty) {
            d.isOpen = isOpen;
            d.isBroken = false;
            return;
        }
    }
    _interactiveDoors.push_back({tx, ty, isOpen, false, 0.0f, 0.0f});
    if (_map[ty][tx] == 2 || _map[ty][tx] == 1) {
        _map[ty][tx] = 0;
    }
    _tilemapDirty = true;
}

bool Arena::removeInteractiveDoor(int tx, int ty) {
    auto it = std::remove_if(_interactiveDoors.begin(), _interactiveDoors.end(),
        [tx, ty](const InteractiveDoor& d) { return d.x == tx && d.y == ty; });
    if (it != _interactiveDoors.end()) {
        _interactiveDoors.erase(it, _interactiveDoors.end());
        _tilemapDirty = true;
        return true;
    }
    return false;
}

InteractiveDoor* Arena::findInteractiveDoor(int tx, int ty) {
    for (auto& d : _interactiveDoors) {
        if (d.x == tx && d.y == ty) return &d;
    }
    return nullptr;
}

const InteractiveDoor* Arena::findInteractiveDoor(int tx, int ty) const {
    for (const auto& d : _interactiveDoors) {
        if (d.x == tx && d.y == ty) return &d;
    }
    return nullptr;
}

bool Arena::isInteractiveDoor(int tx, int ty) const {
    return findInteractiveDoor(tx, ty) != nullptr;
}

bool Arena::toggleInteractiveDoor(int tx, int ty) {
    auto* rootDoor = findInteractiveDoor(tx, ty);
    if (!rootDoor || rootDoor->isBroken) return false;
    bool targetState = !rootDoor->isOpen;
    std::vector<int> targetYs;
    for (const auto& d : _interactiveDoors) {
        if (d.x == tx && std::abs(d.y - ty) <= 3) {
            targetYs.push_back(d.y);
        }
    }
    for (auto& d : _interactiveDoors) {
        if (d.x == tx && std::find(targetYs.begin(), targetYs.end(), d.y) != targetYs.end()) {
            d.isOpen = targetState;
            d.interactCooldown = 0.35f;
        }
    }
    _tilemapDirty = true;
    SoundManager::get().play("hit", 75.0f);
    return true;
}

bool Arena::interactDoorNear(sf::Vector2f playerPos, float maxDist) {
    InteractiveDoor* bestDoor = nullptr;
    float bestDistSq = maxDist * maxDist;

    for (auto& d : _interactiveDoors) {
        if (d.isBroken || d.interactCooldown > 0.0f) continue;
        float dx = (d.x + 0.5f) * _tileSize;
        float dy = (d.y + 0.5f) * _tileSize;
        float distSq = (playerPos.x - dx) * (playerPos.x - dx) + ((playerPos.y - 28.0f) - dy) * ((playerPos.y - 28.0f) - dy);
        if (distSq <= bestDistSq) {
            bestDistSq = distSq;
            bestDoor = &d;
        }
    }

    if (bestDoor) {
        return toggleInteractiveDoor(bestDoor->x, bestDoor->y);
    }
    return false;
}

bool Arena::breakDoorAt(int tx, int ty, sf::Vector2f impactVelocity) {
    auto* rootDoor = findInteractiveDoor(tx, ty);
    if (!rootDoor || rootDoor->isBroken || rootDoor->isOpen) return false; // Открытую дверь сломать нельзя!

    bool found = false;
    std::vector<int> targetYs;
    for (const auto& d : _interactiveDoors) {
        if (d.x == tx && std::abs(d.y - ty) <= 3) {
            targetYs.push_back(d.y);
        }
    }
    for (auto& d : _interactiveDoors) {
        if (d.x == tx && std::find(targetYs.begin(), targetYs.end(), d.y) != targetYs.end()) {
            if (!d.isBroken && !d.isOpen) {
                d.isBroken = true;
                d.isOpen = true;
                d.breakTimer = 0.5f;
                found = true;
                sf::Vector2f doorCenter{(d.x + 0.5f) * _tileSize, (d.y + 0.5f) * _tileSize};
                sf::Vector2f burstDir = impactVelocity;
                if (std::hypot(burstDir.x, burstDir.y) < 1.0f) burstDir = {1.0f, -0.3f};
                spawnDoorSplinters(doorCenter, burstDir, 25);
            }
        }
    }
    if (found) {
        _tilemapDirty = true;
        SoundManager::get().play("adw2", 90.0f);
        if (CombatSystem::getActive()) {
            CombatSystem::getActive()->triggerShake(0.35f, 9.0f);
            CombatSystem::getActive()->addSpecialMarker(
                {(tx + 0.5f) * _tileSize, (ty + 0.5f) * _tileSize - 20.0f},
                "★ DOOR SMASHED! ★",
                sf::Color(255, 120, 50), 1.2f
            );
        }
    }
    return found;
}

bool Arena::checkAndBreakDoorOverlap(const sf::FloatRect& bounds, const sf::Vector2f& velocity, bool isKnockback) {
    if (!isKnockback && std::hypot(velocity.x, velocity.y) < 120.0f) return false;
    bool anyBroken = false;
    for (size_t i = 0; i < _interactiveDoors.size(); ++i) {
        auto& d = _interactiveDoors[i];
        if (d.isBroken || d.isOpen) continue; // Открытую дверь сломать нельзя!
        sf::FloatRect dRect({static_cast<float>(d.x) * _tileSize, static_cast<float>(d.y) * _tileSize}, {_tileSize, _tileSize});
        if (bounds.findIntersection(dRect).has_value()) {
            if (breakDoorAt(d.x, d.y, velocity)) {
                anyBroken = true;
            }
        }
    }
    return anyBroken;
}

void Arena::spawnDoorSplinters(sf::Vector2f pos, sf::Vector2f impactDir, int count) {
    float mag = std::hypot(impactDir.x, impactDir.y);
    sf::Vector2f dir = (mag > 0.001f) ? (impactDir / mag) : sf::Vector2f(1.0f, 0.0f);
    for (int i = 0; i < count; ++i) {
        DoorSplinterParticle p;
        p.position = pos + sf::Vector2f(
            static_cast<float>((std::rand() % 24) - 12),
            static_cast<float>((std::rand() % 24) - 12)
        );
        float spreadAngle = static_cast<float>((std::rand() % 120) - 60) * 3.14159265f / 180.0f;
        float baseAngle = std::atan2(dir.y, dir.x);
        float angle = baseAngle + spreadAngle;
        float speed = 120.0f + static_cast<float>(std::rand() % 280);
        p.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed - 60.0f);
        p.rotation = static_cast<float>(std::rand() % 360);
        p.angularVelocity = static_cast<float>((std::rand() % 720) - 360);
        p.lifetime = 1.2f + static_cast<float>(std::rand() % 80) / 100.0f;
        p.size = { static_cast<float>(4 + std::rand() % 6), static_cast<float>(2 + std::rand() % 4) };
        int colVar = std::rand() % 3;
        if (colVar == 0) p.color = sf::Color(135, 90, 64);
        else if (colVar == 1) p.color = sf::Color(100, 65, 45);
        else p.color = sf::Color(65, 40, 25);
        _doorSplinters.push_back(p);
    }
}

void Arena::updateInteractiveDoors(float dt, Player* player) {
    for (auto it = _doorSplinters.begin(); it != _doorSplinters.end(); ) {
        it->lifetime -= dt;
        if (it->lifetime <= 0.0f) {
            it = _doorSplinters.erase(it);
            continue;
        }
        it->velocity.y += 900.0f * dt;
        it->position += it->velocity * dt;
        it->rotation += it->angularVelocity * dt;
        int tx = static_cast<int>(std::floor(it->position.x / _tileSize));
        int ty = static_cast<int>(std::floor(it->position.y / _tileSize));
        if (isSolid(tx, ty)) {
            it->velocity.x *= 0.4f;
            it->velocity.y = -it->velocity.y * 0.2f;
            it->angularVelocity *= 0.5f;
        }
        ++it;
    }

    for (auto& d : _interactiveDoors) {
        if (d.interactCooldown > 0.0f) d.interactCooldown -= dt;
        if (d.breakTimer > 0.0f) d.breakTimer -= dt;
    }

    if (player) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
            interactDoorNear(player->position, 96.0f);
        }
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
    // Проверка интерактивных физических дверей (если закрыта — твердая преграда, если открыта/сломана — свободный проход)
    for (const auto& d : _interactiveDoors) {
        if (d.x == tx && d.y == ty) {
            return (!d.isOpen && !d.isBroken);
        }
    }
    // Открытый проход у самого левого и правого края внутри карты
    if (ty >= _floorY - 4 && ty < _floorY) {
        if ((tx <= 1 || tx >= _width - 2) && !isBlockSolid(tx, ty)) {
            return false;
        }
    }
    return isBlockSolid(tx, ty);
}

bool Arena::isPointSolid(float wx, float wy) const {
    int tx = static_cast<int>(std::floor(wx / _tileSize));
    int ty = static_cast<int>(std::floor(wy / _tileSize));
    if (!isSolid(tx, ty)) return false;
    TileShape shape = getTileShape(tx, ty);
    if (shape == TileShape::FULL) return true;
    float lx = wx - static_cast<float>(tx) * _tileSize;
    float ly = wy - static_cast<float>(ty) * _tileSize;
    if (shape == TileShape::HALF_BOTTOM) return ly >= _tileSize * 0.5f;
    if (shape == TileShape::SLOPE_UP_RIGHT) return ly >= (_tileSize - lx);
    if (shape == TileShape::SLOPE_DOWN_RIGHT) return ly >= lx;
    return true;
}

Arena::TileShape Arena::getTileShape(int tx, int ty) const {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return TileShape::FULL;
    int t = _map[ty][tx];
    auto it = _tileShapes.find(t);
    if (it != _tileShapes.end()) return it->second;
    return TileShape::FULL;
}

bool Arena::isSlopeNear(float wx, float wy, float radius) const {
    int minTx = std::max(0, static_cast<int>(std::floor((wx - radius) / _tileSize)));
    int maxTx = std::min(_width - 1, static_cast<int>(std::floor((wx + radius) / _tileSize)));
    int minTy = std::max(0, static_cast<int>(std::floor((wy - radius) / _tileSize)));
    int maxTy = std::min(_height - 1, static_cast<int>(std::floor((wy + radius) / _tileSize)));

    for (int ty = minTy; ty <= maxTy; ++ty) {
        for (int tx = minTx; tx <= maxTx; ++tx) {
            TileShape s = getTileShape(tx, ty);
            if (s == TileShape::SLOPE_UP_RIGHT || s == TileShape::SLOPE_DOWN_RIGHT || s == TileShape::HALF_BOTTOM) {
                return true;
            }
        }
    }
    return false;
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
            if (!isSolid(tx, ty)) continue;

            TileShape shape = getTileShape(tx, ty);
            if (shape == TileShape::FULL) {
                return true;
            }

            float tileL = static_cast<float>(tx) * _tileSize;
            float tileT = static_cast<float>(ty) * _tileSize;

            float localL = std::clamp(left - tileL, 0.0f, _tileSize);
            float localR = std::clamp(right - tileL, 0.0f, _tileSize);
            float localB = std::clamp(bottom - tileT, 0.0f, _tileSize);

            if (localR <= localL || localB <= 0.0f) continue;

            if (shape == TileShape::HALF_BOTTOM) {
                if (localB > _tileSize * 0.5f) return true;
            } else if (shape == TileShape::SLOPE_UP_RIGHT) {
                float minSolidY = _tileSize - localR;
                if (localB > minSolidY) return true;
            } else if (shape == TileShape::SLOPE_DOWN_RIGHT) {
                float minSolidY = localL;
                if (localB > minSolidY) return true;
            }
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
        if (isPointSolid(curX, curY)) {
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
        for (auto& l : _lamps) l.dirty = true;
        for (auto& w : _windows) w.dirty = true;
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
    _bgVertices.clear();
    _decorVertices.clear();
    _tilemapVertices.clear();

    auto appendTileQuad = [this](sf::VertexArray& va, int x, int y, int tile) {
        if (tile <= 0 || static_cast<size_t>(tile) >= _tileSprites.size()) return;
        float px = static_cast<float>(x * _tileSize);
        float py = static_cast<float>(y * _tileSize);
        float tx = static_cast<float>(tile * 32);
        float ty = 0.0f;
        float tw = 32.0f;
        float th = 32.0f;

        va.append({{px, py}, sf::Color::White, {tx, ty}});
        va.append({{px + _tileSize, py}, sf::Color::White, {tx + tw, ty}});
        va.append({{px + _tileSize, py + _tileSize}, sf::Color::White, {tx + tw, ty + th}});

        va.append({{px, py}, sf::Color::White, {tx, ty}});
        va.append({{px + _tileSize, py + _tileSize}, sf::Color::White, {tx + tw, ty + th}});
        va.append({{px, py + _tileSize}, sf::Color::White, {tx, ty + th}});
    };

    // 1. Фоновый слой (BackgroundLayer: кирпичи фона, текстуры стен сзади всех всегда)
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            int bgTile = (y < static_cast<int>(_bgMap.size()) && x < static_cast<int>(_bgMap[y].size())) ? _bgMap[y][x] : 0;
            if (bgTile == 0 && _map[y][x] == 8) bgTile = 8;
            if (bgTile > 0) {
                appendTileQuad(_bgVertices, x, y, bgTile);
            }
        }
    }

    // 2. Декоративный слой (DecorLayer: колонны, лава, декор сзади всех блоков кроме фона)
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            int decTile = (y < static_cast<int>(_decorMap.size()) && x < static_cast<int>(_decorMap[y].size())) ? _decorMap[y][x] : 0;
            if (decTile > 0 && decTile != 4 && decTile != 5) {
                appendTileQuad(_decorVertices, x, y, decTile);
            }
        }
    }

    // 3. Основной слой твердых блоков / стен / платформ (TileLayer)
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            int tile = _map[y][x];
            if (tile == 0 || tile == 3 || tile == 4 || tile == 8 || tile == TileId::BARRIER) continue; // 0=воздух, 3=дверь, 4=лампа, 8=фон, 9=невидимый барьер
            appendTileQuad(_tilemapVertices, x, y, tile);
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

    // 0. Заливка фона текущей локации (обычный этаж: дневное небо; подвал: черный полумрак)
    float roomW = static_cast<float>(_width * _tileSize);
    float roomH = static_cast<float>(_height * _tileSize);
    _cachedRoomBg.setSize({roomW, roomH});
    _cachedRoomBg.setPosition({0.0f, 0.0f});
    sf::Color bgColor = isBasement() ? sf::Color(10, 11, 15) : sf::Color(78, 142, 215);
    _cachedRoomBg.setFillColor(bgColor);
    window.draw(_cachedRoomBg);

    if (_tilemapDirty) {
        rebuildTilemapVertices();
    }
    sf::RenderStates tileStates(&_tileTexture);

    // 1. Отрисовка фонового слоя тайлов (из assets/tiles/background/):
    if (_bgVertices.getVertexCount() > 0) {
        window.draw(_bgVertices, tileStates);
    }

    // 2. Пакетная отрисовка стекающей крови на заднем фоне (поверх фоновых кирпичей)
    if (!_bgBloodStreaks.empty()) {
        _bgBloodVA.clear();
        for (const auto& streak : _bgBloodStreaks) {
            if (streak.isSpray) {
                float h = (streak.blotchH > 0.0f) ? streak.blotchH : streak.width;
                float px = std::floor(streak.startPos.x - streak.width * 0.5f);
                float py = std::floor(streak.startPos.y - (streak.blotchH > 0.0f ? streak.blotchH * 0.5f : streak.width * 0.5f));
                addQuadColored(_bgBloodVA, px, py, streak.width, h, streak.color);
            } else if (streak.isBlotch) {
                if (streak.width <= 0.0f || streak.blotchH <= 0.0f) continue;
                float px = std::floor(streak.startPos.x - streak.width * 0.5f);
                float py = std::floor(streak.startPos.y - streak.blotchH * 0.5f);
                addQuadColored(_bgBloodVA, px, py, streak.width, streak.blotchH, streak.color);
            } else if (streak.length <= 1.0f) {
                float px = std::floor(streak.startPos.x - streak.width * 0.5f);
                float py = std::floor(streak.startPos.y);
                addQuadColored(_bgBloodVA, px, py, streak.width, std::max(2.0f, streak.width * 0.8f), streak.color);
            } else {
                float anchorW = streak.width * 1.35f;
                float anchorH = std::max(3.5f, streak.width * 0.85f);
                addQuadColored(_bgBloodVA, std::floor(streak.startPos.x - anchorW * 0.5f), std::floor(streak.startPos.y), anchorW, anchorH, streak.color);

                addQuadColored(_bgBloodVA, std::floor(streak.startPos.x - streak.width * 0.5f), std::floor(streak.startPos.y), streak.width, streak.length, streak.color);

                float headW = streak.width * 1.35f;
                float headH = std::max(4.0f, streak.width * 1.5f);
                addQuadColored(_bgBloodVA, std::floor(streak.currentPos.x - headW * 0.5f), std::floor(streak.currentPos.y), headW, headH, streak.color);
            }
        }
        if (_bgBloodVA.getVertexCount() > 0) {
            window.draw(_bgBloodVA);
        }
    }

    // 3. Отрисовка декоративного слоя тайлов (из assets/tiles/decor/):
    if (_decorVertices.getVertexCount() > 0) {
        window.draw(_decorVertices, tileStates);
    }

    // 4. Отрисовка твердых блоков и платформ (поверх декора и фона)
    window.draw(_tilemapVertices, tileStates);

    // 3. Пакетная отрисовка наружных декалей крови и летящих частиц в воздухе (Batch VertexArray в 1 Draw Call)
    if (!_bloodDecals.empty() || !_bloodParticles.empty()) {
        _fgBloodVA.clear();
        for (const auto& decal : _bloodDecals) {
            float len = (decal.length > 0.0f) ? decal.length : (decal.radiusX * 2.0f);
            float thick = (decal.thickness > 0.0f) ? decal.thickness : 2.5f;
            if (decal.orientation == BloodDecalOrientation::HORIZONTAL) {
                float w = std::max(4.0f, len);
                float h = thick;
                addQuadColored(_fgBloodVA, std::floor(decal.position.x), std::floor(decal.position.y), w, h, decal.color);
            } else if (decal.orientation == BloodDecalOrientation::VERTICAL) {
                float w = thick;
                float h = std::max(4.0f, len);
                addQuadColored(_fgBloodVA, std::floor(decal.position.x), std::floor(decal.position.y), w, h, decal.color);
            } else if (decal.orientation == BloodDecalOrientation::SLOPE_DOWN_RIGHT) {
                constexpr float kInvSqrt2 = 0.70710678f;
                float halfLen = std::max(4.0f, len) * 0.5f;
                sf::Vector2f t(kInvSqrt2, kInvSqrt2);
                sf::Vector2f n(-kInvSqrt2, kInvSqrt2);
                sf::Vector2f c = decal.position;

                int tx = static_cast<int>(std::floor(c.x / _tileSize));
                int ty = static_cast<int>(std::floor(c.y / _tileSize));
                float tLeft = static_cast<float>(tx * _tileSize);
                float tRight = tLeft + _tileSize;
                float tTop = static_cast<float>(ty * _tileSize);

                float p0x = std::clamp(c.x - t.x * halfLen, tLeft, tRight);
                float p0y = tTop + (p0x - tLeft);
                float p1x = std::clamp(c.x + t.x * halfLen, tLeft, tRight);
                float p1y = tTop + (p1x - tLeft);
                sf::Vector2f p0(p0x, p0y);
                sf::Vector2f p1(p1x, p1y);
                sf::Vector2f p2 = p1 + n * thick;
                sf::Vector2f p3 = p0 + n * thick;
                _fgBloodVA.append({p0, decal.color});
                _fgBloodVA.append({p1, decal.color});
                _fgBloodVA.append({p2, decal.color});
                _fgBloodVA.append({p0, decal.color});
                _fgBloodVA.append({p2, decal.color});
                _fgBloodVA.append({p3, decal.color});
            } else if (decal.orientation == BloodDecalOrientation::SLOPE_UP_RIGHT) {
                constexpr float kInvSqrt2 = 0.70710678f;
                float halfLen = std::max(4.0f, len) * 0.5f;
                sf::Vector2f t(kInvSqrt2, -kInvSqrt2);
                sf::Vector2f n(kInvSqrt2, kInvSqrt2);
                sf::Vector2f c = decal.position;

                int tx = static_cast<int>(std::floor(c.x / _tileSize));
                int ty = static_cast<int>(std::floor(c.y / _tileSize));
                float tLeft = static_cast<float>(tx * _tileSize);
                float tRight = tLeft + _tileSize;
                float tTop = static_cast<float>(ty * _tileSize);

                float p0x = std::clamp(c.x - t.x * halfLen, tLeft, tRight);
                float p0y = tTop + (_tileSize - (p0x - tLeft));
                float p1x = std::clamp(c.x + t.x * halfLen, tLeft, tRight);
                float p1y = tTop + (_tileSize - (p1x - tLeft));
                sf::Vector2f p0(p0x, p0y);
                sf::Vector2f p1(p1x, p1y);
                sf::Vector2f p2 = p1 + n * thick;
                sf::Vector2f p3 = p0 + n * thick;
                _fgBloodVA.append({p0, decal.color});
                _fgBloodVA.append({p1, decal.color});
                _fgBloodVA.append({p2, decal.color});
                _fgBloodVA.append({p0, decal.color});
                _fgBloodVA.append({p2, decal.color});
                _fgBloodVA.append({p3, decal.color});
            }
        }

        for (const auto& bp : _bloodParticles) {
            if (!bp.active) continue;
            float pSize = std::max(2.5f, std::floor(bp.radius * 1.1f));
            addQuadColored(_fgBloodVA, std::floor(bp.position.x - pSize * 0.5f), std::floor(bp.position.y - pSize * 0.5f), pSize, pSize, bp.color);
        }

        if (_fgBloodVA.getVertexCount() > 0) {
            window.draw(_fgBloodVA);
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

    // 4.0. Отрисовка интерактивных физических дверей (закрытых, распахнутых и разрушенных)
    if (!_texDoorsLoaded) {
        (void)_texDoorClosed.loadFromFile(resolveAssetPath("assets/tiles/tools/door_closed.png"));
        (void)_texDoorOpen.loadFromFile(resolveAssetPath("assets/tiles/tools/door_open.png"));
        (void)_texDoorBroken.loadFromFile(resolveAssetPath("assets/tiles/tools/door_broken.png"));
        _texDoorsLoaded = true;
    }
    for (const auto& idoor : _interactiveDoors) {
        if (idoor.x < startX - 1 || idoor.x > endX + 1 || idoor.y < startY - 1 || idoor.y > endY + 1) continue;
        float idx = static_cast<float>(idoor.x * _tileSize);
        float idy = static_cast<float>(idoor.y * _tileSize);
        const sf::Texture* texToUse = idoor.isBroken ? &_texDoorBroken : (idoor.isOpen ? &_texDoorOpen : &_texDoorClosed);
        if (texToUse && texToUse->getSize().x > 0) {
            sf::Sprite dSp(*texToUse);
            float scale = _tileSize / 32.0f;
            dSp.setScale({scale, scale});
            dSp.setPosition({idx, idy});
            window.draw(dSp);
        } else {
            sf::RectangleShape dBox({_tileSize, _tileSize});
            dBox.setPosition({idx, idy});
            dBox.setFillColor(idoor.isBroken ? sf::Color(70, 45, 30, 180) : (idoor.isOpen ? sf::Color(30, 30, 40, 140) : sf::Color(112, 74, 52)));
            dBox.setOutlineColor(sf::Color(155, 105, 75));
            dBox.setOutlineThickness(1.0f);
            window.draw(dBox);
        }
    }

    // 4.05. Отрисовка разлетающихся щепок разрушенных дверей
    for (const auto& sp : _doorSplinters) {
        if (!sp.active) continue;
        sf::RectangleShape r(sp.size);
        r.setOrigin({sp.size.x * 0.5f, sp.size.y * 0.5f});
        r.setPosition(sp.position);
        r.setRotation(sf::degrees(sp.rotation));
        float alpha = std::clamp(sp.lifetime / 0.5f, 0.0f, 1.0f);
        sf::Color c = sp.color;
        c.a = static_cast<uint8_t>(255.0f * alpha);
        r.setFillColor(c);
        window.draw(r);
    }

    // 4.1. Отрисовка флуоресцентных лабораторных ламп (быстрая итерация по списку ламп)
    for (const auto& l : _lamps) {
        if (l.tileX < startX - 1 || l.tileX > endX + 1 || l.tileY < startY - 1 || l.tileY > endY + 1) continue;
        float lx = static_cast<float>(l.tileX * _tileSize);
        float ly = static_cast<float>(l.tileY * _tileSize);

        bool isEnabled = l.enabled;
        float flickerDip = l.flickerDip;

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
                static_cast<uint8_t>(255 * factor),
                static_cast<uint8_t>(255 * factor),
                static_cast<uint8_t>(255 * factor)
            );
            tube.setFillColor(tubeColor);
            tube.setOutlineColor(sf::Color(160, 235, 255, static_cast<uint8_t>(100 * factor)));
            tube.setOutlineThickness(1.0f);
            window.draw(tube);
        } else {
            tube.setFillColor(sf::Color(55, 60, 70));
            tube.setOutlineThickness(0.0f);
            window.draw(tube);
        }
    }

    // 5. Отрисовка интерактивных триггеров (батуты и шипы с текстурами)
    for (const auto& tr : _triggers) {
        if (tr.x < startX - 1 || tr.x > endX + 1 || tr.y < startY - 1 || tr.y > endY + 1) continue;
        if (tr.type >= 2) continue; // Логические триггеры полностью невидимы в игре!

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
        } else {
            sf::RectangleShape tBox({_tileSize, _tileSize});
            tBox.setPosition({tx, ty});
            if (tr.type == 0) {
                tBox.setFillColor(sf::Color(255, 170, 0, 160));
                tBox.setOutlineColor(sf::Color(255, 200, 50, 240));
            } else if (tr.type == 1) {
                tBox.setFillColor(sf::Color(230, 40, 40, 160));
                tBox.setOutlineColor(sf::Color(255, 80, 80, 240));
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

void Arena::startBossFight(const std::string& bossName) {
    _isBossFightActive = true;
    _activeBossName = bossName.empty() ? "БОСС 1" : bossName;
    SoundManager::get().playBossMusic("Boss1");
    showNotification("⚔ БОССФАЙТ: " + _activeBossName + " ⚔", 3.0f, sf::Color(255, 60, 50));
}

void Arena::endBossFight() {
    _isBossFightActive = false;
    _activeBossName = "";

    // 1. Убираем динамические блоки барьеров, возвращая исходные тайлы
    for (const auto& sb : _activeBossSpawnedBlocks) {
        int bx = std::get<0>(sb);
        int by = std::get<1>(sb);
        int origTile = std::get<2>(sb);
        if (by >= 0 && by < _height && bx >= 0 && bx < _width) {
            _map[by][bx] = origTile;
        }
    }
    _activeBossSpawnedBlocks.clear();
    _tilemapDirty = true;

    // 2. Деактивируем триггеры боссфайта навсегда
    for (auto& tr : _triggers) {
        if (tr.isBossFight || tr.type == 5) {
            tr.hasTriggered = true;
            tr.currentCooldown = 999999.0f;
        }
    }

    // 3. Сбрасываем фиксацию камеры
    resetCameraLock();

    // 4. Возвращаем музыку уровня
    SoundManager::get().playGameMusic();
    showNotification("★ БОСС ПОБЕЖДЕН! ★", 3.5f, sf::Color(255, 215, 0));
}

void Arena::updateTriggers(float dt, Player& player) {
    updateInteractiveDoors(dt, &player);
    float px = player.position.x;
    float py = player.position.y;
    float halfW = 16.0f;
    float topOffset = 30.0f;
    float bottomOffset = 30.0f;
    sf::FloatRect playerBox({px - halfW, py - topOffset}, {halfW * 2.0f, topOffset + bottomOffset});

    sf::Vector2f curPos = player.position;
    sf::Vector2f prevPos = (_prevPlayerPos == sf::Vector2f(0.0f, 0.0f)) ? curPos : _prevPlayerPos;
    _prevPlayerPos = curPos;

    for (auto& tr : _triggers) {
        if (tr.currentCooldown > 0.0f) tr.currentCooldown -= dt;
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
        } else if (tr.type == 2 || tr.type == 4 || tr.type == 5) {
            // TriggerZone (2), ScriptZone (4), BossFight (5)
            if (tr.triggerOnce && tr.hasTriggered) continue;

            bool conditionMet = false;
            if (tr.condition == 0) {
                // 1) Коснуться триггера самого (TOUCH / OVERLAP)
                conditionMet = overlap;
            } else if (tr.condition == 1) {
                // 2) Пройти через его X координату (CROSS_X)
                float lineX = (tr.x + 0.5f) * _tileSize;
                bool crossed = (prevPos.x <= lineX && curPos.x >= lineX) || (prevPos.x >= lineX && curPos.x <= lineX);
                bool inSpanY = (curPos.y >= tr.y * _tileSize - 120.0f && curPos.y <= (tr.y + std::max(1, tr.height)) * _tileSize + 120.0f) || (tr.height >= 12);
                conditionMet = crossed && inSpanY;
            } else if (tr.condition == 2) {
                // 3) Пройти через его Y координату (CROSS_Y)
                float lineY = (tr.y + 0.5f) * _tileSize;
                bool crossed = (prevPos.y <= lineY && curPos.y >= lineY) || (prevPos.y >= lineY && curPos.y <= lineY);
                bool inSpanX = (curPos.x >= tr.x * _tileSize - 120.0f && curPos.x <= (tr.x + std::max(1, tr.width)) * _tileSize + 120.0f) || (tr.width >= 12);
                conditionMet = crossed && inSpanX;
            } else if (tr.condition == 3) {
                // 4) Триггер попал в поле зрения (IN_VIEW)
                float distToTrig = std::hypot(curPos.x - (tr.x + 0.5f) * _tileSize, curPos.y - (tr.y + 0.5f) * _tileSize);
                conditionMet = (distToTrig <= 650.0f);
            } else if (tr.condition == 4) {
                // Взаимодействие на [E]
                float dist = std::hypot(px - (tr.x + 0.5f) * _tileSize, py - (tr.y + 0.5f) * _tileSize);
                conditionMet = ((overlap || dist <= 56.0f) && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E));
            }

            if (conditionMet && tr.currentCooldown <= 0.0f) {
                tr.hasTriggered = true;
                tr.currentCooldown = tr.cooldown > 0.0f ? tr.cooldown : 1.0f;

                if (tr.isBossFight || tr.type == 5) {
                    startBossFight(tr.bossName.empty() ? "БОСС 1" : tr.bossName);
                }

                if (tr.lockCamera) {
                    _isCameraLocked = true;
                    _cameraLockMode = tr.cameraLockMode > 0 ? tr.cameraLockMode : (tr.cameraLockOnBoss ? 3 : 1);
                    if (tr.cameraLockPos.x != 0.0f || tr.cameraLockPos.y != 0.0f) {
                        _cameraLockPos = tr.cameraLockPos;
                    } else {
                        _cameraLockPos = {(tr.x + 0.5f) * _tileSize, (tr.y + 0.5f) * _tileSize};
                    }
                    _cameraLockOnBoss = (_cameraLockMode == 3 || tr.cameraLockOnBoss);
                    _cameraTargetEnemyIndex = tr.cameraTargetEnemyIndex;
                    _cameraTargetEnemyTag = tr.cameraTargetEnemyTag;
                }

                if (tr.cameraSize.x > 0.0f && tr.cameraSize.y > 0.0f) {
                    _cameraSize = tr.cameraSize;
                    _hasCustomCameraSize = true;
                }

                if (!tr.spawnEnemies.empty()) {
                    for (const auto& sp : tr.spawnEnemies) {
                        _pendingEnemySpawns.push_back(sp);
                    }
                }

                if (!tr.spawnBlocks.empty()) {
                    for (const auto& sb : tr.spawnBlocks) {
                        int bx = std::get<0>(sb);
                        int by = std::get<1>(sb);
                        int bt = std::get<2>(sb);
                        if (bx >= 0 && bx < _width && by >= 0 && by < _height) {
                            if (tr.isBossFight || tr.type == 5) {
                                _activeBossSpawnedBlocks.push_back({bx, by, _map[by][bx]});
                            }
                            _map[by][bx] = bt;
                        }
                    }
                    _tilemapDirty = true;
                    SoundManager::get().play("adw2", 80.0f);
                }

                if (!tr.action.empty()) {
                    executeTriggerAction(tr.action, &player, &tr);
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

LampLight* Arena::findLamp(int tx, int ty) {
    for (auto& l : _lamps) {
        if (l.tileX == tx && l.tileY == ty) return &l;
    }
    return nullptr;
}

const LampLight* Arena::findLamp(int tx, int ty) const {
    for (const auto& l : _lamps) {
        if (l.tileX == tx && l.tileY == ty) return &l;
    }
    return nullptr;
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
                lamp.radius = 980.0f;
                lamp.intensity = 1.45f;
                lamp.color = sf::Color(240, 250, 255);
                lamp.enabled = true;
                lamp.flickering = false;
                lamp.coneAngle = 130.0f;
                lamp.tag = "lamp_" + std::to_string(lampIndex++);
                // Случайное время до первого моргания (1.0 - 7.0 с), чтобы лампы моргали асинхронно
                lamp.nextFlickerTimer = 1.0f + static_cast<float>(rand() % 601) / 100.0f;

                std::string key = std::to_string(x) + "_" + std::to_string(y);
                auto it = oldProps.find(key);
                if (it != oldProps.end()) {
                    lamp.enabled = it->second.enabled;
                    lamp.flickering = it->second.flickering;
                    lamp.nextFlickerTimer = it->second.nextFlickerTimer;
                    lamp.flickerEventTimer = it->second.flickerEventTimer;
                    lamp.flickerDuration = it->second.flickerDuration;
                    lamp.flickerPattern = it->second.flickerPattern;
                    lamp.flickerDip = it->second.flickerDip;
                    if (!it->second.tag.empty()) lamp.tag = it->second.tag;
                    lamp.color = it->second.color;
                    lamp.radius = it->second.radius;
                    lamp.coneAngle = it->second.coneAngle;
                    lamp.intensity = it->second.intensity;
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
    lamp.radius = 980.0f;
    lamp.intensity = 1.45f;
    lamp.color = sf::Color(240, 250, 255);
    lamp.enabled = true;
    lamp.flickering = flickering;
    lamp.coneAngle = 130.0f;
    lamp.nextFlickerTimer = 1.0f + static_cast<float>(rand() % 601) / 100.0f;
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

void Arena::setFloorType(FloorType ft) {
    _floorType = ft;
    markTilemapDirty();
}

void Arena::scanWindows() {
    _windows.clear();
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            if (_map[y][x] == 5) {
                WindowLight win;
                win.tileX = x;
                win.tileY = y;
                win.worldPos = {(x + 0.5f) * _tileSize, (y + 0.5f) * _tileSize};
                win.radius = 1750.0f;
                win.intensity = 0.55f;
                win.color = sf::Color(245, 250, 255);
                win.coneAngle = 135.0f;
                win.dirty = true;
                _windows.push_back(win);
            }
        }
    }
}

void Arena::addWindow(int tx, int ty) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return;
    _map[ty][tx] = 5;
    markTilemapDirty();
    WindowLight win;
    win.tileX = tx;
    win.tileY = ty;
    win.worldPos = {(tx + 0.5f) * _tileSize, (ty + 0.5f) * _tileSize};
    win.radius = 1750.0f;
    win.intensity = 0.55f;
    win.color = sf::Color(245, 250, 255);
    win.coneAngle = 135.0f;
    win.dirty = true;
    _windows.push_back(win);
}

void Arena::removeWindow(int tx, int ty) {
    if (tx >= 0 && tx < _width && ty >= 0 && ty < _height && _map[ty][tx] == 5) {
        _map[ty][tx] = 0;
        markTilemapDirty();
    }
    _windows.erase(std::remove_if(_windows.begin(), _windows.end(), [tx, ty](const WindowLight& w) {
        return w.tileX == tx && w.tileY == ty;
    }), _windows.end());
}

const sf::Texture& Arena::getSharedLampConeTexture() {
    static sf::Texture s_lampTexture;
    static bool s_initialized = false;
    if (!s_initialized) {
        s_initialized = true;
        const unsigned int texW = 256;
        const unsigned int texH = 256;
        sf::Image img({texW, texH}, sf::Color::Transparent);

        for (unsigned int py = 0; py < texH; ++py) {
            float v = static_cast<float>(py) / (texH - 1.0f); // 0.0 (top) .. 1.0 (bottom)
            
            // Плавное рассеивание вниз от трубки к полу
            float vertFactor = std::pow(std::max(0.0f, 1.0f - v), 1.25f);
            // Мягкий плавный вход на первых пикселях у трубки
            float topFade = std::clamp(v * 24.0f, 0.0f, 1.0f);

            for (unsigned int px = 0; px < texW; ++px) {
                float u = (static_cast<float>(px) / (texW - 1.0f)) * 2.0f - 1.0f; // -1.0 .. +1.0
                float absU = std::abs(u);

                float horizFactor = 0.0f;
                if (absU < 1.0f) {
                    // Гладкая колоколообразная функция cos^2 без резких срезов
                    float cosVal = std::cos(absU * 1.57079632679f);
                    horizFactor = std::pow(cosVal * cosVal, 1.25f);
                }

                float totalIntensity = horizFactor * vertFactor * topFade;
                uint8_t a = static_cast<uint8_t>(std::clamp(totalIntensity * 255.0f, 0.0f, 255.0f));
                img.setPixel({px, py}, sf::Color(255, 255, 255, a));
            }
        }
        (void)s_lampTexture.loadFromImage(img);
        s_lampTexture.setSmooth(true); // Аппаратная билинейная фильтрация для абсолютной гладкости
    }
    return s_lampTexture;
}

void Arena::ensureLampConeTextureInitialized() {
    (void)getSharedLampConeTexture();
}

namespace {

void renderSmoothAreaBeamHelper(sf::RenderTarget& target, float cx, float cy, float emitterHalfW,
                                float radius, float coneAngle, const sf::Color& color,
                                float intensity, bool isLightmap,
                                std::function<bool(float wx, float wy)> isSolidWorldFn,
                                std::shared_ptr<sf::Texture>* outTextureCache,
                                sf::Vector2f* outPosCache,
                                sf::Vector2f* outScaleCache,
                                bool* inoutDirty,
                                float bounceBoost,
                                float flickerFactor = 1.0f) {
    if (intensity <= 0.01f) return;

    // 0. Быстрый путь GPU-кэша (0.002 мс на кадр для стабильных 1500+ FPS)
    if (inoutDirty && !(*inoutDirty) && outTextureCache && *outTextureCache) {
        sf::Sprite cachedSpr(**outTextureCache);
        if (outPosCache) cachedSpr.setPosition(*outPosCache);
        if (outScaleCache) cachedSpr.setScale(*outScaleCache);

        float factor = std::clamp(flickerFactor, 0.0f, 1.0f);
        uint8_t a = static_cast<uint8_t>(std::clamp(255.0f * factor, 0.0f, 255.0f));
        cachedSpr.setColor(sf::Color(255, 255, 255, a));

        sf::RenderStates states;
        states.blendMode = sf::BlendAdd;
        target.draw(cachedSpr, states);
        return;
    }

    float emitterL = cx - emitterHalfW;
    float emitterR = cx + emitterHalfW;

    float halfConeDeg = (coneAngle > 0.0f) ? (coneAngle * 0.5f) : 67.5f;
    if (emitterHalfW > 0.0f) {
        // Потолочный светильник освещает всю комнату под потолком широким естественным потоком
        halfConeDeg = std::max(halfConeDeg, 80.0f);
    }
    float halfConeRad = halfConeDeg * (3.141592653589793f / 180.0f);

    float R = std::max(radius, 950.0f);
    const float CS = 16.0f; // 16px на ячейку: быстрое запекание и идеальная гладкость с билинейной фильтрацией

    // Границы светового поля в мировых координатах:
    // Потолочная лампа (emitterHalfW > 0) освещает комнату во всю ширину радиуса R под потолком
    // Свет НАД трубкой физически не генерируется (minWorldY = cy)!
    float spreadX = R + 32.0f;
    float minWorldX = cx - spreadX;
    float maxWorldX = cx + spreadX;
    float minWorldY = (emitterHalfW > 0.0f) ? cy : (cy - R * 0.35f);
    float maxWorldY = cy + R + 32.0f;

    minWorldX = std::floor(minWorldX / CS) * CS;
    maxWorldX = std::ceil(maxWorldX / CS) * CS;
    minWorldY = std::floor(minWorldY / CS) * CS;
    maxWorldY = std::ceil(maxWorldY / CS) * CS;

    int GW = static_cast<int>((maxWorldX - minWorldX) / CS) + 1;
    int GH = static_cast<int>((maxWorldY - minWorldY) / CS) + 1;
    if (GW < 2 || GH < 2) return;

    std::vector<float> lightField(GW * GH, 0.0f);
    std::vector<bool> solidGrid(GW * GH, false);

    // 1. Построение карты твердых блоков сетки
    if (isSolidWorldFn) {
        for (int gy = 0; gy < GH; ++gy) {
            float wy = minWorldY + (gy + 0.5f) * CS;
            for (int gx = 0; gx < GW; ++gx) {
                float wx = minWorldX + (gx + 0.5f) * CS;
                solidGrid[gy * GW + gx] = isSolidWorldFn(wx, wy);
            }
        }
    }

    const float stepDirect = 8.0f; // 8px шаг трассировки лучей
    const int NUM_TUBE_SAMPLES = (emitterHalfW > 0.0f) ? 3 : 1;
    float sampleOffsets[3] = {emitterL, cx, emitterR};

    // 2. Расчет прямого света светильника с физической мягкой полутенью
    for (int gy = 0; gy < GH; ++gy) {
        float wy = minWorldY + (gy + 0.5f) * CS;
        if (wy < cy && emitterHalfW > 0.0f) continue; // Потолочный светильник светит строго вниз

        for (int gx = 0; gx < GW; ++gx) {
            float wx = minWorldX + (gx + 0.5f) * CS;

            float cdx = wx - cx;
            float cdy = wy - cy;
            float dist = std::hypot(cdx, cdy);
            if (dist > R || dist < 2.0f) continue;

            float angFactor = 1.0f;
            if (emitterHalfW > 0.0f) {
                // Потолочный светильник: реалистично наполняет светом всю комнату под потолком
                // Имеет сфокусированный направленный луч вниз (core) + мягкое рассеянное заполнение комнаты (wash)
                float angle = std::atan2(std::abs(cdx), std::max(0.1f, cdy));
                float angleFrac = angle / halfConeRad;
                float coreFactor = (angleFrac < 1.0f) ? (0.5f * (1.0f + std::cos(angleFrac * 3.141592653589793f))) : 0.0f;
                float washFactor = std::pow(std::max(0.0f, std::cos(angle * 0.88f)), 0.25f);
                float ceilingFade = std::clamp((wy - cy) / 10.0f, 0.0f, 1.0f);
                angFactor = (0.70f * washFactor + 0.30f * coreFactor) * ceilingFade;
            } else {
                // Окно: диагональный направленный луч дневного света
                float angle = std::atan2(std::abs(cdx), std::max(0.1f, cdy));
                float angleFrac = angle / halfConeRad;
                if (angleFrac >= 1.0f) continue; // вне угла конуса окна
                angFactor = 0.5f * (1.0f + std::cos(angleFrac * 3.141592653589793f));
            }

            // Проверка для твердых блоков: вглубь стены свет не проникает абсолютно!
            bool isSolid = solidGrid[gy * GW + gx];
            if (isSolid) {
                // Твердый блок освещается только на поверхности, граничащей с воздухом
                // (проверяем 8 соседей, включая диагональные ячейки для ступеней и скосов)
                bool touchesAir = false;
                for (int dy = -1; dy <= 1 && !touchesAir; ++dy) {
                    for (int dx = -1; dx <= 1 && !touchesAir; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int ny = gy + dy;
                        int nx = gx + dx;
                        if (nx >= 0 && nx < GW && ny >= 0 && ny < GH && !solidGrid[ny * GW + nx]) {
                            touchesAir = true;
                        }
                    }
                }

                if (!touchesAir) {
                    continue; // Внутренний монолит блока — 100% тень
                }
            }

            // Плавное затухание с расстоянием (smoothstep)
            float dFrac = 1.0f - dist / R;
            float distFactor = dFrac * dFrac * (3.0f - 2.0f * dFrac);

            // Трассировка видимости к точкам светильника (мягкая полутень без резких срезов)
            float visibleSum = 0.0f;
            for (int s = 0; s < NUM_TUBE_SAMPLES; ++s) {
                float tx = sampleOffsets[s];
                float ty = cy;

                float rdx = wx - tx;
                float rdy = wy - ty;
                float rdist = std::hypot(rdx, rdy);
                if (rdist < 2.0f) {
                    visibleSum += 1.0f;
                    continue;
                }

                bool blocked = false;
                if (isSolidWorldFn) {
                    float maxCheckD = isSolid ? (rdist - CS * 0.95f) : (rdist - CS * 0.45f);
                    int numSteps = static_cast<int>(maxCheckD / stepDirect);
                    for (int st = 1; st <= numSteps; ++st) {
                        float curD = st * stepDirect;
                        if (isSolidWorldFn(tx + (rdx / rdist) * curD, ty + (rdy / rdist) * curD)) {
                            blocked = true;
                            break;
                        }
                    }
                }
                if (!blocked) {
                    visibleSum += 1.0f;
                }
            }

            float vis = visibleSum / static_cast<float>(NUM_TUBE_SAMPLES);
            if (vis <= 0.01f) continue;

            float directIntensity = intensity * vis * angFactor * distFactor;
            lightField[gy * GW + gx] = isSolid ? (directIntensity * 0.85f) : directIntensity;
        }
    }

    // 3. Двухпроходное сглаживание ТОЛЬКО по ячейкам воздуха (Air-Only Smoothing)
    // Дает идеально гладкий световой конус без дискретных артефактов и не трогает твердые блоки
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<float> blurred = lightField;
        for (int gy = 1; gy < GH - 1; ++gy) {
            for (int gx = 1; gx < GW - 1; ++gx) {
                if (solidGrid[gy * GW + gx]) continue;
                float sum = 0.0f;
                float totalWeight = 0.0f;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int ny = gy + dy;
                        int nx = gx + dx;
                        if (!solidGrid[ny * GW + nx]) {
                            float w = (dx == 0 && dy == 0) ? 4.0f : ((dx == 0 || dy == 0) ? 2.0f : 1.0f);
                            sum += lightField[ny * GW + nx] * w;
                            totalWeight += w;
                        }
                    }
                }
                if (totalWeight > 0.0f) {
                    blurred[gy * GW + gx] = sum / totalWeight;
                }
            }
        }
        lightField = std::move(blurred);
    }

    // 3b. Диффузия света на поверхность твердых блоков и ступеней (Surface Lighting Diffusion):
    // Поверхность блоков и ступеней, граничащая с освещенным воздухом комнаты, получает
    // естественную мягкую подсветку от соседних ячеек воздуха, выравнивая освещение склонов
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<float> surfaceLit = lightField;
        for (int gy = 1; gy < GH - 1; ++gy) {
            for (int gx = 1; gx < GW - 1; ++gx) {
                if (!solidGrid[gy * GW + gx]) continue; // Только твердые ячейки
                float airSum = 0.0f;
                float airWeight = 0.0f;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int ny = gy + dy;
                        int nx = gx + dx;
                        if (!solidGrid[ny * GW + nx]) {
                            float w = (dx == 0 || dy == 0) ? 2.5f : 1.5f;
                            airSum += lightField[ny * GW + nx] * w;
                            airWeight += w;
                        }
                    }
                }
                if (airWeight > 0.0f) {
                    float neighborAirLight = airSum / airWeight;
                    surfaceLit[gy * GW + gx] = std::max(surfaceLit[gy * GW + gx], neighborAirLight * 0.95f);
                }
            }
        }
        lightField = std::move(surfaceLit);
    }

    // 4. Построение гладкой текстуры света с аппаратной билинейной интерполяцией
    sf::Image lightImg({static_cast<unsigned int>(GW), static_cast<unsigned int>(GH)}, sf::Color::Transparent);

    for (int gy = 0; gy < GH; ++gy) {
        for (int gx = 0; gx < GW; ++gx) {
            float val = std::clamp(lightField[gy * GW + gx], 0.0f, 1.0f);
            if (val <= 0.002f) continue;

            if (isLightmap) {
                uint8_t r = static_cast<uint8_t>(std::clamp(color.r * 0.95f * val, 0.0f, 255.0f));
                uint8_t g = static_cast<uint8_t>(std::clamp(color.g * 0.96f * val, 0.0f, 255.0f));
                uint8_t b = static_cast<uint8_t>(std::clamp(color.b * 0.98f * val, 0.0f, 255.0f));
                lightImg.setPixel({static_cast<unsigned int>(gx), static_cast<unsigned int>(gy)}, sf::Color(r, g, b, 255));
            } else {
                uint8_t r = static_cast<uint8_t>(std::clamp(color.r * 0.30f * val, 0.0f, 255.0f));
                uint8_t g = static_cast<uint8_t>(std::clamp(color.g * 0.31f * val, 0.0f, 255.0f));
                uint8_t b = static_cast<uint8_t>(std::clamp(color.b * 0.33f * val, 0.0f, 255.0f));
                lightImg.setPixel({static_cast<unsigned int>(gx), static_cast<unsigned int>(gy)}, sf::Color(r, g, b, 255));
            }
        }
    }

    auto bakedTex = std::make_shared<sf::Texture>();
    (void)bakedTex->loadFromImage(lightImg);
    bakedTex->setSmooth(true);

    sf::Vector2f worldPos(minWorldX, minWorldY);
    sf::Vector2f worldScale(CS, CS);

    if (outTextureCache) *outTextureCache = bakedTex;
    if (outPosCache) *outPosCache = worldPos;
    if (outScaleCache) *outScaleCache = worldScale;
    if (inoutDirty) *inoutDirty = false;

    sf::Sprite spr(*bakedTex);
    spr.setPosition(worldPos);
    spr.setScale(worldScale);

    sf::RenderStates states;
    states.blendMode = sf::BlendAdd;
    target.draw(spr, states);
}

} // anonymous namespace

void Arena::renderSmoothLampBeam(sf::RenderTarget& target, float lx, float ly, float tileSize,
                                 float radius, float coneAngle, const sf::Color& color,
                                 float intensity, bool isLightmap,
                                 std::function<bool(float wx, float wy)> isSolidWorldFn,
                                 std::shared_ptr<sf::Texture>* outTextureCache,
                                 sf::Vector2f* outPosCache,
                                 sf::Vector2f* outScaleCache,
                                 bool* inoutDirty,
                                 float flickerFactor) {
    float cx = lx + tileSize * 0.5f;
    float cy = ly + 14.0f;
    float tubeHalfW = tileSize * 0.5f - 4.0f;
    renderSmoothAreaBeamHelper(target, cx, cy, tubeHalfW, radius, coneAngle, color, intensity, isLightmap,
                               isSolidWorldFn, outTextureCache, outPosCache, outScaleCache, inoutDirty, 1.0f, flickerFactor);
}

void Arena::renderSmoothWindowBeam(sf::RenderTarget& target, float wx, float wy, float tileSize,
                                   float radius, float coneAngle, const sf::Color& color,
                                   float intensity, bool isLightmap,
                                   std::function<bool(float wx, float wy)> isSolidWorldFn,
                                   std::shared_ptr<sf::Texture>* outTextureCache,
                                   sf::Vector2f* outPosCache,
                                   sf::Vector2f* outScaleCache,
                                   bool* inoutDirty) {
    float cx = wx + tileSize * 0.5f;
    float cy = wy + tileSize * 0.5f;
    float winHalfW = 0.0f; // Окно: диагональный направленный луч дневного света (emitterHalfW = 0)
    renderSmoothAreaBeamHelper(target, cx, cy, winHalfW, radius, coneAngle, color, intensity, isLightmap,
                               isSolidWorldFn, outTextureCache, outPosCache, outScaleCache, inoutDirty, 1.0f);
}

void Arena::renderLightingLightmapPass(const sf::View& camera) {
    _lightmap.setView(camera);
    // Подвал и обычные этажи имеют одинаковый базовый эмбиент (без искусственного пересвечивания фона)
    _lightmap.clear(_ambientColor);

    auto isSolidWorld = [this](float wx, float wy) -> bool {
        return isPointSolid(wx, wy);
    };

    // 1. Освещение от активных потолочных ламп
    for (auto& lamp : _lamps) {
        if (!lamp.enabled) continue;

        float flickerFactor = std::max(0.05f, 1.0f - lamp.flickerDip);
        float effectiveIntensity = lamp.intensity * flickerFactor;
        if (effectiveIntensity <= 0.01f) continue;

        float lx = static_cast<float>(lamp.tileX * _tileSize);
        float ly = static_cast<float>(lamp.tileY * _tileSize);

        renderSmoothLampBeam(_lightmap, lx, ly, _tileSize, lamp.radius, lamp.coneAngle, lamp.color, effectiveIntensity, true, isSolidWorld,
                             &lamp.bakedTexture, &lamp.bakedWorldPos, &lamp.bakedScale, &lamp.dirty, flickerFactor);
    }

    // 2. Естественный дневной свет из окон (на обычных надземных этажах)
    if (!isBasement()) {
        for (auto& win : _windows) {
            if (win.intensity <= 0.01f) continue;
            float wx = static_cast<float>(win.tileX * _tileSize);
            float wy = static_cast<float>(win.tileY * _tileSize);

            renderSmoothWindowBeam(_lightmap, wx, wy, _tileSize, win.radius, win.coneAngle, win.color, win.intensity, true, isSolidWorld,
                                   &win.bakedTexture, &win.bakedWorldPos, &win.bakedScale, &win.dirty);
        }
    }

    _lightmap.display();
}

void Arena::renderLightingMultiplyPass(sf::RenderWindow& window, const sf::View& camera) {
    sf::RenderStates mulStates;
    mulStates.blendMode = sf::BlendMultiply;
    sf::Sprite lightSprite(_lightmap.getTexture());
    window.setView(window.getDefaultView());
    window.draw(lightSprite, mulStates);
    window.setView(camera);
}

void Arena::renderLightingHalos(sf::RenderWindow& window) {
    // Мягкий плафонный ореол светильника непосредственно вокруг стеклянной колбы (строго вниз в воздух, не касаясь потолка)
    for (const auto& lamp : _lamps) {
        if (!lamp.enabled) continue;
        float effectiveIntensity = lamp.intensity * std::max(0.05f, 1.0f - lamp.flickerDip);
        if (effectiveIntensity <= 0.01f) continue;

        float lx = static_cast<float>(lamp.tileX * _tileSize);
        float ly = static_cast<float>(lamp.tileY * _tileSize);
        sf::Vector2f tubeCenter(lx + _tileSize * 0.5f, ly + 14.0f);
        float haloRadX = 35.0f;
        float haloRadY = 12.0f;
        const int haloSegments = 16;

        sf::VertexArray haloVA(sf::PrimitiveType::Triangles);
        sf::Color centerCol(lamp.color.r, lamp.color.g, lamp.color.b, static_cast<uint8_t>(std::clamp(32.0f * effectiveIntensity, 0.0f, 255.0f)));
        sf::Color edgeCol(0, 0, 0, 0);

        for (int i = 0; i < haloSegments; ++i) {
            float a0 = (3.14159265f / haloSegments) * i;
            float a1 = (3.14159265f / haloSegments) * (i + 1);

            sf::Vector2f p0(tubeCenter.x + std::cos(a0) * haloRadX, tubeCenter.y + std::sin(a0) * haloRadY);
            sf::Vector2f p1(tubeCenter.x + std::cos(a1) * haloRadX, tubeCenter.y + std::sin(a1) * haloRadY);

            haloVA.append({tubeCenter, centerCol});
            haloVA.append({p0, edgeCol});
            haloVA.append({p1, edgeCol});
        }
        sf::RenderStates haloStates;
        haloStates.blendMode = sf::BlendAdd;
        window.draw(haloVA, haloStates);
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

    renderLightingLightmapPass(camera);
    renderLightingMultiplyPass(window, camera);
    renderLightingHalos(window);
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
