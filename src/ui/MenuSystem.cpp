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

    sf::Color fillCol = selected ? sf::Color(btn.hoverColor.r, btn.hoverColor.g, btn.hoverColor.b, 255)
                                 : (hovered ? btn.hoverColor : btn.normalColor);
    shape.setFillColor(fillCol);
    shape.setOutlineThickness(hovered || selected ? 2.5f : 1.5f);
    shape.setOutlineColor(hovered || selected ? sf::Color::White : btn.outlineColor);
    window.draw(shape);

    if (_font) {
        sf::Text text(*_font, toSfString(btn.label), btn.fontSize);
        text.setFillColor(sf::Color::White);
        sf::FloatRect tb = text.getLocalBounds();
        text.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        text.setPosition(sf::Vector2f(btn.rect.position.x + btn.rect.size.x * 0.5f,
                                     btn.rect.position.y + btn.rect.size.y * 0.5f));
        window.draw(text);
    }

    return hovered;
}

void MenuSystem::handleEvent(const sf::Event& event, const sf::Vector2f& mousePos) {
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

    // Фон строки - темно-серый
    sf::RectangleShape rowBg({w, 32.0f});
    rowBg.setPosition(sf::Vector2f(x, y));
    rowBg.setFillColor(sf::Color(20, 20, 24, 255));
    rowBg.setOutlineThickness(1.0f);
    rowBg.setOutlineColor(sf::Color(45, 45, 52));
    window.draw(rowBg);

    // Клавиша - чистый белый текст
    sf::Text keyText(*_font, toSfString(key), 15);
    keyText.setStyle(sf::Text::Bold);
    keyText.setFillColor(sf::Color::White);
    keyText.setPosition(sf::Vector2f(x + 16.0f, y + 5.0f));
    window.draw(keyText);

    // Разделитель
    sf::RectangleShape div({2.0f, 22.0f});
    div.setPosition(sf::Vector2f(x + 225.0f, y + 5.0f));
    div.setFillColor(sf::Color(45, 45, 52));
    window.draw(div);

    // Описание - чистый белый текст
    sf::Text descText(*_font, toSfString(desc), 14);
    descText.setFillColor(sf::Color::White);
    descText.setPosition(sf::Vector2f(x + 245.0f, y + 6.0f));
    window.draw(descText);
}

