#include "ui/MenuSystem.h"
#include "audio/SoundManager.h"
#include <iomanip>
#include <sstream>

MenuSystem::MenuSystem() {
    _currentVolume = SoundManager::get().getMasterVolume();
    if (_currentVolume <= 0.0f) _currentVolume = 50.0f;
    _preMuteVolume = _currentVolume;
}

void MenuSystem::init(const sf::Font* font) {
    _font = font;
}

bool MenuSystem::drawButton(sf::RenderWindow& window, const Button& btn, const sf::Vector2f& mousePos, bool selected) {
    bool hovered = btn.rect.contains(mousePos);

    sf::RectangleShape shape(btn.rect.size);
    shape.setPosition(btn.rect.position);

    sf::Color fillCol = selected ? sf::Color(btn.hoverColor.r, btn.hoverColor.g, btn.hoverColor.b, 220)
                                 : (hovered ? btn.hoverColor : btn.normalColor);
    shape.setFillColor(fillCol);
    shape.setOutlineThickness(hovered || selected ? 2.5f : 1.5f);
    shape.setOutlineColor(hovered || selected ? sf::Color::White : btn.outlineColor);
    window.draw(shape);

    if (_font) {
        sf::Text text(*_font, toSfString(btn.label), btn.fontSize);
        text.setFillColor(hovered || selected ? sf::Color::White : sf::Color(220, 230, 245));
        sf::FloatRect tb = text.getLocalBounds();
        text.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        text.setPosition({btn.rect.position.x + btn.rect.size.x * 0.5f,
                          btn.rect.position.y + btn.rect.size.y * 0.5f});
        window.draw(text);
    }

    return hovered;
}

void MenuSystem::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
    if (const auto* mbPressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbPressed->button != sf::Mouse::Button::Left) return;

        sf::Vector2f winSize(1280.0f, 720.0f); // Default reference, overridden in screen calls

        if (_currentScreen == MenuScreen::MAIN) {
            // Main menu / Pause menu buttons handled in screen logic
        }
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Escape) {
            if (_currentScreen == MenuScreen::SETTINGS || _currentScreen == MenuScreen::GUIDE) {
                _currentScreen = MenuScreen::MAIN;
                SoundManager::get().play("1", 30.0f);
            } else if (_appState == AppState::PAUSED) {
                resumeGame();
            }
        }
    }
}

void MenuSystem::drawKeybindRow(sf::RenderWindow& window, float x, float y, float w, 
                               const std::string& key, const std::string& desc, sf::Color keyCol) {
    if (!_font) return;

    // Фон строки
    sf::RectangleShape rowBg({w, 28.0f});
    rowBg.setPosition({x, y});
    rowBg.setFillColor(sf::Color(20, 26, 38, 160));
    rowBg.setOutlineThickness(1.0f);
    rowBg.setOutlineColor(sf::Color(45, 58, 80, 180));
    window.draw(rowBg);

    // Клавиша
    sf::Text keyText(*_font, toSfString(key), 14);
    keyText.setStyle(sf::Text::Bold);
    keyText.setFillColor(keyCol);
    keyText.setPosition({x + 12.0f, y + 4.0f});
    window.draw(keyText);

    // Описание
    sf::Text descText(*_font, toSfString(desc), 13);
    descText.setFillColor(sf::Color(215, 225, 240));
    descText.setPosition({x + 175.0f, y + 5.0f});
    window.draw(descText);
}

