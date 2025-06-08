#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System/Clock.hpp>
#include <fstream>
#include <string>

using namespace std;

// Game constants with improved naming
const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 960;
const int TILE_SIZE = 32;
const int GRID_COLS = SCREEN_WIDTH / TILE_SIZE;
const int GRID_ROWS = SCREEN_HEIGHT / TILE_SIZE;
const int MAX_PLAYER_ROWS = 5; // Bottom 5 rows
const int NUM_MUSHROOMS = 30;
const int CENTIPEDE_LENGTH = 12;
const float BULLET_SPEED = 10.0f;
const float PLAYER_SPEED = 4.0f;

// Game grid
const int MAX_GROUPS = 10; // Maximum number of centipede groups
int centipedeGroup[CENTIPEDE_LENGTH] = {0}; // Tracks which group each segment belongs to
bool groupMoveLeft[MAX_GROUPS] = {true}; // Movement direction for each group
int groupCount = 1; // Number of active centipede groups

int gameGrid[GRID_COLS][GRID_ROWS] = {};

// Constants for array indices
const int X = 0;
const int Y = 1;
const int EXISTS = 2;
const int DAMAGE = 3;  // For mushrooms
const int IS_POISONOUS = 4;

// Game objects
int mushroomGrid[NUM_MUSHROOMS][5];
int centipedeGrid[CENTIPEDE_LENGTH][3];
float bullet[3] = {};
float player[2] = {};
bool groupReentering[MAX_GROUPS] = {false}; // Tracks if group is re-entering
float reentryOffset[CENTIPEDE_LENGTH] = {0}; // X-offset for each segment during re-entry

// Game state
int score = 0;
bool gameOver = false;
bool gamePaused = false;
bool moveLeft = true;  // Direction for centipede
bool playerWon = false; // Tracks win condition

const int MENU_STATE = 0;
const int GAME_STATE = 1;
const int HIGH_SCORE_STATE = 2;
const int GAME_OVER_STATE = 3;
int currentGameState = MENU_STATE;

// Menu options
const int MENU_PLAY = 0;
const int MENU_HIGH_SCORES = 1;
const int MENU_EXIT = 2;
const int GAME_OVER_MAIN_MENU = 0;
const int GAME_OVER_RESTART = 1;
const int GAME_OVER_LEADERBOARD = 2;
int selectedMenuItem = MENU_PLAY;

const int MAX_HIGH_SCORES = 5;
int highScores[MAX_HIGH_SCORES] = {0};
const char* HIGH_SCORE_FILE = "highscores.txt";
bool groupInPlayerArea[MAX_GROUPS] = {false}; // Tracks if group is in player area
sf::Clock headSpawnClock; // Timer for spawning new heads
const float HEAD_SPAWN_INTERVAL = 5.0f; // Seconds between head spawns

// Function declarations
bool loadResources(sf::Texture& backgroundTexture, sf::Texture& mushroomTexture,
                  sf::Texture& poisonMushroomTexture, sf::Texture& centipedeTexture,
                  sf::Texture& headTexture, sf::Texture& playerTexture, sf::Texture& bulletTexture,
                  sf::Font& font, sf::Music& bgMusic);
void drawMushrooms(sf::RenderWindow& window, sf::Sprite& mushroomSprite, 
                  sf::Sprite& poisonMushroomSprite);
