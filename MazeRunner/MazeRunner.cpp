#include <SFML/Graphics.hpp>

int main()
{
    // Create the game window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Maze Runner Test");

    // Create a green circle
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    // The Game Loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}