void MenuSystem::drawMainMenuScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize) {
    float cx = winSize.x * 0.5f;
    float cy = winSize.y * 0.5f;

    // Заливка всего экрана глубоким черным тоном
    sf::RectangleShape bgFill(winSize);
    bgFill.setFillColor(sf::Color(10, 10, 12, 255));
    window.draw(bgFill);

    // Верхняя акцентная полоса в стильных темно-серых тонах
    sf::RectangleShape topGlow({winSize.x, 3.0f});
    topGlow.setPosition(sf::Vector2f(0.0f, 0.0f));
    topGlow.setFillColor(sf::Color(45, 45, 52));
    window.draw(topGlow);

    if (_font) {
        // Большой заголовок игры - чистый белый цвет
        std::string titleStr = (_appState == AppState::PAUSED) ? "ПАУЗА" : "FIGHTING COMBO ARENA";
        sf::Text title(*_font, toSfString(titleStr), (_appState == AppState::PAUSED) ? 48 : 44);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color::White);
        title.setOutlineThickness(3.0f);
        title.setOutlineColor(sf::Color(15, 15, 18));
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        title.setPosition(sf::Vector2f(cx, cy - 210.0f));
        window.draw(title);

        // Подзаголовок - чистый белый текст
        std::string subStr = (_appState == AppState::PAUSED) ? "ИГРА ПРИОСТАНОВЛЕНА  |  НАЖМИТЕ [ESC] ДЛЯ ВОЗВРАТА" 
                                                             : "ХАРДКОРНЫЙ 2D КОМБО-ФАЙТИНГ НА ДВИЖКЕ SFML 3";
        sf::Text sub(*_font, toSfString(subStr), 15);
        sub.setFillColor(sf::Color::White);
        sf::FloatRect sb = sub.getLocalBounds();
        sub.setOrigin({sb.position.x + sb.size.x * 0.5f, sb.position.y + sb.size.y * 0.5f});
        sub.setPosition(sf::Vector2f(cx, cy - 155.0f));
        window.draw(sub);
    }

    float btnW = 380.0f;
    float btnH = 50.0f;
    float startY = cy - 120.0f;
    float stepY = 60.0f;

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    static bool prevMouse = false;
    bool justClicked = mouseClicked && !prevMouse;
    prevMouse = mouseClicked;

    if (_appState == AppState::MAIN_MENU) {
        // 1. ИГРАТЬ (Обычная игра)
        Button btnPlay{{{cx - btnW * 0.5f, startY}, {btnW, btnH}}, "ИГРАТЬ",
                       sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 22};
        if (drawButton(window, btnPlay, mousePos) && justClicked) {
            SoundManager::get().play("1", 40.0f);
            _isTrainingMode = false;
            if (_onStartGame) _onStartGame();
            _appState = AppState::IN_GAME;
        }

        // 2. ТРЕНИРОВКА (Режим с бессмертным манекеном и комбо-счетчиком)
        Button btnTrain{{{cx - btnW * 0.5f, startY + stepY}, {btnW, btnH}}, "ТРЕНИРОВКА (МАНЕКЕН)",
                        sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 21};
        if (drawButton(window, btnTrain, mousePos) && justClicked) {
            SoundManager::get().play("1", 40.0f);
            _isTrainingMode = true;
            if (_onStartTraining) _onStartTraining();
            _appState = AppState::IN_GAME;
        }

        // 3. НАСТРОЙКИ
        Button btnSettings{{{cx - btnW * 0.5f, startY + stepY * 2.0f}, {btnW, btnH}}, "НАСТРОЙКИ",
                           sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 20};
        if (drawButton(window, btnSettings, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::SETTINGS;
        }

        // 4. ГАЙД
        Button btnGuide{{{cx - btnW * 0.5f, startY + stepY * 3.0f}, {btnW, btnH}}, "ГАЙД ПО ИГРЕ",
                        sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 20};
        if (drawButton(window, btnGuide, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::GUIDE;
        }

        // 5. ВЫХОД
        Button btnExit{{{cx - btnW * 0.5f, startY + stepY * 4.0f}, {btnW, btnH}}, "ВЫХОД",
                       sf::Color(32, 22, 24), sf::Color(55, 30, 34), sf::Color(110, 55, 65), 20};
        if (drawButton(window, btnExit, mousePos) && justClicked) {
            if (_onExitGame) _onExitGame();
        }
    } else {
        // PAUSED STATE
        Button btnResume{{{cx - btnW * 0.5f, startY}, {btnW, btnH}}, "ПРОДОЛЖИТЬ [ESC]",
                         sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 21};
        if (drawButton(window, btnResume, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            resumeGame();
        }

        std::string modeToggleLabel = _isTrainingMode ? "ВЫЙТИ ИЗ ТРЕНИРОВКИ" : "РЕЖИМ ТРЕНИРОВКИ";
        Button btnSwitchMode{{{cx - btnW * 0.5f, startY + stepY}, {btnW, btnH}}, modeToggleLabel,
                             sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 19};
        if (drawButton(window, btnSwitchMode, mousePos) && justClicked) {
            SoundManager::get().play("1", 40.0f);
            if (_isTrainingMode) {
                _isTrainingMode = false;
                if (_onStartGame) _onStartGame();
            } else {
                _isTrainingMode = true;
                if (_onStartTraining) _onStartTraining();
            }
            resumeGame();
        }

        Button btnSettings{{{cx - btnW * 0.5f, startY + stepY * 2.0f}, {btnW, btnH}}, "НАСТРОЙКИ",
                           sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 20};
        if (drawButton(window, btnSettings, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::SETTINGS;
        }

        Button btnGuide{{{cx - btnW * 0.5f, startY + stepY * 3.0f}, {btnW, btnH}}, "ГАЙД ПО ИГРЕ",
                        sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 20};
        if (drawButton(window, btnGuide, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            _currentScreen = MenuScreen::GUIDE;
        }

        Button btnToMenu{{{cx - btnW * 0.5f, startY + stepY * 4.0f}, {btnW, btnH}}, "В ГЛАВНОЕ МЕНЮ",
                         sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 19};
        if (drawButton(window, btnToMenu, mousePos) && justClicked) {
            SoundManager::get().play("1", 35.0f);
            if (_onResetGame) _onResetGame();
            openMainMenu();
        }

        Button btnExit{{{cx - btnW * 0.5f, startY + stepY * 5.0f}, {btnW, btnH}}, "ВЫЙТИ ИЗ ИГРЫ",
                       sf::Color(32, 22, 24), sf::Color(55, 30, 34), sf::Color(110, 55, 65), 19};
        if (drawButton(window, btnExit, mousePos) && justClicked) {
            if (_onExitGame) _onExitGame();
        }
    }
}

void MenuSystem::drawSettingsScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize) {
    float cx = winSize.x * 0.5f;

    // Сплошная заливка на весь экран
    sf::RectangleShape bgFill(winSize);
    bgFill.setFillColor(sf::Color(10, 10, 12));
    window.draw(bgFill);

    // Верхняя плашка шапки
    float topH = 75.0f;
    sf::RectangleShape topBar({winSize.x, topH});
    topBar.setPosition(sf::Vector2f(0.0f, 0.0f));
    topBar.setFillColor(sf::Color(18, 18, 22));
    topBar.setOutlineThickness(1.5f);
    topBar.setOutlineColor(sf::Color(45, 45, 52));
    window.draw(topBar);

    if (_font) {
        sf::Text title(*_font, toSfString("НАСТРОЙКИ"), 30);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color::White);
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        title.setPosition(sf::Vector2f(cx, topH * 0.5f));
        window.draw(title);
    }

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    static bool prevMouse = false;
    bool justClicked = mouseClicked && !prevMouse;
    prevMouse = mouseClicked;

    // Вкладки настроек
    float tabW = 280.0f;
    float tabH = 42.0f;
    float tabY = topH + 16.0f;
    float tabsStartX = cx - (tabW * 3.0f + 24.0f) * 0.5f;

    Button btnTabBinds{{{tabsStartX, tabY}, {tabW, tabH}}, "БИНДЫ (УПРАВЛЕНИЕ)",
                      sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 16};
    if (drawButton(window, btnTabBinds, mousePos, _settingsTab == SettingsTab::BINDS) && justClicked) {
        _settingsTab = SettingsTab::BINDS;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabSound{{{tabsStartX + tabW + 12.0f, tabY}, {tabW, tabH}}, "ЗВУК",
                      sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 16};
    if (drawButton(window, btnTabSound, mousePos, _settingsTab == SettingsTab::SOUND) && justClicked) {
        _settingsTab = SettingsTab::SOUND;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabFps{{{tabsStartX + (tabW + 12.0f) * 2.0f, tabY}, {tabW, tabH}}, "ФПС И ГРАФИКА",
                    sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 16};
    if (drawButton(window, btnTabFps, mousePos, _settingsTab == SettingsTab::FPS) && justClicked) {
        _settingsTab = SettingsTab::FPS;
        SoundManager::get().play("1", 30.0f);
    }

    // Рабочая область на весь экран с комфортными отступами
    float marginX = 80.0f;
    float contentX = marginX;
    float contentY = tabY + tabH + 20.0f;
    float contentW = winSize.x - marginX * 2.0f;
    float bottomBarH = 80.0f;
    float contentH = winSize.y - contentY - bottomBarH - 10.0f;

    sf::RectangleShape contentBox({contentW, contentH});
    contentBox.setPosition(sf::Vector2f(contentX, contentY));
    contentBox.setFillColor(sf::Color(14, 14, 16, 255));
    contentBox.setOutlineThickness(1.5f);
    contentBox.setOutlineColor(sf::Color(40, 40, 46));
    window.draw(contentBox);

    if (_settingsTab == SettingsTab::BINDS) {
        // Две широкие колонки на весь экран — никакого урезания текста!
        float colGap = 40.0f;
        float colW = (contentW - 60.0f - colGap) * 0.5f;
        float rowH = 36.0f;
        float startRowY = contentY + 20.0f;

        // Колонка 1: Боевое управление
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 0, colW, "A / D", "Движение влево / вправо");
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 1, colW, "Space / W", "Прыжок (направление + прыжок)");
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 2, colW, "ЛКМ (1)", "Легкий удар (Combo Starter)");
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 3, colW, "ПКМ (2)", "Тяжелый удар (Knockback)");
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 4, colW, "F или E", "Блок щитом (-80% входящего урона)");
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 5, colW, "Q", "Парирование / Контратака", sf::Color(0, 240, 255));
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 6, colW, "Left Shift", "Рывок / Уворот (кадры неуязвимости)", sf::Color(255, 220, 80));
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 7, colW, "R / СКМ (Mouse 3)", "Бросок булавы по курсору / Подбор", sf::Color(255, 140, 70));
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 8, colW, "S / Down", "Ускоренный спуск в воздухе");
        drawKeybindRow(window, contentX + 25.0f, startRowY + rowH * 9, colW, "ESC", "Пауза / Главное меню");

        // Колонка 2: Системные и режимы
        float col2X = contentX + 25.0f + colW + colGap;
        drawKeybindRow(window, col2X, startRowY + rowH * 0, colW, "F3", "Вкл / Выкл счетчик FPS", sf::Color(110, 240, 130));
        drawKeybindRow(window, col2X, startRowY + rowH * 1, colW, "F4", "Переключение FPS лимита", sf::Color(110, 240, 130));
        drawKeybindRow(window, col2X, startRowY + rowH * 2, colW, "F5", "Перезагрузка баланса и комнаты");
        drawKeybindRow(window, col2X, startRowY + rowH * 3, colW, "F6", "Вкл / Выкл редактор комнат", sf::Color(255, 180, 80));
        drawKeybindRow(window, col2X, startRowY + rowH * 4, colW, "Backspace", "Сброс арены и респавн врагов");
        drawKeybindRow(window, col2X, startRowY + rowH * 5, colW, "Tab", "Режим ИИ (Манекен / Умный ИИ)");
        drawKeybindRow(window, col2X, startRowY + rowH * 6, colW, "Y", "Смена архетипа (Norm/Tank/D-Agr/D-Pas)");
        drawKeybindRow(window, col2X, startRowY + rowH * 7, colW, "PageUp / Down", "Предыдущая / следующая комната");
        drawKeybindRow(window, col2X, startRowY + rowH * 8, colW, "T", "Принудительная атака / комбо бота");
        drawKeybindRow(window, col2X, startRowY + rowH * 9, colW, "Мышь", "Прицеливание броска булавы");
    } else if (_settingsTab == SettingsTab::SOUND) {
        if (_font) {
            sf::Text subTitle(*_font, toSfString("УПРАВЛЕНИЕ ГРОМКОСТЬЮ ЗВУКОВЫХ ЭФФЕКТОВ"), 22);
            subTitle.setStyle(sf::Text::Bold);
            subTitle.setFillColor(sf::Color::White);
            subTitle.setPosition(sf::Vector2f(contentX + 50.0f, contentY + 45.0f));
            window.draw(subTitle);

            std::string volStr = _isMuted ? "ВЫКЛЮЧЕН (MUTE)" : (std::to_string(static_cast<int>(_currentVolume)) + " %");
            sf::Text volText(*_font, toSfString("Общая громкость (Master Volume):  " + volStr), 22);
            volText.setFillColor(_isMuted ? sf::Color(255, 100, 100) : sf::Color::White);
            volText.setPosition(sf::Vector2f(contentX + 50.0f, contentY + 95.0f));
            window.draw(volText);
        }

        // Шкала громкости
        float barX = contentX + 50.0f;
        float barY = contentY + 145.0f;
        float barW = 580.0f;
        float barH = 18.0f;

        sf::RectangleShape barBg({barW, barH});
        barBg.setPosition(sf::Vector2f(barX, barY));
        barBg.setFillColor(sf::Color(22, 22, 26));
        barBg.setOutlineThickness(1.5f);
        barBg.setOutlineColor(sf::Color(55, 55, 62));
        window.draw(barBg);

        float fillW = barW * (_isMuted ? 0.0f : (_currentVolume / 100.0f));
        if (fillW > 0.0f) {
            sf::RectangleShape barFill({fillW, barH});
            barFill.setPosition(sf::Vector2f(barX, barY));
            barFill.setFillColor(sf::Color(210, 210, 215));
            window.draw(barFill);
        }

        // Кнопки громкости
        float by = contentY + 195.0f;
        Button btnVolDown{{{contentX + 50.0f, by}, {140.0f, 48.0f}}, "[ - 10% ]",
                          sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 18};
        if (drawButton(window, btnVolDown, mousePos) && justClicked) {
            _isMuted = false;
            _currentVolume = std::max(0.0f, _currentVolume - 10.0f);
            SoundManager::get().setMasterVolume(_currentVolume);
            SoundManager::get().play("1", 40.0f);
        }

        Button btnVolUp{{{contentX + 210.0f, by}, {140.0f, 48.0f}}, "[ + 10% ]",
                        sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 18};
        if (drawButton(window, btnVolUp, mousePos) && justClicked) {
            _isMuted = false;
            _currentVolume = std::min(100.0f, _currentVolume + 10.0f);
            SoundManager::get().setMasterVolume(_currentVolume);
            SoundManager::get().play("1", 40.0f);
        }

        Button btnMute{{{contentX + 370.0f, by}, {260.0f, 48.0f}}, 
                       _isMuted ? "ВКЛЮЧИТЬ ЗВУК" : "ВЫКЛЮЧИТЬ ЗВУК (MUTE)",
                       _isMuted ? sf::Color(36, 44, 38) : sf::Color(44, 28, 30), 
                       _isMuted ? sf::Color(55, 75, 60) : sf::Color(75, 40, 45), 
                       _isMuted ? sf::Color(100, 160, 110) : sf::Color(160, 70, 80), 16};
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
            sf::Text tip(*_font, toSfString("• При нажатии кнопок громкости воспроизводится тестовый звук удара.\n"
                                           "• Громкость сохраняется для всех боевых атак, парирования, шагов и звуков комбо."), 15);
            tip.setFillColor(sf::Color(210, 210, 215));
            tip.setPosition(sf::Vector2f(contentX + 50.0f, by + 90.0f));
            window.draw(tip);
        }
    } else if (_settingsTab == SettingsTab::FPS) {
        if (_font) {
            sf::Text subTitle(*_font, toSfString("РЕЖИМЫ ОГРАНИЧЕНИЯ КАДРОВОЙ ЧАСТОТЫ (FPS)"), 22);
            subTitle.setStyle(sf::Text::Bold);
            subTitle.setFillColor(sf::Color::White);
            subTitle.setPosition(sf::Vector2f(contentX + 50.0f, contentY + 40.0f));
            window.draw(subTitle);
        }

        float by1 = contentY + 95.0f;
        float bw = 260.0f;
        float bh = 48.0f;
        float bGap = 20.0f;

        auto checkMode = [&](FpsLimitMode m, const std::string& lbl, float x, float y) {
            bool isCurrent = (_fpsMode == m);
            Button btn{{{x, y}, {bw, bh}}, lbl,
                       isCurrent ? sf::Color(48, 48, 56) : sf::Color(24, 24, 28),
                       isCurrent ? sf::Color(65, 65, 75) : sf::Color(44, 44, 52),
                       isCurrent ? sf::Color::White : sf::Color(75, 75, 84), 16};
            if (drawButton(window, btn, mousePos, isCurrent) && justClicked) {
                _fpsMode = m;
                if (_onFpsModeChanged) _onFpsModeChanged(m);
                SoundManager::get().play("1", 30.0f);
            }
        };

        checkMode(FpsLimitMode::UNCAPPED, "MAX (БЕЗ ЛИМИТА)", contentX + 50.0f, by1);
        checkMode(FpsLimitMode::FPS_200,   "200 FPS (МОНИТОР)", contentX + 50.0f + bw + bGap, by1);
        checkMode(FpsLimitMode::FPS_240,   "240 FPS",          contentX + 50.0f + (bw + bGap) * 2.0f, by1);

        float by2 = by1 + bh + 16.0f;
        checkMode(FpsLimitMode::FPS_144,   "144 FPS",          contentX + 50.0f, by2);
        checkMode(FpsLimitMode::FPS_60,    "60 FPS",           contentX + 50.0f + bw + bGap, by2);
        checkMode(FpsLimitMode::VSYNC,     "VSYNC (СИНХРОНИЗАЦИЯ)", contentX + 50.0f + (bw + bGap) * 2.0f, by2);

        float by3 = by2 + bh + 35.0f;
        Button btnToggleCounter{{{contentX + 50.0f, by3}, {420.0f, 48.0f}},
                                _showFpsCounter ? "СЧЕТЧИК FPS: [ ВКЛЮЧЕН ]" : "СЧЕТЧИК FPS: [ ВЫКЛЮЧЕН ]",
                                _showFpsCounter ? sf::Color(32, 42, 36) : sf::Color(42, 26, 28),
                                _showFpsCounter ? sf::Color(50, 75, 58) : sf::Color(75, 40, 45),
                                _showFpsCounter ? sf::Color(100, 170, 120) : sf::Color(170, 75, 85), 17};
        if (drawButton(window, btnToggleCounter, mousePos) && justClicked) {
            _showFpsCounter = !_showFpsCounter;
            SoundManager::get().play("1", 30.0f);
        }

        if (_font) {
            sf::Text tip(*_font, toSfString("• Переключать лимитер кадров можно прямо во время игры клавишей F4.\n"
                                           "• Быстро скрыть / показать счетчик FPS можно горячей клавишей F3."), 15);
            tip.setFillColor(sf::Color(210, 210, 215));
            tip.setPosition(sf::Vector2f(contentX + 50.0f, by3 + 70.0f));
            window.draw(tip);
        }
    }

    // Нижняя плашка с кнопкой НАЗАД
    sf::RectangleShape bottomBar({winSize.x, bottomBarH});
    bottomBar.setPosition(sf::Vector2f(0.0f, winSize.y - bottomBarH));
    bottomBar.setFillColor(sf::Color(18, 18, 22));
    bottomBar.setOutlineThickness(1.5f);
    bottomBar.setOutlineColor(sf::Color(45, 45, 52));
    window.draw(bottomBar);

    float backW = 240.0f;
    float backH = 48.0f;
    Button btnBack{{{cx - backW * 0.5f, winSize.y - bottomBarH * 0.5f - backH * 0.5f}, {backW, backH}}, "НАЗАД",
                   sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 20};
    if (drawButton(window, btnBack, mousePos) && justClicked) {
        _currentScreen = MenuScreen::MAIN;
        SoundManager::get().play("1", 30.0f);
    }
}

