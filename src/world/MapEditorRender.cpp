#include "world/MapEditor.h"
#include "world/MapEditorInternal.h"
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace map_editor_internal;

void MapEditor::drawPlaytestUI(sf::RenderWindow& window, const sf::Font* font) {
    if (!_isPlaytesting || !font) return;
    float winW = static_cast<float>(window.getSize().x);

    sf::RectangleShape banner({winW, 36.0f});
    banner.setPosition({0.0f, 0.0f});
    banner.setFillColor(sf::Color(180, 20, 20, 220));
    banner.setOutlineColor(sf::Color(255, 80, 80));
    banner.setOutlineThickness(1.0f);
    window.draw(banner);

    sf::Text txt(*font, toSfString("<< РЕЖИМ БЫСТРОГО ТЕСТИРОВАНИЯ (PLAYTEST) >>  Нажмите [Tab] или [F6] для возврата в редактор"), 15);
    txt.setFillColor(sf::Color::White);
    txt.setStyle(sf::Text::Bold);
    sf::FloatRect b = txt.getLocalBounds();
    txt.setPosition({(winW - b.size.x) / 2.0f, 8.0f});
    window.draw(txt);
}


void MapEditor::drawJumpArc(sf::RenderWindow& window, const Arena& arena, const sf::Font* font) {
    if (!_showJumpRuler) return;
    float ts = arena.getTileSize();
    sf::Vector2f startPos;
    if (_lastHoverTile.x >= 0 && _lastHoverTile.x < arena.getWidth() &&
        _lastHoverTile.y >= 0 && _lastHoverTile.y < arena.getHeight()) {
        startPos = { static_cast<float>(_lastHoverTile.x * ts + ts / 2.0f),
                     static_cast<float>(_lastHoverTile.y * ts) };
    } else {
        startPos = arena.getSpawnPosition();
    }

    const float vx = 380.0f;
    const float vy0 = -800.0f;
    const float g = 980.0f;
    const float dt = 0.02f;
    const float maxT = 1.70f;

    for (int dir : {1, -1}) {
        sf::VertexArray arc(sf::PrimitiveType::LineStrip);
        sf::Vector2f apexPt = startPos;
        float apexY = startPos.y;

        for (float t = 0.0f; t <= maxT; t += dt) {
            float px = startPos.x + dir * vx * t;
            float py = startPos.y + vy0 * t + 0.5f * g * t * t;
            sf::Vector2f curPt(px, py);

            if (py < apexY) {
                apexY = py;
                apexPt = curPt;
            }

            int tx = static_cast<int>(std::floor(px / ts));
            int ty = static_cast<int>(std::floor(py / ts));
            if (t > 0.08f && (arena.isSolid(tx, ty) || py > startPos.y + 350.0f)) {
                arc.append(sf::Vertex({curPt, sf::Color(255, 215, 0, 180)}));
                break;
            }

            sf::Color col = (t < 0.8f) ? sf::Color(0, 230, 255, 210) : sf::Color(255, 200, 50, 160);
            arc.append(sf::Vertex({curPt, col}));
        }
        window.draw(arc);

        sf::CircleShape apexCircle(4.0f);
        apexCircle.setFillColor(sf::Color(255, 215, 0));
        apexCircle.setOrigin({4.0f, 4.0f});
        apexCircle.setPosition(apexPt);
        window.draw(apexCircle);

        if (font && dir == 1) {
            float jumpHeight = std::abs(apexPt.y - startPos.y);
            sf::Text aTxt(*font, "Apex: -" + std::to_string(static_cast<int>(jumpHeight)) + "px", 11);
            aTxt.setFillColor(sf::Color(255, 235, 100));
            aTxt.setPosition({apexPt.x + 6.0f, apexPt.y - 14.0f});
            window.draw(aTxt);
        }
    }

    // Линейка горизонтального дэша (Dash: 720 px/s * 0.25s = 180 px)
    const float dashDist = 180.0f;
    for (int dir : {1, -1}) {
        sf::VertexArray dashLine(sf::PrimitiveType::Lines);
        dashLine.append(sf::Vertex({startPos, sf::Color(255, 80, 220, 220)}));
        dashLine.append(sf::Vertex({sf::Vector2f(startPos.x + dir * dashDist, startPos.y), sf::Color(255, 80, 220, 220)}));
        window.draw(dashLine);

        sf::CircleShape dashEnd(3.0f);
        dashEnd.setFillColor(sf::Color(255, 80, 220));
        dashEnd.setOrigin({3.0f, 3.0f});
        dashEnd.setPosition({startPos.x + dir * dashDist, startPos.y});
        window.draw(dashEnd);
    }
}

