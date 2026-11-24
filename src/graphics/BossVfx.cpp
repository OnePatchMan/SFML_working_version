#include "graphics/BossVfx.h"

void BossVfxManager::spawnSlash(const sf::FloatRect& hitbox, bool facingLeft, BossSlashType type, float duration) {
    BossSlashInstance inst;
    inst.hitbox = hitbox;
    inst.facingLeft = facingLeft;
    inst.type = type;
    inst.timer = 0.0f;
    inst.maxDuration = duration;
    inst.active = true;

    // Генерация веера искр вдоль дуги лезвия со скоростными шлейфами
    int sparkCount = 36;
    for (int i = 0; i < sparkCount; ++i) {
        BossSlashInstance::Spark s;
        float fx = hitbox.position.x + (facingLeft ? hitbox.size.x * (0.65f + (rand() % 35) / 100.0f) : hitbox.size.x * (0.0f + (rand() % 35) / 100.0f));
        float fy = hitbox.position.y + hitbox.size.y * (0.05f + 0.90f * (i / static_cast<float>(sparkCount)));
        s.pos = {fx, fy};
        s.prevPos = s.pos;
        float spd = 280.0f + static_cast<float>(rand() % 380);
        float baseAngle = (type == BossSlashType::WHIRLWIND_360) ? static_cast<float>(rand() % 360) :
                          (facingLeft ? 180.0f : 0.0f) + (-45.0f + static_cast<float>(rand() % 90));
        float rad = baseAngle * 3.14159f / 180.0f;
        s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        s.timer = 0.0f;
        s.maxLife = 0.20f + (rand() % 100) / 450.0f;
        s.color = (i % 3 == 0) ? sf::Color(255, 255, 230) : ((i % 3 == 1) ? sf::Color(255, 200, 50) : sf::Color(255, 70, 20));
        inst.sparks.push_back(s);
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
        if (s.timer >= s.maxDuration) s.active = false;
        for (auto& sp : s.sparks) {
            sp.timer += dt;
            sp.prevPos = sp.pos;
            sp.pos += sp.vel * dt;
            sp.vel.y += 380.0f * dt; // Гравитация искр
            sp.vel *= 0.94f;
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

    // 8. Отрисовка рубящих взмахов атак босса (точно по хитбоксу)
    for (const auto& s : m_slashes) {
        if (!s.active) continue;
        float prog = s.timer / s.maxDuration;
        float alpha = (1.0f - prog * prog) * 255.0f;
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));

        float x = s.hitbox.position.x;
        float y = s.hitbox.position.y;
        float w = s.hitbox.size.x;
        float h = s.hitbox.size.y;

        // Внешнее светящееся силовое поле атаки
        sf::RectangleShape boxGlow({w, h});
        boxGlow.setPosition({x, y});
        boxGlow.setFillColor(sf::Color(255, 45, 20, static_cast<std::uint8_t>(a * 0.22f)));
        boxGlow.setOutlineThickness(2.2f);
        boxGlow.setOutlineColor(sf::Color(255, 220, 60, a));
        target.draw(boxGlow);

        if (s.type == BossSlashType::WHIRLWIND_360) {
            // КРУГОВОЙ ВИХРЕВОЙ ВЗМАХ НА 360° (Смерч ударов)
            sf::VertexArray spin(sf::PrimitiveType::TriangleStrip);
            int segs = 32;
            float cx = x + w * 0.5f;
            float cy = y + h * 0.5f;
            float rOuterX = w * 0.49f;
            float rOuterY = h * 0.46f;
            float rInnerX = rOuterX * 0.55f;
            float rInnerY = rOuterY * 0.55f;

            for (int i = 0; i <= segs; ++i) {
                float ang = (i / static_cast<float>(segs)) * 6.28318f;
                float cosA = std::cos(ang);
                float sinA = std::sin(ang);

                sf::Color cOut = (i % 2 == 0) ? sf::Color(255, 45, 20, a) : sf::Color(255, 190, 40, a);
                sf::Color cIn = sf::Color(255, 255, 245, a);

                spin.append({{cx + cosA * rOuterX, cy + sinA * rOuterY}, cOut, {0.0f, 0.0f}});
                spin.append({{cx + cosA * rInnerX, cy + sinA * rInnerY}, cIn, {0.0f, 0.0f}});
            }
            target.draw(spin);
        } else if (s.type == BossSlashType::HEAVY_OVERHEAD) {
            // ТЯЖЕЛЫЙ ВЕРТИКАЛЬНЫЙ СОКРУШИТЕЛЬНЫЙ РАЗРЕЗ
            sf::VertexArray cleave(sf::PrimitiveType::TriangleStrip);
            int segs = 20;
            float startX = s.facingLeft ? (x + w * 0.85f) : (x + w * 0.15f);
            float endX = s.facingLeft ? (x + w * 0.05f) : (x + w * 0.95f);

            for (int i = 0; i <= segs; ++i) {
                float t = i / static_cast<float>(segs);
                float curX = startX + (endX - startX) * t;
                float curY = y + t * h;
                float ribW = std::sin(t * 3.14159f) * 45.0f;

                cleave.append({{curX - ribW, curY}, sf::Color(255, 45, 20, a), {0.0f, 0.0f}});
                cleave.append({{curX, curY}, sf::Color(255, 255, 240, a), {0.0f, 0.0f}});
                cleave.append({{curX + ribW, curY}, sf::Color(255, 180, 30, a), {0.0f, 0.0f}});
            }
            target.draw(cleave);
        } else if (s.type == BossSlashType::CROSS_X) {
            // ДВОЙНОЙ КРЕСТООБРАЗНЫЙ РАССЕКАТЕЛЬ
            sf::VertexArray cross(sf::PrimitiveType::TriangleStrip);
            int segs = 16;
            for (int i = 0; i <= segs; ++i) {
                float t = i / static_cast<float>(segs);
                float curX = x + t * w;
                float curY1 = y + t * h;
                float ribW = 18.0f * (1.0f - std::abs(t - 0.5f) * 1.5f);
                cross.append({{curX, curY1 - ribW}, sf::Color(255, 50, 20, a), {0.0f, 0.0f}});
                cross.append({{curX, curY1 + ribW}, sf::Color(255, 255, 220, a), {0.0f, 0.0f}});
            }
            target.draw(cross);

            sf::VertexArray cross2(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= segs; ++i) {
                float t = i / static_cast<float>(segs);
                float curX = x + t * w;
                float curY2 = y + (1.0f - t) * h;
                float ribW = 18.0f * (1.0f - std::abs(t - 0.5f) * 1.5f);
                cross2.append({{curX, curY2 - ribW}, sf::Color(255, 190, 40, a), {0.0f, 0.0f}});
                cross2.append({{curX, curY2 + ribW}, sf::Color(255, 255, 220, a), {0.0f, 0.0f}});
            }
            target.draw(cross2);
        } else {
            // ГОРИЗОНТАЛЬНЫЙ РАЗМАШИСТЫЙ РУБЯЩИЙ ВЗМАХ
            sf::VertexArray cleaveH(sf::PrimitiveType::TriangleStrip);
            int segs = 24;
            float startY = y + h * 0.15f;
            float endY = y + h * 0.85f;

            for (int i = 0; i <= segs; ++i) {
                float t = i / static_cast<float>(segs);
                float curY = startY + (endY - startY) * t;
                float curX = s.facingLeft ? (x + w * (1.0f - t)) : (x + w * t);
                float bowOffset = std::sin(t * 3.14159f) * 35.0f;
                float drawX = s.facingLeft ? (curX - bowOffset) : (curX + bowOffset);
                float ribH = std::sin(t * 3.14159f) * 42.0f;

                cleaveH.append({{drawX, curY - ribH}, sf::Color(255, 45, 20, a), {0.0f, 0.0f}});
                cleaveH.append({{drawX, curY}, sf::Color(255, 255, 245, a), {0.0f, 0.0f}});
                cleaveH.append({{drawX, curY + ribH}, sf::Color(255, 185, 35, a), {0.0f, 0.0f}});
            }
            target.draw(cleaveH);
        }

        // Шлейфы искр со скоростными линиями
        sf::VertexArray sparkLines(sf::PrimitiveType::Lines);
        for (const auto& sp : s.sparks) {
            float sProg = sp.timer / sp.maxLife;
            if (sProg >= 1.0f) continue;
            float spA = (1.0f - sProg) * a;
            std::uint8_t sparkAlpha = static_cast<std::uint8_t>(std::clamp(spA, 0.0f, 255.0f));

            sparkLines.append({sp.pos, sf::Color(sp.color.r, sp.color.g, sp.color.b, sparkAlpha), {0.0f, 0.0f}});
            sparkLines.append({sp.prevPos, sf::Color(255, 60, 20, static_cast<std::uint8_t>(sparkAlpha * 0.35f)), {0.0f, 0.0f}});
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
