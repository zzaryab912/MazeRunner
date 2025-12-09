#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cstdio>
#include <limits>
using namespace std;

// -------------------- CONFIG --------------------
const int CELL_SIZE = 24;
const int MAZE_W = 31;
const int MAZE_H = 31;
const int HUD_HEIGHT = 70;
const int WINDOW_W = MAZE_W * CELL_SIZE;
const int WINDOW_H = MAZE_H * CELL_SIZE + HUD_HEIGHT;

// Files
const string SAVE_FILE = "savegame.txt";
const string SAVE_TMP = "savegame.tmp";
const string WINS_FILE = "winhistory.txt";
const string WINS_COUNT_FILE = "wins_count.txt";
const int MAX_WINS_TO_STORE = 3; // stores last 3 wins in history

// Maze storage: only 2D arrays, simple loops
int maze[MAZE_H][MAZE_W];

// Movement offsets (up, down, left, right) stepping by 2 for maze generation
int moveX[4] = { 0, 0, -2, 2 };
int moveY[4] = { -2, 2, 0, 0 };

// -------------------- GAME MODE CONSTANTS (no enum) --------------------
const int MODE_MENU = 10;
const int MODE_ENTER_P1 = 0;
const int MODE_ENTER_P2 = 1;
const int MODE_COUNTDOWN = 2;
const int MODE_PLAYING = 3;
const int MODE_FINISHED = 4;

// Current mode
int gameMode = MODE_MENU;

// Players
string player1Name = "";
string player2Name = "";
int player1X = 1, player1Y = 1;
int player2X = 1, player2Y = 1;
bool player1Reached = false, player2Reached = false;

// Start/goal
int startX = 1, startY = 1;
int goalX = MAZE_W - 2, goalY = MAZE_H - 2;

// Countdown counter (frames or ticks) - we will use a simple integer and decrement in loop
int countdownTicks = 120;

// Win counters
int player1Wins = 0, player2Wins = 0;

// Menu background (optional)
sf::Texture menuBackgroundTexture;
sf::Sprite menuBackgroundSprite;

// -------------------- MAZE HELPERS --------------------
void fillAllWithWalls() {
    for (int y = 0; y < MAZE_H; y++)
        for (int x = 0; x < MAZE_W; x++)
            maze[y][x] = 1; // 1 means wall
}

bool insideBounds(int x, int y) {
    return x > 0 && x < MAZE_W - 1 && y > 0 && y < MAZE_H - 1;
}