bool isSpaceKeyPressed(sf::RenderWindow& window);
void initializeGame();
void initializeMushrooms();
void initializeCentipede();
void drawCentipede(sf::RenderWindow& window, sf::Sprite& centipedeSprite, sf::Sprite& headSprite);
void moveCentipede(float deltaTime);
void checkCentipedeMushroomCollisions();
bool checkCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2);
void checkBulletMushroomCollisions();
bool checkPlayerCentipedeCollision();
void moveBullet(float deltaTime);
void drawBullet(sf::RenderWindow& window, sf::Sprite& bulletSprite);
void drawPlayer(sf::RenderWindow& window, sf::Sprite& playerSprite);
void checkBulletCentipedeCollisions();
void handleInput(float deltaTime, sf::RenderWindow& window);
void resetGame();
void renderScore(sf::RenderWindow& window, sf::Text& scoreText);
void drawMenu(sf::RenderWindow& window, sf::Font& font);
void handleMenuInput(sf::Event& event, sf::RenderWindow& window);
void saveHighScores();
void loadHighScores();
void updateHighScores(int newScore);
void drawHighScores(sf::RenderWindow& window, sf::Font& font);
void splitCentipede(int hitSegmentIndex);
void spawnNewHead();
void drawGameOverMenu(sf::RenderWindow& window, sf::Font& font);

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Centipede", sf::Style::Close);
    window.setSize(sf::Vector2u(640, 640));
    window.setPosition(sf::Vector2i(100, 0));
    
    // Resources
    sf::Music bgMusic;
    sf::Font font;
    sf::Texture backgroundTexture, mushroomTexture, poisonMushroomTexture;
    sf::Texture centipedeTexture, headTexture, playerTexture, bulletTexture;
    
    // Load resources
    if (!loadResources(backgroundTexture, mushroomTexture, poisonMushroomTexture,
                       centipedeTexture, headTexture, playerTexture, bulletTexture,
                       font, bgMusic)) {
        return -1;
    }
    
    // Validate texture dimensions
    if (mushroomTexture.getSize().x < 128 || mushroomTexture.getSize().y < 32) {
        std::cerr << "Warning: mushroom.png must be at least 128x32 pixels for four damage states!" << std::endl;
    }
    if (poisonMushroomTexture.getSize().x < 128 || poisonMushroomTexture.getSize().y < 32) {
        std::cerr << "Warning: poison_mushroom.png must be at least 128x32 pixels for four damage states!" << std::endl;
    }
    
    // Create sprites
    sf::Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setColor(sf::Color(255, 255, 255, 255 * 0.40));
    
    sf::Sprite mushroomSprite(mushroomTexture);
    mushroomSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite poisonMushroomSprite(poisonMushroomTexture);
    poisonMushroomSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite centipedeSprite(centipedeTexture);
    centipedeSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite headSprite(headTexture);
    headSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite playerSprite(playerTexture);
    playerSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite bulletSprite(bulletTexture);
    bulletSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(31); // Increased by 30% from 24
    scoreText.setFillColor(sf::Color::Green);
    scoreText.setPosition(10, 10);
    
    initializeGame();
    
    sf::Clock gameClock;
    float deltaTime = 0.0f;
    sf::Clock centipedeClock;
    
    loadHighScores();

    while (window.isOpen() && !gameOver) {
        deltaTime = gameClock.restart().asSeconds();
        
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (currentGameState == MENU_STATE || currentGameState == HIGH_SCORE_STATE || currentGameState == GAME_OVER_STATE) {
                handleMenuInput(event, window);
            }
            if (currentGameState == GAME_STATE && event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::P) {
                gamePaused = !gamePaused;
            }
            if (currentGameState == GAME_STATE && event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::Escape) {
                updateHighScores(score);
                currentGameState = GAME_OVER_STATE;
                playerWon = false; // Reset win condition
            }
        }
        window.clear();
        window.draw(backgroundSprite);
        
        switch (currentGameState) {
            case MENU_STATE:
                drawMenu(window, font);
                break;
                
            case HIGH_SCORE_STATE:
                drawHighScores(window, font);
                break;
                
            case GAME_OVER_STATE:
                drawGameOverMenu(window, font);
                break;
                
            case GAME_STATE:
                sf::Time deltaTime = gameClock.restart();
                float dt = deltaTime.asSeconds();
                
                if (!gamePaused) {
                    handleInput(dt, window);
                    
                    if (centipedeClock.getElapsedTime().asMilliseconds() > 10) {
                        moveCentipede(dt);
                        centipedeClock.restart();
                    }
                    
                    if (bullet[EXISTS]) {
                        moveBullet(dt);
                    }
                    
                    // Spawn new heads if any group is in player area
                    bool anyGroupInPlayerArea = false;
                    for (int i = 0; i < groupCount; ++i) {
                        if (groupInPlayerArea[i]) {
                            anyGroupInPlayerArea = true;
                            break;
                        }
                    }
                    if (anyGroupInPlayerArea && headSpawnClock.getElapsedTime().asSeconds() >= HEAD_SPAWN_INTERVAL) {
                        spawnNewHead();
                        headSpawnClock.restart();
                    }
                    
                    checkCentipedeMushroomCollisions();
                    checkBulletMushroomCollisions();
                    checkBulletCentipedeCollisions();
                    
                    if (checkPlayerCentipedeCollision()) {
                        updateHighScores(score);
                        currentGameState = GAME_OVER_STATE;
                        playerWon = false; // Reset win condition
                    }
                }
                
                drawMushrooms(window, mushroomSprite, poisonMushroomSprite);
                drawCentipede(window, centipedeSprite, headSprite);
                drawPlayer(window, playerSprite);
                
                if (bullet[EXISTS]) {
                    drawBullet(window, bulletSprite);
                }
                
                renderScore(window, scoreText);
                
                if (gamePaused) {
                    sf::Text pausedText;
                    pausedText.setFont(font);
                    pausedText.setString("PAUSED");
                    pausedText.setCharacterSize(50);
                    pausedText.setFillColor(sf::Color::White);
                    pausedText.setPosition(
                        SCREEN_WIDTH / 2 - pausedText.getGlobalBounds().width / 2,
                        SCREEN_HEIGHT / 2 - pausedText.getGlobalBounds().height / 2
                    );
                    window.draw(pausedText);
                }
                break;
        }
        
        window.display();
    }
    
    return 0;
}

