#include <SFML/Graphics.hpp>
#include <iostream>

sf::String toUtf8(std::string_view s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

int main() {
    sf::Font font;
    if (!font.openFromFile("assets/fonts/font.ttf")) {
        std::cerr << "Font load failed!" << std::endl;
        return 1;
    }

    std::string str1 = "[ЛКМ / J]: Легкая (1) | [ПКМ / K]: Тяжелая (2) | [Shift]: Рывок (d) | В воздухе: (a) | [R]: Сброс";
    std::string str2 = "Комбо: 111 (Легкая серия) | 222 (Тяжелая серия) | 112 (Вниз по диагонали) | 12d1 (Диагональ вверх) | a1a2 (Слэм) | a2ad1 (В воздухе)";

    sf::Text t1(font, toUtf8(str1), 16);
    sf::Text t2(font, toUtf8(str2), 16);
    t2.setPosition({0.0f, 30.0f});

    sf::RenderTexture rt({900, 70});
    rt.clear(sf::Color(20, 20, 25));
    rt.draw(t1);
    rt.draw(t2);
    rt.display();

    rt.getTexture().copyToImage().saveToFile("scratch/test_cyrillic.png");
    std::cout << "Saved scratch/test_cyrillic.png successfully!" << std::endl;
    return 0;
}