void MenuSystem::drawCombosTab(sf::RenderWindow& window, float x, float y, float w, float h) {
    if (!_font) return;

    sf::Text hint(*_font, toSfString("ОБОЗНАЧЕНИЯ: 1 = ЛКМ | 2 = ПКМ | dw = Вниз (S) | u = Вверх (W) | d = Дэш (Shift) | df = Дэш вперед | db = Дэш назад | a = В воздухе"), 12);
    hint.setFillColor(sf::Color(140, 195, 255));
    hint.setPosition(sf::Vector2f(x + 20.0f, y + 12.0f));
    window.draw(hint);

    struct ComboGuideItem {
        std::string inputs;
        std::string name;
        std::string desc;
        sf::Color col;
    };

    std::vector<ComboGuideItem> items = {
        {"1 -> 1 -> 1", "Triple Light [РУКИ]", "Быстрая базовая серия легких джебов с мягким отталкиванием", sf::Color(255, 240, 90)},
        {"2 -> 2 -> 2", "Triple Heavy [РУКИ]", "Мощная сокрушительная серия тяжелых ударов с дальним сбиванием с ног", sf::Color(255, 110, 50)},
        {"1 -> f2 -> d1", "Diagonal Launcher [РУКИ]", "Джеб -> удар вперед (f2) -> дэш-апперкот (d1): запуск по диагонали вверх", sf::Color(255, 80, 80)},
        {"1 -> 1 -> b2", "Grab & Back Throw [РУКИ]", "Два быстрых джеба -> захват и бросок назад через спину с разворотом (b2)", sf::Color(255, 95, 140)},
        {"1 -> 1 -> 2", "Rapid Launcher [РУКИ]", "Два стремительных джеба и вертикальный подброс врага в воздух для джаггла", sf::Color(255, 120, 80)},
        {"2 -> 1 -> db1", "Reverse Sweep [РУКИ]", "Тяжелый удар -> джеб -> рывок назад с подсечкой (db1): запуск вверх", sf::Color(255, 140, 80)},
        {"a1 -> adw2", "Ground Bounce [РУКИ]", "Удар в воздухе (a1) -> дайв-слэм вниз (adw2): вбивание в пол с отскоком вверх", sf::Color(255, 90, 30)},
        {"a1 -> a1 -> adw2", "Dual Dive Slam [РУКИ]", "Два удара в воздухе (a1) -> финальный дайв-слэм вниз (adw2) об пол с отскоком", sf::Color(255, 75, 25)},
        {"a1 -> a2", "Air Push [РУКИ]", "Удар в воздухе (a1) и тяжелый толчок (a2) далеко по горизонтали", sf::Color(220, 130, 255)},
        {"1 -> 2 -> df2", "Dash Smash [РУКИ]", "Джеб -> силовой удар -> дэш вперед со смешем (df2) строго по горизонтали", sf::Color(255, 180, 60)}
    };

    float startRowY = y + 26.0f;
    float rowH = 24.0f;

    for (size_t i = 0; i < items.size(); ++i) {
        float ry = startRowY + i * rowH;

        sf::RectangleShape rowBg({w - 30.0f, rowH - 2.0f});
        rowBg.setPosition(sf::Vector2f(x + 15.0f, ry));
        rowBg.setFillColor((i % 2 == 0) ? sf::Color(14, 16, 22, 255) : sf::Color(18, 20, 28, 255));
        rowBg.setOutlineThickness(1.0f);
        rowBg.setOutlineColor(sf::Color(35, 42, 55));
        window.draw(rowBg);

        // Инпуты комбо
        sf::Text txtIn(*_font, toSfString(items[i].inputs), 12);
        txtIn.setStyle(sf::Text::Bold);
        txtIn.setFillColor(items[i].col);
        txtIn.setPosition(sf::Vector2f(x + 28.0f, ry + 1.0f));
        window.draw(txtIn);

        // Разделитель
        sf::RectangleShape div1({2.0f, 15.0f});
        div1.setPosition(sf::Vector2f(x + 210.0f, ry + 3.0f));
        div1.setFillColor(sf::Color(48, 65, 92, 170));
        window.draw(div1);

        // Имя комбо
        sf::Text txtName(*_font, toSfString(items[i].name), 12);
        txtName.setStyle(sf::Text::Bold);
        txtName.setFillColor(sf::Color(245, 250, 255));
        txtName.setPosition(sf::Vector2f(x + 225.0f, ry + 1.0f));
        window.draw(txtName);

        // Разделитель
        sf::RectangleShape div2({2.0f, 15.0f});
        div2.setPosition(sf::Vector2f(x + 470.0f, ry + 3.0f));
        div2.setFillColor(sf::Color(48, 65, 92, 170));
        window.draw(div2);

        // Описание эффекта на всю оставшуюся ширину
        sf::Text txtDesc(*_font, toSfString(items[i].desc), 11);
        txtDesc.setFillColor(sf::Color(190, 208, 230));
        txtDesc.setPosition(sf::Vector2f(x + 485.0f, ry + 2.0f));
        window.draw(txtDesc);
    }

    // Анти-спам плашка строго под всеми строками комбо (в собственной аккуратной рамке, без наложения на строки)
    float noteY = startRowY + static_cast<float>(items.size()) * rowH + 10.0f;
    sf::RectangleShape noteBg({w - 30.0f, 26.0f});
    noteBg.setPosition(sf::Vector2f(x + 15.0f, noteY));
    noteBg.setFillColor(sf::Color(38, 28, 16, 220));
    noteBg.setOutlineThickness(1.0f);
    noteBg.setOutlineColor(sf::Color(180, 125, 35, 200));
    window.draw(noteBg);

    sf::Text antiSpam(*_font, toSfString("[!] ПРАВИЛО АНТИ-СПАМА: Повтор одного и того же комбо дважды за 2.5 секунды наказывается оглушением игрока на 0.75с! Варьируйте связки!"), 11);
    antiSpam.setStyle(sf::Text::Bold);
    antiSpam.setFillColor(sf::Color(255, 195, 65));
    antiSpam.setPosition(sf::Vector2f(x + 25.0f, noteY + 5.0f));
    window.draw(antiSpam);
}