bool loadResources(sf::Texture& backgroundTexture, sf::Texture& mushroomTexture,
                  sf::Texture& poisonMushroomTexture, sf::Texture& centipedeTexture,
                  sf::Texture& headTexture, sf::Texture& playerTexture, sf::Texture& bulletTexture,
                  sf::Font& font, sf::Music& bgMusic) {
    if (!backgroundTexture.loadFromFile("Textures/background1.jpg")) {
        std::cerr << "Failed to load background texture!" << std::endl;
        return false;
    }
    if (!mushroomTexture.loadFromFile("Textures/mushroom.png")) {
        std::cerr << "Failed to load mushroom texture!" << std::endl;
        return false;
    }
    if (!poisonMushroomTexture.loadFromFile("Textures/poison_mushroom.png")) {
        std::cerr << "Failed to load poisonous mushroom texture!" << std::endl;
        return false;
    }
    if (!centipedeTexture.loadFromFile("Textures/c_body_left_walk.png")) {
        std::cerr << "Failed to load centipede texture!" << std::endl;
        return false;
    }
    if (!headTexture.loadFromFile("Textures/c_head_left_walk.png")) {
        std::cerr << "Failed to load head texture!" << std::endl;
        return false;
    }
    if (!playerTexture.loadFromFile("Textures/player.png")) {
        std::cerr << "Failed to load player texture!" << std::endl;
        return false;
    }
    if (!bulletTexture.loadFromFile("Textures/bullet.png")) {
        std::cerr << "Failed to load bullet texture!" << std::endl;
        return false;
    }
    if (!font.loadFromFile("Retro Gaming.ttf")) {
        std::cerr << "Failed to load font!" << std::endl;
        return false;
    }
    if (!bgMusic.openFromFile("Music/field_of_hopes.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
        return false;
    }
    bgMusic.play();
    bgMusic.setVolume(50);
    bgMusic.setLoop(true);
    return true;
}

void initializeGame() {
    // Initialize player position within bottom 5 rows
    player[X] = (GRID_COLS / 2) * TILE_SIZE;
    player[Y] = (GRID_ROWS - 5) * TILE_SIZE; // Spawn at top of bottom 5 rows
    
    // Initialize bullet
    bullet[X] = player[X];
    bullet[Y] = player[Y] - TILE_SIZE;
    bullet[EXISTS] = false;
    
    // Reset score
    score = 0;
    
    // Initialize game objects
    initializeMushrooms();
    initializeCentipede();
    
    // Reset game state
    gameOver = false;
    moveLeft = true;
    playerWon = false;
}

void initializeMushrooms() {
    for (int i = 0; i < NUM_MUSHROOMS; ++i) {
        bool uniquePosition = false;
        int newX, newY;

        // Keep generating positions until a unique one is found
        while (!uniquePosition) {
            newX = rand() % GRID_COLS * TILE_SIZE;
            newY = rand() % (GRID_ROWS - MAX_PLAYER_ROWS) * TILE_SIZE;
            uniquePosition = true;

            // Check against all previously placed mushrooms
            for (int j = 0; j < i; ++j) {
                if (mushroomGrid[j][EXISTS] && mushroomGrid[j][X] == newX && mushroomGrid[j][Y] == newY) {
                    uniquePosition = false;
                    break;
                }
            }
        }

        // Assign the unique position and initialize all fields
        mushroomGrid[i][X] = newX;
        mushroomGrid[i][Y] = newY;
        mushroomGrid[i][EXISTS] = true;
        mushroomGrid[i][DAMAGE] = 0;
        mushroomGrid[i][IS_POISONOUS] = false; // Initialize as non-poisonous
    }
}

void initializeCentipede() {
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        centipedeGrid[i][X] = (GRID_COLS - i - 1) * TILE_SIZE; // Start from right
        centipedeGrid[i][Y] = 0;
        centipedeGrid[i][EXISTS] = true;
        centipedeGroup[i] = 0; // All segments start in group 0
    }
    groupMoveLeft[0] = true; // Initial group moves left
    groupInPlayerArea[0] = false; // Start outside player area
    groupCount = 1; // Start with one group
    headSpawnClock.restart(); // Initialize spawn timer
}

void drawMushrooms(sf::RenderWindow& window, sf::Sprite& mushroomSprite, 
                  sf::Sprite& poisonMushroomSprite) {
    for (int i = 0; i < NUM_MUSHROOMS; ++i) {
        if (mushroomGrid[i][EXISTS]) {
            int damage = mushroomGrid[i][DAMAGE];
            int textureX = damage * 32; // 0, 32, 64, 96 for damage 0, 1, 2, 3
            
            if (mushroomGrid[i][IS_POISONOUS]) {
                poisonMushroomSprite.setTextureRect(sf::IntRect(textureX, 0, TILE_SIZE, TILE_SIZE));
                poisonMushroomSprite.setPosition(mushroomGrid[i][X], mushroomGrid[i][Y]);
                window.draw(poisonMushroomSprite);
            } else {
                mushroomSprite.setTextureRect(sf::IntRect(textureX, 0, TILE_SIZE, TILE_SIZE));
                mushroomSprite.setPosition(mushroomGrid[i][X], mushroomGrid[i][Y]);
                window.draw(mushroomSprite);
            }
        }
    }
}

void drawCentipede(sf::RenderWindow& window, sf::Sprite& centipedeSprite, sf::Sprite& headSprite) {
    for (int group = 0; group < groupCount; ++group) {
        // Find the head of the current group based on movement direction
        int headIndex = -1;
        if (groupMoveLeft[group]) {
            int minX = SCREEN_WIDTH;
            for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group && centipedeGrid[i][X] < minX) {
                    minX = centipedeGrid[i][X];
                    headIndex = i;
                }
            }
        } else {
            int maxX = -1;
            for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group && centipedeGrid[i][X] > maxX) {
                    maxX = centipedeGrid[i][X];
                    headIndex = i;
                }
            }
        }

        for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
            if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group) {
                bool isHead = (i == headIndex);
                if (isHead) {
                    headSprite.setPosition(centipedeGrid[i][X], centipedeGrid[i][Y]);
                    if (groupMoveLeft[group]) {
                        headSprite.setScale(1.0f, 1.0f); // Normal (left-facing)
                    } else {
                        headSprite.setScale(-1.0f, 1.0f); // Flip horizontally (right-facing)
                        headSprite.setPosition(centipedeGrid[i][X] + TILE_SIZE, centipedeGrid[i][Y]);
                    }
                    window.draw(headSprite);
                } else {
                    centipedeSprite.setPosition(centipedeGrid[i][X], centipedeGrid[i][Y]);
                    window.draw(centipedeSprite);
                }
            }
        }
    }
}

