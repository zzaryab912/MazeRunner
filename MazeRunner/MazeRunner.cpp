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
#include <cstring>

using namespace std;

// -------- CONSTANTS --------
const int CELL = 24;
const int W = 31;
const int H = 31;
const int HUD_HEIGHT = 70;
const int WINDOW_WIDTH = W * CELL;
const int WINDOW_HEIGHT = H * CELL + HUD_HEIGHT;

// Files
const string SAVE_FILE = "savegame.txt";
const string SAVE_TMP = "savegame.tmp";
const string WINS_FILE = "winhistory.txt";
const int MAX_WINS = 3;
const string WINS_COUNT_FILE = "wins_count.txt";

// Maze array
int maze[H][W];

// Directions
int dx[4] = { 0, 0, -2, 2 };
int dy[4] = { -2, 2, 0, 0 };

// Game state
enum GameState { MENU = 10, ENTER_P1 = 0, ENTER_P2 = 1, COUNTDOWN = 2, PLAYING = 3, FINISHED = 4 };
GameState state = MENU;

// Player info
string p1 = "", p2 = "";
int p1x = 1, p1y = 1;
int p2x = 1, p2y = 1;
bool p1_done = false, p2_done = false;

// Start & goal
int startX = 1, startY = 1;
int goalX = W - 2, goalY = H - 2;

// Countdown
int countdown = 120;

// Wins
int p1Wins = 0, p2Wins = 0;

// ---- NEW: Menu background ----
sf::Texture menuBgTexture;
sf::Sprite menuBgSprite;

// -------- Maze Functions --------
void fillMaze() {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            maze[y][x] = 1;
}
bool isInside(int x, int y) { return x > 0 && x < W - 1 && y > 0 && y < H - 1; }

