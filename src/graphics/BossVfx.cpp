#include "graphics/BossVfx.h"

void BossVfxManager::spawnSlash(const sf::FloatRect& hitbox, bool facingLeft, BossSlashType type, float duration) {
    BossSlashInstance inst;
    inst.hitbox = hitbox;
    inst.facingLeft = facingLeft;
    inst.type = type;
    inst.timer = 0.0f;
    inst.maxDuration = duration;
    inst.active = true;

    // Генерация искр вдоль лезвия
    int sparkCount = 12;
    for (int i = 0; i < sparkCount; ++i) {
        BossSlashInstance::Spark s;
        float fx = hitbox.position.x + (facingLeft ? hitbox.size.x * 0.8f : hitbox.size.x * 0.2f);
        float fy = hitbox.position.y + hitbox.size.y * (0.15f + 0.7f * (i / static_cast<float>(sparkCount)));
        s.pos = {fx, fy};
        float spd = 200.0f + static_cast<float>(rand() % 250);
        float angle = (facingLeft ? 180.0f : 0.0f) + (-35.0f + static_cast<float>(rand() % 70));
        float rad = angle * 3.14159f / 180.0f;
        s.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        s.timer = 0.0f;
        s.maxLife = 0.12f + (rand() % 100) / 700.0f;
        inst.sparks.push_back(s);
    }

    m_slashes.push_back(inst);
}

void BossVfxManager::spawnGroundImpact(const sf::Vector2f& groundPos, float width, bool intense) {
    int spikeCount = intense ? 8 : 6;
    float halfW = width * 0.5f;

    // 1. Каменные шипы, вырывающиеся из разлома земли
    for (int i = 0; i < spikeCount; ++i) {
        BossGroundSpike sp;
        float offsetRatio = (i / static_cast<float>(spikeCount - 1)) * 2.0f - 1.0f; // -1 to 1
        float jitter = ((rand() % 100) / 100.0f - 0.5f) * (width / spikeCount) * 0.4f;
        sp.basePos = {groundPos.x + offsetRatio * (halfW * 0.85f) + jitter, groundPos.y};
        sp.width = (intense ? 26.0f : 20.0f) + (rand() % 12);
        sp.targetHeight = (intense ? 55.0f : 40.0f) + (rand() % 35);
        if (std::abs(offsetRatio) < 0.3f) sp.targetHeight *= 1.35f; // Центральные шипы массивнее
        sp.currentHeight = 0.0f;
        sp.tilt = offsetRatio * 0.25f + ((rand() % 100) / 100.0f - 0.5f) * 0.15f;
        sp.timer = 0.0f;
        sp.maxDuration = intense ? 0.95f : 0.75f;
        sp.active = true;
        m_spikes.push_back(sp);
    }

    // 2. Расширяющиеся ударные кольца по полу
    for (int r = 0; r < (intense ? 3 : 2); ++r) {
        BossShockwaveRing ring;
        ring.center = groundPos;
        ring.radiusX = 15.0f;
        ring.radiusY = 6.0f;
        ring.maxRadiusX = halfW * (0.9f + r * 0.25f);
        ring.speed = 420.0f + r * 140.0f;
        ring.timer = 0.0f;
        ring.maxDuration = 0.38f + r * 0.08f;
        ring.color = (r == 0) ? sf::Color(255, 230, 90) : sf::Color(255, 45, 25);
        ring.active = true;
        m_rings.push_back(ring);
    }

    // 3. Разлетающиеся комья земли и осколки камня
    int rockCount = intense ? 18 : 12;
    for (int i = 0; i < rockCount; ++i) {
        BossDebrisChunk d;
        d.pos = {groundPos.x + ((rand() % 100) / 50.0f - 1.0f) * (halfW * 0.4f), groundPos.y - 4.0f};
        float angle = -160.0f + (rand() % 140); // Вверх веером
        float spd = 220.0f + (rand() % 380);
        float rad = angle * 3.14159f / 180.0f;
        d.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        d.rot = static_cast<float>(rand() % 360);
        d.rotSpeed = -360.0f + (rand() % 720);
        d.size = 5.0f + (rand() % 9);
        d.timer = 0.0f;
        d.maxDuration = 0.55f + (rand() % 100) / 300.0f;
        d.color = (i % 3 == 0) ? sf::Color(70, 60, 62) : ((i % 3 == 1) ? sf::Color(100, 85, 75) : sf::Color(190, 70, 35));
        d.active = true;
        m_debris.push_back(d);
    }
}