void MapEditor::drawWorld(sf::RenderWindow& window, const Arena& arena, const sf::Vector2f& mouseWorld,
                          const sf::Font* font) {
    if (!_active || _isPlaytesting) return;

    float ts = arena.getTileSize();
    sf::View view = window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    int startX = std::max(0, static_cast<int>((center.x - size.x / 2.0f) / ts) - 1);
    int endX = std::min(arena.getWidth(), static_cast<int>((center.x + size.x / 2.0f) / ts) + 2);
    int startY = std::max(0, static_cast<int>((center.y - size.y / 2.0f) / ts) - 1);
    int endY = std::min(arena.getHeight(), static_cast<int>((center.y + size.y / 2.0f) / ts) + 2);

    // 1. Сетка тайлов (Grid Overlay)
    if (_showGrid) {
        sf::VertexArray lines(sf::PrimitiveType::Lines);
        for (int x = startX; x <= endX; ++x) {
            lines.append(sf::Vertex({sf::Vector2f(x * ts, startY * ts), sf::Color(100, 120, 150, 40)}));
            lines.append(sf::Vertex({sf::Vector2f(x * ts, endY * ts), sf::Color(100, 120, 150, 40)}));
        }
        for (int y = startY; y <= endY; ++y) {
            lines.append(sf::Vertex({sf::Vector2f(startX * ts, y * ts), sf::Color(100, 120, 150, 40)}));
            lines.append(sf::Vertex({sf::Vector2f(endX * ts, y * ts), sf::Color(100, 120, 150, 40)}));
        }
        window.draw(lines);
    }

    // 2. Рамка экрана 1920x1080 (Viewfinder)
    if (_showScreenFrame) {
        sf::RectangleShape frame({1920.0f, 1080.0f});
        frame.setOrigin({960.0f, 540.0f});
        frame.setPosition(center);
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineColor(sf::Color(255, 80, 220, 180));
        frame.setOutlineThickness(2.0f);
        window.draw(frame);

        if (font) {
            sf::Text fTxt(*font, "1920x1080 Viewport", 12);
            fTxt.setFillColor(sf::Color(255, 120, 230));
            fTxt.setPosition({center.x - 950.0f, center.y - 530.0f});
            window.draw(fTxt);
        }
    }

    // 3. Маркер спавна игрока
    sf::Vector2f pSpawn = arena.getSpawnPosition();
    float pBoxX = std::round(pSpawn.x / ts) * ts;
    float pBoxY = (std::round((pSpawn.y + 30.0f) / ts) - 1.0f) * ts;
    sf::RectangleShape pBox({ts, ts});
    pBox.setPosition({pBoxX, pBoxY});
    pBox.setFillColor(sf::Color(0, 230, 118, 50));
    pBox.setOutlineColor(sf::Color(0, 230, 118, 220));
    pBox.setOutlineThickness(2.0f);
    window.draw(pBox);

    if (font) {
        sf::Text pText(*font, "P", 18);
        pText.setFillColor(sf::Color(0, 230, 118));
        pText.setPosition({pBoxX + ts * 0.35f, pBoxY + ts * 0.25f});
        window.draw(pText);
    }

    // 4. Маркеры спавна врагов с инспектором
    // Отрисовка спавнов врагов (полное визуальное соответствие игровому рендеру)
    const auto& eSpawns = arena.getEnemySpawns();
    int hoveredEnemyIdx = -1;
    for (size_t i = 0; i < eSpawns.size(); ++i) {
        sf::Vector2f eSpawn = eSpawns[i].position;
        int eType = eSpawns[i].type;

        int stx = static_cast<int>(std::floor(eSpawn.x / ts));
        int sty = static_cast<int>(std::floor((eSpawn.y + 28.0f) / ts) - 1.0f);
        if (stx == _lastHoverTile.x && sty == _lastHoverTile.y) {
            hoveredEnemyIdx = static_cast<int>(i);
        }

        float scale = (eType == 1) ? 1.5f : 1.0f;
        float halfW = 24.0f * scale;
        float halfH = 21.0f * scale;

        float posX = eSpawn.x;
        float posY = eSpawn.y;

        // 1. Овальная тень под ногами
        sf::CircleShape shadow(16.0f * scale);
        shadow.setScale({1.0f, 0.35f});
        shadow.setOrigin({16.0f * scale, 16.0f * scale});
        shadow.setPosition({posX, posY + halfH - 2.0f});
        shadow.setFillColor(sf::Color(0, 0, 0, 95));
        window.draw(shadow);

        // 2. Тело противника
        sf::RectangleShape body({halfW * 2.0f, halfH * 2.0f});
        body.setOrigin({halfW, halfH});
        body.setPosition({posX, posY});
        body.setOutlineThickness(2.0f);
        body.setOutlineColor(sf::Color(25, 25, 30, 240));

        sf::Color baseColor;
        std::string typeBadge;
        sf::Color badgeColor;

        if (eType == 1) { // ТАНК
            baseColor = sf::Color(215, 38, 48);
            typeBadge = "ТАНК #" + std::to_string(i + 1);
            badgeColor = sf::Color(255, 90, 90);
        } else if (eType == 2) { // ДУЭЛЯНТ АГРЕССИВНЫЙ
            baseColor = sf::Color(155, 45, 225);
            typeBadge = "ДУЭЛЬ-АГР #" + std::to_string(i + 1);
            badgeColor = sf::Color(220, 140, 255);
        } else if (eType == 3) { // ДУЭЛЯНТ ПАССИВНЫЙ
            baseColor = sf::Color(45, 185, 245);
            typeBadge = "ДУЭЛЬ-ПАСС #" + std::to_string(i + 1);
            badgeColor = sf::Color(100, 220, 255);
        } else if (eType == 4) { // МАНЕКЕН
            baseColor = sf::Color(220, 175, 75);
            typeBadge = "МАНЕКЕН #" + std::to_string(i + 1);
            badgeColor = sf::Color(255, 225, 120);
        } else { // ОБЫЧНЫЙ
            baseColor = sf::Color(135, 140, 150);
            typeBadge = "ОБЫЧНЫЙ #" + std::to_string(i + 1);
            badgeColor = sf::Color(210, 215, 225);
        }

        body.setFillColor(baseColor);
        window.draw(body);

        // 3. Мишень / визор на теле
        if (eType == 4) {
            sf::CircleShape ringOuter(10.0f * scale);
            ringOuter.setOrigin({10.0f * scale, 10.0f * scale});
            ringOuter.setPosition({posX, posY});
            ringOuter.setFillColor(sf::Color(185, 45, 40));
            ringOuter.setOutlineThickness(1.5f);
            ringOuter.setOutlineColor(sf::Color(255, 255, 255, 220));
            window.draw(ringOuter);

            sf::CircleShape ringMid(5.0f * scale);
            ringMid.setOrigin({5.0f * scale, 5.0f * scale});
            ringMid.setPosition({posX, posY});
            ringMid.setFillColor(sf::Color(255, 255, 255, 240));
            window.draw(ringMid);

            sf::CircleShape ringDot(2.0f * scale);
            ringDot.setOrigin({2.0f * scale, 2.0f * scale});
            ringDot.setPosition({posX, posY});
            ringDot.setFillColor(sf::Color(185, 45, 40));
            window.draw(ringDot);
        } else {
            sf::RectangleShape target({14.0f * scale, 14.0f * scale});
            target.setOrigin({7.0f * scale, 7.0f * scale});
            target.setPosition({posX, posY});
            target.setFillColor(sf::Color(40, 40, 50, 160));
            window.draw(target);

            sf::RectangleShape eye({4.0f * scale, 4.0f * scale});
            eye.setPosition({posX + 3.0f * scale, posY - 14.0f * scale});
            sf::Color eyeColor = (eType == 1) ? sf::Color(255, 50, 60) :
                                 (eType == 2) ? sf::Color(255, 225, 0) : sf::Color(60, 220, 255);
            eye.setFillColor(eyeColor);
            window.draw(eye);
        }

        // 4. Текстовый бейдж архетипа над головой с подложкой
        if (font) {
            sf::Text bTxt(*font, toSfString(typeBadge), 10);
            bTxt.setFillColor(badgeColor);
            sf::FloatRect tb = bTxt.getLocalBounds();

            float pillW = tb.size.x + 8.0f;
            float pillH = tb.size.y + 4.0f;
            float pillX = posX - pillW * 0.5f;
            float pillY = posY - halfH - pillH - 4.0f;

            sf::RectangleShape pill({pillW, pillH});
            pill.setPosition({pillX, pillY});
            pill.setFillColor(sf::Color(16, 18, 24, 215));
            pill.setOutlineColor(sf::Color(badgeColor.r, badgeColor.g, badgeColor.b, 160));
            pill.setOutlineThickness(1.0f);
            window.draw(pill);

            bTxt.setPosition({pillX + 4.0f, pillY + 1.0f});
            window.draw(bTxt);
        }
    }

    // 5. Инспектор объекта под курсором (Floating Inspector)
    if (font && hoveredEnemyIdx >= 0) {
        int eType = eSpawns[hoveredEnemyIdx].type;
        std::string eName = (eType == 1) ? "ТАНК (110 HP, Стан -45%)" :
                            (eType == 2) ? "ДУЭЛЯНТ АГР (65 HP, Прессинг, Зацеп 210px)" :
                            (eType == 3) ? "ДУЭЛЯНТ ПАСС (65 HP, Спейсинг, Инвиз 1с)" : "ОБЫЧНЫЙ (45 HP, Базовый)";
        std::string inspStr = "ВРАГ #" + std::to_string(hoveredEnemyIdx + 1) + "\n" + eName + "\n[ПКМ] - Сменить архетип | [Ластик] - Удалить";
        sf::Text inspTxt(*font, toSfString(inspStr), 12);
        inspTxt.setFillColor(sf::Color(255, 240, 150));

        float insX = _lastHoverTile.x * ts + ts + 8.0f;
        float insY = _lastHoverTile.y * ts - 10.0f;
        sf::RectangleShape inspBg({260.0f, 56.0f});
        inspBg.setPosition({insX, insY});
        inspBg.setFillColor(sf::Color(15, 20, 30, 235));
        inspBg.setOutlineColor(sf::Color(255, 215, 0, 220));
        inspBg.setOutlineThickness(1.5f);
        window.draw(inspBg);

        inspTxt.setPosition({insX + 8.0f, insY + 6.0f});
        window.draw(inspTxt);
    } else if (font && arena.isDoor(_lastHoverTile.x, _lastHoverTile.y)) {
        int target = arena.getDoorTargetRoom(_lastHoverTile.x, _lastHoverTile.y);
        std::string inspStr = "ДВЕРЬ ПЕРЕХОДА (Door Block)\nЦель: Комната " + std::to_string(target) + "\n[ПКМ] - След. цель (1..10) | [1..10] - Палитра";
        sf::Text inspTxt(*font, toSfString(inspStr), 12);
        inspTxt.setFillColor(sf::Color(0, 255, 255));

        float insX = _lastHoverTile.x * ts + ts + 8.0f;
        float insY = _lastHoverTile.y * ts - 10.0f;
        sf::RectangleShape inspBg({250.0f, 56.0f});
        inspBg.setPosition({insX, insY});
        inspBg.setFillColor(sf::Color(10, 25, 35, 235));
        inspBg.setOutlineColor(sf::Color(0, 229, 255, 220));
        inspBg.setOutlineThickness(1.5f);
        window.draw(inspBg);

        inspTxt.setPosition({insX + 8.0f, insY + 6.0f});
        window.draw(inspTxt);
    }

    // 6. Превью прямоугольной заливки Box Fill
    if (_boxDragActive) {
        int x1 = std::min(_boxStartTile.x, _boxCurTile.x);
        int x2 = std::max(_boxStartTile.x, _boxCurTile.x);
        int y1 = std::min(_boxStartTile.y, _boxCurTile.y);
        int y2 = std::max(_boxStartTile.y, _boxCurTile.y);
        float bw = (x2 - x1 + 1) * ts;
        float bh = (y2 - y1 + 1) * ts;

        sf::RectangleShape boxRect({bw, bh});
        boxRect.setPosition({x1 * ts, y1 * ts});
        if (_boxIsErase) {
            boxRect.setFillColor(sf::Color(240, 40, 40, 85));
            boxRect.setOutlineColor(sf::Color(255, 60, 60, 255));
        } else {
            boxRect.setFillColor(sf::Color(76, 175, 80, 85));
            boxRect.setOutlineColor(sf::Color(100, 255, 120, 255));
        }
        boxRect.setOutlineThickness(2.0f);
        window.draw(boxRect);

        if (font) {
            std::string dimStr = std::to_string(x2 - x1 + 1) + "x" + std::to_string(y2 - y1 + 1);
            sf::Text dimTxt(*font, dimStr, 13);
            dimTxt.setFillColor(sf::Color::White);
            dimTxt.setPosition({x1 * ts + 6.0f, y1 * ts + 4.0f});
            window.draw(dimTxt);
        }
    }

    // 7. Превью линии Line Tool
    if (_lineDragActive) {
        auto tiles = getLineTiles(_lineStartTile.x, _lineStartTile.y, _lineCurTile.x, _lineCurTile.y);
        for (const auto& pt : tiles) {
            sf::RectangleShape lineBlock({ts, ts});
            lineBlock.setPosition({pt.x * ts, pt.y * ts});
            lineBlock.setFillColor(sf::Color(33, 150, 243, 90));
            lineBlock.setOutlineColor(sf::Color(100, 200, 255, 230));
            lineBlock.setOutlineThickness(1.5f);
            window.draw(lineBlock);
        }
    }

    // 8. Превью выделения Selection (Ctrl+Drag)
    if (_selectionActive) {
        int x1 = std::min(_selStart.x, _selEnd.x);
        int x2 = std::max(_selStart.x, _selEnd.x);
        int y1 = std::min(_selStart.y, _selEnd.y);
        int y2 = std::max(_selStart.y, _selEnd.y);
        float sw = (x2 - x1 + 1) * ts;
        float sh = (y2 - y1 + 1) * ts;

        sf::RectangleShape selRect({sw, sh});
        selRect.setPosition({x1 * ts, y1 * ts});
        selRect.setFillColor(sf::Color(255, 215, 0, 45));
        selRect.setOutlineColor(sf::Color(255, 215, 0, 240));
        selRect.setOutlineThickness(2.0f);
        window.draw(selRect);
    }

    // 9. Превью вставки Paste Mode
    if (_pasteModeActive && _clipboard.hasData) {
        for (const auto& item : _clipboard.items) {
            int tx = _lastHoverTile.x + item.relX;
            int ty = _lastHoverTile.y + item.relY;
            sf::RectangleShape pTile({ts, ts});
            pTile.setPosition({tx * ts, ty * ts});
            if (item.tile == 1) pTile.setFillColor(sf::Color(76, 175, 80, 140));
            else if (item.tile == 2) pTile.setFillColor(sf::Color(90, 110, 140, 140));
            else if (item.doorTarget > 0) pTile.setFillColor(sf::Color(0, 229, 255, 140));
            else pTile.setFillColor(sf::Color(100, 100, 100, 60));
            pTile.setOutlineColor(sf::Color(255, 255, 255, 200));
            pTile.setOutlineThickness(1.0f);
            window.draw(pTile);
        }
    }

    // 10. Линейка прыжка (Jump Arc Ruler)
    drawJumpArc(window, arena, font);

    // 11. Подсветка курсора и кисти
    if (!_boxDragActive && !_lineDragActive && !_pasteModeActive &&
        _lastHoverTile.x >= 0 && _lastHoverTile.x < arena.getWidth() &&
        _lastHoverTile.y >= 0 && _lastHoverTile.y < arena.getHeight()) {
        int half = _brushSize / 2;
        float bW = static_cast<float>(_brushSize) * ts;
        float bH = static_cast<float>(_brushSize) * ts;
        float bX = static_cast<float>(_lastHoverTile.x - half) * ts;
        float bY = static_cast<float>(_lastHoverTile.y - half) * ts;

        sf::RectangleShape brushRect({bW, bH});
        brushRect.setPosition({bX, bY});
        brushRect.setOutlineThickness(2.0f);

        if (_tool == EditorTool::PLATFORM) {
            brushRect.setFillColor(sf::Color(76, 175, 80, 80));
            brushRect.setOutlineColor(sf::Color(76, 175, 80, 240));
        } else if (_tool == EditorTool::WALL) {
            brushRect.setFillColor(sf::Color(33, 150, 243, 80));
            brushRect.setOutlineColor(sf::Color(33, 150, 243, 240));
        } else if (_tool == EditorTool::DOOR) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(0, 229, 255, 90));
            brushRect.setOutlineColor(sf::Color(0, 229, 255, 255));
        } else if (_tool == EditorTool::PLAYER_SPAWN) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(0, 230, 118, 90));
            brushRect.setOutlineColor(sf::Color(0, 230, 118, 255));
        } else if (_tool == EditorTool::ENEMY_SPAWN) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(160, 160, 160, 90));
            brushRect.setOutlineColor(sf::Color(210, 210, 210, 255));
        } else if (_tool == EditorTool::ENEMY_TANK) {
            brushRect.setSize({ts * 1.5f, ts * 1.5f});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts - ts * 0.25f, static_cast<float>(_lastHoverTile.y) * ts - ts * 0.5f});
            brushRect.setFillColor(sf::Color(255, 50, 50, 90));
            brushRect.setOutlineColor(sf::Color(255, 80, 80, 255));
        } else if (_tool == EditorTool::ENEMY_DUELIST) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(170, 70, 230, 90));
            brushRect.setOutlineColor(sf::Color(200, 90, 255, 255));
        } else if (_tool == EditorTool::ENEMY_DUELIST_PASSIVE) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(35, 150, 225, 90));
            brushRect.setOutlineColor(sf::Color(45, 185, 245, 255));
        } else if (_tool == EditorTool::ERASER) {
            brushRect.setSize({bW, bH});
            brushRect.setPosition({bX, bY});
            brushRect.setFillColor(sf::Color(180, 30, 30, 60));
            brushRect.setOutlineColor(sf::Color(255, 50, 50, 220));
        } else if (_tool == EditorTool::LAMP) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(180, 240, 255, 90));
            brushRect.setOutlineColor(sf::Color(220, 250, 255, 255));
        } else if (_tool == EditorTool::FLOOD_FILL) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(255, 180, 0, 90));
            brushRect.setOutlineColor(sf::Color(255, 200, 0, 255));
        } else if (_tool == EditorTool::EYEDROPPER) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(200, 200, 255, 90));
            brushRect.setOutlineColor(sf::Color(255, 255, 255, 255));
        } else if (_tool == EditorTool::BOX_FILL || _tool == EditorTool::LINE) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(100, 220, 255, 80));
            brushRect.setOutlineColor(sf::Color(100, 220, 255, 240));
        }
        window.draw(brushRect);
    }
}