void MenuSystem::drawMainMenuScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize) {
    float cx = winSize.x * 0.5f;
    float cy = winSize.y * 0.5f;

    // Затемненная центральная панель
    float panelW = 480.0f;
    float panelH = (_appState == AppState::PAUSED) ? 520.0f : 470.0f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setOrigin({panelW * 0.5f, panelH * 0.5f});
    panel.setPosition({cx, cy});
    panel.setFillColor(sf::Color(15, 20, 30, 245));
    panel.setOutlineThickness(3.0f);
    panel.setOutlineColor(sf::Color(65, 110, 180, 230));
    window.draw(panel);

    if (_font) {
        // Заголовок
        std::string titleStr = (_appState == AppState::PAUSED) ? "ПАУЗА" : "FIGHTING COMBO ARENA";
        sf::Text title(*_font, toSfString(titleStr), (_appState == AppState::PAUSED) ? 40 : 34);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(245, 250, 255));
        title.setOutlineThickness(2.0f);
        title.setOutlineColor(sf::Color(10, 15, 25));
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        title.setPosition({cx, cy - panelH * 0.5f + 55.0f});
        window.draw(title);

        // Подзаголовок
        std::string subStr = (_appState == AppState::PAUSED) ? "ИГРА ПРИОСТАНОВЛЕНА" : "ГЛАВНОЕ МЕНЮ";
        sf::Text sub(*_font, toSfString(subStr), 14);
        sub.setFillColor(sf::Color(130, 175, 240, 200));
        sf::FloatRect sb = sub.getLocalBounds();
        sub.setOrigin({sb.position.x + sb.size.x * 0.5f, sb.position.y + sb.size.y * 0.5f});
        sub.setPosition({cx, cy - panelH * 0.5f + 92.0f});
        window.draw(sub);
    }

    float btnW = 340.0f;
    float btnH = 50.0f;
    float startY = cy - panelH * 0.5f + 125.0f;
    float stepY = 64.0f;

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    static bool prevMouse = false;
    bool justClicked = mouseClicked && !prevMouse;
    prevMouse = mouseClicked;

    if (_appState == AppState::MAIN_MENU) {
        // 1. ИГРАТЬ
        Button btnPlay{{{cx - btnW * 0.5f, startY}, {btnW, btnH}}, "ИГРАТЬ",
                       sf::Color(35, 75, 135), sf::Color(55, 115, 210), sf::Color(90, 150, 240), 22};
        if (drawButton(window, btnPlay, mousePos) && justClicked) {
            SoundManager::get().play("1", 40.0f);
            if (_onStartGame) _onStartGame();
            _appState = AppState::IN_GAME;
        }

        // 2. НАСТРОЙКИ
        Button btnSettings{{{cx - btnW * 0.5f, startY + stepY}, {btnW, btnH}}, "НАСТРОЙКИ",
                           sf::Color(28, 45, 70), sf::Color(45, 75, 120), sf::Color(80, 120, 180), 20};
        if (drawButton(window, btnSettings, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::SETTINGS;
        }

        // 3. ГАЙД
        Button btnGuide{{{cx - btnW * 0.5f, startY + stepY * 2.0f}, {btnW, btnH}}, "ГАЙД ПО ИГРЕ",
                        sf::Color(35, 55, 60), sf::Color(50, 95, 105), sf::Color(85, 160, 175), 20};
        if (drawButton(window, btnGuide, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::GUIDE;
        }

        // 4. ВЫХОД
        Button btnExit{{{cx - btnW * 0.5f, startY + stepY * 3.0f}, {btnW, btnH}}, "ВЫХОД",
                       sf::Color(65, 28, 35), sf::Color(145, 45, 55), sf::Color(215, 80, 90), 20};
        if (drawButton(window, btnExit, mousePos) && justClicked) {
            if (_onExitGame) _onExitGame();
        }
    } else {
        // PAUSED STATE
        // 1. ПРОДОЛЖИТЬ
        Button btnResume{{{cx - btnW * 0.5f, startY}, {btnW, btnH}}, "ПРОДОЛЖИТЬ [ESC]",
                         sf::Color(35, 75, 135), sf::Color(55, 115, 210), sf::Color(90, 150, 240), 21};
        if (drawButton(window, btnResume, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            resumeGame();
        }

        // 2. НАСТРОЙКИ
        Button btnSettings{{{cx - btnW * 0.5f, startY + stepY}, {btnW, btnH}}, "НАСТРОЙКИ",
                           sf::Color(28, 45, 70), sf::Color(45, 75, 120), sf::Color(80, 120, 180), 20};
        if (drawButton(window, btnSettings, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::SETTINGS;
        }

        // 3. ГАЙД
        Button btnGuide{{{cx - btnW * 0.5f, startY + stepY * 2.0f}, {btnW, btnH}}, "ГАЙД ПО ИГРЕ",
                        sf::Color(35, 55, 60), sf::Color(50, 95, 105), sf::Color(85, 160, 175), 20};
        if (drawButton(window, btnGuide, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::GUIDE;
        }

        // 4. В ГЛАВНОЕ МЕНЮ
        Button btnToMenu{{{cx - btnW * 0.5f, startY + stepY * 3.0f}, {btnW, btnH}}, "В ГЛАВНОЕ МЕНЮ",
                         sf::Color(38, 38, 55), sf::Color(65, 65, 95), sf::Color(115, 115, 155), 19};
        if (drawButton(window, btnToMenu, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            if (_onResetGame) _onResetGame();
            openMainMenu();
        }

        // 5. ВЫХОД
        Button btnExit{{{cx - btnW * 0.5f, startY + stepY * 4.0f}, {btnW, btnH}}, "ВЫЙТИ ИЗ ИГРЫ",
                       sf::Color(65, 28, 35), sf::Color(145, 45, 55), sf::Color(215, 80, 90), 20};
        if (drawButton(window, btnExit, mousePos) && justClicked) {
            if (_onExitGame) _onExitGame();
        }
    }
}

void MenuSystem::drawSettingsScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize) {
    float cx = winSize.x * 0.5f;
    float cy = winSize.y * 0.5f;

    float panelW = 820.0f;
    float panelH = 610.0f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setOrigin({panelW * 0.5f, panelH * 0.5f});
    panel.setPosition({cx, cy});
    panel.setFillColor(sf::Color(15, 20, 30, 248));
    panel.setOutlineThickness(3.0f);
    panel.setOutlineColor(sf::Color(65, 110, 180, 230));
    window.draw(panel);

    float leftX = cx - panelW * 0.5f;
    float topY = cy - panelH * 0.5f;

    if (_font) {
        sf::Text title(*_font, toSfString("НАСТРОЙКИ"), 32);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(245, 250, 255));
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        title.setPosition({cx, topY + 45.0f});
        window.draw(title);
    }

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    static bool prevMouse = false;
    bool justClicked = mouseClicked && !prevMouse;
    prevMouse = mouseClicked;

    // Вкладки настроек: [ БИНДЫ ] [ ЗВУК ] [ FPS И РЕЖИМЫ ]
    float tabW = 210.0f;
    float tabH = 38.0f;
    float tabY = topY + 80.0f;
    float tabsStartX = cx - (tabW * 3.0f + 20.0f) * 0.5f;

    Button btnTabBinds{{{tabsStartX, tabY}, {tabW, tabH}}, "БИНДЫ (УПРАВЛЕНИЕ)",
                      sf::Color(25, 38, 55), sf::Color(45, 80, 125), sf::Color(80, 130, 195), 15};
    if (drawButton(window, btnTabBinds, mousePos, _settingsTab == SettingsTab::BINDS) && justClicked) {
        _settingsTab = SettingsTab::BINDS;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabSound{{{tabsStartX + tabW + 10.0f, tabY}, {tabW, tabH}}, "ЗВУК",
                      sf::Color(25, 38, 55), sf::Color(45, 80, 125), sf::Color(80, 130, 195), 15};
    if (drawButton(window, btnTabSound, mousePos, _settingsTab == SettingsTab::SOUND) && justClicked) {
        _settingsTab = SettingsTab::SOUND;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabFps{{{tabsStartX + (tabW + 10.0f) * 2.0f, tabY}, {tabW, tabH}}, "ФПС И ГРАФИКА",
                    sf::Color(25, 38, 55), sf::Color(45, 80, 125), sf::Color(80, 130, 195), 15};
    if (drawButton(window, btnTabFps, mousePos, _settingsTab == SettingsTab::FPS) && justClicked) {
        _settingsTab = SettingsTab::FPS;
        SoundManager::get().play("1", 30.0f);
    }

    // Содержимое выбранной вкладки
    float contentX = leftX + 40.0f;
    float contentY = topY + 135.0f;
    float contentW = panelW - 80.0f;
    float contentH = 390.0f;

    sf::RectangleShape contentBox({contentW, contentH});
    contentBox.setPosition({contentX, contentY});
    contentBox.setFillColor(sf::Color(12, 16, 24, 210));
    contentBox.setOutlineThickness(1.5f);
    contentBox.setOutlineColor(sf::Color(40, 55, 80, 200));
    window.draw(contentBox);

    if (_settingsTab == SettingsTab::BINDS) {
        // Список клавиш в две колонки
        float colW = (contentW - 30.0f) * 0.5f;
        float rowH = 26.0f;
        float startRowY = contentY + 12.0f;

        // Колонка 1: Боевые действия
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 0, colW, "A / D", "Движение влево / вправо");
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 1, colW, "Space / W", "Прыжок (направление + прыжок)");
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 2, colW, "ЛКМ (1)", "Легкий удар (Combo Starter)");
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 3, colW, "ПКМ (2)", "Тяжелый удар (Knockback)");
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 4, colW, "F или E", "Блок щитом (-80% входящего урона)");
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 5, colW, "Q", "Парирование / Контратака", sf::Color(0, 240, 255));
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 6, colW, "Left Shift", "Рывок / Уворот (i-frames)", sf::Color(255, 220, 80));
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 7, colW, "R", "Бросок булавы по прицелу / Подбор", sf::Color(255, 140, 70));
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 8, colW, "S / Down", "Ускоренный спуск в воздухе");
        drawKeybindRow(window, contentX + 10.0f, startRowY + rowH * 9, colW, "ESC", "Пауза / Главное меню");

        // Колонка 2: Системные и режимы
        float col2X = contentX + 20.0f + colW;
        drawKeybindRow(window, col2X, startRowY + rowH * 0, colW, "F3", "Вкл / Выкл счетчик FPS", sf::Color(110, 240, 130));
        drawKeybindRow(window, col2X, startRowY + rowH * 1, colW, "F4", "Переключение FPS лимита", sf::Color(110, 240, 130));
        drawKeybindRow(window, col2X, startRowY + rowH * 2, colW, "F5", "Перезагрузка баланса и комнаты");
        drawKeybindRow(window, col2X, startRowY + rowH * 3, colW, "F6", "Вкл / Выкл редактор комнат", sf::Color(255, 180, 80));
        drawKeybindRow(window, col2X, startRowY + rowH * 4, colW, "Backspace", "Сброс арены и респавн врагов");
        drawKeybindRow(window, col2X, startRowY + rowH * 5, colW, "Tab", "Режим ИИ (Манекен / Умный ИИ)");
        drawKeybindRow(window, col2X, startRowY + rowH * 6, colW, "Y", "Смена архетипа (Normal/Tank/Duelist)");
        drawKeybindRow(window, col2X, startRowY + rowH * 7, colW, "PageUp / Down", "Предыдущая / следующая комната");
        drawKeybindRow(window, col2X, startRowY + rowH * 8, colW, "T", "Принудительная атака / комбо бота");
        drawKeybindRow(window, col2X, startRowY + rowH * 9, colW, "Мышь", "Прицеливание броска булавы");
    } else if (_settingsTab == SettingsTab::SOUND) {
        if (_font) {
            sf::Text subTitle(*_font, toSfString("УПРАВЛЕНИЕ ГРОМКОСТЬЮ ЗВУКОВ И ЭФФЕКТОВ"), 18);
            subTitle.setStyle(sf::Text::Bold);
            subTitle.setFillColor(sf::Color(140, 190, 255));
            subTitle.setPosition({contentX + 30.0f, contentY + 30.0f});
            window.draw(subTitle);

            std::string volStr = _isMuted ? "ВЫКЛЮЧЕН (MUTE)" : (std::to_string(static_cast<int>(_currentVolume)) + " %");
            sf::Text volText(*_font, toSfString("Общая громкость (Master Volume):  " + volStr), 20);
            volText.setFillColor(_isMuted ? sf::Color(255, 90, 90) : sf::Color(240, 245, 255));
            volText.setPosition({contentX + 30.0f, contentY + 75.0f});
            window.draw(volText);
        }

        // Кнопки регулировки громкости
        float by = contentY + 130.0f;
        Button btnVolDown{{{contentX + 30.0f, by}, {120.0f, 44.0f}}, "[ - 10% ]",
                          sf::Color(35, 55, 80), sf::Color(55, 90, 135), sf::Color(90, 140, 210), 16};
        if (drawButton(window, btnVolDown, mousePos) && justClicked) {
            _isMuted = false;
            _currentVolume = std::max(0.0f, _currentVolume - 10.0f);
            SoundManager::get().setMasterVolume(_currentVolume);
            SoundManager::get().play("1", 40.0f);
        }

        Button btnVolUp{{{contentX + 170.0f, by}, {120.0f, 44.0f}}, "[ + 10% ]",
                        sf::Color(35, 55, 80), sf::Color(55, 90, 135), sf::Color(90, 140, 210), 16};
        if (drawButton(window, btnVolUp, mousePos) && justClicked) {
            _isMuted = false;
            _currentVolume = std::min(100.0f, _currentVolume + 10.0f);
            SoundManager::get().setMasterVolume(_currentVolume);
            SoundManager::get().play("1", 40.0f);
        }

        Button btnMute{{{contentX + 310.0f, by}, {240.0f, 44.0f}}, 
                       _isMuted ? "ВКЛЮЧИТЬ ЗВУК" : "ВЫКЛЮЧИТЬ ЗВУК (MUTE)",
                       _isMuted ? sf::Color(45, 95, 55) : sf::Color(95, 35, 45), 
                       _isMuted ? sf::Color(70, 155, 85) : sf::Color(155, 55, 70), 
                       _isMuted ? sf::Color(110, 220, 130) : sf::Color(220, 90, 110), 15};
        if (drawButton(window, btnMute, mousePos) && justClicked) {
            if (_isMuted) {
                _isMuted = false;
                _currentVolume = (_preMuteVolume > 0.0f) ? _preMuteVolume : 50.0f;
                SoundManager::get().setMasterVolume(_currentVolume);
                SoundManager::get().play("1", 40.0f);
            } else {
                _isMuted = true;
                _preMuteVolume = _currentVolume;
                _currentVolume = 0.0f;
                SoundManager::get().setMasterVolume(0.0f);
            }
        }

        if (_font) {
            sf::Text tip(*_font, toSfString("• При изменении громкости воспроизводится тестовый удар.\n"
                                           "• Звуки применяются ко всем комбо, ударам булавы, парированию и шагам."), 14);
            tip.setFillColor(sf::Color(165, 185, 210));
            tip.setPosition({contentX + 30.0f, by + 85.0f});
            window.draw(tip);
        }
    } else if (_settingsTab == SettingsTab::FPS) {
        if (_font) {
            sf::Text subTitle(*_font, toSfString("РЕЖИМЫ ОГРАНИЧЕНИЯ КАДРОВОЙ ЧАСТОТЫ (FPS)"), 18);
            subTitle.setStyle(sf::Text::Bold);
            subTitle.setFillColor(sf::Color(140, 190, 255));
            subTitle.setPosition({contentX + 30.0f, contentY + 25.0f});
            window.draw(subTitle);
        }

        float by1 = contentY + 70.0f;
        float bw = 220.0f;
        float bh = 42.0f;

        auto checkMode = [&](FpsLimitMode m, const std::string& lbl, float x, float y) {
            bool isCurrent = (_fpsMode == m);
            Button btn{{{x, y}, {bw, bh}}, lbl,
                       isCurrent ? sf::Color(45, 105, 75) : sf::Color(28, 42, 65),
                       isCurrent ? sf::Color(65, 160, 110) : sf::Color(48, 75, 115),
                       isCurrent ? sf::Color(110, 240, 165) : sf::Color(80, 125, 185), 15};
            if (drawButton(window, btn, mousePos, isCurrent) && justClicked) {
                _fpsMode = m;
                if (_onFpsModeChanged) _onFpsModeChanged(m);
                SoundManager::get().play("1", 30.0f);
            }
        };

        checkMode(FpsLimitMode::UNCAPPED, "MAX (БЕЗ ЛИМИТА)", contentX + 30.0f, by1);
        checkMode(FpsLimitMode::FPS_200,   "200 FPS (МОНИТОР)", contentX + 270.0f, by1);
        checkMode(FpsLimitMode::FPS_240,   "240 FPS",          contentX + 510.0f, by1);

        float by2 = by1 + 54.0f;
        checkMode(FpsLimitMode::FPS_144,   "144 FPS",          contentX + 30.0f, by2);
        checkMode(FpsLimitMode::FPS_60,    "60 FPS",           contentX + 270.0f, by2);
        checkMode(FpsLimitMode::VSYNC,     "VSYNC (СИНХРОНИЗАЦИЯ)", contentX + 510.0f, by2);

        float by3 = by2 + 75.0f;
        Button btnToggleCounter{{{contentX + 30.0f, by3}, {380.0f, 44.0f}},
                                _showFpsCounter ? "СЧЕТЧИК FPS: [ ВКЛЮЧЕН ]" : "СЧЕТЧИК FPS: [ ВЫКЛЮЧЕН ]",
                                _showFpsCounter ? sf::Color(35, 80, 55) : sf::Color(65, 35, 45),
                                _showFpsCounter ? sf::Color(55, 125, 85) : sf::Color(105, 55, 70),
                                _showFpsCounter ? sf::Color(90, 210, 140) : sf::Color(175, 90, 110), 16};
        if (drawButton(window, btnToggleCounter, mousePos) && justClicked) {
            _showFpsCounter = !_showFpsCounter;
            SoundManager::get().play("1", 30.0f);
        }

        if (_font) {
            sf::Text tip(*_font, toSfString("• Переключать режим FPS также можно прямо во время игры по горячей клавише F4.\n"
                                           "• Быстро скрыть / показать счетчик FPS можно клавишей F3."), 14);
            tip.setFillColor(sf::Color(165, 185, 210));
            tip.setPosition({contentX + 30.0f, by3 + 60.0f});
            window.draw(tip);
        }
    }

    // Кнопка НАЗАД внизу
    float backW = 200.0f;
    float backH = 44.0f;
    Button btnBack{{{cx - backW * 0.5f, topY + panelH - 62.0f}, {backW, backH}}, "НАЗАД",
                   sf::Color(35, 50, 75), sf::Color(55, 85, 130), sf::Color(95, 140, 205), 18};
    if (drawButton(window, btnBack, mousePos) && justClicked) {
        _currentScreen = MenuScreen::MAIN;
        SoundManager::get().play("1", 30.0f);
    }
}

