#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

// -------- CONSTANTS --------
const int CELL = 24;
const int W = 31;
const int H = 31;
const int HUD_HEIGHT = 70;
const int WINDOW_WIDTH = W * CELL;
const int WINDOW_HEIGHT = H * CELL + HUD_HEIGHT;

// Maze: 0 = path, 1 = wall
int maze[H][W];

// Directions (up, down, left, right)
int dx[4] = { 0, 0, -2, 2 };
int dy[4] = { -2, 2, 0, 0 };

// Game state
enum GameState { ENTER_P1, ENTER_P2, COUNTDOWN, PLAYING, FINISHED };
GameState state = ENTER_P1;

// Player info
string p1 = "", p2 = "";
int p1x = 1, p1y = 1;
int p2x = 1, p2y = 1;
bool p1_done = false, p2_done = false;

// Start & goal
int startX = 1, startY = 1;
int goalX = W - 2, goalY = H - 2;

// -------- FUNCTIONS --------

// Fill maze with walls
void fillMaze() {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            maze[y][x] = 1;
}

// Check if a cell is inside maze
bool isInside(int x, int y) {
    return x > 0 && x < W - 1 && y > 0 && y < H - 1;
}

// Shuffle directions
void shuffle(int arr[4]) {
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

// Generate maze using simple iterative DFS with arrays
void generateMaze() {
    fillMaze();

    int stack[H * W][2]; // stack of x,y
    int top = 0;

    stack[top][0] = 1; stack[top][1] = 1; top++;
    maze[1][1] = 0;

    while (top > 0) {
        int curX = stack[top - 1][0];
        int curY = stack[top - 1][1];
        top--;

        int dirs[4] = { 0,1,2,3 };
        shuffle(dirs);

        for (int i = 0; i < 4; i++) {
            int nx = curX + dx[dirs[i]];
            int ny = curY + dy[dirs[i]];

            if (isInside(nx, ny) && maze[ny][nx] == 1) {
                maze[curY + dy[dirs[i]] / 2][curX + dx[dirs[i]] / 2] = 0;
                maze[ny][nx] = 0;

                stack[top][0] = nx;
                stack[top][1] = ny;
                top++;
            }
        }
    }

    maze[startY][startX] = 0;
    maze[goalY][goalX] = 0;
}

// Convert grid to pixel center
sf::Vector2f center(int gx, int gy) {
    return sf::Vector2f(gx * CELL + CELL / 2, gy * CELL + CELL / 2);
}

// Reset players & round
void resetRound() {
    generateMaze();
    p1x = p1y = 1;
    p2x = p2y = 1;
    p1_done = p2_done = false;
    state = COUNTDOWN;
}

// Draw maze, players, HUD
void drawAll(sf::RenderWindow& win, const sf::Font& font) {
    win.clear(sf::Color(10, 10, 30));

    sf::RectangleShape cell(sf::Vector2f(CELL, CELL));
    for (int y = 0;y < H;y++)
        for (int x = 0;x < W;x++) {
            if (maze[y][x] == 1) cell.setFillColor(sf::Color(40, 40, 60));
            else cell.setFillColor(sf::Color(120, 120, 160));
            cell.setPosition(x * CELL, y * CELL);
            win.draw(cell);
        }

    sf::RectangleShape goal(sf::Vector2f(CELL, CELL));
    goal.setPosition(goalX * CELL, goalY * CELL);
    goal.setFillColor(sf::Color::Yellow);
    win.draw(goal);

    sf::CircleShape c1(CELL * 0.45f);
    c1.setOrigin(c1.getRadius(), c1.getRadius());
    c1.setPosition(center(p1x, p1y));
    c1.setFillColor(sf::Color::Blue);
    win.draw(c1);

    sf::CircleShape c2(CELL * 0.45f);
    c2.setOrigin(c2.getRadius(), c2.getRadius());
    c2.setPosition(center(p2x, p2y));
    c2.setFillColor(sf::Color::Red);
    win.draw(c2);

    // HUD
    sf::RectangleShape hud(sf::Vector2f(WINDOW_WIDTH, HUD_HEIGHT));
    hud.setPosition(0, H * CELL);
    hud.setFillColor(sf::Color(0, 0, 0));
    win.draw(hud);

    sf::Text t("", font, 20);
    if (state == ENTER_P1) { t.setString("Enter Player 1: " + p1 + "_"); t.setPosition(10, H * CELL + 20); win.draw(t); return; }
    if (state == ENTER_P2) { t.setString("Enter Player 2: " + p2 + "_"); t.setPosition(10, H * CELL + 20); win.draw(t); return; }
    if (state == COUNTDOWN) { t.setString("Get Ready..."); t.setPosition(WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 - 40); t.setCharacterSize(40); win.draw(t); return; }
    if (state == PLAYING) { t.setString(p1 + " (WASD) vs " + p2 + " (ARROWS)"); t.setPosition(10, H * CELL + 20); win.draw(t); return; }
    if (state == FINISHED) {
        string winner;
        if (p1_done && p2_done) winner = "It's a tie!";
        else if (p1_done) winner = p1 + " WINS!";
        else if (p2_done) winner = p2 + " WINS!";
        t.setString(winner + "\nPress SPACE to restart");
        t.setCharacterSize(30);
        t.setPosition(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 40);
        win.draw(t);
    }
}

// -------- MAIN --------
int main() {
    srand(time(0));
    sf::RenderWindow win(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Maze Race - Simple");
    win.setFramerateLimit(60);

    sf::Font font;
#if defined(_WIN32)
    font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
#elif defined(__APPLE__)
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");
#else
    font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    generateMaze();
    int countdown = 120;

    while (win.isOpen()) {
        sf::Event e;
        while (win.pollEvent(e)) {
            if (e.type == sf::Event::Closed) win.close();

            // NAME ENTRY
            if (state == ENTER_P1 || state == ENTER_P2) {
                if (e.type == sf::Event::TextEntered) {
                    char c = e.text.unicode;
                    string& ref = (state == ENTER_P1) ? p1 : p2;
                    if (c == 8 && !ref.empty()) ref.pop_back();
                    else if (c == 13 && !ref.empty()) { if (state == ENTER_P1) state = ENTER_P2; else resetRound(); }
                    else if (c < 128 && ref.size() < 12) ref.push_back(c);
                }
            }

            // MOVEMENT with tie validation
            if (state == PLAYING && e.type == sf::Event::KeyPressed) {
                bool justFinishedP1 = false, justFinishedP2 = false;

                // Player 1
                if (!p1_done) {
                    int nx = p1x, ny = p1y;
                    if (e.key.code == sf::Keyboard::W) ny--;
                    if (e.key.code == sf::Keyboard::S) ny++;
                    if (e.key.code == sf::Keyboard::A) nx--;
                    if (e.key.code == sf::Keyboard::D) nx++;
                    if (maze[ny][nx] == 0) { p1x = nx; p1y = ny; }
                    if (p1x == goalX && p1y == goalY) { p1_done = true; justFinishedP1 = true; }
                }

                // Player 2
                if (!p2_done) {
                    int nx = p2x, ny = p2y;
                    if (e.key.code == sf::Keyboard::Up) ny--;
                    if (e.key.code == sf::Keyboard::Down) ny++;
                    if (e.key.code == sf::Keyboard::Left) nx--;
                    if (e.key.code == sf::Keyboard::Right) nx++;
                    if (maze[ny][nx] == 0) { p2x = nx; p2y = ny; }
                    if (p2x == goalX && p2y == goalY) { p2_done = true; justFinishedP2 = true; }
                }

                // Check finish
                if (justFinishedP1 && justFinishedP2) state = FINISHED; // Tie
                else if (justFinishedP1 || justFinishedP2) state = FINISHED; // One winner
            }

            // Restart
            if (state == FINISHED && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Space) {
                p1.clear(); p2.clear();
                state = ENTER_P1;
            }
        }

        if (state == COUNTDOWN) { countdown--; if (countdown <= 0) { countdown = 120; state = PLAYING; } }

        drawAll(win, font);
        win.display();
    }
    return 0;
}