void MapEditor::drawUI(sf::RenderWindow& window, const Arena& arena, const sf::Font* font,
                       const sf::View& camera) {
    if (!_active || _isPlaytesting) return;

    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);

    // 1. Верхняя панель инструментов
    sf::RectangleShape bar({winW, 118.0f});
    bar.setPosition({0.0f, 0.0f});
    bar.setFillColor(sf::Color(18, 18, 22, 250));
    bar.setOutlineColor(sf::Color(45, 45, 52, 220));
    bar.setOutlineThickness(1.0f);
    window.draw(bar);

    if (font) {
        // Заголовок - чистый белый текст
        sf::Text title(*font, toSfString("[ РЕДАКТОР КАРТ ]  (Tab/P - Тест с курсора | Ctrl+Z/Y - Undo/Redo | Ctrl+S / F7 - Сохранить)"), 16);
        title.setFillColor(sf::Color::White);
        title.setPosition({20.0f, 6.0f});
        window.draw(title);

        // Индикатор комнат и зума
        std::string roomInfo = "<< [PageDown] Комната " + std::to_string(arena.getCurrentRoomIndex()) + 
                               " / " + std::to_string(arena.getTotalRooms()) + 
                               " (" + arena.getCurrentRoomPath() + ") [PageUp] >>    " +
                               "Размер: " + std::to_string(arena.getWidth()) + "x" + std::to_string(arena.getHeight()) + " (Alt+Стрелки)    " +
                               "Зум: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "% (Z / C / Wheel)";
        sf::Text roomText(*font, toSfString(roomInfo), 13);
        roomText.setFillColor(sf::Color(220, 220, 230));
        roomText.setPosition({20.0f, 30.0f});
        window.draw(roomText);

        // Инструменты и кисть
        std::string toolsStr = "Инструменты: ";
        toolsStr += (_tool == EditorTool::PLATFORM ? ">> [1] Пол <<  " : "[1] Пол  ");
        toolsStr += (_tool == EditorTool::WALL ? ">> [2] Стена <<  " : "[2] Стена  ");
        toolsStr += (_tool == EditorTool::DOOR ? (">> [3] Дверь(R" + std::to_string(_doorTargetRoom) + ") <<  ") : "[3] Дверь  ");
        toolsStr += (_tool == EditorTool::PLAYER_SPAWN ? ">> [4] Игрок <<  " : "[4] Игрок  ");
        toolsStr += (_tool == EditorTool::ENEMY_SPAWN ? ">> [5] Обычн <<  " : "[5] Обычн  ");
        toolsStr += (_tool == EditorTool::ENEMY_TANK ? ">> [6] Танк <<  " : "[6] Танк  ");
        toolsStr += (_tool == EditorTool::ENEMY_DUELIST ? ">> [7] Дуэль-Агр <<  " : "[7] Дуэль-Агр  ");
        toolsStr += (_tool == EditorTool::ENEMY_DUELIST_PASSIVE ? ">> [8] Дуэль-Пасс <<  " : "[8] Дуэль-Пасс  ");
        toolsStr += (_tool == EditorTool::ERASER ? ">> [9] Ластик <<  " : "[9] Ластик  ");
        toolsStr += (_tool == EditorTool::LAMP ? ">> [0] Лампа <<  " : "[0] Лампа  ");
        toolsStr += (_tool == EditorTool::BOX_FILL ? ">> [X] Box <<  " : "[X] Box  ");
        toolsStr += (_tool == EditorTool::LINE ? ">> [L] Линия <<  " : "[L] Линия  ");
        toolsStr += (_tool == EditorTool::FLOOD_FILL ? ">> [G] Ведро <<  " : "[G] Ведро  ");
        toolsStr += (_tool == EditorTool::EYEDROPPER ? ">> [Q] Пипетка <<  " : "[Q] Пипетка  ");
        toolsStr += "| Кисть: [B] " + std::to_string(_brushSize) + "x" + std::to_string(_brushSize);

        sf::Text toolsText(*font, toSfString(toolsStr), 13);
        toolsText.setFillColor(sf::Color::White);
        toolsText.setPosition({20.0f, 54.0f});
        window.draw(toolsText);

        // Подсказки управления
        std::string hints = "Shift+Drag: Box Fill | Alt+Drag: Линия | Ctrl+Z/Y: Отмена/Повтор (" + 
                            std::to_string(_undoStack.size()) + ") | Ctrl+C/V: Копия/Вставка | J: Дуга прыжка (" +
                            (_showJumpRuler ? "ВКЛ" : "ВЫКЛ") + ") | Ctrl+G: Сетка (" + (_showGrid ? "ВКЛ" : "ВЫКЛ") + ")";
        sf::Text hintText(*font, toSfString(hints), 12);
        hintText.setFillColor(sf::Color(200, 200, 210));
        hintText.setPosition({20.0f, 82.0f});
        window.draw(hintText);

        // Статусное всплывающее сообщение
        if (!_statusMessage.empty() && _statusTimer > 0.0f) {
            sf::Text statusText(*font, toSfString(_statusMessage), 15);
            statusText.setFillColor(sf::Color(100, 255, 120));
            sf::FloatRect bounds = statusText.getLocalBounds();

            float bgW = bounds.size.x + 36.0f;
            float bgH = bounds.size.y + 16.0f;
            float bgX = (winW - bgW) / 2.0f;
            float bgY = 130.0f;

            sf::RectangleShape msgBg({bgW, bgH});
            msgBg.setPosition({bgX, bgY});
            msgBg.setFillColor(sf::Color(15, 20, 28, 235));
            msgBg.setOutlineColor(sf::Color(100, 255, 120, 220));
            msgBg.setOutlineThickness(1.5f);

            statusText.setPosition({bgX + 18.0f, bgY + 6.0f});

            window.draw(msgBg);
            window.draw(statusText);
        }
    }

    // 2. Интерактивная миникарта
    float mmW = 260.0f;
    float mmH = 140.0f;
    float mmX = winW - mmW - 20.0f;
    float mmY = winH - mmH - 20.0f;
    _minimapRect = sf::FloatRect({mmX, mmY}, {mmW, mmH});

    sf::RectangleShape mmBg({mmW, mmH});
    mmBg.setPosition({mmX, mmY});
    mmBg.setFillColor(sf::Color(18, 18, 22, 250));
    mmBg.setOutlineColor(sf::Color(45, 45, 52, 220));
    mmBg.setOutlineThickness(1.5f);
    window.draw(mmBg);

    if (font) {
        sf::Text mmTitle(*font, toSfString("МИНИКАРТА (Клик для прыжка)"), 12);
        mmTitle.setFillColor(sf::Color::White);
        mmTitle.setPosition({mmX + 8.0f, mmY + 5.0f});
        window.draw(mmTitle);
    }

    float mapW = arena.getWidth() * arena.getTileSize();
    float mapH = arena.getHeight() * arena.getTileSize();
    float innerW = mmW - 16.0f;
    float innerH = mmH - 30.0f;
    float scaleX = innerW / mapW;
    float scaleY = innerH / mapH;
    float scale = std::min(scaleX, scaleY);

    float offX = mmX + 8.0f;
    float offY = mmY + 24.0f;

    for (int y = 0; y < arena.getHeight(); ++y) {
        for (int x = 0; x < arena.getWidth(); ++x) {
            int tile = arena.getTile(x, y);
            if (tile == 0) continue;

            sf::RectangleShape dot({std::max(1.5f, arena.getTileSize() * scale), 
                                    std::max(1.5f, arena.getTileSize() * scale)});
            dot.setPosition({offX + x * arena.getTileSize() * scale, 
                             offY + y * arena.getTileSize() * scale});

            if (tile == 1) dot.setFillColor(sf::Color(76, 175, 80));
            else if (tile == 2) dot.setFillColor(sf::Color(90, 110, 140));
            else if (tile == 3) dot.setFillColor(sf::Color(0, 229, 255));
            else if (tile == 4) dot.setFillColor(sf::Color(180, 240, 255));

            window.draw(dot);
        }
    }

    sf::Vector2f pSpawn = arena.getSpawnPosition();
    sf::CircleShape pDot(3.0f);
    pDot.setFillColor(sf::Color(0, 230, 118));
    pDot.setPosition({offX + pSpawn.x * scale - 3.0f, offY + pSpawn.y * scale - 3.0f});
    window.draw(pDot);

    for (const auto& sp : arena.getEnemySpawns()) {
        sf::CircleShape eDot(sp.type == 1 ? 4.0f : 3.0f);
        if (sp.type == 1) eDot.setFillColor(sf::Color(255, 60, 60));
        else if (sp.type == 2) eDot.setFillColor(sf::Color(190, 90, 255));
        else if (sp.type == 3) eDot.setFillColor(sf::Color(45, 185, 245));
        else eDot.setFillColor(sf::Color(190, 190, 190));
        eDot.setPosition({offX + sp.position.x * scale - 3.0f, offY + sp.position.y * scale - 3.0f});
        window.draw(eDot);
    }

    sf::Vector2f camCenter = camera.getCenter();
    sf::Vector2f camSize = camera.getSize();
    float camLeft = camCenter.x - camSize.x / 2.0f;
    float camTop = camCenter.y - camSize.y / 2.0f;

    sf::RectangleShape camFrame({camSize.x * scale, camSize.y * scale});
    camFrame.setPosition({offX + camLeft * scale, offY + camTop * scale});
    camFrame.setFillColor(sf::Color::Transparent);
    camFrame.setOutlineColor(sf::Color(255, 215, 0, 220));
    camFrame.setOutlineThickness(1.5f);
    window.draw(camFrame);

    // 3. Нижняя панель: палитра и галерея комнат
    float palX = 15.0f;
    float palH = 112.0f;
    float palY = winH - palH - 12.0f;
    float palW = mmX - palX - 12.0f;

    sf::RectangleShape palBg({palW, palH});
    palBg.setPosition({palX, palY});
    palBg.setFillColor(sf::Color(18, 18, 22, 250));
    palBg.setOutlineColor(sf::Color(45, 45, 52, 220));
    palBg.setOutlineThickness(1.5f);
    window.draw(palBg);

    if (font) {
        sf::Text palTitle(*font, toSfString("ПАЛИТРА И ГАЛЕРЕЯ КОМНАТ"), 13);
        palTitle.setFillColor(sf::Color::White);
        palTitle.setPosition({palX + 10.0f, palY + 6.0f});
        window.draw(palTitle);

        sf::Text doorLabel(*font, toSfString("ЦЕЛЬ ДВЕРИ:"), 12);
        doorLabel.setFillColor(sf::Color::White);
        doorLabel.setPosition({palX + 220.0f, palY + 6.0f});
        window.draw(doorLabel);

        // Кнопка [ < ]
        sf::RectangleShape btnPrev({20.0f, 20.0f});
        btnPrev.setPosition({palX + 328.0f, palY + 4.0f});
        btnPrev.setFillColor(sf::Color(30, 45, 65));
        btnPrev.setOutlineColor(sf::Color(0, 200, 255));
        btnPrev.setOutlineThickness(1.0f);
        window.draw(btnPrev);
        sf::Text btnPrevT(*font, "<", 12);
        btnPrevT.setFillColor(sf::Color::White);
        btnPrevT.setPosition({palX + 334.0f, palY + 4.0f});
        window.draw(btnPrevT);

        // Бейдж текущей целевой комнаты
        sf::RectangleShape badgeCur({40.0f, 20.0f});
        badgeCur.setPosition({palX + 351.0f, palY + 4.0f});
        badgeCur.setFillColor(sf::Color(0, 100, 140));
        badgeCur.setOutlineColor(sf::Color(0, 255, 255));
        badgeCur.setOutlineThickness(1.0f);
        window.draw(badgeCur);
        sf::Text badgeCurT(*font, "R" + std::to_string(_doorTargetRoom), 12);
        badgeCurT.setFillColor(sf::Color(0, 255, 255));
        badgeCurT.setPosition({palX + 357.0f, palY + 4.0f});
        window.draw(badgeCurT);

        // Кнопка [ > ]
        sf::RectangleShape btnNext({20.0f, 20.0f});
        btnNext.setPosition({palX + 394.0f, palY + 4.0f});
        btnNext.setFillColor(sf::Color(30, 45, 65));
        btnNext.setOutlineColor(sf::Color(0, 200, 255));
        btnNext.setOutlineThickness(1.0f);
        window.draw(btnNext);
        sf::Text btnNextT(*font, ">", 12);
        btnNextT.setFillColor(sf::Color::White);
        btnNextT.setPosition({palX + 400.0f, palY + 4.0f});
        window.draw(btnNextT);

        // 10 Цифровых кнопок комнат
        for (int r = 1; r <= 10; ++r) {
            float rx = palX + 420.0f + (r - 1) * 25.0f;
            float ry = palY + 4.0f;
            bool isCurrentTarget = (_doorTargetRoom == r);

            sf::RectangleShape rBtn({22.0f, 20.0f});
            rBtn.setPosition({rx, ry});
            rBtn.setFillColor(isCurrentTarget ? sf::Color(0, 200, 255) : sf::Color(25, 36, 50));
            rBtn.setOutlineColor(isCurrentTarget ? sf::Color(255, 255, 255) : sf::Color(60, 80, 105));
            rBtn.setOutlineThickness(1.0f);
            window.draw(rBtn);

            sf::Text rTxt(*font, std::to_string(r), 11);
            rTxt.setFillColor(isCurrentTarget ? sf::Color(10, 15, 25) : sf::Color(180, 200, 220));
            float textOffset = (r >= 10) ? 4.0f : 7.0f;
            rTxt.setPosition({rx + textOffset, ry + 3.0f});
            window.draw(rTxt);
        }

        // Кнопка [Клон в R...]
        sf::RectangleShape btnDup({100.0f, 20.0f});
        btnDup.setPosition({palX + 680.0f, palY + 4.0f});
        btnDup.setFillColor(sf::Color(35, 50, 70));
        btnDup.setOutlineColor(sf::Color(0, 200, 255));
        btnDup.setOutlineThickness(1.0f);
        window.draw(btnDup);
        sf::Text btnDupT(*font, toSfString("Клон в R" + std::to_string(_doorTargetRoom)), 11);
        btnDupT.setFillColor(sf::Color(150, 230, 255));
        btnDupT.setPosition({palX + 685.0f, palY + 3.0f});
        window.draw(btnDupT);

        // Кнопка [Очистить]
        sf::RectangleShape btnClr({80.0f, 20.0f});
        btnClr.setPosition({palX + 785.0f, palY + 4.0f});
        btnClr.setFillColor(sf::Color(65, 25, 25));
        btnClr.setOutlineColor(sf::Color(255, 80, 80));
        btnClr.setOutlineThickness(1.0f);
        window.draw(btnClr);
        sf::Text btnClrT(*font, toSfString("Очистить"), 11);
        btnClrT.setFillColor(sf::Color(255, 120, 120));
        btnClrT.setPosition({palX + 795.0f, palY + 3.0f});
        window.draw(btnClrT);

        // Кнопка [F5 - Обновить]
        float reloadX = std::max(palX + 875.0f, palX + palW - 120.0f);
        sf::RectangleShape btnReload({115.0f, 20.0f});
        btnReload.setPosition({reloadX, palY + 4.0f});
        btnReload.setFillColor(sf::Color(25, 40, 60));
        btnReload.setOutlineColor(sf::Color(100, 220, 255));
        btnReload.setOutlineThickness(1.0f);
        window.draw(btnReload);
        sf::Text btnReloadT(*font, toSfString("[F5 - Обновить]"), 11);
        btnReloadT.setFillColor(sf::Color(120, 230, 255));
        btnReloadT.setPosition({reloadX + 8.0f, palY + 4.0f});
        window.draw(btnReloadT);
    }

    // Отрисовка карточек палитры
    float startX = palX + 10.0f;
    float cardY = palY + 28.0f;
    float cardW = 60.0f;
    float cardH = 76.0f;

    for (size_t i = 0; i < _paletteSprites.size(); ++i) {
        float cardX = startX + i * 65.0f;
        if (cardX + cardW > palX + palW) break;

        bool isSelected = (_tool == _paletteSprites[i].tool && (_tool != EditorTool::WALL || _currentTileCode == _paletteSprites[i].tileCode));
        sf::RectangleShape card({cardW, cardH});
        card.setPosition({cardX, cardY});
        card.setFillColor(isSelected ? sf::Color(38, 38, 44) : sf::Color(22, 22, 26));
        card.setOutlineColor(isSelected ? sf::Color::White : sf::Color(55, 55, 62));
        card.setOutlineThickness(isSelected ? 2.5f : 1.0f);
        window.draw(card);

        if (font) {
            std::string hotkeyStr = "";
            if (_paletteSprites[i].tool == EditorTool::PLATFORM) hotkeyStr = "[1]";
            else if (_paletteSprites[i].tool == EditorTool::WALL && _paletteSprites[i].filename.find("02_wall") != std::string::npos) hotkeyStr = "[2]";
            else if (_paletteSprites[i].tool == EditorTool::DOOR) hotkeyStr = "[3]";
            else if (_paletteSprites[i].tool == EditorTool::PLAYER_SPAWN) hotkeyStr = "[4]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_SPAWN) hotkeyStr = "[5]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_TANK) hotkeyStr = "[6]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST) hotkeyStr = "[7]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST_PASSIVE) hotkeyStr = "[8]";
            else if (_paletteSprites[i].tool == EditorTool::ERASER) hotkeyStr = "[9]";
            else if (_paletteSprites[i].tool == EditorTool::LAMP) hotkeyStr = "[0]";
            else if (_paletteSprites[i].tool == EditorTool::BOX_FILL) hotkeyStr = "[X]";
            else if (_paletteSprites[i].tool == EditorTool::LINE) hotkeyStr = "[L]";
            else if (_paletteSprites[i].tool == EditorTool::FLOOD_FILL) hotkeyStr = "[G]";
            else if (_paletteSprites[i].tool == EditorTool::EYEDROPPER) hotkeyStr = "[Q]";

            if (!hotkeyStr.empty()) {
                sf::Text hkTxt(*font, hotkeyStr, 10);
                hkTxt.setFillColor(sf::Color::White);
                hkTxt.setPosition({cardX + 4.0f, cardY + 3.0f});
                window.draw(hkTxt);
            }
        }

        float iconX = cardX + (cardW - 32.0f) / 2.0f;
        float iconY = cardY + 17.0f;

        if (_paletteSprites[i].sprite) {
            _paletteSprites[i].sprite->setPosition({iconX, iconY});
            window.draw(*_paletteSprites[i].sprite);
        } else {
            sf::RectangleShape colorBox({32.0f, 24.0f});
            colorBox.setPosition({iconX, iconY});
            if (_paletteSprites[i].tool == EditorTool::PLATFORM) colorBox.setFillColor(sf::Color(76, 175, 80));
            else if (_paletteSprites[i].tool == EditorTool::WALL) colorBox.setFillColor(sf::Color(90, 110, 140));
            else if (_paletteSprites[i].tool == EditorTool::DOOR) colorBox.setFillColor(sf::Color(0, 229, 255));
            else if (_paletteSprites[i].tool == EditorTool::PLAYER_SPAWN) colorBox.setFillColor(sf::Color(0, 230, 118));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_SPAWN) colorBox.setFillColor(sf::Color(180, 180, 180));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_TANK) colorBox.setFillColor(sf::Color(240, 50, 50));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST) colorBox.setFillColor(sf::Color(170, 70, 230));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST_PASSIVE) colorBox.setFillColor(sf::Color(45, 185, 245));
            else if (_paletteSprites[i].tool == EditorTool::ERASER) colorBox.setFillColor(sf::Color(60, 60, 60));
            else if (_paletteSprites[i].tool == EditorTool::LAMP) colorBox.setFillColor(sf::Color(180, 240, 255));
            else if (_paletteSprites[i].tool == EditorTool::BOX_FILL) colorBox.setFillColor(sf::Color(100, 180, 255));
            else if (_paletteSprites[i].tool == EditorTool::LINE) colorBox.setFillColor(sf::Color(80, 220, 160));
            else if (_paletteSprites[i].tool == EditorTool::FLOOD_FILL) colorBox.setFillColor(sf::Color(255, 180, 0));
            else if (_paletteSprites[i].tool == EditorTool::EYEDROPPER) colorBox.setFillColor(sf::Color(200, 100, 255));

            colorBox.setOutlineColor(sf::Color(200, 220, 240, 150));
            colorBox.setOutlineThickness(1.0f);
            window.draw(colorBox);

            if (font) {
                std::string letter = "";
                if (_paletteSprites[i].tool == EditorTool::PLAYER_SPAWN) letter = "P";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_SPAWN) letter = "N";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_TANK) letter = "T";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST) letter = "DA";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST_PASSIVE) letter = "DP";
                else if (_paletteSprites[i].tool == EditorTool::DOOR) letter = "D";
                else if (_paletteSprites[i].tool == EditorTool::ERASER) letter = "X";
                else if (_paletteSprites[i].tool == EditorTool::LAMP) letter = "L";
                else if (_paletteSprites[i].tool == EditorTool::BOX_FILL) letter = "[]";
                else if (_paletteSprites[i].tool == EditorTool::LINE) letter = "/";
                else if (_paletteSprites[i].tool == EditorTool::FLOOD_FILL) letter = "F";
                else if (_paletteSprites[i].tool == EditorTool::EYEDROPPER) letter = "Q";

                if (!letter.empty()) {
                    sf::Text lTxt(*font, letter, 12);
                    lTxt.setFillColor(sf::Color::White);
                    lTxt.setPosition({iconX + 6.0f, iconY + 3.0f});
                    window.draw(lTxt);
                }
            }
        }

        if (font) {
            std::string label = _paletteSprites[i].name;
            if (label.size() > 9) label = label.substr(0, 8) + ".";
            sf::Text cardTxt(*font, toSfString(label), 10);
            cardTxt.setFillColor(sf::Color::White);
            cardTxt.setPosition({cardX + 4.0f, cardY + 56.0f});
            window.draw(cardTxt);
        }
    }

    drawLeftToolbar(window, font, winW, winH);
}