void shuffleDirs(int arr[4]) {
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void generateMaze() {
    fillMaze();
    int stack[H * W][2];
    int top = 0;

    stack[top][0] = startX; stack[top][1] = startY; top++;
    maze[startY][startX] = 0;

    while (top > 0) {
        int curX = stack[top - 1][0];
        int curY = stack[top - 1][1];
        top--;

        int dirs[4] = { 0,1,2,3 };
        shuffleDirs(dirs);

        for (int i = 0; i < 4; i++) {
            int dir = dirs[i];
            int nx = curX + dx[dir];
            int ny = curY + dy[dir];
            if (isInside(nx, ny) && maze[ny][nx] == 1) {
                maze[curY + dy[dir] / 2][curX + dx[dir] / 2] = 0;
                maze[ny][nx] = 0;
                stack[top][0] = nx; stack[top][1] = ny; top++;
            }
        }
    }

    maze[startY][startX] = 0;
    maze[goalY][goalX] = 0;
}

float centerX(int gx) { return gx * CELL + CELL / 2; }
float centerY(int gy) { return gy * CELL + CELL / 2; }

// -------- SAVE/LOAD -------- (unchanged)
void saveWin(const string& winner) {
    string wins[MAX_WINS];
    int count = 0;

    ifstream fin(WINS_FILE);
    string line;
    while (getline(fin, line) && count < MAX_WINS - 1) {
        if (!line.empty()) { wins[count] = line; count++; }
    }
    fin.close();

    time_t now = time(nullptr);
    char buf[64];
    struct tm* tm_info = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    string entry = winner + " at " + buf;

    ofstream fout(WINS_FILE, ios::trunc);
    fout << entry << "\n";
    for (int i = 0; i < count; ++i) fout << wins[i] << "\n";
    fout.close();
}

bool writeSaveTempAndReplace(const string& content) {
    ofstream fout(SAVE_TMP, ios::trunc);
    if (!fout) return false;
    fout << content;
    fout.close();

    std::remove(SAVE_FILE.c_str());
    if (std::rename(SAVE_TMP.c_str(), SAVE_FILE.c_str()) != 0) {
        std::remove(SAVE_TMP.c_str());
        return false;
    }
    return true;
}

void saveGameState() {
    string content;
    content += to_string((int)state) + "\n";
    content += p1 + "\n" + p2 + "\n";
    content += to_string(p1x) + " " + to_string(p1y) + "\n";
    content += to_string(p2x) + " " + to_string(p2y) + "\n";
    content += to_string(p1_done) + " " + to_string(p2_done) + "\n";
    content += to_string(countdown) + "\n";
    content += to_string(startX) + " " + to_string(startY) + "\n";
    content += to_string(goalX) + " " + to_string(goalY) + "\n";

    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            content += to_string(maze[y][x]) + (x < W - 1 ? " " : "\n");

    writeSaveTempAndReplace(content);
}

bool loadGameState() {
    ifstream fin(SAVE_FILE);
    if (!fin) return false;

    int st;
    if (!(fin >> st)) return false;
    state = (GameState)st;

    fin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(fin, p1);
    getline(fin, p2);

    fin >> p1x >> p1y >> p2x >> p2y;
    int d1, d2;
    fin >> d1 >> d2;
    p1_done = d1; p2_done = d2;

    fin >> countdown >> startX >> startY >> goalX >> goalY;

    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            fin >> maze[y][x];

    return true;
}

void deleteSave() { remove(SAVE_FILE.c_str()); }
bool saveExists() { ifstream f(SAVE_FILE); return f.good(); }

void loadWins() {
    ifstream fin(WINS_COUNT_FILE);
    if (!fin) { p1Wins = p2Wins = 0; return; }
    fin >> p1Wins >> p2Wins;
}
void saveWins() {
    ofstream fout(WINS_COUNT_FILE, ios::trunc);
    fout << p1Wins << " " << p2Wins;
}
void resetWins() { p1Wins = p2Wins = 0; saveWins(); }

// -------- DRAW MENU WITH BACKGROUND --------
void drawMenu(sf::RenderWindow& win, const sf::Font& font, bool hasSave) {
    win.clear();
    win.draw(menuBgSprite);

    // Optional: dark overlay
    sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    overlay.setFillColor(sf::Color(0, 0, 0, 120));
    win.draw(overlay);

    sf::Text title("MAZE RACE", font, 64);
    title.setPosition(WINDOW_WIDTH / 2 - title.getLocalBounds().width / 2, 80);
    win.draw(title);

    sf::Text sub("Press N for New Game | C to Continue | R Reset Wins | ESC Exit", font, 18);
    sub.setPosition(WINDOW_WIDTH / 2 - sub.getLocalBounds().width / 2, 150);
    win.draw(sub);

    sf::Text players("", font, 24);
    players.setPosition(WINDOW_WIDTH / 2 - 150, 200);
    players.setString("Player 1: " + p1 + " (" + to_string(p1Wins) + " wins)\nPlayer 2: " + p2 + " (" + to_string(p2Wins) + " wins)");
    win.draw(players);

    sf::Text btnNew("Start New Game (N)", font, 36);
    btnNew.setPosition(WINDOW_WIDTH / 2 - btnNew.getLocalBounds().width / 2, 300);
    win.draw(btnNew);

    sf::Text btnContinue("Continue Saved Game (C)", font, 36);
    btnContinue.setFillColor(hasSave ? sf::Color::White : sf::Color(120, 120, 120));
    btnContinue.setPosition(WINDOW_WIDTH / 2 - btnContinue.getLocalBounds().width / 2, 380);
    win.draw(btnContinue);

    win.display();
}

// -------- DRAW GAME (unchanged) --------
void drawAll(sf::RenderWindow& win, const sf::Font& font) {
    win.clear(sf::Color(10, 10, 30));

    sf::RectangleShape cell(sf::Vector2f((float)CELL, (float)CELL));
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            cell.setFillColor(maze[y][x] ? sf::Color(40, 40, 60) : sf::Color(120, 120, 160));
            cell.setPosition(x * CELL, y * CELL);
            win.draw(cell);
        }

    sf::RectangleShape goal(sf::Vector2f((float)CELL, (float)CELL));
    goal.setPosition(goalX * CELL, goalY * CELL);
    goal.setFillColor(sf::Color::Yellow);
    win.draw(goal);

    sf::CircleShape c1(CELL * 0.45f); c1.setOrigin(c1.getRadius(), c1.getRadius());
    c1.setPosition(centerX(p1x), centerY(p1y)); c1.setFillColor(sf::Color::Blue);
    win.draw(c1);

    sf::CircleShape c2(CELL * 0.45f); c2.setOrigin(c2.getRadius(), c2.getRadius());
    c2.setPosition(centerX(p2x), centerY(p2y)); c2.setFillColor(sf::Color::Red);
    win.draw(c2);

    sf::RectangleShape hud(sf::Vector2f(WINDOW_WIDTH, HUD_HEIGHT));
    hud.setPosition(0, H * CELL);
    hud.setFillColor(sf::Color::Black);
    win.draw(hud);

    sf::Text t("", font, 20);

    if (state == ENTER_P1) {
        t.setString("Enter Player 1: " + p1 + "_");
        t.setPosition(10, H * CELL + 20);
        win.draw(t); return;
    }
    if (state == ENTER_P2) {
        t.setString("Enter Player 2: " + p2 + "_");
        t.setPosition(10, H * CELL + 20);
        win.draw(t); return;
    }
    if (state == COUNTDOWN) {
        t.setString("Get Ready...");
        t.setPosition(WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 - 40);
        t.setCharacterSize(40);
        win.draw(t); return;
    }
    if (state == PLAYING) {
        t.setString(p1 + " (WASD) vs " + p2 + " (ARROWS)");
        t.setPosition(10, H * CELL + 20);
        win.draw(t); return;
    }
    if (state == FINISHED) {
        string winner = (p1_done && p2_done) ? "It's a tie!"
            : (p1_done ? p1 + " WINS!" : p2 + " WINS!");
        t.setString(winner + "\nPress SPACE to restart");
        t.setCharacterSize(30);
        t.setPosition(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 40);
        win.draw(t);
    }
}