void MenuSystem::drawWeaponCombosTab(sf::RenderWindow& window, float x, float y, float w, float h) {
    if (!_font) return;

    sf::Text hint(*_font, toSfString("ОБОЗНАЧЕНИЯ: dw1 = Вниз+ЛКМ | j = Прыжок (Space) | adw1 = Вниз+ЛКМ в воздухе | f1/f2 = Вперед+Атака | b2 = Назад+ПКМ"), 12);
    hint.setFillColor(sf::Color(255, 180, 80));
    hint.setPosition(sf::Vector2f(x + 20.0f, y + 12.0f));
    window.draw(hint);

    struct ComboGuideItem {
        std::string inputs;
        std::string name;
        std::string desc;
        sf::Color col;
    };

    std::vector<ComboGuideItem> items = {
        {"dw1 -> j -> adu2", "Air Grab Meteor Slam [ОРУЖИЕ]", "Топот о пол (dw1) -> прыжок [Space] -> дэш и захват цели над собой (adu2) со слэмом о пол", sf::Color(255, 95, 35)},
        {"dw1 -> j -> adw1", "Stomp & Sky Hook [ОРУЖИЕ]", "Топот о пол (dw1) -> прыжок [Space] -> зацеп крюком (adw1) ввысь с зависанием", sf::Color(255, 160, 40)},
        {"dw1 -> adw1", "Quick Sky Hook [ОРУЖИЕ]", "Прямая воздушная связка: топот о землю + удар крюком под себя в воздухе", sf::Color(255, 175, 50)},
        {"1 -> 2 -> f1", "Bat Swing Smash [ОРУЖИЕ]", "Джеб (1) -> удар о землю (2) -> сокрушительный удар булавой словно битой (f1)", sf::Color(255, 125, 40)},
        {"b2 -> 1 -> u2", "Sky Launcher [ОРУЖИЕ]", "Дальний зацеп булавой (b2) -> тычок рукоятью (1) -> апперкот булавой (u2) в небо", sf::Color(255, 70, 120)},
        {"1 -> 2 -> 1", "Tenderizer Push [ОРУЖИЕ]", "Джеб (1) -> удар о землю (2) -> силовой горизонтальный толчок булавой (1)", sf::Color(255, 165, 45)},
        {"db1 -> u1", "Reverse Sky Pop-Up [ОРУЖИЕ]", "Удар назад с разворотом (db1) -> апперкот булавой (u1) с вертикальным подбросом", sf::Color(255, 110, 180)},
        {"dw1 (стоя)", "Mace Ground Stomp", "Нижний силовой удар булавой о пол: пробивает верхний блок и подбрасывает цель", sf::Color(255, 190, 70)},
        {"b2 (дальний)", "Mace Reach Hook", "Дальний притягивающий взмах булавой: подтягивает врага ближе к игроку", sf::Color(255, 90, 130)},
        {"R / ПКМ", "Throw & Retrieve", "Бросок булавы по 8 направлениям (вверх зависает для джаггла; повторно притягивает)", sf::Color(100, 220, 255)}
    };

    float startRowY = y + 26.0f;
    float rowH = 24.0f;

    for (size_t i = 0; i < items.size(); ++i) {
        float ry = startRowY + i * rowH;

        sf::RectangleShape rowBg({w - 30.0f, rowH - 2.0f});
        rowBg.setPosition(sf::Vector2f(x + 15.0f, ry));
        rowBg.setFillColor((i % 2 == 0) ? sf::Color(14, 16, 22, 255) : sf::Color(18, 20, 28, 255));
        rowBg.setOutlineThickness(1.0f);
        rowBg.setOutlineColor(sf::Color(35, 42, 55));
        window.draw(rowBg);

        // Инпуты комбо
        sf::Text txtIn(*_font, toSfString(items[i].inputs), 12);
        txtIn.setStyle(sf::Text::Bold);
        txtIn.setFillColor(items[i].col);
        txtIn.setPosition(sf::Vector2f(x + 28.0f, ry + 1.0f));
        window.draw(txtIn);

        // Разделитель
        sf::RectangleShape div1({2.0f, 15.0f});
        div1.setPosition(sf::Vector2f(x + 210.0f, ry + 3.0f));
        div1.setFillColor(sf::Color(48, 65, 92, 170));
        window.draw(div1);

        // Имя комбо
        sf::Text txtName(*_font, toSfString(items[i].name), 12);
        txtName.setStyle(sf::Text::Bold);
        txtName.setFillColor(sf::Color(245, 250, 255));
        txtName.setPosition(sf::Vector2f(x + 225.0f, ry + 1.0f));
        window.draw(txtName);

        // Разделитель
        sf::RectangleShape div2({2.0f, 15.0f});
        div2.setPosition(sf::Vector2f(x + 470.0f, ry + 3.0f));
        div2.setFillColor(sf::Color(48, 65, 92, 170));
        window.draw(div2);

        // Описание эффекта на всю оставшуюся ширину
        sf::Text txtDesc(*_font, toSfString(items[i].desc), 11);
        txtDesc.setFillColor(sf::Color(190, 208, 230));
        txtDesc.setPosition(sf::Vector2f(x + 485.0f, ry + 2.0f));
        window.draw(txtDesc);
    }

    // Подсказка по оружию внизу
    float noteY = startRowY + static_cast<float>(items.size()) * rowH + 10.0f;
    sf::RectangleShape noteBg({w - 30.0f, 26.0f});
    noteBg.setPosition(sf::Vector2f(x + 15.0f, noteY));
    noteBg.setFillColor(sf::Color(20, 30, 42, 220));
    noteBg.setOutlineThickness(1.0f);
    noteBg.setOutlineColor(sf::Color(45, 120, 200, 200));
    window.draw(noteBg);

    sf::Text weaponNote(*_font, toSfString("[i] ОРУЖИЕ В БОЮ: Чтобы подобрать брошенную булаву, подойдите к ней или нажмите [R] / [ПКМ]. С оружием в руках все атаки наносят повышенный урон!"), 11);
    weaponNote.setStyle(sf::Text::Bold);
    weaponNote.setFillColor(sf::Color(120, 210, 255));
    weaponNote.setPosition(sf::Vector2f(x + 25.0f, noteY + 5.0f));
    window.draw(weaponNote);
}

