#include "graphics/BossVfx.h"

void BossVfxManager::spawnSlash(const sf::FloatRect& hitbox, bool facingLeft, BossSlashType type, float duration, float groundY) {
    BossSlashInstance inst;
    inst.hitbox = hitbox;
    inst.facingLeft = facingLeft;
    inst.type = type;
    inst.timer = 0.0f;
    inst.maxDuration = duration;
    inst.groundY = (groundY > 0.0f) ? groundY : (hitbox.position.y + hitbox.size.y);
    inst.active = true;

    float dir = facingLeft ? -1.0f : 1.0f;
    float x = hitbox.position.x;
    float y = hitbox.position.y;
    float w = hitbox.size.x;
    float h = hitbox.size.y;
    float pivotX = facingLeft ? (x + w * 0.90f) : (x + w * 0.10f);

    int sparkCount = 38;
    if (type == BossSlashType::WHIRLWIND_360) sparkCount = 52;
    else if (type == BossSlashType::CROSS_X) sparkCount = 48;
    else if (type == BossSlashType::HEAVY_OVERHEAD) sparkCount = 46;

    for (int i = 0; i < sparkCount; ++i) {
        BossSlashInstance::Spark s;
        float u = i / static_cast<float>(sparkCount);

        if (type == BossSlashType::WHIRLWIND_360) {
            float angle = static_cast<float>(rand() % 360);
            float rad = angle * 3.14159f / 180.0f;
            float rDist = (w * 0.22f) + static_cast<float>(rand() % static_cast<int>(w * 0.26f + 1.0f));
            s.pos = {x + w * 0.5f + std::cos(rad) * rDist, y + h * 0.52f + std::sin(rad) * (rDist * 0.85f)};
            float spd = 340.0f + static_cast<float>(rand() % 340);
            s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        } else if (type == BossSlashType::PIERCING_THRUST) {
            float fx = pivotX + dir * (w * (0.2f + 0.75f * u));
            float fy = y + h * 0.52f + ((rand() % 36) - 18.0f);
            s.pos = {fx, fy};
            float spd = 560.0f + static_cast<float>(rand() % 420);
            float ang = (facingLeft ? 180.0f : 0.0f) + (-16.0f + static_cast<float>(rand() % 32));
            float rad = ang * 3.14159f / 180.0f;
            s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        } else if (type == BossSlashType::UPWARD_CLEAVE) {
            float ang = 1.15f - u * 2.25f;
            float px = pivotX + dir * std::cos(ang) * (w * 0.85f);
            float py = y + h * 0.62f + std::sin(ang) * (h * 0.50f);
            s.pos = {px, py};
            float spd = 320.0f + static_cast<float>(rand() % 340);
            float outAng = (facingLeft ? 180.0f : 0.0f) + (-65.0f - static_cast<float>(rand() % 50));
            float rad = outAng * 3.14159f / 180.0f;
            s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        } else if (type == BossSlashType::HEAVY_OVERHEAD) {
            float ang = -1.45f + u * 2.80f;
            float px = pivotX + dir * std::cos(ang) * (w * 0.78f);
            float py = y + h * 0.18f + std::sin(ang) * (h * 0.82f);
            s.pos = {px, py};
            float spd = 360.0f + static_cast<float>(rand() % 380);
            float outAng = (facingLeft ? 180.0f : 0.0f) + (35.0f + static_cast<float>(rand() % 55));
            float rad = outAng * 3.14159f / 180.0f;
            s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        } else if (type == BossSlashType::CROSS_X) {
            bool d2 = (i % 2 == 1);
            float ang = d2 ? (1.05f - u * 1.90f) : (-1.05f + u * 1.90f);
            float pyOff = d2 ? (h * 0.72f) : (h * 0.28f);
            float px = pivotX + dir * std::cos(ang) * (w * 0.82f);
            float py = y + pyOff + std::sin(ang) * (h * 0.45f);
            s.pos = {px, py};
            float spd = 300.0f + static_cast<float>(rand() % 350);
            float baseAngle = (facingLeft ? 180.0f : 0.0f) + (-45.0f + static_cast<float>(rand() % 90));
            float rad = baseAngle * 3.14159f / 180.0f;
            s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        } else { // HORIZONTAL_CLEAVE
            float ang = -0.95f + u * 1.75f;
            float px = pivotX + dir * std::cos(ang) * (w * 0.86f);
            float py = y + h * 0.52f + std::sin(ang) * (h * 0.42f);
            s.pos = {px, py};
            float spd = 300.0f + static_cast<float>(rand() % 320);
            float baseAngle = (facingLeft ? 180.0f : 0.0f) + (-35.0f + static_cast<float>(rand() % 70));
            float rad = baseAngle * 3.14159f / 180.0f;
            s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        }

        s.prevPos = s.pos;
        s.timer = 0.0f;
        s.maxLife = 0.22f + (rand() % 100) / 400.0f;
        s.size = (i % 4 == 0) ? 3.0f : 1.8f;
        s.color = (i % 3 == 0) ? sf::Color(255, 255, 240) :
                  ((i % 3 == 1) ? sf::Color(255, 195, 45) : sf::Color(255, 65, 20));
        s.isFloorSpark = false;
        inst.sparks.push_back(s);
    }

    // Искры от трения о пол при низких или вертикальных ударах
    if (type == BossSlashType::HEAVY_OVERHEAD || type == BossSlashType::UPWARD_CLEAVE ||
        (inst.groundY - (y + h) < 35.0f)) {
        int floorSparks = (type == BossSlashType::HEAVY_OVERHEAD) ? 18 : 10;
        float sparkX = pivotX + dir * (w * 0.55f);
        for (int k = 0; k < floorSparks; ++k) {
            BossSlashInstance::Spark fs;
            fs.pos = {sparkX + ((rand() % 40) - 20.0f), inst.groundY - 2.0f};
            fs.prevPos = fs.pos;
            float fspd = 280.0f + static_cast<float>(rand() % 380);
            float fdir = (k % 3 == 0) ? -dir : dir;
            float fAng = (fdir > 0.0f ? 0.0f : 180.0f) + (-10.0f - (rand() % 35));
            float frad = fAng * 3.14159f / 180.0f;
            fs.vel = {std::cos(frad) * fspd, std::sin(frad) * fspd};
            fs.timer = 0.0f;
            fs.maxLife = 0.16f + (rand() % 100) / 500.0f;
            fs.size = 2.2f;
            fs.color = (k % 2 == 0) ? sf::Color(255, 250, 220) : sf::Color(255, 160, 40);
            fs.isFloorSpark = true;
            inst.sparks.push_back(fs);
        }
    }

    m_slashes.push_back(inst);
}