bool checkCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void checkCentipedeMushroomCollisions() {
    for (int i = 0; i < NUM_MUSHROOMS; ++i) {
        if (mushroomGrid[i][EXISTS]) {
            for (int j = 0; j < CENTIPEDE_LENGTH; ++j) {
                if (centipedeGrid[j][EXISTS]) {
                    if (checkCollision(
                        centipedeGrid[j][X], centipedeGrid[j][Y], TILE_SIZE, TILE_SIZE,
                        mushroomGrid[i][X], mushroomGrid[i][Y], TILE_SIZE, TILE_SIZE)) {
                        
                        // Move centipede down and change direction
                        int group = centipedeGroup[j];
                        groupMoveLeft[group] = !groupMoveLeft[group];
                        for (int k = 0; k < CENTIPEDE_LENGTH; ++k) {
                            if (centipedeGrid[k][EXISTS] && centipedeGroup[k] == group) {
                                int currentRow = centipedeGrid[k][Y] / TILE_SIZE;
                                int nextRow = currentRow + 1;
                                if (groupInPlayerArea[group] && nextRow > GRID_ROWS - 1) {
                                    nextRow = GRID_ROWS - MAX_PLAYER_ROWS; // Wrap to top of player area
                                }
                                centipedeGrid[k][Y] = nextRow * TILE_SIZE;
                            }
                        }
                        return;
                    }
                }
            }
        }
    }
}

void checkBulletMushroomCollisions() {
    if (!bullet[EXISTS]) return;
    
    for (int i = 0; i < NUM_MUSHROOMS; ++i) {
        if (mushroomGrid[i][EXISTS]) {
            if (checkCollision(
                bullet[X], bullet[Y], TILE_SIZE, TILE_SIZE,
                mushroomGrid[i][X], mushroomGrid[i][Y], TILE_SIZE, TILE_SIZE)) {
                
                // Increase damage level
                mushroomGrid[i][DAMAGE]++;
                
                // Check if mushroom should be destroyed
                if (mushroomGrid[i][DAMAGE] >= 4) {
                    mushroomGrid[i][EXISTS] = false;
                    score += 1;
                }
                
                // Reset bullet
                bullet[EXISTS] = false;
                return;
            }
        }
    }
}

bool checkPlayerCentipedeCollision() {
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        if (centipedeGrid[i][EXISTS]) {
            if (checkCollision(
                player[X], player[Y], TILE_SIZE, TILE_SIZE,
                centipedeGrid[i][X], centipedeGrid[i][Y], TILE_SIZE, TILE_SIZE)) {
                return true;
            }
        }
    }
    return false;
}

void moveBullet(float deltaTime) {
    const float bulletSpeed = 5000.0f * deltaTime; // Pixels per second
    
    bullet[Y] -= bulletSpeed;
    
    // Bullet goes off screen
    if (bullet[Y] < -TILE_SIZE) {
        bullet[EXISTS] = false;
    }
}

void drawBullet(sf::RenderWindow& window, sf::Sprite& bulletSprite) {
    bulletSprite.setPosition(bullet[X], bullet[Y]);
    window.draw(bulletSprite);
}

void drawPlayer(sf::RenderWindow& window, sf::Sprite& playerSprite) {
    playerSprite.setPosition(player[X], player[Y]);
    window.draw(playerSprite);
}

void handleInput(float deltaTime, sf::RenderWindow& window) {
    const float playerSpeed = 1200.0f * deltaTime; // Pixels per second
    
    // Player movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && player[X] > 0) {
        player[X] -= playerSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && player[X] < SCREEN_WIDTH - TILE_SIZE) {
        player[X] += playerSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && 
        player[Y] > (GRID_ROWS - MAX_PLAYER_ROWS) * TILE_SIZE) {
        player[Y] -= playerSpeed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && player[Y] < SCREEN_HEIGHT - TILE_SIZE) {
        player[Y] += playerSpeed;
    }
    
    // Fire bullet
    if (isSpaceKeyPressed(window) && !bullet[EXISTS]) {
        bullet[X] = player[X];
        bullet[Y] = player[Y] - TILE_SIZE;
        bullet[EXISTS] = true;
    }
}

bool isSpaceKeyPressed(sf::RenderWindow& window) {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
}

void resetGame() {
    initializeGame();
}

void renderScore(sf::RenderWindow& window, sf::Text& scoreText) {
    scoreText.setString("Score: " + std::to_string(score));
    window.draw(scoreText);
}