// -------- MAIN --------
int main() {
    srand((unsigned int)time(nullptr));

    sf::RenderWindow win(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Maze Race - Persistent");
    win.setFramerateLimit(60);

    // --- AUDIO SETUP ---
    sf::Music bgMusic;
    if (!bgMusic.openFromFile("assets/sounds/background.mp3")) {
        cout << "Error: assets/sounds/background.mp3 not found!" << endl;
    }
    bgMusic.setLoop(true); // Loop the background track
    bgMusic.setVolume(30); // 30% Volume

    sf::Music winMusic;
    if (!winMusic.openFromFile("assets/sounds/win.mp3")) {
        cout << "Error: assets/sounds/win.mp3 not found!" << endl;
    }
    winMusic.setVolume(50);
    // -------------------

    // --- BACKGROUND IMAGE SETUP ---
    // 1. Load the file
    if (!menuBgTexture.loadFromFile("assets/textures/menu_bg.png")) {
        std::cout << "Error: assets/textures/menu_bg.png not found!" << std::endl;
    }
    else {
        // 2. Apply texture to sprite
        menuBgSprite.setTexture(menuBgTexture);

        // 3. Auto-Scaling Logic (Forces image to fit window)
        sf::Vector2u imgSize = menuBgTexture.getSize();
        float scaleX = (float)WINDOW_WIDTH / imgSize.x;
        float scaleY = (float)WINDOW_HEIGHT / imgSize.y;
        menuBgSprite.setScale(scaleX, scaleY);
    }
    // ------------------------------

    sf::Font font;
    // (Keep your font loading logic here exactly as it was)
#if defined(_WIN32)
    font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
#elif defined(APPLE)
    font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");
#else
    font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    loadWins();

    bool inMenu = true;
    generateMaze();
    sf::Clock autosaveClock; autosaveClock.restart();

    while (win.isOpen()) {
        sf::Event e;
        while (win.pollEvent(e)) {
            if (e.type == sf::Event::Closed) { if (!inMenu) saveGameState(); win.close(); }

            if (inMenu) {
                bool hasSave = saveExists();
                if (e.type == sf::Event::KeyPressed) {
                    if (e.key.code == sf::Keyboard::Escape) win.close();
                    if (e.key.code == sf::Keyboard::R) resetWins();

                    // START NEW GAME
                    if (e.key.code == sf::Keyboard::N) {
                        deleteSave();
                        generateMaze();
                        p1 = ""; p2 = "";
                        p1x = startX; p1y = startY;
                        p2x = startX; p2y = startY;
                        p1_done = p2_done = false;
                        countdown = 120;
                        state = ENTER_P1;
                        inMenu = false;
                        autosaveClock.restart();

                        // --- AUDIO: Ensure silence during setup ---
                        bgMusic.stop();
                        winMusic.stop();
                    }

                    // CONTINUE GAME
                    if (e.key.code == sf::Keyboard::C && hasSave) {
                        if (!loadGameState()) { generateMaze(); state = ENTER_P1; countdown = 120; }
                        inMenu = false;
                        autosaveClock.restart();

                        // --- AUDIO: Resume music if game is active ---
                        if (state == PLAYING || state == COUNTDOWN) {
                            if (bgMusic.getStatus() != sf::SoundSource::Playing)
                                bgMusic.play();
                        }
                    }
                }
                drawMenu(win, font, hasSave);
                continue;
            }

            if (state == ENTER_P1 || state == ENTER_P2) {
                if (e.type == sf::Event::TextEntered) {
                    uint32_t u = e.text.unicode;
                    string& ref = (state == ENTER_P1) ? p1 : p2;
                    if (u >= 32 && u < 127 && ref.size() < 12)
                        ref.push_back((char)u);
                }
                if (e.type == sf::Event::KeyPressed) {
                    string& ref = (state == ENTER_P1) ? p1 : p2;
                    if (e.key.code == sf::Keyboard::BackSpace) {
                        if (!ref.empty()) ref.pop_back();
                    }
                    else if (e.key.code == sf::Keyboard::Enter) {
                        if (!ref.empty()) {
                            if (state == ENTER_P1) state = ENTER_P2;
                            else {
                                // BOTH NAMES ENTERED -> START GAME
                                generateMaze();
                                p1x = startX; p1y = startY;
                                p2x = startX; p2y = startY;
                                p1_done = p2_done = false;
                                countdown = 120;
                                state = COUNTDOWN;

                                // --- AUDIO: START THE HYPE ---
                                bgMusic.play();
                            }
                        }
                    }
                }
            }

            if (state == PLAYING && e.type == sf::Event::KeyPressed) {
                bool f1 = false, f2 = false;
                int nx, ny;

                if (!p1_done) {
                    nx = p1x; ny = p1y;
                    if (e.key.code == sf::Keyboard::W) ny--;
                    if (e.key.code == sf::Keyboard::S) ny++;
                    if (e.key.code == sf::Keyboard::A) nx--;
                    if (e.key.code == sf::Keyboard::D) nx++;
                    if (maze[ny][nx] == 0) { p1x = nx; p1y = ny; }
                    if (p1x == goalX && p1y == goalY) { p1_done = true; f1 = true; }
                }

                if (!p2_done) {
                    nx = p2x; ny = p2y;
                    if (e.key.code == sf::Keyboard::Up) ny--;
                    if (e.key.code == sf::Keyboard::Down) ny++;
                    if (e.key.code == sf::Keyboard::Left) nx--;
                    if (e.key.code == sf::Keyboard::Right) nx++;
                    if (maze[ny][nx] == 0) { p2x = nx; p2y = ny; }
                    if (p2x == goalX && p2y == goalY) { p2_done = true; f2 = true; }
                }

                if (f1 && f2) { state = FINISHED; saveWin("Tie"); saveGameState(); }
                else if (f1) { state = FINISHED; saveWin(p1); p1Wins++; saveWins(); saveGameState(); }
                else if (f2) { state = FINISHED; saveWin(p2); p2Wins++; saveWins(); saveGameState(); }

                // --- AUDIO: GAME OVER ---
                if (state == FINISHED) {
                    bgMusic.stop();  // Cut the music
                    winMusic.play(); // Play victory sound
                }
            }

            if (state == FINISHED && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Space) {
                deleteSave();
                p1 = ""; p2 = "";
                state = ENTER_P1;

                // --- AUDIO: RESET ---
                winMusic.stop();
            }
        }

        if (state == COUNTDOWN) {
            countdown--;
            if (countdown <= 0) { countdown = 120; state = PLAYING; }
        }

        if (!inMenu && autosaveClock.getElapsedTime().asSeconds() >= 1.0f) {
            saveGameState();
            autosaveClock.restart();
        }

        if (!inMenu) drawAll(win, font);
        win.display();
    }
    //testinghi2
    return 0;
}