void MenuSystem::drawMechanicsTab(sf::RenderWindow& window, float x, float y, float w, float h) {
    if (!_font) return;

    struct MechItem {
        std::string title;
        std::string desc;
        sf::Color col;
    };

    std::vector<MechItem> items = {
        {"БЛОК ЩИТОМ [F / E]", "Снижает весь входящий урон на 80%. Если враг наносит финишный удар серии по блоку, он сам оглушается и сбрасывает комбо!", sf::Color(80, 210, 255)},
        {"ПАРИРОВАНИЕ [Q]", "Стойка идеального парирования (0.35с). Попадание врага в этот тайминг блокирует атаку, оглушает бота и наносит мощный встречный контрудар!", sf::Color(0, 240, 255)},
        {"ИДЕАЛЬНЫЙ УВОРОТ [SHIFT]", "Если нажать рывок прямо перед попаданием удара, персонаж совершает идеальный додж: золотая волна станит врага, а следующий удар наносит повышенный урон!", sf::Color(255, 220, 60)},
        {"ЖОНГЛИРОВАНИЕ В ВОЗДУХЕ (JUGGLE)", "Враги, запущенные в воздух лаунчерами (1f2d1, 112, 21db1), парят с пониженной гравитацией, позволяя проводить серии ударов в прыжке (a1, a2, adw2).", sf::Color(140, 180, 255)},
        {"МЕТАНИЕ БУЛАВЫ [R / СКМ (MOUSE 3)]", "8-направленное метание оружия по позиции курсора (R или СКМ / колесико). Скорость растет от бега. Без булавы активируется рукопашный бой со всеми комбо!", sf::Color(255, 140, 70)},
        {"ОТСКОКИ ОТ ПОЛА И СТЕН (BOUNCES)", "Удары дайв-слэма (adw2) вбивают врага в пол с упругим отскоком вверх (Ground Bounce), открывая возможность продолжить серию комбо.", sf::Color(255, 110, 90)},
        {"СКЕЙЛИНГ УРОНА (PRORATION)", "Каждый последующий удар в непрерывной серии наносит на 10% меньше урона (вплоть до минимума в 30%), поощряя быстрые и разнообразные связки.", sf::Color(200, 220, 240)}
    };

    float startRowY = y + 14.0f;
    float cardH = 52.0f;

    for (size_t i = 0; i < items.size(); ++i) {
        float cy = startRowY + i * (cardH + 6.0f);

        sf::RectangleShape cardBg({w - 30.0f, cardH});
        cardBg.setPosition(sf::Vector2f(x + 15.0f, cy));
        cardBg.setFillColor(sf::Color(14, 16, 22, 255));
        cardBg.setOutlineThickness(1.0f);
        cardBg.setOutlineColor(sf::Color(35, 42, 55));
        window.draw(cardBg);

        sf::Text txtTitle(*_font, toSfString(items[i].title), 15);
        txtTitle.setStyle(sf::Text::Bold);
        txtTitle.setFillColor(items[i].col);
        txtTitle.setPosition(sf::Vector2f(x + 28.0f, cy + 6.0f));
        window.draw(txtTitle);

        sf::Text txtDesc(*_font, toSfString(items[i].desc), 13);
        txtDesc.setFillColor(sf::Color(205, 222, 242));
        txtDesc.setPosition(sf::Vector2f(x + 28.0f, cy + 28.0f));
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
         "Серый силуэт. Базовый противник со стандартным набором одиночных ударов и средним радиусом обзора. Идеальная цель для отработки базовых таймингов и комбо-связок.",
         sf::Color(170, 180, 195)},
        {"АРХЕТИП 2: TANK (ТЯЖЕЛЫЙ БРОНЕНОСЕЦ)", "HP: 110 | Скорость: Медленная | Броня: Тяжелая (В 1.5 раза крупнее)",
         "Красный силуэт. Обладает огромным запасом здоровья и высокой устойчивостью к оглушению. Наносит сокрушительный урон с большой дистанции замаха.",
         sf::Color(255, 80, 80)},
        {"АРХЕТИП 3: DUELIST [AGGRESSIVE] (ДУЭЛЯНТ АГРЕССИВНЫЙ)", "HP: 65 | Скорость: Высокая | КД Рывка: 1.5с | Агрессия: 95%",
         "Фиолетовый силуэт. Беспощадный прессинг через дэши: держит ближний спейсинг вне зоны ударов игрока и не подходит в упор, при малейшем бездействии игрока стремительно нападает с рывка!",
         sf::Color(180, 80, 255)},
        {"АРХЕТИП 4: DUELIST [PASSIVE] (ДУЭЛЯНТ ПАССИВНЫЙ)", "HP: 60 | Скорость: Высокая | КД Рывка: 2.0с | Интеллект: 95%",
         "Голубой силуэт. Хладнокровный мастер спейсинга: держит дистанцию, входит в невидимость на 1 сек раз в 7 сек, филигранно блокирует и парирует удары, моментально панишит игрока в стане!",
         sf::Color(45, 185, 245)}
    };

    float startY = y + 12.0f;
    float cardH = 76.0f;
    float cardGap = 8.0f;

    for (size_t i = 0; i < items.size(); ++i) {
        float cy = startY + i * (cardH + cardGap);

        sf::RectangleShape cardBg({w - 30.0f, cardH});
        cardBg.setPosition(sf::Vector2f(x + 15.0f, cy));
        cardBg.setFillColor(sf::Color(14, 16, 22, 255));
        cardBg.setOutlineThickness(1.5f);
        cardBg.setOutlineColor(items[i].col);
        window.draw(cardBg);

        sf::Text txtName(*_font, toSfString(items[i].name), 15);
        txtName.setStyle(sf::Text::Bold);
        txtName.setFillColor(items[i].col);
        txtName.setPosition(sf::Vector2f(x + 28.0f, cy + 7.0f));
        window.draw(txtName);

        sf::Text txtStats(*_font, toSfString(items[i].hpSpeed), 12);
        txtStats.setFillColor(sf::Color(255, 220, 120));
        txtStats.setPosition(sf::Vector2f(x + 28.0f, cy + 28.0f));
        window.draw(txtStats);

        sf::Text txtDesc(*_font, toSfString(items[i].desc), 12);
        txtDesc.setFillColor(sf::Color(210, 225, 245));
        txtDesc.setPosition(sf::Vector2f(x + 28.0f, cy + 48.0f));
        window.draw(txtDesc);
    }

    // Состояния восприятия ИИ
    float aiInfoY = startY + 4 * (cardH + cardGap) + 6.0f;
    sf::Text aiInfo(*_font, toSfString("ПОВЕДЕНИЕ ИИ: [Лазурный визор] Патруль -> [Желтый маркер !] Замечает игрока -> [Красный визор] Атака -> [! PUNISH !] Наказание стана"), 12);
    aiInfo.setStyle(sf::Text::Bold);
    aiInfo.setFillColor(sf::Color(80, 225, 255));
    aiInfo.setPosition(sf::Vector2f(x + 20.0f, aiInfoY));
    window.draw(aiInfo);
}