void MapEditor::drawLeftToolbar(sf::RenderWindow& window, const sf::Font* font, float winW, float winH) {
    float tbW = 54.0f;
    float startY = 126.0f;
    float btnH = 24.0f;
    float btnGap = 3.0f;
    float totalH = static_cast<float>(s_ingameLeftButtons.size()) * (btnH + btnGap) + 8.0f;

    sf::RectangleShape panel({tbW, totalH});
    panel.setPosition({6.0f, startY});
    panel.setFillColor(sf::Color(16, 18, 22, 245));
    panel.setOutlineColor(sf::Color(45, 50, 60));
    panel.setOutlineThickness(1.0f);
    window.draw(panel);

    sf::Vector2i mPixel = sf::Mouse::getPosition(window);
    sf::Vector2f mPos(static_cast<float>(mPixel.x), static_cast<float>(mPixel.y));

    for (size_t i = 0; i < s_ingameLeftButtons.size(); ++i) {
        const auto& btn = s_ingameLeftButtons[i];
        float by = startY + 4.0f + i * (btnH + btnGap);
        sf::FloatRect btnRect({10.0f, by}, {46.0f, btnH});

        bool isActive = (_tool == btn.tool);
        bool isHover = btnRect.contains(mPos);

        sf::RectangleShape bShape({46.0f, btnH});
        bShape.setPosition({10.0f, by});
        bShape.setFillColor(isActive ? sf::Color(38, 65, 95) : isHover ? sf::Color(30, 35, 44) : sf::Color(22, 25, 31));
        bShape.setOutlineColor(isActive ? sf::Color(100, 180, 255) : isHover ? sf::Color(70, 80, 100) : sf::Color(40, 45, 55));
        bShape.setOutlineThickness(isActive ? 1.5f : 1.0f);
        window.draw(bShape);

        if (font) {
            sf::Text t(*font, toSfString(btn.icon), 11);
            t.setFillColor(isActive ? sf::Color::White : btn.accent);
            sf::FloatRect tb = t.getLocalBounds();
            t.setPosition({10.0f + (46.0f - tb.size.x) * 0.5f, by + (btnH - tb.size.y) * 0.5f - 2.0f});
            window.draw(t);
        }

        if (isHover && font) {
            sf::Text tip(*font, toSfString(btn.label), 12);
            tip.setFillColor(sf::Color::White);
            sf::FloatRect tb = tip.getLocalBounds();

            sf::RectangleShape tipBg({tb.size.x + 14.0f, tb.size.y + 8.0f});
            tipBg.setPosition({64.0f, by});
            tipBg.setFillColor(sf::Color(18, 20, 26, 245));
            tipBg.setOutlineColor(sf::Color(80, 140, 200));
            tipBg.setOutlineThickness(1.0f);

            tip.setPosition({71.0f, by + 3.0f});
            window.draw(tipBg);
            window.draw(tip);
        }
    }
}