void MenuSystem::drawCombosTab(sf::RenderWindow& window, float x, float y, float w, float h) {
    if (!_font) return;

    sf::Text hint(*_font, toSfString("ОБОЗНАЧЕНИЯ: 1 = ЛКМ (Легкий удар), 2 = ПКМ (Тяжелый удар), f = Вперед (к врагу), b = Назад, d = Вниз (S), a = В воздухе"), 12);
    hint.setFillColor(sf::Color(140, 195, 255));
    hint.setPosition({x + 12.0f, y + 8.0f});
    window.draw(hint);

    struct ComboGuideItem {
        std::string inputs;
        std::string name;
        std::string desc;
        sf::Color col;
    };

    std::vector<ComboGuideItem> items = {
        {"1 -> 1 -> 1", "Triple Light Strike", "Быстрая базовая серия легких ударов с небольшим отталкиванием", sf::Color(255, 240, 90)},
        {"2 -> 2 -> 2", "Triple Heavy Strike", "Мощная серия сильных ударов с тяжелым отбрасыванием врага", sf::Color(255, 110, 50)},
        {"1 -> f2 -> d1", "Diagonal Launcher", "Удар -> выпад вперед -> мощный апперкот, подбрасывающий врага высоко в воздух", sf::Color(255, 80, 80)},
        {"1 -> 1 -> b2", "Grab & Back Throw", "Два удара -> резкий захват и бросок врага назад через себя с разворотом", sf::Color(255, 95, 140)},
        {"1 -> 1 -> 2", "Rapid Combo Launcher", "Два быстрых джеба и вертикальный подброс врага в воздух для джаггла", sf::Color(255, 120, 80)},
        {"2 -> 1 -> db1", "Reverse Sweep Launcher", "Тяжелый замах -> отмена в рывок за спину -> подсечка и вертикальный запуск", sf::Color(255, 140, 80)},
        {"a1 -> adw2", "Ground Bounce Slam", "Удар в воздухе -> стремительный дайв-слэм, вбивающий врага в пол с отскоком", sf::Color(255, 90, 30)},
        {"a1 -> a1 -> adw2", "Dual Air Dive Slam", "Двойное жонглирование в воздухе и финишный слэм об пол", sf::Color(255, 75, 25)},
        {"a1 -> a2", "Horizontal Air Push", "Удар в воздухе и сильный горизонтальный толчок на дистанцию", sf::Color(220, 130, 255)},
        {"1 -> 2 -> df2", "Horizontal Dash Smash", "Джеб -> силовой удар -> стремительный рывок вперед и мощный смеш", sf::Color(255, 180, 60)}
    };

    float startRowY = y + 30.0f;
    float rowH = 34.0f;

    for (size_t i = 0; i < items.size() && i < 10; ++i) {
        float ry = startRowY + i * rowH;

        sf::RectangleShape rowBg({w - 20.0f, rowH - 4.0f});
        rowBg.setPosition({x + 10.0f, ry});
        rowBg.setFillColor((i % 2 == 0) ? sf::Color(18, 24, 35, 190) : sf::Color(24, 32, 48, 190));
        rowBg.setOutlineThickness(1.0f);
        rowBg.setOutlineColor(sf::Color(45, 60, 85, 160));
        window.draw(rowBg);

        // Инпуты комбо
        sf::Text txtIn(*_font, toSfString(items[i].inputs), 14);
        txtIn.setStyle(sf::Text::Bold);
        txtIn.setFillColor(items[i].col);
        txtIn.setPosition({x + 18.0f, ry + 4.0f});
        window.draw(txtIn);

        // Имя комбо
        sf::Text txtName(*_font, toSfString(items[i].name), 13);
        txtName.setStyle(sf::Text::Bold);
        txtName.setFillColor(sf::Color(240, 245, 255));
        txtName.setPosition({x + 180.0f, ry + 5.0f});
        window.draw(txtName);

        // Описание эффекта
        sf::Text txtDesc(*_font, toSfString(items[i].desc), 12);
        txtDesc.setFillColor(sf::Color(180, 195, 215));
        txtDesc.setPosition({x + 370.0f, ry + 6.0f});
        window.draw(txtDesc);
    }

    // Анти-спам предупреждение
    float noteY = startRowY + 10 * rowH + 4.0f;
    sf::Text antiSpam(*_font, toSfString("★ АНТИ-СПАМ: Повтор одного и того же комбо дважды за 3 сек наказывается станом игрока на 0.75с! Вариируйте серии!"), 12);
    antiSpam.setStyle(sf::Text::Bold);
    antiSpam.setFillColor(sf::Color(255, 185, 60));
    antiSpam.setPosition({x + 14.0f, noteY});
    window.draw(antiSpam);
}