void drawMenu(sf::RenderWindow& window, sf::Font& font) {
    sf::Text titleText, playText, scoresText, exitText, winText;
    
    // Title
    titleText.setFont(font);
    titleText.setString("CENTIPEDE");
    titleText.setCharacterSize(60);
    titleText.setFillColor(sf::Color::Green);
    titleText.setPosition(
        SCREEN_WIDTH / 2 - titleText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 4
    );
    
    // Win message if player won
    if (playerWon) {
        winText.setFont(font);
        winText.setString("You Won!");
        winText.setCharacterSize(40);
        winText.setFillColor(sf::Color::Yellow);
        winText.setPosition(
            SCREEN_WIDTH / 2 - winText.getGlobalBounds().width / 2,
            SCREEN_HEIGHT / 4 + 80
        );
    }
    
    // Play option
    playText.setFont(font);
    playText.setString("Play Game");
    playText.setCharacterSize(36);
    playText.setFillColor(selectedMenuItem == MENU_PLAY ? sf::Color::Yellow : sf::Color::White);
    playText.setPosition(
        SCREEN_WIDTH / 2 - playText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2
    );
    if (selectedMenuItem == MENU_PLAY) {
        playText.setScale(1.1f, 1.1f); // Hover effect
    } else {
        playText.setScale(1.0f, 1.0f);
    }
    
    // High scores option
    scoresText.setFont(font);
    scoresText.setString("High Scores");
    scoresText.setCharacterSize(36);
    scoresText.setFillColor(selectedMenuItem == MENU_HIGH_SCORES ? sf::Color::Yellow : sf::Color::White);
    scoresText.setPosition(
        SCREEN_WIDTH / 2 - scoresText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2 + 60
    );
    if (selectedMenuItem == MENU_HIGH_SCORES) {
        scoresText.setScale(1.1f, 1.1f); // Hover effect
    } else {
        scoresText.setScale(1.0f, 1.0f);
    }
    
    // Exit option
    exitText.setFont(font);
    exitText.setString("Exit");
    exitText.setCharacterSize(36);
    exitText.setFillColor(selectedMenuItem == MENU_EXIT ? sf::Color::Yellow : sf::Color::White);
    exitText.setPosition(
        SCREEN_WIDTH / 2 - exitText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2 + 120
    );
    if (selectedMenuItem == MENU_EXIT) {
        exitText.setScale(1.1f, 1.1f); // Hover effect
    } else {
        exitText.setScale(1.0f, 1.0f);
    }
    
    window.draw(titleText);
    if (playerWon) window.draw(winText);
    window.draw(playText);
    window.draw(scoresText);
    window.draw(exitText);
}

void drawGameOverMenu(sf::RenderWindow& window, sf::Font& font) {
    sf::Text titleText, mainMenuText, restartText, leaderboardText;
    
    // Title
    titleText.setFont(font);
    titleText.setString("GAME OVER");
    titleText.setCharacterSize(60);
    titleText.setFillColor(sf::Color::Red);
    titleText.setPosition(
        SCREEN_WIDTH / 2 - titleText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 4
    );
    
    // Main Menu option
    mainMenuText.setFont(font);
    mainMenuText.setString("Return to Main Menu");
    mainMenuText.setCharacterSize(36);
    mainMenuText.setFillColor(selectedMenuItem == GAME_OVER_MAIN_MENU ? sf::Color::Yellow : sf::Color::White);
    mainMenuText.setPosition(
        SCREEN_WIDTH / 2 - mainMenuText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2
    );
    if (selectedMenuItem == GAME_OVER_MAIN_MENU) {
        mainMenuText.setScale(1.1f, 1.1f); // Hover effect
    } else {
        mainMenuText.setScale(1.0f, 1.0f);
    }
    
    // Restart option
    restartText.setFont(font);
    restartText.setString("Restart");
    restartText.setCharacterSize(36);
    restartText.setFillColor(selectedMenuItem == GAME_OVER_RESTART ? sf::Color::Yellow : sf::Color::White);
    restartText.setPosition(
        SCREEN_WIDTH / 2 - restartText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2 + 60
    );
    if (selectedMenuItem == GAME_OVER_RESTART) {
        restartText.setScale(1.1f, 1.1f); // Hover effect
    } else {
        restartText.setScale(1.0f, 1.0f);
    }
    
    // Leaderboard option
    leaderboardText.setFont(font);
    leaderboardText.setString("View Leaderboard");
    leaderboardText.setCharacterSize(36);
    leaderboardText.setFillColor(selectedMenuItem == GAME_OVER_LEADERBOARD ? sf::Color::Yellow : sf::Color::White);
    leaderboardText.setPosition(
        SCREEN_WIDTH / 2 - leaderboardText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2 + 120
    );
    if (selectedMenuItem == GAME_OVER_LEADERBOARD) {
        leaderboardText.setScale(1.1f, 1.1f); // Hover effect
    } else {
        leaderboardText.setScale(1.0f, 1.0f);
    }
    
    window.draw(titleText);
    window.draw(mainMenuText);
    window.draw(restartText);
    window.draw(leaderboardText);
}

