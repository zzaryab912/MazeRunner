#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <filesystem>

using namespace std;

// 
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

// Movement offsets up, down, left, right
int moveX[4] = { 0, 0, -2, 2 };
int moveY[4] = { -2, 2, 0, 0 };

// game constants
const int MODE_MENU = 10;
const int MODE_ENTER_P1 = 0;
const int MODE_ENTER_P2 = 1;
const int MODE_COUNTDOWN = 2;
const int MODE_PLAYING = 3;
const int MODE_FINISHED = 4;
const int MODE_PAUSED = 5;

// Current mode
int gameMode = MODE_MENU;

// Players
string player1Name = "";
string player2Name = "";
int player1X = 1, player1Y = 1;
int player2X = 1, player2Y = 1;
bool player1Reached = false, player2Reached = false;

// Start and goal
int startX = 1, startY = 1;
int goalX = MAZE_W - 2, goalY = MAZE_H - 2;

// Countdown counter 
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

void generateMazeSimple() {
    fillAllWithWalls();

    int x = startX;
    int y = startY;
    maze[y][x] = 0; // start position

    bool madeProgress;
    do {
        madeProgress = false;
        int dirs[4] = { 0,1,2,3 };
        shuffleArray(dirs, 4);

        for (int i = 0; i < 4; i++) {
            int d = dirs[i];
            int nx = x + moveX[d];
            int ny = y + moveY[d];

            if (insideBounds(nx, ny) && maze[ny][nx] == 1) {
                // knock down wall
                maze[y + moveY[d] / 2][x + moveX[d] / 2] = 0;
                maze[ny][nx] = 0;

                x = nx;
                y = ny;
                madeProgress = true;
                break; // take one step at a time
            }
        }

        //  scan for any unvisited neighbor and jump to continue
        if (!madeProgress) {
            for (int ty = 1; ty < MAZE_H - 1; ty += 2) {
                for (int tx = 1; tx < MAZE_W - 1; tx += 2) {
                    if (maze[ty][tx] == 0) {
                        bool hasWallNeighbor = false;
                        for (int d = 0; d < 4; d++) {
                            int nx = tx + moveX[d];
                            int ny = ty + moveY[d];
                            if (insideBounds(nx, ny) && maze[ny][nx] == 1) {
                                hasWallNeighbor = true;
                                break;
                            }
                        }
                        if (hasWallNeighbor) {
                            x = tx; y = ty;
                            madeProgress = true;
                            break;
                        }
                    }
                }
                if (madeProgress) break;
            }
        }

    } while (madeProgress);

    maze[startY][startX] = 0;
    maze[goalY][goalX] = 0;
}

// center helpers for rendering
float centerPixelX(int gridX) { return gridX * CELL_SIZE + CELL_SIZE / 2.0f; }
float centerPixelY(int gridY) { return gridY * CELL_SIZE + CELL_SIZE / 2.0f; }

// -------------------- FILE & SAVE HELPERS --------------------
bool atomicWriteReplace(const string& filename, const string& tempname, const string& data) {
    // write temp file
    ofstream fout(tempname, ios::trunc);
    if (!fout) return false;
    fout << data;
    fout.close();

    // remove original file if exists
    remove(filename.c_str());

    // rename temp to original
    if (rename(tempname.c_str(), filename.c_str()) != 0) {
        remove(tempname.c_str());
        return false;
    }
    return true;
}

void saveWinToHistory(const string& winner) {
    string previous[MAX_WINS_TO_STORE];
    int count = 0;
    ifstream fin(WINS_FILE);
    string line;
    while (getline(fin, line) && count < MAX_WINS_TO_STORE) {
        if (!line.empty()) previous[count++] = line;
    }
    fin.close();

    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    string entry = winner + " at " + buf;

    ofstream fout(WINS_FILE, ios::trunc);
    fout << entry << "\n";
    for (int i = 0; i < count; i++) fout << previous[i] << "\n";
}

void saveWinsCount() {
    ofstream fout(WINS_COUNT_FILE, ios::trunc);
    if (!fout) return;
    fout << player1Wins << " " << player2Wins << "\n";
}

void loadWinsCount() {
    ifstream fin(WINS_COUNT_FILE);
    if (!fin) { player1Wins = 0; player2Wins = 0; return; }
    fin >> player1Wins >> player2Wins;
}

bool saveGameStateToFile() {
    string content;
    content += to_string(gameMode) + "\n";
    content += player1Name + "\n" + player2Name + "\n";
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
    ifstream fin(SAVE_FILE);
    if (!fin) return false;

    fin >> gameMode;
    fin.ignore(); 
    getline(fin, player1Name);
    getline(fin, player2Name);
    fin >> player1X >> player1Y >> player2X >> player2Y;
    int d1, d2;
    fin >> d1 >> d2;
    player1Reached = (d1 != 0);
    player2Reached = (d2 != 0);
    fin >> countdownTicks >> startX >> startY >> goalX >> goalY;

    for (int y = 0; y < MAZE_H; y++)
        for (int x = 0; x < MAZE_W; x++) fin >> maze[y][x];

    return true;
}

void deleteSaveFile() {
    error_code ec;
    filesystem::remove(SAVE_FILE, ec);
}