void MenuSystem::drawMechanicsTab(sf::RenderWindow& window, float x, float y, float w, float h) {
    if (!_font) return;

    struct MechItem {
        std::string title;
        std::string desc;
        sf::Color col;
    };

    std::vector<MechItem> items = {
        {"БЛОК ЩИТОМ [F / E]", "Снижает весь входящий урон на 80%. Если враг наносит финишный удар серии по блоку, он сам оглушается и прерывает комбо!", sf::Color(80, 210, 255)},
        {"ПАРИРОВАНИЕ [Q]", "Стойка идеального парирования (0.35с). Попадание врага в этот тайминг блокирует атаку, оглушает бота и наносит встречный критический урон!", sf::Color(0, 240, 255)},
        {"ИДЕАЛЬНЫЙ УВОРОТ [SHIFT]", "Если нажать рывок прямо перед попаданием вражеского удара, персонаж совершает идеальный додж: золотая ударная волна оглушает врага, а следующий удар наносит повышенный урон!", sf::Color(255, 220, 60)},
        {"ЖОНГЛИРОВАНИЕ В ВОЗДУХЕ (JUGGLE)", "Враги, запущенные в воздух (Лаунчерами 1f2d1, 112, 21db1), парят с пониженной гравитацией, позволяя проводить связки в воздухе (a1, a2, adw2).", sf::Color(140, 180, 255)},
        {"МЕТАНИЕ БУЛАВЫ [R]", "8-направленное метание оружия по позиции курсора мыши. Скорость полета растет от скорости бега игрока. Без булавы активируется рукопашный бой со всеми комбо!", sf::Color(255, 140, 70)},
        {"ОТСКОКИ ОТ ПОЛА И СТЕН (BOUNCES)", "Удары дайв-слэма (adw2) вбивают врага в пол с упругим отскоком вверх (Ground Bounce), открывая возможность продолжить серию комбо.", sf::Color(255, 110, 90)},
        {"СКЕЙЛИНГ УРОНА (PRORATION)", "Каждый последующий удар в непрерывной комбо-серии наносит на 10% меньше урона (вплоть до минимума в 30%), поощряя быстрые и продуманные связки.", sf::Color(200, 220, 240)}
    };

    float startRowY = y + 10.0f;
    float cardH = 50.0f;

    for (size_t i = 0; i < items.size(); ++i) {
        float cy = startRowY + i * (cardH + 4.0f);

        sf::RectangleShape cardBg({w - 20.0f, cardH});
        cardBg.setPosition({x + 10.0f, cy});
        cardBg.setFillColor(sf::Color(18, 24, 36, 210));
        cardBg.setOutlineThickness(1.0f);
        cardBg.setOutlineColor(sf::Color(45, 60, 85, 180));
        window.draw(cardBg);

        sf::Text txtTitle(*_font, toSfString(items[i].title), 14);
        txtTitle.setStyle(sf::Text::Bold);
        txtTitle.setFillColor(items[i].col);
        txtTitle.setPosition({x + 20.0f, cy + 4.0f});
        window.draw(txtTitle);

        sf::Text txtDesc(*_font, toSfString(items[i].desc), 12);
        txtDesc.setFillColor(sf::Color(200, 215, 235));
        txtDesc.setPosition({x + 20.0f, cy + 24.0f});
        window.draw(txtDesc);
    }
}