void handleMenuInput(sf::Event& event, sf::RenderWindow& window) {
    if (event.type == sf::Event::KeyPressed) {
        if (currentGameState == MENU_STATE) {
            switch (event.key.code) {
                case sf::Keyboard::Up:
                    selectedMenuItem = (selectedMenuItem - 1);
                    if (selectedMenuItem < 0) selectedMenuItem = MENU_EXIT;
                    break;
                    
                case sf::Keyboard::Down:
                    selectedMenuItem = (selectedMenuItem + 1) % 3;
                    break;
                    
                case sf::Keyboard::Return:
                    switch (selectedMenuItem) {
                        case MENU_PLAY:
                            currentGameState = GAME_STATE;
                            resetGame();
                            break;
                            
                        case MENU_HIGH_SCORES:
                            currentGameState = HIGH_SCORE_STATE;
                            break;
                            
                        case MENU_EXIT:
                            window.close();
                            break;
                    }
                    break;
            }
        } else if (currentGameState == GAME_OVER_STATE) {
            switch (event.key.code) {
                case sf::Keyboard::Up:
                    selectedMenuItem = (selectedMenuItem - 1);
                    if (selectedMenuItem < 0) selectedMenuItem = GAME_OVER_LEADERBOARD;
                    break;
                    
                case sf::Keyboard::Down:
                    selectedMenuItem = (selectedMenuItem + 1) % 3;
                    break;
                    
                case sf::Keyboard::Return:
                    switch (selectedMenuItem) {
                        case GAME_OVER_MAIN_MENU:
                            currentGameState = MENU_STATE;
                            break;
                            
                        case GAME_OVER_RESTART:
                            currentGameState = GAME_STATE;
                            resetGame();
                            break;
                            
                        case GAME_OVER_LEADERBOARD:
                            currentGameState = HIGH_SCORE_STATE;
                            break;
                    }
                    break;
            }
        } else if (currentGameState == HIGH_SCORE_STATE && event.key.code == sf::Keyboard::Escape) {
            currentGameState = MENU_STATE;
        }
    }
}

void saveHighScores() {
    std::ofstream file(HIGH_SCORE_FILE);
    if (file.is_open()) {
        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            file << highScores[i] << std::endl;
        }
        file.close();
    }
}

void loadHighScores() {
    std::ifstream file(HIGH_SCORE_FILE);
    if (file.is_open()) {
        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            if (!(file >> highScores[i])) {
                highScores[i] = 0;
            }
        }
        file.close();
    }
}

void updateHighScores(int newScore) {
    // Find position for new score
    int position = -1;
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        if (newScore > highScores[i]) {
            position = i;
            break;
        }
    }
    
    // If score is high enough to be on the board
    if (position != -1) {
        // Shift lower scores down
        for (int i = MAX_HIGH_SCORES - 1; i > position; i--) {
            highScores[i] = highScores[i - 1];
        }
        // Insert new score
        highScores[position] = newScore;
        saveHighScores();
    }
}

void drawHighScores(sf::RenderWindow& window, sf::Font& font) {
    sf::Text titleText, scoreText, backText;
    
    // Title
    titleText.setFont(font);
    titleText.setString("HIGH SCORES");
    titleText.setCharacterSize(50);
    titleText.setFillColor(sf::Color::Green);
    titleText.setPosition(
        SCREEN_WIDTH / 2 - titleText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 6
    );
    
    window.draw(titleText);
    
    // Scores
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        scoreText.setFont(font);
        scoreText.setString(std::to_string(i + 1) + ". " + std::to_string(highScores[i]));
        scoreText.setCharacterSize(36);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(
            SCREEN_WIDTH / 2 - scoreText.getGlobalBounds().width / 2,
            SCREEN_HEIGHT / 3 + i * 50
        );
        window.draw(scoreText);
    }
    
    // Back instruction
    backText.setFont(font);
    backText.setString("Press ESC to return to menu");
    backText.setCharacterSize(24);
    backText.setFillColor(sf::Color::Yellow);
    backText.setPosition(
        SCREEN_WIDTH / 2 - backText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT * 3 / 4
    );
    window.draw(backText);
}