void BossVfxManager::spawnDustCloud(const sf::Vector2f& pos, const sf::Vector2f& vel, float radius, sf::Color color, float life) {
    BossDustPuff p;
    p.pos = pos;
    p.vel = vel;
    p.currentRadius = radius * 0.45f;
    p.maxRadius = radius * (1.3f + (rand() % 40) / 100.0f);
    p.rot = static_cast<float>(rand() % 360);
    p.rotSpeed = -140.0f + static_cast<float>(rand() % 280);
    p.timer = 0.0f;
    p.maxDuration = life;
    p.color = color;
    p.active = true;
    m_dustPuffs.push_back(p);
}

void BossVfxManager::spawnArenaShockwaves(const sf::Vector2f& groundPos, float maxDist) {
    // Две бегущие в противоположные стороны волны сотрясения земли
    BossArenaShockwave waveL;
    waveL.x = groundPos.x - 30.0f;
    waveL.groundY = groundPos.y;
    waveL.dir = -1.0f;
    waveL.speed = 700.0f;
    waveL.currentDist = 0.0f;
    waveL.maxDist = maxDist;
    waveL.puffTimer = 0.0f;
    waveL.active = true;
    m_arenaWaves.push_back(waveL);

    BossArenaShockwave waveR;
    waveR.x = groundPos.x + 30.0f;
    waveR.groundY = groundPos.y;
    waveR.dir = 1.0f;
    waveR.speed = 700.0f;
    waveR.currentDist = 0.0f;
    waveR.maxDist = maxDist;
    waveR.puffTimer = 0.0f;
    waveR.active = true;
    m_arenaWaves.push_back(waveR);
}

void BossVfxManager::spawnGroundImpact(const sf::Vector2f& groundPos, float width, bool intense) {
    float halfW = width * 0.5f;

    // 1. Огромный купол клубящейся пыли в эпицентре удара (пыль поднимается высоко в воздух!)
    int centerPuffs = intense ? 36 : 24;
    for (int i = 0; i < centerPuffs; ++i) {
        float offX = ((rand() % 100) / 50.0f - 1.0f) * (halfW * 0.75f);
        sf::Vector2f pPos = {groundPos.x + offX, groundPos.y - 4.0f};
        float upwardSpeed = 120.0f + static_cast<float>(rand() % 240);
        float horizSpeed = offX * 1.8f + (-40.0f + static_cast<float>(rand() % 80));
        sf::Color dustCol = (i % 3 == 0) ? sf::Color(175, 145, 110) :
                            ((i % 3 == 1) ? sf::Color(115, 95, 80) : sf::Color(70, 58, 52));
        spawnDustCloud(pPos, {horizSpeed, -upwardSpeed}, (intense ? 28.0f : 20.0f) + (rand() % 16), dustCol, 1.0f + (rand() % 40) / 100.0f);
    }

    // 2. Растущие каменные шипы из разлома земли
    int spikeCount = intense ? 8 : 6;
    for (int i = 0; i < spikeCount; ++i) {
        BossGroundSpike sp;
        float offsetRatio = (i / static_cast<float>(spikeCount - 1)) * 2.0f - 1.0f; // -1 to 1
        float jitter = ((rand() % 100) / 100.0f - 0.5f) * (width / spikeCount) * 0.4f;
        sp.basePos = {groundPos.x + offsetRatio * (halfW * 0.85f) + jitter, groundPos.y};
        sp.width = (intense ? 28.0f : 22.0f) + (rand() % 14);
        sp.targetHeight = (intense ? 65.0f : 45.0f) + (rand() % 35);
        if (std::abs(offsetRatio) < 0.35f) sp.targetHeight *= 1.40f;
        sp.currentHeight = 0.0f;
        sp.tilt = offsetRatio * 0.28f + ((rand() % 100) / 100.0f - 0.5f) * 0.15f;
        sp.timer = 0.0f;
        sp.maxDuration = intense ? 1.15f : 0.85f;
        sp.rockColor = (i % 2 == 0) ? sf::Color(72, 60, 55) : sf::Color(95, 82, 74);
        sp.active = true;
        m_spikes.push_back(sp);
    }

    // 3. Разломы и пылающие трещины в земле
    BossGroundCrack crack;
    crack.center = groundPos;
    crack.length = width * 0.95f;
    crack.timer = 0.0f;
    crack.maxDuration = 1.7f;
    crack.active = true;
    int segs = 14;
    for (int i = 0; i <= segs; ++i) {
        float px = groundPos.x - crack.length * 0.5f + (i / static_cast<float>(segs)) * crack.length;
        float py = groundPos.y + ((rand() % 100) / 100.0f - 0.5f) * 8.0f;
        crack.jaggedPoints.push_back({px, py});
    }
    m_cracks.push_back(crack);

    // 4. Расширяющиеся кольца ударной волны
    for (int r = 0; r < (intense ? 3 : 2); ++r) {
        BossShockwaveRing ring;
        ring.center = groundPos;
        ring.radiusX = 18.0f;
        ring.radiusY = 7.0f;
        ring.maxRadiusX = halfW * (1.0f + r * 0.35f);
        ring.speed = 460.0f + r * 160.0f;
        ring.timer = 0.0f;
        ring.maxDuration = 0.45f + r * 0.09f;
        ring.color = (r == 0) ? sf::Color(255, 235, 110) : sf::Color(255, 55, 25);
        ring.active = true;
        m_rings.push_back(ring);
    }

    // 5. Разлетающиеся комья земли и осколки камня
    int rockCount = intense ? 26 : 16;
    for (int i = 0; i < rockCount; ++i) {
        BossDebrisChunk d;
        d.pos = {groundPos.x + ((rand() % 100) / 50.0f - 1.0f) * (halfW * 0.45f), groundPos.y - 4.0f};
        float angle = -165.0f + (rand() % 150);
        float spd = 240.0f + (rand() % 420);
        float rad = angle * 3.14159f / 180.0f;
        d.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        d.rot = static_cast<float>(rand() % 360);
        d.rotSpeed = -400.0f + (rand() % 800);
        d.size = 6.0f + (rand() % 10);
        d.timer = 0.0f;
        d.maxDuration = 0.65f + (rand() % 100) / 250.0f;
        d.color = (i % 3 == 0) ? sf::Color(65, 55, 58) : ((i % 3 == 1) ? sf::Color(105, 90, 78) : sf::Color(195, 75, 38));
        d.active = true;
        m_debris.push_back(d);
    }

    // 6. Запуск бегущей волны сотрясения земли на всю арену (пылевая буря по всему полу)
    spawnArenaShockwaves(groundPos, 1100.0f);
}

void BossVfxManager::spawnJumpSlamBeam(const sf::FloatRect& hitbox, float duration) {
    BossJumpSlamPillar p;
    p.hitbox = hitbox;
    p.timer = 0.0f;
    p.maxDuration = duration;
    p.active = true;

    // Генерация 4 ветвящихся молний вдоль силового столба
    for (int b = 0; b < 4; ++b) {
        std::vector<sf::Vector2f> branch;
        float startX = hitbox.position.x + hitbox.size.x * (0.25f + b * 0.18f);
        float curX = startX;
        float startY = hitbox.position.y;
        float endY = hitbox.position.y + hitbox.size.y;
        int pts = 10;
        for (int i = 0; i <= pts; ++i) {
            float y = startY + (i / static_cast<float>(pts)) * (endY - startY);
            curX += (-18.0f + (rand() % 36));
            branch.push_back({curX, y});
        }
        p.lightningBranches.push_back(branch);
    }

    m_pillars.push_back(p);
}

