#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <stack>
#include <queue> 
#include <algorithm>
#include <random>
#include <ctime>
#include <utility>
#include <sstream> 
#include <cmath> 
#include <limits> 
#include <functional> 

// --- Game Constants ---
const int CELL_SIZE = 32;
const int MAZE_CELL_W = 15;
const int MAZE_CELL_H = 10;
const int GRID_W = 2 * MAZE_CELL_W + 1;
const int GRID_H = 2 * MAZE_CELL_H + 1;

// Define a slightly larger window than the maze itself for a centered look
const int PADDING = 3 * CELL_SIZE; // Padding around the maze
const int WINDOW_WIDTH = (GRID_W * CELL_SIZE) + PADDING;
const int WINDOW_HEIGHT = (GRID_H * CELL_SIZE) + PADDING;

// Calculate offset to center the maze within the window
const int MAZE_OFFSET_X = PADDING / 2;
const int MAZE_OFFSET_Y = PADDING / 2;

// --- Game Settings (Unchanged) ---
const float GAME_TIME_LIMIT = 30.0f;
const float PLAYER_ANIM_SPEED = 0.15f;
const int PLAYER_FLASH_STATES = 2;

// --- Enums and State ---
enum GameState {
    TITLE_SCREEN,
    PLAYING,
    P1_WON,
    P2_WON,
    LOST_TIME
};

GameState current_state = TITLE_SCREEN;
long long player1_score = 0;
long long player2_score = 0;
int min_path_length = 0;

// --- Direction Arrays (Unchanged) ---
int dx[] = { 2, -2, 0, 0 };
int dy[] = { 0, 0, 2, -2 };

int move_dx[] = { 1, -1, 0, 0 };
int move_dy[] = { 0, 0, 1, -1 };


// Helper functions (Updated for centering)
sf::Vector2f gridToPixel(const sf::Vector2i& grid_pos, float radius) {
    float x = grid_pos.x * CELL_SIZE + CELL_SIZE / 2.0f - radius + MAZE_OFFSET_X;
    float y = grid_pos.y * CELL_SIZE + CELL_SIZE / 2.0f - radius + MAZE_OFFSET_Y;
    return sf::Vector2f(x, y);
}

int calculateMinPathLength(const std::vector<std::vector<int>>& maze,
    const sf::Vector2i& start, const sf::Vector2i& end) {

    if (maze[start.y][start.x] == 1 || maze[end.y][end.x] == 1) {
        return std::numeric_limits<int>::max();
    }

    std::queue<sf::Vector2i> q;
    q.push(start);

    std::vector<std::vector<int>> distance(GRID_H, std::vector<int>(GRID_W, -1));
    distance[start.y][start.x] = 0;

    while (!q.empty()) {
        sf::Vector2i current = q.front();
        q.pop();

        if (current == end) {
            return distance[current.y][current.x];
        }

        for (int i = 0; i < 4; ++i) {
            int nx = current.x + move_dx[i];
            int ny = current.y + move_dy[i];

            if (nx > 0 && ny > 0 && nx < GRID_W - 1 && ny < GRID_H - 1 &&
                maze[ny][nx] == 0 && distance[ny][nx] == -1) {

                distance[ny][nx] = distance[current.y][current.x] + 1;
                q.push({ nx, ny });
            }
        }
    }
    return std::numeric_limits<int>::max();
}


// --- Maze Generation Function (Unchanged) ---
void generateMaze(std::vector<std::vector<int>>& maze, std::default_random_engine& engine) {
    for (auto& row : maze) {
        std::fill(row.begin(), row.end(), 1);
    }
    std::stack<std::pair<int, int>> stk;
    int start_x = 1;
    int start_y = 1;

    stk.push({ start_x, start_y });
    maze[start_y][start_x] = 0;

    while (!stk.empty()) {
        auto [x, y] = stk.top();

        std::vector<int> dirs = { 0, 1, 2, 3 };
        std::shuffle(dirs.begin(), dirs.end(), engine);

        bool moved = false;

        for (int i : dirs) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            int wall_x = x + dx[i] / 2;
            int wall_y = y + dy[i] / 2;

            if (nx > 0 && ny > 0 && nx < GRID_W - 1 && ny < GRID_H - 1 && maze[ny][nx] == 1) {
                maze[wall_y][wall_x] = 0;
                maze[ny][nx] = 0;

                stk.push({ nx, ny });
                moved = true;
                break;
            }
        }

        if (!moved) {
            stk.pop();
        }
    }
}