void splitCentipede(int hitSegmentIndex) {
    int hitX = centipedeGrid[hitSegmentIndex][X];
    int hitY = centipedeGrid[hitSegmentIndex][Y];
    int hitGroup = centipedeGroup[hitSegmentIndex];

    // Create poisonous mushroom at hit position
    bool mushroomCreated = false;
    for (int i = 0; i < NUM_MUSHROOMS; i++) {
        if (!mushroomGrid[i][EXISTS]) {
            mushroomGrid[i][X] = hitX;
            mushroomGrid[i][Y] = hitY;
            mushroomGrid[i][EXISTS] = true;
            mushroomGrid[i][DAMAGE] = 0;
            mushroomGrid[i][IS_POISONOUS] = true; // Mark as poisonous
            mushroomCreated = true;
            break;
        }
    }

    // Award points: more for head, less for body
    bool isHead = false;
    if (groupMoveLeft[hitGroup]) {
        int minX = SCREEN_WIDTH;
        int headIndex = -1;
        for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
            if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == hitGroup && centipedeGrid[i][X] < minX) {
                minX = centipedeGrid[i][X];
                headIndex = i;
            }
        }
        isHead = (hitSegmentIndex == headIndex);
    } else {
        int maxX = -1;
        int headIndex = -1;
        for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
            if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == hitGroup && centipedeGrid[i][X] > maxX) {
                maxX = centipedeGrid[i][X];
                headIndex = i;
            }
        }
        isHead = (hitSegmentIndex == headIndex);
    }
    score += isHead ? 100 : 10;

    // Mark hit segment as non-existent
    centipedeGrid[hitSegmentIndex][EXISTS] = false;

    // Check for segments to the left and right of the hit segment
    bool hasLeftSegments = false;
    bool hasRightSegments = false;
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == hitGroup) {
            if ((groupMoveLeft[hitGroup] && centipedeGrid[i][X] < hitX) ||
                (!groupMoveLeft[hitGroup] && centipedeGrid[i][X] > hitX)) {
                hasLeftSegments = true;
            }
            if ((groupMoveLeft[hitGroup] && centipedeGrid[i][X] > hitX) ||
                (!groupMoveLeft[hitGroup] && centipedeGrid[i][X] < hitX)) {
                hasRightSegments = true;
            }
        }
    }

    // If there are right segments, create a new group for them
    if (hasRightSegments && groupCount < MAX_GROUPS) {
        int newGroup = groupCount++;
        groupMoveLeft[newGroup] = !groupMoveLeft[hitGroup];
        groupInPlayerArea[newGroup] = groupInPlayerArea[hitGroup]; // Inherit player area status
        for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
            if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == hitGroup) {
                if ((groupMoveLeft[hitGroup] && centipedeGrid[i][X] > hitX) ||
                    (!groupMoveLeft[hitGroup] && centipedeGrid[i][X] < hitX)) {
                    centipedeGroup[i] = newGroup;
                    if (groupInPlayerArea[newGroup]) {
                        int currentRow = centipedeGrid[i][Y] / TILE_SIZE;
                        int nextRow = currentRow + 1;
                        if (nextRow > GRID_ROWS - 1) {
                            nextRow = GRID_ROWS - MAX_PLAYER_ROWS; // Wrap to top of player area
                        }
                        centipedeGrid[i][Y] = nextRow * TILE_SIZE;
                    } else {
                        centipedeGrid[i][Y] += TILE_SIZE;
                    }
                }
            }
        }
    }
}

void spawnNewHead() {
    // Find an unused segment slot
    int newSegmentIndex = -1;
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        if (!centipedeGrid[i][EXISTS]) {
            newSegmentIndex = i;
            break;
        }
    }
    if (newSegmentIndex == -1 || groupCount >= MAX_GROUPS) return; // No space or too many groups

    // Create new group for the head
    int newGroup = groupCount++;
    groupInPlayerArea[newGroup] = true; // Head spawns in player area
    groupMoveLeft[newGroup] = rand() % 2; // Random direction (true = left, false = right)

    // Set position in player area (rows 25-29)
    int newRow = (rand() % MAX_PLAYER_ROWS) + (GRID_ROWS - MAX_PLAYER_ROWS);
    centipedeGrid[newSegmentIndex][Y] = newRow * TILE_SIZE;
    centipedeGrid[newSegmentIndex][X] = groupMoveLeft[newGroup] ? SCREEN_WIDTH : -TILE_SIZE;
    centipedeGrid[newSegmentIndex][EXISTS] = true;
    centipedeGroup[newSegmentIndex] = newGroup;
}

void checkBulletCentipedeCollisions() {
    if (!bullet[EXISTS]) return;
    
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        if (centipedeGrid[i][EXISTS]) {
            if (checkCollision(
                bullet[X], bullet[Y], TILE_SIZE, TILE_SIZE,
                centipedeGrid[i][X], centipedeGrid[i][Y], TILE_SIZE, TILE_SIZE)) {
                
                // Spawn poisonous mushroom at hit position
                bool mushroomCreated = false;
                for (int j = 0; j < NUM_MUSHROOMS; j++) {
                    if (!mushroomGrid[j][EXISTS]) {
                        mushroomGrid[j][X] = centipedeGrid[i][X];
                        mushroomGrid[j][Y] = centipedeGrid[i][Y];
                        mushroomGrid[j][EXISTS] = true;
                        mushroomGrid[j][DAMAGE] = 0;
                        mushroomGrid[j][IS_POISONOUS] = true;
                        mushroomCreated = true;
                        break;
                    }
                }
                
                // Split the centipede
                splitCentipede(i);
                
                // Reset bullet
                bullet[EXISTS] = false;

                // Check if all segments are eliminated
                bool allEliminated = true;
                for (int j = 0; j < CENTIPEDE_LENGTH; ++j) {
                    if (centipedeGrid[j][EXISTS]) {
                        allEliminated = false;
                        break;
                    }
                }
                if (allEliminated) {
                    updateHighScores(score);
                    currentGameState = MENU_STATE;
                    playerWon = true;
                }
                return;
            }
        }
    }
}