void BossVfxManager::spawnWallCrash(const sf::Vector2f& impactPos, float dirX) {
    BossWallCrashEffect w;
    w.pos = impactPos;
    w.dirX = dirX;
    w.timer = 0.0f;
    w.maxDuration = 0.75f;
    w.active = true;

    int chunkCount = 28;
    for (int i = 0; i < chunkCount; ++i) {
        BossDebrisChunk d;
        d.pos = impactPos;
        float angle = (dirX > 0.0f) ? (100.0f + (rand() % 160)) : (-80.0f + (rand() % 160));
        float spd = 260.0f + (rand() % 480);
        float rad = angle * 3.14159f / 180.0f;
        d.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        d.rot = static_cast<float>(rand() % 360);
        d.rotSpeed = -450.0f + (rand() % 900);
        d.size = 5.0f + (rand() % 10);
        d.timer = 0.0f;
        d.maxDuration = 0.65f + (rand() % 100) / 250.0f;
        d.color = (i % 2 == 0) ? sf::Color(78, 68, 65) : sf::Color(120, 105, 95);
        d.active = true;
        w.chunks.push_back(d);
    }

    // Выброс облака пыли из точки сокрушения о стену
    for (int p = 0; p < 16; ++p) {
        float vy = -80.0f + static_cast<float>(rand() % 160);
        float vx = (dirX > 0.0f ? -1.0f : 1.0f) * (110.0f + static_cast<float>(rand() % 180));
        spawnDustCloud(impactPos, {vx, vy}, 22.0f + (rand() % 14), sf::Color(150, 130, 110), 0.85f);
    }

    m_wallCrashes.push_back(w);
}

void BossVfxManager::spawnAfterimage(const sf::Vector2f& pos, const sf::Vector2f& size, const sf::Vector2f& origin, bool facingLeft) {
    BossAfterimageGhost g;
    g.pos = pos;
    g.size = size;
    g.origin = origin;
    g.facingLeft = facingLeft;
    g.timer = 0.0f;
    g.maxDuration = 0.22f;
    g.active = true;
    m_afterimages.push_back(g);
}

void BossVfxManager::update(float dt) {
    // 1. Взмахи
    for (auto& s : m_slashes) {
        if (!s.active) continue;
        s.timer += dt;
        if (s.timer >= s.maxDuration) {
            s.active = false;
        } else {
            for (auto& sp : s.sparks) {
                sp.timer += dt;
                sp.prevPos = sp.pos;
                sp.pos += sp.vel * dt;
                sp.vel.x *= std::max(0.0f, 1.0f - 1.8f * dt);
                if (!sp.isFloorSpark) {
                    sp.vel.y += 420.0f * dt;
                }
                if (s.groundY > 0.0f && sp.pos.y >= s.groundY) {
                    sp.pos.y = s.groundY;
                    sp.vel.y = -sp.vel.y * 0.35f;
                    sp.vel.x *= 0.72f;
                }
            }
        }
    }
    m_slashes.erase(std::remove_if(m_slashes.begin(), m_slashes.end(), [](const BossSlashInstance& s) { return !s.active; }), m_slashes.end());

    // 2. Шипы
    for (auto& sp : m_spikes) {
        if (!sp.active) continue;
        sp.timer += dt;
        if (sp.timer >= sp.maxDuration) {
            sp.active = false;
        } else {
            float growTime = 0.08f;
            if (sp.timer < growTime) {
                sp.currentHeight = sp.targetHeight * (sp.timer / growTime);
            } else {
                float fadeStart = sp.maxDuration * 0.65f;
                if (sp.timer > fadeStart) {
                    float fadeProg = (sp.timer - fadeStart) / (sp.maxDuration - fadeStart);
                    sp.currentHeight = sp.targetHeight * (1.0f - fadeProg);
                } else {
                    sp.currentHeight = sp.targetHeight;
                }
            }
        }
    }
    m_spikes.erase(std::remove_if(m_spikes.begin(), m_spikes.end(), [](const BossGroundSpike& sp) { return !sp.active; }), m_spikes.end());

    // 3. Осколки
    for (auto& d : m_debris) {
        if (!d.active) continue;
        d.timer += dt;
        if (d.timer >= d.maxDuration) {
            d.active = false;
        } else {
            d.pos += d.vel * dt;
            d.vel.y += 1400.0f * dt;
            d.rot += d.rotSpeed * dt;
        }
    }
    m_debris.erase(std::remove_if(m_debris.begin(), m_debris.end(), [](const BossDebrisChunk& d) { return !d.active; }), m_debris.end());

    // 4. Клубы пыли
    for (auto& p : m_dustPuffs) {
        if (!p.active) continue;
        p.timer += dt;
        if (p.timer >= p.maxDuration) {
            p.active = false;
        } else {
            p.pos += p.vel * dt;
            p.vel.y *= (1.0f - dt * 1.6f); // Плавное замедление подъема
            p.vel.x *= (1.0f - dt * 2.0f);
            p.rot += p.rotSpeed * dt;
            p.currentRadius += (p.maxRadius - p.currentRadius) * (dt * 2.8f);
        }
    }
    m_dustPuffs.erase(std::remove_if(m_dustPuffs.begin(), m_dustPuffs.end(), [](const BossDustPuff& p) { return !p.active; }), m_dustPuffs.end());

    // 5. Бегущие по арене волны пылевой бури
    for (auto& w : m_arenaWaves) {
        if (!w.active) continue;
        float moveDelta = w.speed * w.dir * dt;
        w.x += moveDelta;
        w.currentDist += std::abs(moveDelta);
        w.puffTimer -= dt;

        // Каждые ~0.022 секунды фронт волны поднимает новый столб клубящейся пыли
        if (w.puffTimer <= 0.0f) {
            w.puffTimer = 0.024f;
            float pY = w.groundY - 6.0f;
            float upwardVel = 110.0f + static_cast<float>(rand() % 190);
            float horizVel = w.dir * (40.0f + static_cast<float>(rand() % 70));
            sf::Color col = ((rand() % 2 == 0) ? sf::Color(165, 138, 105) : sf::Color(120, 100, 82));
            spawnDustCloud({w.x + ((rand() % 20) - 10.0f), pY}, {horizVel, -upwardVel}, 18.0f + (rand() % 14), col, 0.95f + (rand() % 30) / 100.0f);
        }

        if (w.currentDist >= w.maxDist) {
            w.active = false;
        }
    }
    m_arenaWaves.erase(std::remove_if(m_arenaWaves.begin(), m_arenaWaves.end(), [](const BossArenaShockwave& w) { return !w.active; }), m_arenaWaves.end());

    // 6. Трещины в земле
    for (auto& c : m_cracks) {
        if (!c.active) continue;
        c.timer += dt;
        if (c.timer >= c.maxDuration) c.active = false;
    }
    m_cracks.erase(std::remove_if(m_cracks.begin(), m_cracks.end(), [](const BossGroundCrack& c) { return !c.active; }), m_cracks.end());

    // 7. Кольца
    for (auto& r : m_rings) {
        if (!r.active) continue;
        r.timer += dt;
        if (r.timer >= r.maxDuration) {
            r.active = false;
        } else {
            r.radiusX += r.speed * dt;
            r.radiusY = r.radiusX * 0.38f;
        }
    }
    m_rings.erase(std::remove_if(m_rings.begin(), m_rings.end(), [](const BossShockwaveRing& r) { return !r.active; }), m_rings.end());

    // 8. Столбы Jump Slam
    for (auto& p : m_pillars) {
        if (!p.active) continue;
        p.timer += dt;
        if (p.timer >= p.maxDuration) p.active = false;
    }
    m_pillars.erase(std::remove_if(m_pillars.begin(), m_pillars.end(), [](const BossJumpSlamPillar& p) { return !p.active; }), m_pillars.end());

    // 9. Врезания в стену
    for (auto& w : m_wallCrashes) {
        if (!w.active) continue;
        w.timer += dt;
        if (w.timer >= w.maxDuration) {
            w.active = false;
        } else {
            for (auto& d : w.chunks) {
                d.timer += dt;
                d.pos += d.vel * dt;
                d.vel.y += 1400.0f * dt;
                d.rot += d.rotSpeed * dt;
            }
        }
    }
    m_wallCrashes.erase(std::remove_if(m_wallCrashes.begin(), m_wallCrashes.end(), [](const BossWallCrashEffect& w) { return !w.active; }), m_wallCrashes.end());

    // 10. Фантомы
    for (auto& g : m_afterimages) {
        if (!g.active) continue;
        g.timer += dt;
        if (g.timer >= g.maxDuration) g.active = false;
    }
    m_afterimages.erase(std::remove_if(m_afterimages.begin(), m_afterimages.end(), [](const BossAfterimageGhost& g) { return !g.active; }), m_afterimages.end());
}