// --- Main Game Logic ---
int main() {
    // 1. Core Setup
    // Window size reflects maze + padding, no extra UI bar
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2-Player Maze Race (WASD vs ARROWS)");
    window.setFramerateLimit(60);

    // Random engine
    unsigned seed = std::time(nullptr);
    std::default_random_engine engine(seed);

    // Global assets (Font loading via OS path) - REQUIRED FOR TEXT
    sf::Font font;
    std::string font_path = "";

#ifdef _WIN32
    font_path = "C:\\Windows\\Fonts\\arial.ttf";
#elif __APPLE__
    font_path = "/System/Library/Fonts/Supplemental/Arial.ttf";
#else
    font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif

    if (!font.loadFromFile(font_path)) {
        std::cerr << "FATAL: Could not load system font. Check console output for path.\n";
        return -1;
    }
    // ------------------------------------

    // Grid and Positions
    std::vector<std::vector<int>> maze(GRID_H, std::vector<int>(GRID_W, 1));
    sf::Vector2i start_pos;
    sf::Vector2i end_pos;

    // Player Entities
    sf::Vector2i p1_grid_pos;
    sf::Vector2i p2_grid_pos;

    const float PLAYER_RADIUS = CELL_SIZE / 2.5f;

    sf::CircleShape p1_shape(PLAYER_RADIUS);
    sf::CircleShape p2_shape(PLAYER_RADIUS);

    // Player 1 animation variables (Color flashing)
    sf::Clock anim_clock;
    int current_player_flash_state = 0;

    // Scoring and Time
    int p1_move_count = 0;
    int p2_move_count = 0;
    sf::Clock game_timer;
    float time_remaining = GAME_TIME_LIMIT;

    auto reset_game = [&]() {
        generateMaze(maze, engine);

        start_pos = { 1, 1 };
        end_pos = { GRID_W - 2, GRID_H - 2 };
        maze[start_pos.y][start_pos.x] = 0;
        maze[end_pos.y][end_pos.x] = 0;

        p1_grid_pos = start_pos;
        p2_grid_pos = start_pos;

        p1_shape.setPosition(gridToPixel(p1_grid_pos, PLAYER_RADIUS));
        p2_shape.setPosition(gridToPixel(p2_grid_pos, PLAYER_RADIUS));

        min_path_length = calculateMinPathLength(maze, start_pos, end_pos);

        player1_score = 0;
        player2_score = 0;
        p1_move_count = 0;
        p2_move_count = 0;
        time_remaining = GAME_TIME_LIMIT;
        game_timer.restart();
        current_state = PLAYING;
        };

    // Generate initial maze for title screen
    generateMaze(maze, engine);

    // 3. Game Loop
    while (window.isOpen()) {
        float dt = 0;
        if (current_state == PLAYING) {
            dt = game_timer.restart().asSeconds();
            time_remaining -= dt;

            // Player 1 "Animation" update (Color Flash)
            if (anim_clock.getElapsedTime().asSeconds() >= PLAYER_ANIM_SPEED) {
                current_player_flash_state = (current_player_flash_state + 1) % PLAYER_FLASH_STATES;
                if (current_player_flash_state == 0) {
                    p1_shape.setFillColor(sf::Color::Cyan); // Bright state
                }
                else {
                    p1_shape.setFillColor(sf::Color(0, 150, 150)); // Dim state
                }
                // P2 flash state
                if (current_player_flash_state == 0) {
                    p2_shape.setFillColor(sf::Color::Magenta); // Bright state
                }
                else {
                    p2_shape.setFillColor(sf::Color::Red); // Dim state
                }
                anim_clock.restart();
            }
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (current_state == TITLE_SCREEN || current_state == P1_WON || current_state == P2_WON || current_state == LOST_TIME) {
                    if (event.key.code == sf::Keyboard::Space) {
                        reset_game();
                    }
                }

                if (current_state == PLAYING) {

                    sf::Vector2i p1_new_pos = p1_grid_pos;
                    sf::Vector2i p2_new_pos = p2_grid_pos;
                    bool p1_moved = false;
                    bool p2_moved = false;

                    // --- Player 1 Input (WASD) ---
                    if (event.key.code == sf::Keyboard::W) p1_new_pos.y -= 1;
                    else if (event.key.code == sf::Keyboard::S) p1_new_pos.y += 1;
                    else if (event.key.code == sf::Keyboard::A) p1_new_pos.x -= 1;
                    else if (event.key.code == sf::Keyboard::D) p1_new_pos.x += 1;

                    // FIX: Moved bounds check before maze check
                    if (p1_new_pos != p1_grid_pos &&
                        p1_new_pos.x > 0 && p1_new_pos.y > 0 &&
                        p1_new_pos.x < GRID_W - 1 && p1_new_pos.y < GRID_H - 1 &&
                        maze[p1_new_pos.y][p1_new_pos.x] == 0) {

                        p1_grid_pos = p1_new_pos;
                        p1_moved = true;
                    }
                    if (p1_moved) {
                        p1_move_count++;
                        p1_shape.setPosition(gridToPixel(p1_grid_pos, PLAYER_RADIUS));
                    }


                    // --- Player 2 Input (Arrow Keys) ---
                    if (event.key.code == sf::Keyboard::Up) p2_new_pos.y -= 1;
                    else if (event.key.code == sf::Keyboard::Down) p2_new_pos.y += 1;
                    else if (event.key.code == sf::Keyboard::Left) p2_new_pos.x -= 1;
                    else if (event.key.code == sf::Keyboard::Right) p2_new_pos.x += 1;

                    // FIX: Moved bounds check before maze check
                    if (p2_new_pos != p2_grid_pos &&
                        p2_new_pos.x > 0 && p2_new_pos.y > 0 &&
                        p2_new_pos.x < GRID_W - 1 && p2_new_pos.y < GRID_H - 1 &&
                        maze[p2_new_pos.y][p2_new_pos.x] == 0) {

                        p2_grid_pos = p2_new_pos;
                        p2_moved = true;
                    }
                    if (p2_moved) {
                        p2_move_count++;
                        p2_shape.setPosition(gridToPixel(p2_grid_pos, PLAYER_RADIUS));
                    }
                }
            }
        } // End PollEvent loop

        // --- Update Logic (Only while Playing) ---
        if (current_state == PLAYING) {

            // 1. Time Check
            if (time_remaining <= 0.0f) {
                time_remaining = 0.0f;
                current_state = LOST_TIME;
            }

            // 2. Win Check
            auto calculate_score = [&](int moves) -> long long {
                long long base_score = 5000;

                // Correct floating point calculation
                long long time_bonus = static_cast<long long>(std::round(time_remaining * 50.0f));

                int path_difference = moves - min_path_length;
                long long efficiency_penalty = (path_difference > 0) ? (path_difference * 15) : 0;

                long long score = base_score + time_bonus - efficiency_penalty;
                return std::max(0LL, score);
                };

            if (p1_grid_pos == end_pos) {
                player1_score = calculate_score(p1_move_count);
                current_state = P1_WON;
            }
            else if (p2_grid_pos == end_pos) {
                player2_score = calculate_score(p2_move_count);
                current_state = P2_WON;
            }
        }

        // --- Drawing ---
        // Changed clear color to black for better contrast with the gray maze
        window.clear(sf::Color::Black);

        // Draw the Maze (Centered)
        sf::RectangleShape block(sf::Vector2f(CELL_SIZE, CELL_SIZE));
        for (int y = 0; y < GRID_H; ++y) {
            for (int x = 0; x < GRID_W; ++x) {
                block.setPosition(x * CELL_SIZE + MAZE_OFFSET_X, y * CELL_SIZE + MAZE_OFFSET_Y);

                if (maze[y][x] == 1) { // WALL (Dark Grey)
                    block.setFillColor(sf::Color(40, 40, 40));
                }
                else { // PATH / START / END
                    block.setFillColor(sf::Color(100, 100, 100)); // Path (Light Grey)

                    if (x == start_pos.x && y == start_pos.y) {
                        block.setFillColor(sf::Color(0, 150, 0)); // Start (Green)
                    }
                    else if (x == end_pos.x && y == end_pos.y) {
                        block.setFillColor(sf::Color(150, 0, 0)); // End (Red)
                    }
                }
                window.draw(block);
            }
        }

        // Draw Entities
        if (current_state != TITLE_SCREEN) {
            window.draw(p1_shape);
            window.draw(p2_shape);
        }

        // --- UI Overlays (Title, Win, Loss, and Info) ---
        sf::Text message_text;
        message_text.setFont(font);
        message_text.setFillColor(sf::Color::White);
        message_text.setStyle(sf::Text::Bold);

        // Score/Time Info (Drawn directly on the top black border)
        sf::Text info_text;
        info_text.setFont(font);
        info_text.setCharacterSize(18);
        info_text.setFillColor(sf::Color::Yellow);

        std::stringstream ss;
        ss << "Time: " << static_cast<int>(time_remaining) << "s | P1 Moves: " << p1_move_count
            << " | P2 Moves: " << p2_move_count << " | Optimal: " << min_path_length;


        info_text.setString(ss.str());
        // Positioned at the top center of the screen
        sf::FloatRect info_bounds = info_text.getLocalBounds();
        info_text.setOrigin(info_bounds.left + info_bounds.width / 2.0f, info_bounds.top + info_bounds.height / 2.0f);
        info_text.setPosition(WINDOW_WIDTH / 2.0f, MAZE_OFFSET_Y / 2.0f);
        window.draw(info_text);


        // Center Screen Messages (Title, Win, Loss)
        std::string line1 = "";
        std::string line2 = "Press SPACE to restart.";

        if (current_state == TITLE_SCREEN) {
            line1 = "MAZE RACE: WASD (Cyan) vs ARROWS (Red)";
            message_text.setCharacterSize(36);
            message_text.setFillColor(sf::Color::Yellow);
            line2 = "Race to the Exit! Press SPACE to start.";
        }
        else if (current_state == P1_WON) {
            line1 = "PLAYER 1 WINS! Score: " + std::to_string(player1_score);
            message_text.setCharacterSize(40);
            message_text.setFillColor(sf::Color::Cyan);
        }
        else if (current_state == P2_WON) {
            line1 = "PLAYER 2 WINS! Score: " + std::to_string(player2_score);
            message_text.setCharacterSize(40);
            message_text.setFillColor(sf::Color::Red);
        }
        else if (current_state == LOST_TIME) {
            line1 = "TIME UP! No winner.";
            message_text.setCharacterSize(40);
            message_text.setFillColor(sf::Color::Red);
        }

        if (current_state != PLAYING) {
            // Draw Main Message (Line 1)
            message_text.setString(line1);
            sf::FloatRect bounds1 = message_text.getLocalBounds();
            message_text.setOrigin(bounds1.left + bounds1.width / 2.0f, bounds1.top + bounds1.height / 2.0f);
            // Center the message vertically on the window
            message_text.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f - 30.0f);
            window.draw(message_text);

            // Draw Prompt (Line 2)
            message_text.setString(line2);
            message_text.setCharacterSize(24);
            sf::FloatRect bounds2 = message_text.getLocalBounds();
            message_text.setOrigin(bounds2.left + bounds2.width / 2.0f, bounds2.top + bounds2.height / 2.0f);
            message_text.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + 30.0f);
            window.draw(message_text);
        }

        window.display();
    }

    return 0;
}