void MenuSystem::drawEnemiesTab(sf::RenderWindow& window, float x, float y, float w, float h) {
    if (!_font) return;

    struct EnemyItem {
        std::string name;
        std::string hpSpeed;
        std::string desc;
        sf::Color col;
    };

    std::vector<EnemyItem> items = {
        {"АРХЕТИП 1: NORMAL (ОБЫЧНЫЙ БОЕЦ)", "HP: 45 | Скорость: Средняя | Броня: Легкая",
         "Серый силуэт. Базовый противник со стандартным набором одиночных ударов и средним радиусом обзора. Отличная цель для отработки базовых таймингов и комбо-связок.",
         sf::Color(170, 180, 195)},
        {"АРХЕТИП 2: TANK (ТЯЖЕЛЫЙ БРОНЕНОСЕЦ)", "HP: 110 | Скорость: Медленная | Броня: Тяжелая (В 1.5 раза крупнее)",
         "Красный силуэт. Обладает увеличенным запасом здоровья и высокой устойчивостью к оглушению. Наносит сокрушительный урон с большой дистанции замаха.",
         sf::Color(255, 80, 80)},
        {"АРХЕТИП 3: DUELIST (ДУЭЛЯНТ-МАСТЕР)", "HP: 65 | Скорость: Очень высокая | Броня: Средняя",
         "Фиолетовый силуэт. Наиболее опасный и умный противник: ставит энергощит, парирует ваши атаки лазурной стойкой, совершает идеальные доджи с золотой волной и проводит полноценные комбо!",
         sf::Color(215, 110, 255)}
    };

    float startY = y + 12.0f;
    float cardH = 92.0f;

    for (size_t i = 0; i < items.size(); ++i) {
        float cy = startY + i * (cardH + 8.0f);

        sf::RectangleShape cardBg({w - 20.0f, cardH});
        cardBg.setPosition({x + 10.0f, cy});
        cardBg.setFillColor(sf::Color(18, 24, 36, 210));
        cardBg.setOutlineThickness(1.5f);
        cardBg.setOutlineColor(items[i].col);
        window.draw(cardBg);

        sf::Text txtName(*_font, toSfString(items[i].name), 15);
        txtName.setStyle(sf::Text::Bold);
        txtName.setFillColor(items[i].col);
        txtName.setPosition({x + 22.0f, cy + 8.0f});
        window.draw(txtName);

        sf::Text txtStats(*_font, toSfString(items[i].hpSpeed), 12);
        txtStats.setFillColor(sf::Color(255, 220, 120));
        txtStats.setPosition({x + 22.0f, cy + 30.0f});
        window.draw(txtStats);

        sf::Text txtDesc(*_font, toSfString(items[i].desc), 12);
        txtDesc.setFillColor(sf::Color(205, 220, 240));
        txtDesc.setPosition({x + 22.0f, cy + 50.0f});
        window.draw(txtDesc);
    }

    // Состояния восприятия ИИ
    float aiInfoY = startY + 3 * (cardH + 8.0f);
    sf::Text aiInfo(*_font, toSfString("ПОВЕДЕНИЕ ИИ: [Лазурный глаз] Патруль платформы -> [Желтый маркер !] Замечает игрока -> [Красный визор] Погоня и комбо-атака"), 12);
    aiInfo.setStyle(sf::Text::Bold);
    aiInfo.setFillColor(sf::Color(80, 220, 255));
    aiInfo.setPosition({x + 14.0f, aiInfoY + 6.0f});
    window.draw(aiInfo);
}