void MenuSystem::drawGuideScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize) {
    float cx = winSize.x * 0.5f;

    // Сплошная заливка на весь экран
    sf::RectangleShape bgFill(winSize);
    bgFill.setFillColor(sf::Color(10, 10, 12));
    window.draw(bgFill);

    // Верхняя плашка шапки
    float topH = 75.0f;
    sf::RectangleShape topBar({winSize.x, topH});
    topBar.setPosition(sf::Vector2f(0.0f, 0.0f));
    topBar.setFillColor(sf::Color(18, 18, 22));
    topBar.setOutlineThickness(1.5f);
    topBar.setOutlineColor(sf::Color(45, 45, 52));
    window.draw(topBar);

    if (_font) {
        sf::Text title(*_font, toSfString("ГАЙД ПО ИГРЕ: КОМБО, МЕХАНИКИ И ВРАГИ"), 28);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color::White);
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
        title.setPosition(sf::Vector2f(cx, topH * 0.5f));
        window.draw(title);
    }

    bool mouseClicked = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    static bool prevMouse = false;
    bool justClicked = mouseClicked && !prevMouse;
    prevMouse = mouseClicked;

    // Вкладки гайда (4 вкладки)
    float tabW = 230.0f;
    float tabH = 42.0f;
    float tabY = topH + 16.0f;
    float tabsStartX = cx - (tabW * 4.0f + 36.0f) * 0.5f;

    Button btnTabCombos{{{tabsStartX, tabY}, {tabW, tabH}}, "1. РУКОПАШНЫЕ КОМБО",
                       sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 14};
    if (drawButton(window, btnTabCombos, mousePos, _guideTab == GuideTab::COMBOS) && justClicked) {
        _guideTab = GuideTab::COMBOS;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabWeapon{{{tabsStartX + tabW + 12.0f, tabY}, {tabW, tabH}}, "2. КОМБО С ОРУЖИЕМ",
                       sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 14};
    if (drawButton(window, btnTabWeapon, mousePos, _guideTab == GuideTab::WEAPON_COMBOS) && justClicked) {
        _guideTab = GuideTab::WEAPON_COMBOS;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabMech{{{tabsStartX + (tabW + 12.0f) * 2.0f, tabY}, {tabW, tabH}}, "3. МЕХАНИКИ БОЯ",
                     sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 14};
    if (drawButton(window, btnTabMech, mousePos, _guideTab == GuideTab::MECHANICS) && justClicked) {
        _guideTab = GuideTab::MECHANICS;
        SoundManager::get().play("1", 30.0f);
    }

    Button btnTabEnemies{{{tabsStartX + (tabW + 12.0f) * 3.0f, tabY}, {tabW, tabH}}, "4. АРХЕТИПЫ ВРАГОВ",
                        sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 14};
    if (drawButton(window, btnTabEnemies, mousePos, _guideTab == GuideTab::ENEMIES) && justClicked) {
        _guideTab = GuideTab::ENEMIES;
        SoundManager::get().play("1", 30.0f);
    }

    // Рабочая область гайда на весь экран
    float marginX = 80.0f;
    float contentX = marginX;
    float contentY = tabY + tabH + 20.0f;
    float contentW = winSize.x - marginX * 2.0f;
    float bottomBarH = 80.0f;
    float contentH = winSize.y - contentY - bottomBarH - 10.0f;

    sf::RectangleShape contentBox({contentW, contentH});
    contentBox.setPosition(sf::Vector2f(contentX, contentY));
    contentBox.setFillColor(sf::Color(14, 14, 16, 255));
    contentBox.setOutlineThickness(1.5f);
    contentBox.setOutlineColor(sf::Color(40, 40, 46));
    window.draw(contentBox);

    if (_guideTab == GuideTab::COMBOS) {
        drawCombosTab(window, contentX, contentY, contentW, contentH);
    } else if (_guideTab == GuideTab::WEAPON_COMBOS) {
        drawWeaponCombosTab(window, contentX, contentY, contentW, contentH);
    } else if (_guideTab == GuideTab::MECHANICS) {
        drawMechanicsTab(window, contentX, contentY, contentW, contentH);
    } else if (_guideTab == GuideTab::ENEMIES) {
        drawEnemiesTab(window, contentX, contentY, contentW, contentH);
    }

    // Нижняя плашка с кнопкой НАЗАД
    sf::RectangleShape bottomBar({winSize.x, bottomBarH});
    bottomBar.setPosition(sf::Vector2f(0.0f, winSize.y - bottomBarH));
    bottomBar.setFillColor(sf::Color(18, 18, 22));
    bottomBar.setOutlineThickness(1.5f);
    bottomBar.setOutlineColor(sf::Color(45, 45, 52));
    window.draw(bottomBar);

    float backW = 240.0f;
    float backH = 48.0f;
    Button btnBack{{{cx - backW * 0.5f, winSize.y - bottomBarH * 0.5f - backH * 0.5f}, {backW, backH}}, "НАЗАД",
                   sf::Color(24, 24, 28), sf::Color(44, 44, 52), sf::Color(75, 75, 84), 20};
    if (drawButton(window, btnBack, mousePos) && justClicked) {
        _currentScreen = MenuScreen::MAIN;
        SoundManager::get().play("1", 30.0f);
    }
}

void MenuSystem::draw(sf::RenderWindow& window, const sf::Vector2f& mousePos) {
    if (!isInMenu()) return;

    sf::Vector2f winSize(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));

    if (_currentScreen == MenuScreen::MAIN) {
        drawMainMenuScreen(window, mousePos, winSize);
    } else if (_currentScreen == MenuScreen::SETTINGS) {
        drawSettingsScreen(window, mousePos, winSize);
    } else if (_currentScreen == MenuScreen::GUIDE) {
        drawGuideScreen(window, mousePos, winSize);
    }
}