void BossVfxManager::spawnJumpSlamBeam(const sf::FloatRect& hitbox, float duration) {
    BossJumpSlamPillar p;
    p.hitbox = hitbox;
    p.timer = 0.0f;
    p.maxDuration = duration;
    p.active = true;
    m_pillars.push_back(p);
}

void BossVfxManager::spawnWallCrash(const sf::Vector2f& impactPos, float dirX) {
    BossWallCrashEffect w;
    w.pos = impactPos;
    w.dirX = dirX;
    w.timer = 0.0f;
    w.maxDuration = 0.60f;
    w.active = true;

    // Взрыв осколков стены
    int debrisCount = 20;
    for (int i = 0; i < debrisCount; ++i) {
        BossDebrisChunk d;
        d.pos = impactPos;
        float angle = (dirX > 0.0f) ? (-150.0f + (rand() % 120)) : (-30.0f - (rand() % 120));
        float spd = 260.0f + (rand() % 420);
        float rad = angle * 3.14159f / 180.0f;
        d.vel = {std::cos(rad) * spd, std::sin(rad) * spd};
        d.rot = static_cast<float>(rand() % 360);
        d.rotSpeed = -400.0f + (rand() % 800);
        d.size = 6.0f + (rand() % 10);
        d.timer = 0.0f;
        d.maxDuration = 0.50f + (rand() % 100) / 300.0f;
        d.color = (i % 2 == 0) ? sf::Color(115, 100, 95) : sf::Color(70, 65, 70);
        d.active = true;
        w.chunks.push_back(d);
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
    g.maxDuration = 0.18f;
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
            sp.pos += sp.vel * dt;
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
            float growTime = 0.07f;
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
            d.vel.y += 1400.0f * dt; // Гравитация
            d.rot += d.rotSpeed * dt;
        }
    }
    m_debris.erase(std::remove_if(m_debris.begin(), m_debris.end(), [](const BossDebrisChunk& d) { return !d.active; }), m_debris.end());

    // 4. Ударные кольца
    for (auto& r : m_rings) {
        if (!r.active) continue;
        r.timer += dt;
        if (r.timer >= r.maxDuration) {
            r.active = false;
        } else {
            r.radiusX += r.speed * dt;
            r.radiusY = r.radiusX * 0.42f;
        }
    }
    m_rings.erase(std::remove_if(m_rings.begin(), m_rings.end(), [](const BossShockwaveRing& r) { return !r.active; }), m_rings.end());

    // 5. Столбы Jump Slam
    for (auto& p : m_pillars) {
        if (!p.active) continue;
        p.timer += dt;
        if (p.timer >= p.maxDuration) p.active = false;
    }
    m_pillars.erase(std::remove_if(m_pillars.begin(), m_pillars.end(), [](const BossJumpSlamPillar& p) { return !p.active; }), m_pillars.end());

    // 6. Врезания в стену
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

    // 7. Фантомы
    for (auto& g : m_afterimages) {
        if (!g.active) continue;
        g.timer += dt;
        if (g.timer >= g.maxDuration) g.active = false;
    }
    m_afterimages.erase(std::remove_if(m_afterimages.begin(), m_afterimages.end(), [](const BossAfterimageGhost& g) { return !g.active; }), m_afterimages.end());
}