void BossVfxManager::draw(sf::RenderTarget& target) {
    // 1. Отрисовка пылающих разломов и трещин на полу
    for (const auto& c : m_cracks) {
        if (!c.active || c.jaggedPoints.size() < 2) continue;
        float prog = c.timer / c.maxDuration;
        float alpha = (1.0f - prog) * 230.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        // Внешнее свечение трещины
        sf::VertexArray glowLines(sf::PrimitiveType::LineStrip);
        for (const auto& pt : c.jaggedPoints) {
            glowLines.append({pt, sf::Color(255, 60, 20, static_cast<std::uint8_t>(a * 0.7f)), {0.0f, 0.0f}});
        }
        target.draw(glowLines);

        // Внутренняя раскаленная жила разлома
        sf::VertexArray coreLines(sf::PrimitiveType::LineStrip);
        for (const auto& pt : c.jaggedPoints) {
            coreLines.append({{pt.x, pt.y - 1.0f}, sf::Color(255, 230, 90, a), {0.0f, 0.0f}});
        }
        target.draw(coreLines);
    }

    // 2. Отрисовка клубящейся пыли (Volumetric Dust Clouds по всей арене)
    for (const auto& p : m_dustPuffs) {
        if (!p.active) continue;
        float prog = p.timer / p.maxDuration;
        float alpha = (prog < 0.15f) ? (prog / 0.15f * 180.0f) : ((1.0f - prog) / 0.85f * 180.0f);
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        // Внешнее мягкое облако
        sf::CircleShape outer(p.currentRadius);
        outer.setOrigin({p.currentRadius, p.currentRadius});
        outer.setPosition(p.pos);
        outer.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b, static_cast<std::uint8_t>(a * 0.55f)));
        target.draw(outer);

        // Внутреннее плотное ядро дыма/пыли
        float innerRad = p.currentRadius * 0.60f;
        sf::CircleShape inner(innerRad);
        inner.setOrigin({innerRad, innerRad});
        inner.setPosition({p.pos.x + 2.0f, p.pos.y - 3.0f});
        inner.setFillColor(sf::Color(std::min(255, p.color.r + 25), std::min(255, p.color.g + 20), std::min(255, p.color.b + 15), a));
        target.draw(inner);
    }

    // 3. Отрисовка каменных шипов
    for (const auto& sp : m_spikes) {
        if (!sp.active || sp.currentHeight <= 1.0f) continue;
        float prog = sp.timer / sp.maxDuration;
        float alpha = (prog > 0.80f) ? (1.0f - (prog - 0.80f) / 0.20f) : 1.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));

        float bx = sp.basePos.x;
        float by = sp.basePos.y;
        float hw = sp.width * 0.5f;
        float tipX = bx + sp.tilt * sp.currentHeight;
        float tipY = by - sp.currentHeight;

        // Теневая грань шипа
        sf::VertexArray shadowSide(sf::PrimitiveType::Triangles, 3);
        shadowSide[0] = {{bx - hw, by}, sf::Color(sp.rockColor.r - 20, sp.rockColor.g - 20, sp.rockColor.b - 20, a), {0.0f, 0.0f}};
        shadowSide[1] = {{tipX, tipY}, sf::Color(sp.rockColor.r + 10, sp.rockColor.g + 10, sp.rockColor.b + 10, a), {0.0f, 0.0f}};
        shadowSide[2] = {{bx, by + 2.0f}, sf::Color(sp.rockColor.r - 15, sp.rockColor.g - 15, sp.rockColor.b - 15, a), {0.0f, 0.0f}};
        target.draw(shadowSide);

        // Освещенная грань шипа
        sf::VertexArray lightSide(sf::PrimitiveType::Triangles, 3);
        lightSide[0] = {{bx, by + 2.0f}, sf::Color(sp.rockColor.r + 25, sp.rockColor.g + 25, sp.rockColor.b + 25, a), {0.0f, 0.0f}};
        lightSide[1] = {{tipX, tipY}, sf::Color(230, 200, 160, a), {0.0f, 0.0f}};
        lightSide[2] = {{bx + hw, by}, sf::Color(sp.rockColor.r + 15, sp.rockColor.g + 15, sp.rockColor.b + 15, a), {0.0f, 0.0f}};
        target.draw(lightSide);

        // Огненная подсветка у основания шипа
        sf::CircleShape baseGlow(hw * 0.9f);
        baseGlow.setOrigin({hw * 0.9f, hw * 0.9f});
        baseGlow.setPosition({bx, by});
        baseGlow.setFillColor(sf::Color(255, 90, 20, static_cast<std::uint8_t>(a * 0.35f)));
        target.draw(baseGlow);
    }

    // 4. Отрисовка осколков камней
    for (const auto& d : m_debris) {
        if (!d.active) continue;
        float prog = d.timer / d.maxDuration;
        float alpha = (1.0f - prog * prog) * 255.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        sf::RectangleShape chunk({d.size, d.size * 0.75f});
        chunk.setOrigin({d.size * 0.5f, d.size * 0.375f});
        chunk.setPosition(d.pos);
        chunk.setRotation(sf::degrees(d.rot));
        chunk.setFillColor(sf::Color(d.color.r, d.color.g, d.color.b, a));
        chunk.setOutlineThickness(1.0f);
        chunk.setOutlineColor(sf::Color(40, 35, 32, a));
        target.draw(chunk);
    }

    // 5. Отрисовка расширяющихся ударных колец
    for (const auto& r : m_rings) {
        if (!r.active) continue;
        float prog = r.timer / r.maxDuration;
        float alpha = (1.0f - prog) * 240.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        sf::VertexArray ringVA(sf::PrimitiveType::TriangleStrip);
        int segs = 28;
        for (int i = 0; i <= segs; ++i) {
            float ang = (i / static_cast<float>(segs)) * 3.14159f; // Полукольцо по земле
            float cosA = std::cos(ang);
            float sinA = std::sin(ang);
            float outX = r.center.x + cosA * r.radiusX;
            float outY = r.center.y - sinA * r.radiusY;
            float inX = r.center.x + cosA * (r.radiusX - 8.0f);
            float inY = r.center.y - sinA * (r.radiusY - 3.5f);

            ringVA.append({{outX, outY}, sf::Color(r.color.r, r.color.g, r.color.b, static_cast<std::uint8_t>(a * 0.2f)), {0.0f, 0.0f}});
            ringVA.append({{inX, inY}, sf::Color(255, 240, 180, a), {0.0f, 0.0f}});
        }
        target.draw(ringVA);
    }

    // 6. Отрисовка фантомов тарана
    for (const auto& g : m_afterimages) {
        if (!g.active) continue;
        float prog = g.timer / g.maxDuration;
        float alpha = (1.0f - prog) * 160.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        sf::RectangleShape ghost(g.size);
        ghost.setOrigin(g.origin);
        ghost.setPosition(g.pos);
        ghost.setFillColor(sf::Color(255, 30, 20, static_cast<std::uint8_t>(a * 0.5f)));
        ghost.setOutlineThickness(2.0f);
        ghost.setOutlineColor(sf::Color(255, 190, 40, a));
        target.draw(ghost);
    }

    // 7. Отрисовка сокрушительного столба Jump Slam
    for (const auto& p : m_pillars) {
        if (!p.active) continue;
        float prog = p.timer / p.maxDuration;
        float alpha = (1.0f - prog) * 255.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        float x = p.hitbox.position.x;
        float y = p.hitbox.position.y;
        float w = p.hitbox.size.x;
        float h = p.hitbox.size.y;

        // Внешнее силовое поле
        sf::RectangleShape outerBox({w, h});
        outerBox.setPosition({x, y});
        outerBox.setFillColor(sf::Color(255, 35, 15, static_cast<std::uint8_t>(a * 0.30f)));
        outerBox.setOutlineThickness(3.0f);
        outerBox.setOutlineColor(sf::Color(255, 230, 70, a));
        target.draw(outerBox);

        // Внутренний раскаленный луч
        float coreW = w * 0.50f;
        float coreX = x + (w - coreW) * 0.5f;
        sf::RectangleShape coreBeam({coreW, h});
        coreBeam.setPosition({coreX, y});
        coreBeam.setFillColor(sf::Color(255, 130, 40, static_cast<std::uint8_t>(a * 0.65f)));
        target.draw(coreBeam);

        // Центральный белый столб молнии
        float whiteW = w * 0.18f;
        float whiteX = x + (w - whiteW) * 0.5f;
        sf::RectangleShape whiteBeam({whiteW, h});
        whiteBeam.setPosition({whiteX, y});
        whiteBeam.setFillColor(sf::Color(255, 255, 240, a));
        target.draw(whiteBeam);

        // Ветвящиеся молнии
        for (const auto& branch : p.lightningBranches) {
            if (branch.size() < 2) continue;
            sf::VertexArray lva(sf::PrimitiveType::LineStrip);
            for (const auto& pt : branch) {
                lva.append({pt, sf::Color(255, 255, 220, a), {0.0f, 0.0f}});
            }
            target.draw(lva);
        }
    }

    // 8. Отрисовка рубящих взмахов атак босса (кинематографичные процедурные клинки)
    for (const auto& s : m_slashes) {
        if (!s.active) continue;
        float prog = s.timer / s.maxDuration;
        float alpha = (1.0f - prog * prog) * 255.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        float x = s.hitbox.position.x;
        float y = s.hitbox.position.y;
        float w = s.hitbox.size.x;
        float h = s.hitbox.size.y;
        float dir = s.facingLeft ? -1.0f : 1.0f;

        // Вспомогательная лямбда генерации многослойного серповидного лезвия
        auto drawCrescentBlade = [&](sf::Vector2f pivot, float rx, float ry, float startAng, float endAng,
                                     float thickness, float progress, float bladeAlpha, float sweepDir) {
            float headT = std::min(1.0f, progress / 0.28f);
            float tailT = std::clamp((progress - 0.10f) / 0.90f, 0.0f, 1.0f);
            float curStart = startAng + (endAng - startAng) * (tailT * 0.32f);
            float curEnd = startAng + (endAng - startAng) * headT;

            int segs = 32;
            std::vector<sf::Vector2f> outerPts;
            std::vector<sf::Vector2f> midPts;
            std::vector<sf::Vector2f> innerPts;
            std::vector<sf::Vector2f> auraOuterPts;
            std::vector<sf::Vector2f> auraInnerPts;

            outerPts.reserve(segs + 1);
            midPts.reserve(segs + 1);
            innerPts.reserve(segs + 1);
            auraOuterPts.reserve(segs + 1);
            auraInnerPts.reserve(segs + 1);

            for (int i = 0; i <= segs; ++i) {
                float u = i / static_cast<float>(segs);
                float ang = curStart + (curEnd - curStart) * u;
                float cosA = std::cos(ang);
                float sinA = std::sin(ang);

                float px = pivot.x + sweepDir * cosA * rx;
                float py = pivot.y + sinA * ry;
                sf::Vector2f pSpine(px, py);

                float tx = -sweepDir * sinA * rx;
                float ty = cosA * ry;
                float tlen = std::sqrt(tx * tx + ty * ty);
                if (tlen > 0.001f) { tx /= tlen; ty /= tlen; }
                float nx = -ty * sweepDir;
                float ny = tx * sweepDir;

                float thick = thickness * std::sin(u * 3.14159f) * (0.35f + 0.65f * u);
                sf::Vector2f norm(nx, ny);

                outerPts.push_back(pSpine + norm * (thick * 0.55f));
                midPts.push_back(pSpine + norm * (thick * 0.10f));
                innerPts.push_back(pSpine - norm * (thick * 0.45f));

                auraOuterPts.push_back(pSpine + norm * (thick * 0.55f + 16.0f));
                auraInnerPts.push_back(pSpine - norm * (thick * 0.45f + 12.0f));
            }

            // Слой 1: Внешняя тепловая атмосфера и шлейф дыма
            sf::VertexArray aura(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= segs; ++i) {
                float u = i / static_cast<float>(segs);
                float uAlpha = std::sin(u * 3.14159f);
                std::uint8_t aMid = static_cast<std::uint8_t>(bladeAlpha * uAlpha * 0.40f);

                aura.append({auraOuterPts[i], sf::Color(190, 25, 10, 0), {0.0f, 0.0f}});
                aura.append({midPts[i], sf::Color(255, 60, 20, aMid), {0.0f, 0.0f}});
            }
            target.draw(aura);

            sf::VertexArray auraIn(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= segs; ++i) {
                float u = i / static_cast<float>(segs);
                float uAlpha = std::sin(u * 3.14159f);
                std::uint8_t aMid = static_cast<std::uint8_t>(bladeAlpha * uAlpha * 0.40f);

                auraIn.append({midPts[i], sf::Color(255, 60, 20, aMid), {0.0f, 0.0f}});
                auraIn.append({auraInnerPts[i], sf::Color(120, 15, 10, 0), {0.0f, 0.0f}});
            }
            target.draw(auraIn);

            // Слой 2: Раскаленное тело плазменного клинка (золотисто-алый градиент)
            sf::VertexArray bladeOut(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= segs; ++i) {
                float u = i / static_cast<float>(segs);
                float uAlpha = std::pow(std::sin(u * 3.14159f), 0.65f);
                std::uint8_t aOut = static_cast<std::uint8_t>(bladeAlpha * uAlpha * 0.95f);
                std::uint8_t aMid = static_cast<std::uint8_t>(bladeAlpha * uAlpha * 0.90f);

                bladeOut.append({outerPts[i], sf::Color(255, 205, 50, aOut), {0.0f, 0.0f}});
                bladeOut.append({midPts[i], sf::Color(255, 75, 20, aMid), {0.0f, 0.0f}});
            }
            target.draw(bladeOut);

            sf::VertexArray bladeIn(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= segs; ++i) {
                float u = i / static_cast<float>(segs);
                float uAlpha = std::pow(std::sin(u * 3.14159f), 0.65f);
                std::uint8_t aMid = static_cast<std::uint8_t>(bladeAlpha * uAlpha * 0.90f);
                std::uint8_t aIn = static_cast<std::uint8_t>(bladeAlpha * uAlpha * 0.55f);

                bladeIn.append({midPts[i], sf::Color(255, 75, 20, aMid), {0.0f, 0.0f}});
                bladeIn.append({innerPts[i], sf::Color(180, 25, 15, aIn), {0.0f, 0.0f}});
            }
            target.draw(bladeIn);

            // Слой 3: Ослепительно-белая режущая кромка
            sf::VertexArray cutEdge(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= segs; ++i) {
                float u = i / static_cast<float>(segs);
                float uAlpha = std::pow(std::sin(u * 3.14159f), 0.5f);
                std::uint8_t aWhite = static_cast<std::uint8_t>(bladeAlpha * uAlpha);
                std::uint8_t aEdge = static_cast<std::uint8_t>(bladeAlpha * uAlpha * 0.65f);

                sf::Vector2f edgeIn = outerPts[i] + (midPts[i] - outerPts[i]) * 0.28f;
                cutEdge.append({outerPts[i], sf::Color(255, 255, 245, aWhite), {0.0f, 0.0f}});
                cutEdge.append({edgeIn, sf::Color(255, 230, 90, aEdge), {0.0f, 0.0f}});
            }
            target.draw(cutEdge);

            // Слой 4: Скоростные шлейфы рассечения воздуха
            if (progress < 0.60f) {
                float streakFade = (1.0f - progress / 0.60f) * bladeAlpha;
                std::uint8_t aStreak = static_cast<std::uint8_t>(streakFade * 0.75f);
                sf::VertexArray streak1(sf::PrimitiveType::LineStrip);
                sf::VertexArray streak2(sf::PrimitiveType::LineStrip);
                for (int i = 4; i < segs - 2; ++i) {
                    streak1.append({outerPts[i] + (midPts[i] - outerPts[i]) * 0.15f, sf::Color(255, 245, 180, aStreak), {0.0f, 0.0f}});
                    streak2.append({midPts[i] + (innerPts[i] - midPts[i]) * 0.25f, sf::Color(255, 190, 60, static_cast<std::uint8_t>(aStreak * 0.6f)), {0.0f, 0.0f}});
                }
                target.draw(streak1);
                target.draw(streak2);
            }

            // Слой 5: Вспышка на острие в момент максимальной кинетической энергии
            if (progress < 0.35f && !outerPts.empty()) {
                float glintT = 1.0f - progress / 0.35f;
                float glintLen = 28.0f * glintT;
                float glintThick = 4.0f * glintT;
                sf::Vector2f tip = outerPts.back();
                std::uint8_t aGlint = static_cast<std::uint8_t>(bladeAlpha * glintT);

                sf::VertexArray glint(sf::PrimitiveType::Triangles);
                glint.append({{tip.x - glintLen, tip.y}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});
                glint.append({{tip.x, tip.y - glintThick}, sf::Color(255, 255, 245, aGlint), {0.0f, 0.0f}});
                glint.append({{tip.x + glintLen, tip.y}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});

                glint.append({{tip.x + glintLen, tip.y}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});
                glint.append({{tip.x, tip.y + glintThick}, sf::Color(255, 255, 245, aGlint), {0.0f, 0.0f}});
                glint.append({{tip.x - glintLen, tip.y}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});

                glint.append({{tip.x, tip.y - glintLen}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});
                glint.append({{tip.x + glintThick, tip.y}, sf::Color(255, 255, 245, aGlint), {0.0f, 0.0f}});
                glint.append({{tip.x, tip.y + glintLen}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});

                glint.append({{tip.x, tip.y + glintLen}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});
                glint.append({{tip.x - glintThick, tip.y}, sf::Color(255, 255, 245, aGlint), {0.0f, 0.0f}});
                glint.append({{tip.x, tip.y - glintLen}, sf::Color(255, 210, 80, 0), {0.0f, 0.0f}});
                target.draw(glint);

                sf::CircleShape glintCore(4.5f * glintT);
                glintCore.setOrigin({glintCore.getRadius(), glintCore.getRadius()});
                glintCore.setPosition(tip);
                glintCore.setFillColor(sf::Color(255, 255, 255, aGlint));
                target.draw(glintCore);
            }
        };

        if (s.type == BossSlashType::WHIRLWIND_360) {
            // КРУГОВОЙ ВИХРЕВОЙ СМЕРЧ НА 360°
            float cx = x + w * 0.5f;
            float cy = y + h * 0.52f;
            float rx = w * 0.46f;
            float ry = h * 0.42f;
            float spinAng = prog * 6.28318f * 2.2f;

            for (int k = 0; k < 3; ++k) {
                float finStart = spinAng + k * 2.0944f;
                float finEnd = finStart + 1.45f;
                drawCrescentBlade({cx, cy}, rx, ry, finStart, finEnd, 30.0f, prog, alpha, 1.0f);
            }

            // Внутреннее концентрическое энергетическое кольцо
            sf::VertexArray spinRing(sf::PrimitiveType::TriangleStrip);
            int segs = 28;
            for (int i = 0; i <= segs; ++i) {
                float ang = (i / static_cast<float>(segs)) * 6.28318f;
                float ca = std::cos(ang);
                float sa = std::sin(ang);
                float rOutX = rx * 0.65f;
                float rOutY = ry * 0.65f;
                float rInX = rx * 0.48f;
                float rInY = ry * 0.48f;

                std::uint8_t aRing = static_cast<std::uint8_t>(a * 0.65f);
                spinRing.append({{cx + ca * rOutX, cy + sa * rOutY}, sf::Color(255, 140, 30, aRing), {0.0f, 0.0f}});
                spinRing.append({{cx + ca * rInX, cy + sa * rInY}, sf::Color(255, 240, 200, aRing), {0.0f, 0.0f}});
            }
            target.draw(spinRing);

        } else if (s.type == BossSlashType::HEAVY_OVERHEAD) {
            // СОКРУШИТЕЛЬНЫЙ ВЕРТИКАЛЬНЫЙ РАЗРЕЗ СВЕРХУ ВНИЗ
            sf::Vector2f pivot = {s.facingLeft ? (x + w * 0.82f) : (x + w * 0.18f), y + h * 0.16f};
            drawCrescentBlade(pivot, w * 0.84f, h * 0.86f, -1.45f, 1.35f, 50.0f, prog, alpha, dir);

            // Вспышка и полукольцо удара о землю
            if (prog < 0.45f) {
                float hitT = 1.0f - prog / 0.45f;
                float groundImpactX = pivot.x + dir * (w * 0.60f);
                float gY = (s.groundY > 0.0f) ? s.groundY : (y + h);

                sf::CircleShape impactFlash(26.0f * hitT);
                impactFlash.setOrigin({impactFlash.getRadius(), impactFlash.getRadius()});
                impactFlash.setPosition({groundImpactX, gY - 4.0f});
                impactFlash.setFillColor(sf::Color(255, 255, 220, static_cast<std::uint8_t>(a * hitT * 0.85f)));
                target.draw(impactFlash);

                // Расширяющаяся ударная волна по полу
                sf::VertexArray groundShock(sf::PrimitiveType::TriangleStrip);
                int gsegs = 16;
                float waveR = 40.0f + prog * 110.0f;
                for (int i = 0; i <= gsegs; ++i) {
                    float ga = (i / static_cast<float>(gsegs)) * 3.14159f;
                    float gx = groundImpactX + std::cos(ga) * waveR;
                    float gy = gY - std::sin(ga) * (waveR * 0.32f);
                    float inR = waveR - 6.0f;
                    float inGx = groundImpactX + std::cos(ga) * inR;
                    float inGy = gY - std::sin(ga) * (inR * 0.32f);

                    std::uint8_t aWave = static_cast<std::uint8_t>(a * hitT * 0.70f);
                    groundShock.append({{gx, gy}, sf::Color(255, 70, 20, 0), {0.0f, 0.0f}});
                    groundShock.append({{inGx, inGy}, sf::Color(255, 230, 90, aWave), {0.0f, 0.0f}});
                }
                target.draw(groundShock);
            }

        } else if (s.type == BossSlashType::UPWARD_CLEAVE) {
            // ВОСХОДЯЩИЙ ПОДСЕКАЮЩИЙ РАЗРЕЗ СНИЗУ ВВЕРХ
            sf::Vector2f pivot = {s.facingLeft ? (x + w * 0.90f) : (x + w * 0.10f), y + h * 0.62f};
            drawCrescentBlade(pivot, w * 0.86f, h * 0.54f, 1.15f, -1.10f, 34.0f, prog, alpha, dir);

        } else if (s.type == BossSlashType::CROSS_X) {
            // ДВОЙНОЙ КРЕСТООБРАЗНЫЙ РАССЕКАТЕЛЬ
            sf::Vector2f pivot1 = {s.facingLeft ? (x + w * 0.88f) : (x + w * 0.12f), y + h * 0.28f};
            drawCrescentBlade(pivot1, w * 0.82f, h * 0.46f, -1.05f, 0.85f, 32.0f, prog, alpha, dir);

            sf::Vector2f pivot2 = {s.facingLeft ? (x + w * 0.88f) : (x + w * 0.12f), y + h * 0.72f};
            drawCrescentBlade(pivot2, w * 0.82f, h * 0.46f, 1.05f, -0.85f, 32.0f, prog, alpha, dir);

            // 8-лучевая звезда в точке перекрестия
            if (prog < 0.40f) {
                float crossT = 1.0f - prog / 0.40f;
                sf::Vector2f crossCenter = {x + w * 0.50f, y + h * 0.50f};
                float starLen = 32.0f * crossT;
                float starThick = 4.5f * crossT;
                std::uint8_t aCross = static_cast<std::uint8_t>(a * crossT);

                sf::VertexArray star(sf::PrimitiveType::Triangles);
                // Горизонтальный и вертикальный лучи
                star.append({{crossCenter.x - starLen, crossCenter.y}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});
                star.append({{crossCenter.x, crossCenter.y - starThick}, sf::Color(255, 255, 245, aCross), {0.0f, 0.0f}});
                star.append({{crossCenter.x + starLen, crossCenter.y}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});

                star.append({{crossCenter.x + starLen, crossCenter.y}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});
                star.append({{crossCenter.x, crossCenter.y + starThick}, sf::Color(255, 255, 245, aCross), {0.0f, 0.0f}});
                star.append({{crossCenter.x - starLen, crossCenter.y}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});

                star.append({{crossCenter.x, crossCenter.y - starLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});
                star.append({{crossCenter.x + starThick, crossCenter.y}, sf::Color(255, 255, 245, aCross), {0.0f, 0.0f}});
                star.append({{crossCenter.x, crossCenter.y + starLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});

                star.append({{crossCenter.x, crossCenter.y + starLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});
                star.append({{crossCenter.x - starThick, crossCenter.y}, sf::Color(255, 255, 245, aCross), {0.0f, 0.0f}});
                star.append({{crossCenter.x, crossCenter.y - starLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});

                // Диагональные лучи
                float diagLen = starLen * 0.707f;
                float diagThick = starThick * 0.707f;
                star.append({{crossCenter.x - diagLen, crossCenter.y - diagLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});
                star.append({{crossCenter.x + diagThick, crossCenter.y - diagThick}, sf::Color(255, 255, 245, aCross), {0.0f, 0.0f}});
                star.append({{crossCenter.x + diagLen, crossCenter.y + diagLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});

                star.append({{crossCenter.x - diagLen, crossCenter.y + diagLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});
                star.append({{crossCenter.x + diagThick, crossCenter.y + diagThick}, sf::Color(255, 255, 245, aCross), {0.0f, 0.0f}});
                star.append({{crossCenter.x + diagLen, crossCenter.y - diagLen}, sf::Color(255, 180, 40, 0), {0.0f, 0.0f}});
                target.draw(star);

                sf::CircleShape burstCore(6.0f * crossT);
                burstCore.setOrigin({burstCore.getRadius(), burstCore.getRadius()});
                burstCore.setPosition(crossCenter);
                burstCore.setFillColor(sf::Color(255, 255, 255, aCross));
                target.draw(burstCore);
            }

        } else if (s.type == BossSlashType::PIERCING_THRUST) {
            // СВЕРХЗВУКОВОЙ КОЛЮЩИЙ ВЫПАД
            float startX = s.facingLeft ? (x + w * 0.90f) : (x + w * 0.10f);
            float endX = s.facingLeft ? (x + w * 0.05f) : (x + w * 0.95f);
            float midY = y + h * 0.52f;
            float curReachX = startX + (endX - startX) * std::min(1.0f, prog / 0.25f);

            // Копьевидный сердечник
            sf::VertexArray spear(sf::PrimitiveType::TriangleStrip);
            int segs = 16;
            for (int i = 0; i <= segs; ++i) {
                float u = i / static_cast<float>(segs);
                float curX = startX + (curReachX - startX) * u;
                float th = (1.0f - u) * 16.0f + u * 3.0f;
                std::uint8_t aSp = static_cast<std::uint8_t>(a * (0.4f + 0.6f * u));

                spear.append({{curX, midY - th}, sf::Color(255, 90, 20, aSp), {0.0f, 0.0f}});
                spear.append({{curX, midY}, sf::Color(255, 255, 245, aSp), {0.0f, 0.0f}});
                spear.append({{curX, midY + th}, sf::Color(255, 190, 40, aSp), {0.0f, 0.0f}});
            }
            target.draw(spear);

            // Конические ударные волны рассечения воздуха
            for (int r = 0; r < 3; ++r) {
                float coneT = std::fmod(prog * 2.8f + r * 0.33f, 1.0f);
                float coneX = startX + (curReachX - startX) * coneT;
                float coneW = 12.0f + coneT * 32.0f;
                float coneH = 14.0f + coneT * 40.0f;
                float coneFade = (1.0f - coneT) * a;

                sf::VertexArray cone(sf::PrimitiveType::LineStrip);
                int csegs = 12;
                for (int ci = 0; ci <= csegs; ++ci) {
                    float ca = -1.57f + (ci / static_cast<float>(csegs)) * 3.14159f;
                    float cpx = coneX - dir * std::cos(ca) * coneW;
                    float cpy = midY + std::sin(ca) * coneH;
                    cone.append({{cpx, cpy}, sf::Color(255, 220, 110, static_cast<std::uint8_t>(coneFade * 0.65f)), {0.0f, 0.0f}});
                }
                target.draw(cone);
            }

        } else {
            // ГОРИЗОНТАЛЬНЫЙ СИЛОВОЙ РАЗРЕЗ (HORIZONTAL_CLEAVE)
            sf::Vector2f pivot = {s.facingLeft ? (x + w * 0.92f) : (x + w * 0.08f), y + h * 0.52f};
            drawCrescentBlade(pivot, w * 0.88f, h * 0.44f, -0.95f, 0.80f, 38.0f, prog, alpha, dir);
        }

        // Шлейфы искр со скоростными штрихами и раскаленными ядрами
        sf::VertexArray sparkLines(sf::PrimitiveType::Lines);
        for (const auto& sp : s.sparks) {
            float sProg = sp.timer / sp.maxLife;
            if (sProg >= 1.0f) continue;
            float spA = (1.0f - sProg) * a;
            std::uint8_t sparkAlpha = static_cast<std::uint8_t>(std::clamp(spA, 0.0f, 255.0f));

            sparkLines.append({sp.pos, sf::Color(sp.color.r, sp.color.g, sp.color.b, sparkAlpha), {0.0f, 0.0f}});
            sparkLines.append({sp.prevPos, sf::Color(255, 60, 20, static_cast<std::uint8_t>(sparkAlpha * 0.35f)), {0.0f, 0.0f}});

            if (sp.size > 2.2f) {
                sf::RectangleShape sparkHead({sp.size, sp.size});
                sparkHead.setOrigin({sp.size * 0.5f, sp.size * 0.5f});
                sparkHead.setPosition(sp.pos);
                sparkHead.setRotation(sf::degrees(45.0f));
                sparkHead.setFillColor(sf::Color(sp.color.r, sp.color.g, sp.color.b, sparkAlpha));
                target.draw(sparkHead);
            }
        }
        target.draw(sparkLines);
    }

    // 9. Отрисовка обломков и пыли врезания тарана в стену
    for (const auto& w : m_wallCrashes) {
        if (!w.active) continue;
        float prog = w.timer / w.maxDuration;
        float alpha = (1.0f - prog) * 255.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        for (const auto& d : w.chunks) {
            sf::RectangleShape chunk({d.size, d.size * 0.8f});
            chunk.setOrigin({d.size * 0.5f, d.size * 0.4f});
            chunk.setPosition(d.pos);
            chunk.setRotation(sf::degrees(d.rot));
            chunk.setFillColor(sf::Color(d.color.r, d.color.g, d.color.b, a));
            chunk.setOutlineThickness(1.0f);
            chunk.setOutlineColor(sf::Color(35, 30, 28, a));
            target.draw(chunk);
        }
    }
}

void BossVfxManager::clear() {
    m_slashes.clear();
    m_spikes.clear();
    m_debris.clear();
    m_dustPuffs.clear();
    m_cracks.clear();
    m_arenaWaves.clear();
    m_rings.clear();
    m_pillars.clear();
    m_wallCrashes.clear();
    m_afterimages.clear();
}

size_t BossVfxManager::getActiveCount() const {
    return m_slashes.size() + m_spikes.size() + m_debris.size() + m_dustPuffs.size() +
           m_cracks.size() + m_arenaWaves.size() + m_rings.size() + m_pillars.size() +
           m_wallCrashes.size() + m_afterimages.size();
}