void shuffleArray(int arr[], int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

// Simple depth-first maze using an explicit stack array (allowed constructs only)
void generateMazeSimple() {
    fillAllWithWalls();
    int stackArr[MAZE_W * MAZE_H][2];
    int top = 0;
    stackArr[top][0] = startX;
    stackArr[top][1] = startY;
    top++;
    maze[startY][startX] = 0; // 0 means empty/path

    while (top > 0) {
        int curX = stackArr[top - 1][0];
        int curY = stackArr[top - 1][1];
        top--; // pop

        int dirs[4] = { 0,1,2,3 };
        shuffleArray(dirs, 4);

        for (int i = 0; i < 4; i++) {
            int d = dirs[i];
            int nx = curX + moveX[d];
            int ny = curY + moveY[d];
            if (insideBounds(nx, ny) && maze[ny][nx] == 1) {
                // knock down wall between
                maze[curY + moveY[d] / 2][curX + moveX[d] / 2] = 0;
                maze[ny][nx] = 0;
                stackArr[top][0] = nx;
                stackArr[top][1] = ny;
                top++;
            }
        }
    }
    maze[startY][startX] = 0;
    maze[goalY][goalX] = 0;
}

// center helpers for rendering
float centerPixelX(int gridX) { return gridX * CELL_SIZE + CELL_SIZE / 2.0f; }
float centerPixelY(int gridY) { return gridY * CELL_SIZE + CELL_SIZE / 2.0f; }

// -------------------- SIMPLE SAVE / LOAD --------------------
// We only use basic file streams and simple formatting so no complex libs.

bool atomicWriteReplace(const string& filename, const string& tempname, const string& data) {
    ofstream fout(tempname.c_str(), ios::trunc);
    if (!fout) return false;
    fout << data;
    fout.close();
    remove(filename.c_str());
    if (rename(tempname.c_str(), filename.c_str()) != 0) {
        remove(tempname.c_str());
        return false;
    }
    return true;
}

void saveWinToHistory(const string& winner) {
    // read last lines (simple approach)
    string previous[MAX_WINS_TO_STORE];
    int c = 0;
    ifstream fin(WINS_FILE.c_str());
    string line;
    while (getline(fin, line) && c < MAX_WINS_TO_STORE - 1) {
        if (!line.empty()) { previous[c] = line; c++; }
    }
    fin.close();

    time_t now = time(nullptr);
    char buf[64];
    struct tm* t = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    string entry = winner + " at " + buf;

    ofstream fout(WINS_FILE.c_str(), ios::trunc);
    fout << entry << "\n";
    for (int i = 0; i < c; i++) fout << previous[i] << "\n";
    fout.close();
}

void saveWinsCount() {
    ofstream fout(WINS_COUNT_FILE.c_str(), ios::trunc);
    if (!fout) return;
    fout << player1Wins << " " << player2Wins << "\n";
    fout.close();
}

void loadWinsCount() {
    ifstream fin(WINS_COUNT_FILE.c_str());
    if (!fin) { player1Wins = 0; player2Wins = 0; return; }
    fin >> player1Wins >> player2Wins;
    fin.close();
}

bool saveGameStateToFile() {
    string content = "";
    content += to_string(gameMode) + "\n";
    content += player1Name + "\n";
    content += player2Name + "\n";
    content += to_string(player1X) + " " + to_string(player1Y) + "\n";
    content += to_string(player2X) + " " + to_string(player2Y) + "\n";
    content += to_string(player1Reached) + " " + to_string(player2Reached) + "\n";
    content += to_string(countdownTicks) + "\n";
    content += to_string(startX) + " " + to_string(startY) + "\n";
    content += to_string(goalX) + " " + to_string(goalY) + "\n";
    for (int y = 0; y < MAZE_H; y++) {
        for (int x = 0; x < MAZE_W; x++) {
            content += to_string(maze[y][x]);
            if (x < MAZE_W - 1) content += " ";
        }
        content += "\n";
    }
    return atomicWriteReplace(SAVE_FILE, SAVE_TMP, content);
}

bool loadGameStateFromFile() {
    ifstream fin(SAVE_FILE.c_str());
    if (!fin) return false;
    int m;
    if (!(fin >> m)) { fin.close(); return false; }
    gameMode = m;
    fin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(fin, player1Name);
    getline(fin, player2Name);
    fin >> player1X >> player1Y >> player2X >> player2Y;
    int d1, d2; fin >> d1 >> d2; player1Reached = d1; player2Reached = d2;
    fin >> countdownTicks >> startX >> startY >> goalX >> goalY;
    for (int y = 0; y < MAZE_H; y++)
        for (int x = 0; x < MAZE_W; x++) fin >> maze[y][x];
    fin.close();
    return true;
}

void deleteSaveFile() { remove(SAVE_FILE.c_str()); }

bool saveFileExists() {
    ifstream f(SAVE_FILE.c_str());
    return f.good();
}

void resetWinCounters() { player1Wins = 0; player2Wins = 0; saveWinsCount(); }

// -------------------- DRAWING --------------------

void drawMenuScreen(sf::RenderWindow& window, const sf::Font& font, bool hasSave) {
    window.clear();
    // background image if loaded
    window.draw(menuBackgroundSprite);
    // dark overlay
    sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
    overlay.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(overlay);

    sf::Text title("MAZE RACE", font, 64);
    title.setPosition(WINDOW_W / 2 - title.getLocalBounds().width / 2, 80);
    window.draw(title);

    sf::Text hint("Press N = New | C = Continue | R = Reset Wins | ESC = Exit", font, 18);
    hint.setPosition(WINDOW_W / 2 - hint.getLocalBounds().width / 2, 150);
    window.draw(hint);

    sf::Text playersTxt("", font, 24);
    playersTxt.setPosition(40, 200);
    string s = "Player1: " + player1Name + " (" + to_string(player1Wins) + ")\nPlayer2: " + player2Name + " (" + to_string(player2Wins) + ")";
    playersTxt.setString(s);
    window.draw(playersTxt);

    sf::Text btnNew("Start New Game (N)", font, 36);
    btnNew.setPosition(WINDOW_W / 2 - btnNew.getLocalBounds().width / 2, 300);
    window.draw(btnNew);

    sf::Text btnContinue("Continue Saved Game (C)", font, 36);
    if (!hasSave) btnContinue.setFillColor(sf::Color(120, 120, 120));
    btnContinue.setPosition(WINDOW_W / 2 - btnContinue.getLocalBounds().width / 2, 360);
    window.draw(btnContinue);

    window.display();
}

void drawGameScreen(sf::RenderWindow& window, const sf::Font& font) {
    window.clear(sf::Color(10, 10, 30));

    sf::RectangleShape cellShape(sf::Vector2f((float)CELL_SIZE, (float)CELL_SIZE));
    for (int y = 0; y < MAZE_H; y++) {
        for (int x = 0; x < MAZE_W; x++) {
            if (maze[y][x] == 1) cellShape.setFillColor(sf::Color(40, 40, 60));
            else cellShape.setFillColor(sf::Color(120, 120, 160));
            cellShape.setPosition(x * CELL_SIZE, y * CELL_SIZE);
            window.draw(cellShape);
        }
    }
    // draw goal
    sf::RectangleShape goalShape(sf::Vector2f((float)CELL_SIZE, (float)CELL_SIZE));
    goalShape.setPosition(goalX * CELL_SIZE, goalY * CELL_SIZE);
    goalShape.setFillColor(sf::Color::Yellow);
    window.draw(goalShape);

    // players
    sf::CircleShape p1(CELL_SIZE * 0.45f); p1.setOrigin(p1.getRadius(), p1.getRadius());
    p1.setPosition(centerPixelX(player1X), centerPixelY(player1Y)); p1.setFillColor(sf::Color::Blue);
    window.draw(p1);

    sf::CircleShape p2(CELL_SIZE * 0.45f); p2.setOrigin(p2.getRadius(), p2.getRadius());
    p2.setPosition(centerPixelX(player2X), centerPixelY(player2Y)); p2.setFillColor(sf::Color::Red);
    window.draw(p2);

    // HUD
    sf::RectangleShape hud(sf::Vector2f(WINDOW_W, HUD_HEIGHT)); hud.setPosition(0, MAZE_H * CELL_SIZE); hud.setFillColor(sf::Color::Black);
    window.draw(hud);

    sf::Text info("", font, 20);
    if (gameMode == MODE_ENTER_P1) {
        info.setString("Enter Player 1: " + player1Name + "_"); info.setPosition(10, MAZE_H * CELL_SIZE + 20); window.draw(info); window.display(); return;
    }
    if (gameMode == MODE_ENTER_P2) {
        info.setString("Enter Player 2: " + player2Name + "_"); info.setPosition(10, MAZE_H * CELL_SIZE + 20); window.draw(info); window.display(); return;
    }
    if (gameMode == MODE_COUNTDOWN) {
        info.setString("Get Ready..."); info.setCharacterSize(40); info.setPosition(WINDOW_W / 2 - 80, WINDOW_H / 2 - 40); window.draw(info); window.display(); return;
    }
    if (gameMode == MODE_PLAYING) {
        info.setString(player1Name + " (WASD) vs " + player2Name + " (ARROWS)"); info.setPosition(10, MAZE_H * CELL_SIZE + 20); window.draw(info); window.display(); return;
    }
    if (gameMode == MODE_FINISHED) {
        string winner;
        if (player1Reached && player2Reached) winner = "It's a tie!";
        else if (player1Reached) winner = player1Name + " WINS!";
        else winner = player2Name + " WINS!";
        info.setString(winner + "\nPress SPACE to restart"); info.setCharacterSize(30); info.setPosition(WINDOW_W / 2 - 150, WINDOW_H / 2 - 40);
        window.draw(info); window.display(); return;
    }

    window.display();
}

// -------------------- MAIN --------------------
int main() {
    srand((unsigned int)time(nullptr));

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Maze Race - Simple");
    window.setFramerateLimit(60);

    // audio (optional) - if missing files just print message and continue
    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("assets/sounds/background.mp3")) {
        cout << "Warning: background music not found." << endl;
    }
    else { backgroundMusic.setLoop(true); backgroundMusic.setVolume(30); }

    sf::Music victoryMusic;
    if (!victoryMusic.openFromFile("assets/sounds/win.mp3")) {
        cout << "Warning: victory music not found." << endl;
    }
    else { victoryMusic.setVolume(50); }

    // menu background image (optional)
    if (!menuBackgroundTexture.loadFromFile("assets/textures/menu_bg.png")) {
        // leave sprite empty if image not found
    }
    else {
        menuBackgroundSprite.setTexture(menuBackgroundTexture);
        sf::Vector2u s = menuBackgroundTexture.getSize();
        float sx = (float)WINDOW_W / s.x; float sy = (float)WINDOW_H / s.y;
        menuBackgroundSprite.setScale(sx, sy);
    }

    // font - platform dependent paths; keep same approach as original
    sf::Font font;
#if defined(_WIN32)
    font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
#elif defined(APPLE)
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");
#else
    font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    loadWinsCount();
    bool inMenu = true;
    generateMazeSimple();

    sf::Clock autosaveClock; autosaveClock.restart();

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) { if (!inMenu) saveGameStateToFile(); window.close(); }

            if (inMenu) {
                bool hasSave = saveFileExists();
                if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::Escape) window.close();
                    if (e.key.code == sf::Keyboard::R) { resetWinCounters(); }

                    // New game
                    if (e.key.code == sf::Keyboard::N) {
                        deleteSaveFile();
                        generateMazeSimple();
                        player1Name = ""; player2Name = "";
                        player1X = startX; player1Y = startY; player2X = startX; player2Y = startY;
                        player1Reached = false; player2Reached = false; countdownTicks = 120;
                        gameMode = MODE_ENTER_P1; inMenu = false; autosaveClock.restart();
                        if (backgroundMusic.getStatus() == sf::SoundSource::Stopped) backgroundMusic.stop();
                    }

                    // Continue saved game
                    if (e.key.code == sf::Keyboard::C && hasSave) {
                        if (!loadGameStateFromFile()) { generateMazeSimple(); gameMode = MODE_ENTER_P1; countdownTicks = 120; }
                        inMenu = false; autosaveClock.restart();
                        if (gameMode == MODE_PLAYING || gameMode == MODE_COUNTDOWN) { if (backgroundMusic.getStatus() != sf::SoundSource::Playing) backgroundMusic.play(); }
                    }
                }
                drawMenuScreen(window, font, hasSave);
                continue; // skip rest of event processing
            }

            // TEXT ENTRY for player names
            if (gameMode == MODE_ENTER_P1 || gameMode == MODE_ENTER_P2) {
                if (e.type == sf::Event::TextEntered) {
                    uint32_t u = e.text.unicode;
                    string& ref = (gameMode == MODE_ENTER_P1) ? player1Name : player2Name;
                    if (u >= 32 && u < 127 && ref.size() < 12) ref.push_back((char)u);
                }
                if (e.type == sf::Event::KeyPressed) {
                    string& ref = (gameMode == MODE_ENTER_P1) ? player1Name : player2Name;
                    if (e.key.code == sf::Keyboard::BackSpace) { if (!ref.empty()) ref.pop_back(); }
                    else if (e.key.code == sf::Keyboard::Enter) {
                        if (!ref.empty()) {
                            if (gameMode == MODE_ENTER_P1) gameMode = MODE_ENTER_P2;
                            else {
                                // both names entered, start
                                generateMazeSimple();
                                player1X = startX; player1Y = startY; player2X = startX; player2Y = startY;
                                player1Reached = false; player2Reached = false; countdownTicks = 120;
                                gameMode = MODE_COUNTDOWN; if (backgroundMusic.getStatus() != sf::SoundSource::Playing) backgroundMusic.play();
                            }
                        }
                    }
                }
            }

            // PLAYER MOVEMENT when playing
            if (gameMode == MODE_PLAYING && e.type == sf::Event::KeyPressed) {
                bool flag1 = false, flag2 = false;
                int nx, ny;
                if (!player1Reached) {
                    nx = player1X; ny = player1Y;
                    if (e.key.code == sf::Keyboard::W) ny--;
                    if (e.key.code == sf::Keyboard::S) ny++;
                    if (e.key.code == sf::Keyboard::A) nx--;
                    if (e.key.code == sf::Keyboard::D) nx++;
                    if (nx >= 0 && nx < MAZE_W && ny >= 0 && ny < MAZE_H && maze[ny][nx] == 0) { player1X = nx; player1Y = ny; }
                    if (player1X == goalX && player1Y == goalY) { player1Reached = true; flag1 = true; }
                }
                if (!player2Reached) {
                    nx = player2X; ny = player2Y;
                    if (e.key.code == sf::Keyboard::Up) ny--;
                    if (e.key.code == sf::Keyboard::Down) ny++;
                    if (e.key.code == sf::Keyboard::Left) nx--;
                    if (e.key.code == sf::Keyboard::Right) nx++;
                    if (nx >= 0 && nx < MAZE_W && ny >= 0 && ny < MAZE_H && maze[ny][nx] == 0) { player2X = nx; player2Y = ny; }
                    if (player2X == goalX && player2Y == goalY) { player2Reached = true; flag2 = true; }
                }

                if (flag1 && flag2) {
                    gameMode = MODE_FINISHED; saveWinToHistory("Tie"); saveGameStateToFile();
                    backgroundMusic.stop(); if (victoryMusic.getStatus() != sf::SoundSource::Playing) victoryMusic.play();
                }
                else if (flag1) {
                    gameMode = MODE_FINISHED; saveWinToHistory(player1Name); player1Wins++; saveWinsCount(); saveGameStateToFile();
                    backgroundMusic.stop(); if (victoryMusic.getStatus() != sf::SoundSource::Playing) victoryMusic.play();
                }
                else if (flag2) {
                    gameMode = MODE_FINISHED; saveWinToHistory(player2Name); player2Wins++; saveWinsCount(); saveGameStateToFile();
                    backgroundMusic.stop(); if (victoryMusic.getStatus() != sf::SoundSource::Playing) victoryMusic.play();
                }
            }

            // Restart after finished
            if (gameMode == MODE_FINISHED && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Space) {
                deleteSaveFile(); player1Name = ""; player2Name = ""; gameMode = MODE_ENTER_P1; victoryMusic.stop();
            }
        }

        // Countdown logic outside event handling
        if (gameMode == MODE_COUNTDOWN) {
            countdownTicks--;
            if (countdownTicks <= 0) { countdownTicks = 120; gameMode = MODE_PLAYING; }
        }

        // Autosave every second
        if (!inMenu && autosaveClock.getElapsedTime().asSeconds() >= 1.0f) {
            saveGameStateToFile(); autosaveClock.restart();
        }

        // Draw current screen
        if (!inMenu) drawGameScreen(window, font);
        else drawMenuScreen(window, font, saveFileExists());
    }

    return 0;
}