void BossVfxManager::draw(sf::RenderTarget& target) {
    // 1. Отрисовка фантомов тарана
    for (const auto& g : m_afterimages) {
        if (!g.active) continue;
        float prog = g.timer / g.maxDuration;
        float alpha = (1.0f - prog) * 140.0f;
        sf::RectangleShape ghost(g.size);
        ghost.setOrigin(g.origin);
        ghost.setPosition(g.pos);
        ghost.setFillColor(sf::Color(255, 30, 20, static_cast<std::uint8_t>(alpha)));
        ghost.setOutlineThickness(1.5f);
        ghost.setOutlineColor(sf::Color(255, 180, 40, static_cast<std::uint8_t>(alpha * 1.4f)));
        target.draw(ghost);
    }

    // 2. Отрисовка энергетического столба удара с воздуха по полу (Jump Slam Pillar)
    for (const auto& p : m_pillars) {
        if (!p.active) continue;
        float prog = p.timer / p.maxDuration;
        float alpha = (1.0f - prog) * 255.0f;
        float x = p.hitbox.position.x;
        float y = p.hitbox.position.y;
        float w = p.hitbox.size.x;
        float h = p.hitbox.size.y;

        // Внешнее кроваво-алое силовое поле на весь хитбокс
        sf::RectangleShape outerBox({w, h});
        outerBox.setPosition({x, y});
        outerBox.setFillColor(sf::Color(255, 40, 20, static_cast<std::uint8_t>(alpha * 0.28f)));
        outerBox.setOutlineThickness(2.5f);
        outerBox.setOutlineColor(sf::Color(255, 220, 60, static_cast<std::uint8_t>(alpha * 0.90f)));
        target.draw(outerBox);

        // Внутренний силовой столб со скоростными вертикальными линиями
        float coreW = w * 0.55f;
        float coreX = x + (w - coreW) * 0.5f;
        sf::RectangleShape coreBeam({coreW, h});
        coreBeam.setPosition({coreX, y});
        coreBeam.setFillColor(sf::Color(255, 120, 40, static_cast<std::uint8_t>(alpha * 0.45f)));
        target.draw(coreBeam);

        // Вертикальные ниспадающие силовые молнии / шевроны
        sf::VertexArray chevrons(sf::PrimitiveType::Lines);
        int lineCount = 8;
        for (int i = 0; i < lineCount; ++i) {
            float lx = x + (i + 1) * (w / (lineCount + 1));
            float ly1 = y + 10.0f;
            float ly2 = y + h - 10.0f;
            chevrons.append({{lx, ly1}, sf::Color(255, 255, 230, static_cast<std::uint8_t>(alpha)), {0.0f, 0.0f}});
            chevrons.append({{lx, ly2}, sf::Color(255, 60, 20, static_cast<std::uint8_t>(alpha * 0.6f)), {0.0f, 0.0f}});
        }
        target.draw(chevrons);

        // Сокрушительный рубящий полумесяц на вершине взмаха
        sf::VertexArray topCleave(sf::PrimitiveType::TriangleStrip, 6);
        float midY = y + 25.0f;
        topCleave[0].position = {x + 10.0f, y};
        topCleave[0].color = sf::Color(255, 40, 20, static_cast<std::uint8_t>(alpha));
        topCleave[1].position = {x + w * 0.5f, midY};
        topCleave[1].color = sf::Color(255, 255, 240, static_cast<std::uint8_t>(alpha));
        topCleave[2].position = {x + w - 10.0f, y};
        topCleave[2].color = sf::Color(255, 40, 20, static_cast<std::uint8_t>(alpha));
        topCleave[3].position = {x + w * 0.5f, midY + 12.0f};
        topCleave[3].color = sf::Color(255, 180, 50, static_cast<std::uint8_t>(alpha));
        topCleave[4].position = {x + w - 20.0f, y + 15.0f};
        topCleave[4].color = sf::Color(180, 20, 20, static_cast<std::uint8_t>(alpha * 0.5f));
        topCleave[5].position = {x + 20.0f, y + 15.0f};
        topCleave[5].color = sf::Color(180, 20, 20, static_cast<std::uint8_t>(alpha * 0.5f));
        target.draw(topCleave);
    }

    // 3. Отрисовка рубящих взмахов атак босса (точно по хитбоксу!)
    for (const auto& s : m_slashes) {
        if (!s.active) continue;
        float prog = s.timer / s.maxDuration;
        float alpha = (1.0f - prog * prog) * 255.0f;
        float x = s.hitbox.position.x;
        float y = s.hitbox.position.y;
        float w = s.hitbox.size.x;
        float h = s.hitbox.size.y;

        // А) Полупрозрачный фоновый энергетический шлейф на весь хитбокс
        sf::RectangleShape boxGlow({w, h});
        boxGlow.setPosition({x, y});
        boxGlow.setFillColor(sf::Color(255, 45, 20, static_cast<std::uint8_t>(alpha * 0.18f)));
        boxGlow.setOutlineThickness(1.8f);
        boxGlow.setOutlineColor(sf::Color(255, 215, 60, static_cast<std::uint8_t>(alpha * 0.85f)));
        target.draw(boxGlow);

        // Б) Отрисовка формы взмаха в зависимости от типа атаки
        if (s.type == BossSlashType::WHIRLWIND_360) {
            // КРУГОВОЙ ВИХРЕВОЙ ВЗМАХ НА 360° (Смерч)
            sf::VertexArray spin(sf::PrimitiveType::TriangleStrip);
            int segs = 24;
            float cx = x + w * 0.5f;
            float cy = y + h * 0.5f;
            float rOuterX = w * 0.48f;
            float rOuterY = h * 0.45f;
            float rInnerX = rOuterX * 0.55f;
            float rInnerY = rOuterY * 0.55f;

            for (int i = 0; i <= segs; ++i) {
                float a = (i / static_cast<float>(segs)) * 6.28318f;
                float cosA = std::cos(a);
                float sinA = std::sin(a);

                sf::Color cOut = (i % 2 == 0) ? sf::Color(255, 50, 20, static_cast<std::uint8_t>(alpha)) : sf::Color(255, 180, 40, static_cast<std::uint8_t>(alpha));
                sf::Color cIn = sf::Color(255, 255, 240, static_cast<std::uint8_t>(alpha));

                spin.append({{cx + cosA * rOuterX, cy + sinA * rOuterY}, cOut, {0.0f, 0.0f}});
                spin.append({{cx + cosA * rInnerX, cy + sinA * rInnerY}, cIn, {0.0f, 0.0f}});
            }
            target.draw(spin);
        } else if (s.type == BossSlashType::HEAVY_OVERHEAD) {
            // ВЕРТИКАЛЬНЫЙ ТЯЖЕЛЫЙ СМЭШ (сверху вниз через весь хитбокс)
            sf::VertexArray cleave(sf::PrimitiveType::TriangleStrip);
            int segs = 14;
            float startX = s.facingLeft ? (x + w * 0.9f) : (x + w * 0.1f);
            float endX = s.facingLeft ? (x + w * 0.05f) : (x + w * 0.95f);
            float startY = y + 10.0f;
            float endY = y + h - 10.0f;

            for (int i = 0; i <= segs; ++i) {
                float t = i / static_cast<float>(segs);
                float curveX = (1.0f - t) * startX + t * endX + std::sin(t * 3.14159f) * (s.facingLeft ? -w * 0.25f : w * 0.25f);
                float curveY = (1.0f - t) * startY + t * endY;
                float thick = std::sin(t * 3.14159f) * 28.0f + 4.0f;

                float normX = -(endY - startY);
                float normY = (endX - startX);
                float nlen = std::hypot(normX, normY);
                if (nlen > 0.001f) { normX /= nlen; normY /= nlen; }

                cleave.append({{curveX + normX * thick, curveY + normY * thick}, sf::Color(255, 45, 25, static_cast<std::uint8_t>(alpha * 0.8f)), {0.0f, 0.0f}});
                cleave.append({{curveX - normX * thick, curveY - normY * thick}, sf::Color(255, 255, 240, static_cast<std::uint8_t>(alpha)), {0.0f, 0.0f}});
            }
            target.draw(cleave);
        } else {
            // ГОРИЗОНТАЛЬНЫЙ РАЗМАШИСТЫЙ ВЗМАХ (НА ВСЮ ШИРИНУ И ВЫСОТУ ХИТБОКСА)
            sf::VertexArray arc(sf::PrimitiveType::TriangleStrip);
            int segs = 16;
            float startX = s.facingLeft ? (x + w * 0.95f) : (x + w * 0.05f);
            float endX = s.facingLeft ? (x + w * 0.05f) : (x + w * 0.95f);
            float midY = y + h * 0.5f;

            for (int i = 0; i <= segs; ++i) {
                float t = i / static_cast<float>(segs);
                float curX = (1.0f - t) * startX + t * endX;
                float curY = midY - std::cos(t * 3.14159f) * (h * 0.42f);
                float thick = std::sin(t * 3.14159f) * (h * 0.26f) + 3.0f;

                arc.append({{curX, curY - thick}, sf::Color(255, 40, 20, static_cast<std::uint8_t>(alpha * 0.85f)), {0.0f, 0.0f}});
                arc.append({{curX, curY + thick}, sf::Color(255, 255, 240, static_cast<std::uint8_t>(alpha)), {0.0f, 0.0f}});
            }
            target.draw(arc);

            // Скоростные линии рассекания
            sf::VertexArray slashLines(sf::PrimitiveType::Lines);
            for (int k = -1; k <= 1; ++k) {
                float ly = midY + k * 18.0f;
                slashLines.append({{startX, ly}, sf::Color(255, 220, 80, static_cast<std::uint8_t>(alpha * 0.9f)), {0.0f, 0.0f}});
                slashLines.append({{endX, ly + (s.facingLeft ? 8.0f : -8.0f)}, sf::Color(255, 50, 30, static_cast<std::uint8_t>(alpha * 0.4f)), {0.0f, 0.0f}});
            }
            target.draw(slashLines);
        }

        // В) Отрисовка вылетающих искр взмаха
        sf::VertexArray sparksVA(sf::PrimitiveType::Triangles);
        for (const auto& sp : s.sparks) {
            float sProg = sp.timer / sp.maxLife;
            if (sProg >= 1.0f) continue;
            float spAlpha = (1.0f - sProg) * alpha;
            float sz = 3.5f * (1.0f - sProg);
            sf::Color spCol = (sProg < 0.4f) ? sf::Color(255, 255, 220, static_cast<std::uint8_t>(spAlpha)) : sf::Color(255, 120, 30, static_cast<std::uint8_t>(spAlpha));

            sparksVA.append({{sp.pos.x - sz, sp.pos.y - sz}, spCol, {0.0f, 0.0f}});
            sparksVA.append({{sp.pos.x + sz, sp.pos.y - sz}, spCol, {0.0f, 0.0f}});
            sparksVA.append({{sp.pos.x, sp.pos.y + sz * 1.5f}, spCol, {0.0f, 0.0f}});
        }
        target.draw(sparksVA);
    }

    // 4. Отрисовка каменных шипов (Землетрясение / Слэм)
    for (const auto& sp : m_spikes) {
        if (!sp.active || sp.currentHeight <= 1.0f) continue;
        float h = sp.currentHeight;
        float halfSpkW = sp.width * 0.5f;
        float tipX = sp.basePos.x + std::sin(sp.tilt) * h;
        float tipY = sp.basePos.y - std::cos(sp.tilt) * h;

        // Массивный граненый каменный шип (Convex polygon)
        sf::ConvexShape spikePoly(4);
        spikePoly.setPoint(0, {sp.basePos.x - halfSpkW, sp.basePos.y});
        spikePoly.setPoint(1, {sp.basePos.x - halfSpkW * 0.4f, sp.basePos.y - h * 0.65f});
        spikePoly.setPoint(2, {tipX, tipY});
        spikePoly.setPoint(3, {sp.basePos.x + halfSpkW, sp.basePos.y});
        spikePoly.setFillColor(sf::Color(52, 48, 55));
        spikePoly.setOutlineThickness(1.8f);
        spikePoly.setOutlineColor(sf::Color(255, 80, 35));
        target.draw(spikePoly);

        // Внутреннее огненное ребро разлома
        sf::Vertex crackLine[] = {
            sf::Vertex{sf::Vector2f{sp.basePos.x, sp.basePos.y}, sf::Color(255, 220, 80)},
            sf::Vertex{sf::Vector2f{tipX, tipY}, sf::Color(255, 60, 20)}
        };
        target.draw(crackLine, 2, sf::PrimitiveType::Lines);
    }

    // 5. Отрисовка разлетающихся каменных обломков и комьев земли
    for (const auto& d : m_debris) {
        if (!d.active) continue;
        float prog = d.timer / d.maxDuration;
        float alpha = (1.0f - prog * prog) * 255.0f;
        sf::RectangleShape rock({d.size, d.size * 0.75f});
        rock.setOrigin({d.size * 0.5f, d.size * 0.375f});
        rock.setPosition(d.pos);
        rock.setRotation(sf::degrees(d.rot));
        sf::Color rc = d.color;
        rc.a = static_cast<std::uint8_t>(alpha);
        rock.setFillColor(rc);
        rock.setOutlineThickness(1.0f);
        rock.setOutlineColor(sf::Color(255, 120, 40, static_cast<std::uint8_t>(alpha * 0.75f)));
        target.draw(rock);
    }

    // 6. Отрисовка расширяющихся ударных колец
    for (const auto& r : m_rings) {
        if (!r.active) continue;
        float prog = r.timer / r.maxDuration;
        float alpha = (1.0f - prog) * 220.0f;
        int pts = 24;
        sf::VertexArray ringVA(sf::PrimitiveType::LineStrip);
        for (int i = 0; i <= pts; ++i) {
            float a = (i / static_cast<float>(pts)) * 6.28318f;
            float rx = r.center.x + std::cos(a) * r.radiusX;
            float ry = r.center.y + std::sin(a) * r.radiusY;
            ringVA.append({{rx, ry}, sf::Color(r.color.r, r.color.g, r.color.b, static_cast<std::uint8_t>(alpha)), {0.0f, 0.0f}});
        }
        target.draw(ringVA);
    }

    // 7. Отрисовка взрыва врезания в стену (Wall Crash)
    for (const auto& w : m_wallCrashes) {
        if (!w.active) continue;
        float prog = w.timer / w.maxDuration;
        float alpha = (1.0f - prog) * 255.0f;

        // Вспышка на стене
        sf::CircleShape flash(32.0f * (1.0f + prog * 0.8f));
        flash.setOrigin({flash.getRadius(), flash.getRadius()});
        flash.setPosition(w.pos);
        flash.setFillColor(sf::Color(255, 230, 80, static_cast<std::uint8_t>(alpha * 0.45f)));
        target.draw(flash);

        // Осколки стены
        for (const auto& d : w.chunks) {
            float dProg = d.timer / d.maxDuration;
            if (dProg >= 1.0f) continue;
            float dAlpha = (1.0f - dProg) * 255.0f;
            sf::RectangleShape rk({d.size, d.size * 0.8f});
            rk.setOrigin({d.size * 0.5f, d.size * 0.4f});
            rk.setPosition(d.pos);
            rk.setRotation(sf::degrees(d.rot));
            sf::Color rc = d.color;
            rc.a = static_cast<std::uint8_t>(dAlpha);
            rk.setFillColor(rc);
            rk.setOutlineThickness(1.0f);
            rk.setOutlineColor(sf::Color(255, 140, 30, static_cast<std::uint8_t>(dAlpha * 0.8f)));
            target.draw(rk);
        }
    }
}

void BossVfxManager::clear() {
    m_slashes.clear();
    m_spikes.clear();
    m_debris.clear();
    m_rings.clear();
    m_pillars.clear();
    m_wallCrashes.clear();
    m_afterimages.clear();
}

size_t BossVfxManager::getActiveCount() const {
    size_t count = m_slashes.size() + m_spikes.size() + m_debris.size() +
                  m_rings.size() + m_pillars.size() + m_wallCrashes.size() +
                  m_afterimages.size();
    return count;
}

