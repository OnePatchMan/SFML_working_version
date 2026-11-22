#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>
#include <optional>
#include "core/Common.h"
#include "audio/SoundManager.h"

enum class AppState {
    MAIN_MENU,
    IN_GAME,
    PAUSED
};

enum class MenuScreen {
    MAIN,
    SETTINGS,
    GUIDE
};

enum class SettingsTab {
    BINDS,
    SOUND,
    FPS
};

enum class GuideTab {
    COMBOS,
    WEAPON_COMBOS,
    MECHANICS,
    ENEMIES
};

enum class FpsLimitMode {
    UNCAPPED,
    FPS_200,
    FPS_240,
    FPS_144,
    FPS_60,
    VSYNC
};

namespace MenuTheme {
    constexpr float BUTTON_OUTLINE = 1.5f;
    constexpr float BUTTON_HOVER_OUTLINE = 2.5f;
    constexpr sf::Color BG_DARK{10, 10, 12, 255};
    constexpr sf::Color BG_PANEL{18, 18, 22, 255};
    constexpr sf::Color ROW_BG{20, 20, 24, 255};
    constexpr sf::Color OUTLINE_GRAY{45, 45, 52, 255};
    constexpr sf::Color TEXT_WHITE{255, 255, 255, 255};
    constexpr sf::Color ACCENT_CYAN{100, 210, 255, 255};
}

class MenuSystem {
public:
    MenuSystem();

    void init(const sf::Font* font);

    AppState getAppState() const { return _appState; }
    void setAppState(AppState state) { _appState = state; }

    bool isInMenu() const { return _appState == AppState::MAIN_MENU || _appState == AppState::PAUSED; }

    void handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);
    void draw(sf::RenderWindow& window, const sf::Vector2f& mousePos);

    // Коллбеки
    void setOnStartGame(std::function<void()> cb) { _onStartGame = std::move(cb); }
    void setOnStartTraining(std::function<void()> cb) { _onStartTraining = std::move(cb); }
    void setOnResetGame(std::function<void()> cb) { _onResetGame = std::move(cb); }
    void setOnExitGame(std::function<void()> cb) { _onExitGame = std::move(cb); }
    void setOnFpsModeChanged(std::function<void(FpsLimitMode)> cb) { _onFpsModeChanged = std::move(cb); }
    
    void setFpsMode(FpsLimitMode mode) { _fpsMode = mode; }
    FpsLimitMode getFpsMode() const { return _fpsMode; }

    bool getShowFpsCounter() const { return _showFpsCounter; }
    void setShowFpsCounter(bool show) { _showFpsCounter = show; }

    bool isTrainingMode() const { return _isTrainingMode; }
    void setTrainingMode(bool tm) { _isTrainingMode = tm; }

    void openPauseMenu() {
        _appState = AppState::PAUSED;
        _currentScreen = MenuScreen::MAIN;
        _returnScreen = MenuScreen::MAIN;
        SoundManager::get().pauseMusic();
    }

    void resumeGame() {
        _appState = AppState::IN_GAME;
        SoundManager::get().resumeMusic();
    }

    void openMainMenu() {
        _appState = AppState::MAIN_MENU;
        _currentScreen = MenuScreen::MAIN;
        _returnScreen = MenuScreen::MAIN;
        SoundManager::get().playMenuMusic();
    }

private:
    struct Button {
        sf::FloatRect rect;
        std::string label;
        sf::Color normalColor;
        sf::Color hoverColor;
        sf::Color outlineColor;
        unsigned int fontSize = 20;
    };

    bool drawButton(sf::RenderWindow& window, const Button& btn, const sf::Vector2f& mousePos, bool selected = false);

    void drawMainMenuScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize);
    void drawSettingsScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize);
    void drawGuideScreen(sf::RenderWindow& window, const sf::Vector2f& mousePos, const sf::Vector2f& winSize);

    void drawCombosTab(sf::RenderWindow& window, float x, float y, float w, float h);
    void drawWeaponCombosTab(sf::RenderWindow& window, float x, float y, float w, float h);
    void drawMechanicsTab(sf::RenderWindow& window, float x, float y, float w, float h);
    void drawEnemiesTab(sf::RenderWindow& window, float x, float y, float w, float h);

    void drawKeybindRow(sf::RenderWindow& window, float x, float y, float w, 
                        const std::string& key, const std::string& desc, sf::Color keyCol = sf::Color(100, 210, 255));

    AppState _appState = AppState::MAIN_MENU;
    MenuScreen _currentScreen = MenuScreen::MAIN;
    MenuScreen _returnScreen = MenuScreen::MAIN;

    SettingsTab _settingsTab = SettingsTab::BINDS;
    GuideTab _guideTab = GuideTab::COMBOS;

    FpsLimitMode _fpsMode = FpsLimitMode::UNCAPPED;
    bool _showFpsCounter = true;

    const sf::Font* _font = nullptr;

    // Звук
    float _currentVolume = 50.0f; // 0..100
    bool _isMuted = false;
    float _preMuteVolume = 50.0f;

    bool _isTrainingMode = false;

    mutable sf::RectangleShape _cachedButtonRect;
    mutable std::optional<sf::Text> _cachedButtonText;
    mutable sf::RectangleShape _cachedRowBg;
    mutable std::optional<sf::Text> _cachedRowText;

    std::function<void()> _onStartGame;
    std::function<void()> _onStartTraining;
    std::function<void()> _onResetGame;
    std::function<void()> _onExitGame;
    std::function<void(FpsLimitMode)> _onFpsModeChanged;
};