bool saveFileExists() {
    return filesystem::exists(SAVE_FILE);
}

void resetWinCounters() { player1Wins = 0; player2Wins = 0; saveWinsCount(); }

// -------------------- DRAWING --------------------
void drawMenuScreen(sf::RenderWindow& window, const sf::Font& font, bool hasSave) {
    window.clear();
    // Draw background sprite if loaded
    if (menuBackgroundTexture.getSize().x > 0) window.draw(menuBackgroundSprite);

    sf::RectangleShape overlay(sf::Vector2f((float)WINDOW_W, (float)WINDOW_H));
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

    sf::RectangleShape goalShape(sf::Vector2f((float)CELL_SIZE, (float)CELL_SIZE));
    goalShape.setPosition(goalX * CELL_SIZE, goalY * CELL_SIZE);
    goalShape.setFillColor(sf::Color::Yellow);
    window.draw(goalShape);

    sf::CircleShape p1(CELL_SIZE * 0.45f); p1.setOrigin(p1.getRadius(), p1.getRadius());
    p1.setPosition(centerPixelX(player1X), centerPixelY(player1Y)); p1.setFillColor(sf::Color::Blue);
    window.draw(p1);

    sf::CircleShape p2(CELL_SIZE * 0.45f); p2.setOrigin(p2.getRadius(), p2.getRadius());
    p2.setPosition(centerPixelX(player2X), centerPixelY(player2Y)); p2.setFillColor(sf::Color::Red);
    window.draw(p2);

    sf::RectangleShape hud(sf::Vector2f((float)WINDOW_W, (float)HUD_HEIGHT)); hud.setPosition(0, MAZE_H * CELL_SIZE); hud.setFillColor(sf::Color::Black);
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
        info.setString(player1Name + " (WASD) vs " + player2Name + " (ARROWS)  |  Press P to Pause"); info.setPosition(10, MAZE_H * CELL_SIZE + 20); window.draw(info); window.display(); return;
    }
    if (gameMode == MODE_PAUSED) {
        info.setString("PAUSED\nPress P to resume"); info.setCharacterSize(40); info.setPosition(WINDOW_W / 2 - 120, WINDOW_H / 2 - 40); window.draw(info); window.display(); return;
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

    if (!menuBackgroundTexture.loadFromFile("assets/textures/menu_bg.png")) {
        // optional: it's okay if missing
    }
    else {
        menuBackgroundSprite.setTexture(menuBackgroundTexture);
        sf::Vector2u s = menuBackgroundTexture.getSize();
        float sx = (float)WINDOW_W / s.x; float sy = (float)WINDOW_H / s.y;
        menuBackgroundSprite.setScale(sx, sy);
    }

    sf::Font font;
#if defined(_WIN32)
    if (!font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
        cout << "Warning: failed to load font from C:\\Windows\\Fonts\\arial.ttf" << endl;
    }
#elif defined(_APPLE_)
    if (!font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        cout << "Warning: failed to load system font on macOS." << endl;
    }
#else
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        cout << "Warning: failed to load DejaVuSans.ttf; ensure font path is correct." << endl;
    }
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
                        // background music will start when countdown begins
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

            // PAUSE toggle (works when playing or during countdown)
            if ((gameMode == MODE_PLAYING || gameMode == MODE_COUNTDOWN) && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::P) {
                if (gameMode == MODE_PLAYING || gameMode == MODE_COUNTDOWN) {
                    // go to paused
                    gameMode = MODE_PAUSED;
                    if (backgroundMusic.getStatus() == sf::SoundSource::Playing) backgroundMusic.pause();
                }
            }
            else if (gameMode == MODE_PAUSED && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::P) {
                // resume to previous playing state (resume as PLAYING)
                gameMode = MODE_PLAYING;
                if (backgroundMusic.getStatus() != sf::SoundSource::Playing) backgroundMusic.play();
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
                    if (backgroundMusic.getStatus() == sf::SoundSource::Playing) backgroundMusic.stop();
                    if (victoryMusic.getStatus() != sf::SoundSource::Playing) victoryMusic.play();
                }
                else if (flag1) {
                    gameMode = MODE_FINISHED; saveWinToHistory(player1Name); player1Wins++; saveWinsCount(); saveGameStateToFile();
                    if (backgroundMusic.getStatus() == sf::SoundSource::Playing) backgroundMusic.stop();
                    if (victoryMusic.getStatus() != sf::SoundSource::Playing) victoryMusic.play();
                }
                else if (flag2) {
                    gameMode = MODE_FINISHED; saveWinToHistory(player2Name); player2Wins++; saveWinsCount(); saveGameStateToFile();
                    if (backgroundMusic.getStatus() == sf::SoundSource::Playing) backgroundMusic.stop();
                    if (victoryMusic.getStatus() != sf::SoundSource::Playing) victoryMusic.play();
                }
            }

            // Restart after finished
            if (gameMode == MODE_FINISHED && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Space) {
                deleteSaveFile(); player1Name = ""; player2Name = ""; gameMode = MODE_ENTER_P1; victoryMusic.stop();
            }
        }

        // Countdown logic outside event handling (only if not paused)
        if (gameMode == MODE_COUNTDOWN) {
            // if still in countdown (and not paused), decrement
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