void MenuSystem::drawGuideScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize) {
    float cx = winSize.x * 0.5f;
    float cy = winSize.y * 0.5f;

    float panelW = 960.0f;
    float panelH = 630.0f;
    sf::RectangleShape panel({panelW, panelH});
    panel.setOrigin({panelW * 0.5f, panelH * 0.5f});
    panel.setPosition({cx, cy});
    panel.setFillColor(sf::Color(15, 20, 30, 248));
    panel.setOutlineThickness(3.0f);
    panel.setOutlineColor(sf::Color(65, 110, 180, 230));
    window.draw(panel);

    float leftX = cx - panelW * 0.5f;
    float topY = cy - panelH * 0.5f;

    if (_font) {
        sf::Text title(*_font, toSfString("ГАЙД ПО ИГРЕ: КОМБО, МЕХАНИКИ И ВРАГИ"), 28);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(245, 250, 255));
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        title.setPosition({cx, topY + 42.0f});
        window.draw(title);
    }

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    static bool prevMouse = false;
    bool justClicked = mouseClicked && !prevMouse;
    prevMouse = mouseClicked;

    // Вкладки гайда
    float tabW = 260.0f;
    float tabH = 38.0f;
    float tabY = topY + 74.0f;
    float tabsStartX = cx - (tabW * 3.0f + 20.0f) * 0.5f;

    Button btnTabCombos{{{tabsStartX, tabY}, {tabW, tabH}}, "1. КОМБО-ЦЕПОЧКИ",
                       sf::Color(25, 38, 55), sf::Color(45, 80, 125), sf::Color(80, 130, 195), 15};
    if (drawButton(window, btnTabCombos, mousePos, _guideTab == GuideTab::COMBOS) && justClicked) {
        _guideTab = GuideTab::COMBOS;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabMech{{{tabsStartX + tabW + 10.0f, tabY}, {tabW, tabH}}, "2. МЕХАНИКИ БОЯ",
                     sf::Color(25, 38, 55), sf::Color(45, 80, 125), sf::Color(80, 130, 195), 15};
    if (drawButton(window, btnTabMech, mousePos, _guideTab == GuideTab::MECHANICS) && justClicked) {
        _guideTab = GuideTab::MECHANICS;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabEnemies{{{tabsStartX + (tabW + 10.0f) * 2.0f, tabY}, {tabW, tabH}}, "3. АРХЕТИПЫ ВРАГОВ",
                        sf::Color(25, 38, 55), sf::Color(45, 80, 125), sf::Color(80, 130, 195), 15};
    if (drawButton(window, btnTabEnemies, mousePos, _guideTab == GuideTab::ENEMIES) && justClicked) {
        _guideTab = GuideTab::ENEMIES;
        SoundManager::get().play("1", 30.0f);
    }

    // Контент вкладки
    float contentX = leftX + 30.0f;
    float contentY = topY + 124.0f;
    float contentW = panelW - 60.0f;
    float contentH = 430.0f;

    sf::RectangleShape contentBox({contentW, contentH});
    contentBox.setPosition({contentX, contentY});
    contentBox.setFillColor(sf::Color(12, 16, 24, 210));
    contentBox.setOutlineThickness(1.5f);
    contentBox.setOutlineColor(sf::Color(40, 55, 80, 200));
    window.draw(contentBox);

    if (_guideTab == GuideTab::COMBOS) {
        drawCombosTab(window, contentX, contentY, contentW, contentH);
    } else if (_guideTab == GuideTab::MECHANICS) {
        drawMechanicsTab(window, contentX, contentY, contentW, contentH);
    } else if (_guideTab == GuideTab::ENEMIES) {
        drawEnemiesTab(window, contentX, contentY, contentW, contentH);
    }

    // Кнопка НАЗАД внизу
    float backW = 200.0f;
    float backH = 42.0f;
    Button btnBack{{{cx - backW * 0.5f, topY + panelH - 58.0f}, {backW, backH}}, "НАЗАД",
                   sf::Color(35, 50, 75), sf::Color(55, 85, 130), sf::Color(95, 140, 205), 18};
    if (drawButton(window, btnBack, mousePos) && justClicked) {
        _currentScreen = MenuScreen::MAIN;
        SoundManager::get().play("1", 30.0f);
    }
}

void MenuSystem::draw(sf::RenderWindow& window, const sf::Vector2f& mousePos) {
    if (!isInMenu()) return;

    sf::Vector2f winSize(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));

    // Общий полупрозрачный фон оверлея
    sf::RectangleShape overlay(winSize);
    overlay.setFillColor(sf::Color(8, 12, 18, 225));
    window.draw(overlay);

    if (_currentScreen == MenuScreen::MAIN) {
        drawMainMenuScreen(window, mousePos, winSize);
    } else if (_currentScreen == MenuScreen::SETTINGS) {
        drawSettingsScreen(window, mousePos, winSize);
    } else if (_currentScreen == MenuScreen::GUIDE) {
        drawGuideScreen(window, mousePos, winSize);
    }
}