void moveCentipede(float deltaTime) {
    int moveStep = 3; // Speed multiplier

    for (int group = 0; group < groupCount; ++group) {
        // Find head for the current group
        int headIndex = -1;
        if (groupMoveLeft[group]) {
            int minX = SCREEN_WIDTH;
            for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group && centipedeGrid[i][X] < minX) {
                    minX = centipedeGrid[i][X];
                    headIndex = i;
                }
            }
        } else {
            int maxX = -1;
            for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group && centipedeGrid[i][X] > maxX) {
                    maxX = centipedeGrid[i][X];
                    headIndex = i;
                }
            }
        }

        // Check if group is entering player area
        if (!groupInPlayerArea[group] && headIndex != -1 && centipedeGrid[headIndex][Y] >= (GRID_ROWS - MAX_PLAYER_ROWS) * TILE_SIZE) {
            groupInPlayerArea[group] = true;
            // Spawn poisonous mushroom at head position
            bool mushroomCreated = false;
            for (int i = 0; i < NUM_MUSHROOMS; i++) {
                if (!mushroomGrid[i][EXISTS]) {
                    mushroomGrid[i][X] = centipedeGrid[headIndex][X];
                    mushroomGrid[i][Y] = centipedeGrid[headIndex][Y];
                    mushroomGrid[i][EXISTS] = true;
                    mushroomGrid[i][DAMAGE] = 0;
                    mushroomGrid[i][IS_POISONOUS] = true;
                    mushroomCreated = true;
                    break;
                }
            }
        }

        // Handle movement (rest of the function remains the same)
        if (groupInPlayerArea[group]) {
            // In player area: move horizontally, reverse on collision or screen edge
            bool mushroomCollision = false;
            bool edgeCollision = false;
            if (headIndex != -1) {
                int nextX = centipedeGrid[headIndex][X] + (groupMoveLeft[group] ? -moveStep : moveStep);
                // Check for mushroom collision
                for (int i = 0; i < NUM_MUSHROOMS; ++i) {
                    if (mushroomGrid[i][EXISTS]) {
                        if (checkCollision(
                            nextX, centipedeGrid[headIndex][Y], TILE_SIZE, TILE_SIZE,
                            mushroomGrid[i][X], mushroomGrid[i][Y], TILE_SIZE, TILE_SIZE)) {
                            mushroomCollision = true;
                            break;
                        }
                    }
                }
                // Check for screen edge collision
                if (groupMoveLeft[group] && nextX < 0) {
                    edgeCollision = true;
                } else if (!groupMoveLeft[group] && nextX + TILE_SIZE > SCREEN_WIDTH) {
                    edgeCollision = true;
                }
            }

            if (mushroomCollision || edgeCollision) {
                groupMoveLeft[group] = !groupMoveLeft[group];
                // Move all segments to the next row in the player area
                for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                    if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group) {
                        int currentRow = centipedeGrid[i][Y] / TILE_SIZE;
                        int nextRow = currentRow + (groupMoveLeft[group] ? 1 : 1);
                        if (nextRow > GRID_ROWS - 1) {
                            nextRow = GRID_ROWS - MAX_PLAYER_ROWS;
                        }
                        centipedeGrid[i][Y] = nextRow * TILE_SIZE;
                    }
                }
            } else {
                // Normal horizontal movement
                for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                    if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group) {
                        centipedeGrid[i][X] += groupMoveLeft[group] ? -moveStep : moveStep;
                    }
                }
            }
        } else {
            // Normal movement outside player area
            bool shouldChangeDirection = false;
            int tailIndex = -1;
            if (groupMoveLeft[group]) {
                int maxX = -SCREEN_WIDTH;
                for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                    if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group && centipedeGrid[i][X] > maxX) {
                        maxX = centipedeGrid[i][X];
                        tailIndex = i;
                    }
                }
                if (tailIndex != -1 && centipedeGrid[tailIndex][X] + TILE_SIZE < 0) {
                    shouldChangeDirection = true;
                }
            } else {
                int minX = SCREEN_WIDTH * 2;
                for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                    if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group && centipedeGrid[i][X] < minX) {
                        minX = centipedeGrid[i][X];
                        tailIndex = i;
                    }
                }
                if (tailIndex != -1 && centipedeGrid[tailIndex][X] > SCREEN_WIDTH) {
                    shouldChangeDirection = true;
                }
            }

            if (shouldChangeDirection) {
                groupMoveLeft[group] = !groupMoveLeft[group];
                for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                    if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group) {
                        centipedeGrid[i][Y] += TILE_SIZE;
                        if (groupMoveLeft[group]) {
                            centipedeGrid[i][X] = SCREEN_WIDTH + (CENTIPEDE_LENGTH - i - 1) * TILE_SIZE;
                        } else {
                            centipedeGrid[i][X] = -(CENTIPEDE_LENGTH - i) * TILE_SIZE;
                        }
                    }
                }
            } else {
                if (groupMoveLeft[group]) {
                    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                        if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group) {
                            centipedeGrid[i][X] -= moveStep;
                        }
                    }
                } else {
                    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
                        if (centipedeGrid[i][EXISTS] && centipedeGroup[i] == group) {
                            centipedeGrid[i][X] += moveStep;
                        }
                    }
                }
            }

            // Handle mushroom collisions with the head
            if (headIndex != -1) {
                for (int i = 0; i < NUM_MUSHROOMS; ++i) {
                    if (mushroomGrid[i][EXISTS]) {
                        if (checkCollision(
                            centipedeGrid[headIndex][X], centipedeGrid[headIndex][Y], TILE_SIZE, TILE_SIZE,
                            mushroomGrid[i][X], mushroomGrid[i][Y], TILE_SIZE, TILE_SIZE)) {
                            groupMoveLeft[group] = !groupMoveLeft[group];
                            for (int j = 0; j < CENTIPEDE_LENGTH; ++j) {
                                if (centipedeGrid[j][EXISTS] && centipedeGroup[j] == group) {
                                    centipedeGrid[j][Y] += TILE_SIZE;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}