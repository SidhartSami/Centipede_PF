#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System/Clock.hpp>
// Add these to your existing includes
#include <fstream>
#include <string>

using namespace std;

// Game constants with improved naming
const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 960;
const int TILE_SIZE = 32;
const int GRID_COLS = SCREEN_WIDTH / TILE_SIZE;
const int GRID_ROWS = SCREEN_HEIGHT / TILE_SIZE;
const int MAX_PLAYER_ROWS = 6;
const int NUM_MUSHROOMS = 30;
const int CENTIPEDE_LENGTH = 12;
const float BULLET_SPEED = 10.0f;
const float PLAYER_SPEED = 4.0f;

// Game grid
int gameGrid[GRID_COLS][GRID_ROWS] = {};

// Constants for array indices
const int X = 0;
const int Y = 1;
const int EXISTS = 2;
const int DAMAGE = 3;  // For mushrooms

// Game objects
int mushroomGrid[NUM_MUSHROOMS][4];
int centipedeGrid[CENTIPEDE_LENGTH][3];
float bullet[3] = {};
float player[2] = {};

// Game state
int score = 0;
bool gameOver = false;
bool gamePaused = false;
bool moveLeft = true;  // Direction for centipede

const int MENU_STATE = 0;
const int GAME_STATE = 1;
const int HIGH_SCORE_STATE = 2;
int currentGameState = MENU_STATE;

// Menu options
const int MENU_PLAY = 0;
const int MENU_HIGH_SCORES = 1;
const int MENU_EXIT = 2;
int selectedMenuItem = MENU_PLAY;

const int MAX_HIGH_SCORES = 5;
int highScores[MAX_HIGH_SCORES] = {0};
const char* HIGH_SCORE_FILE = "highscores.txt";


// Function declarations
bool loadResources(sf::Texture& backgroundTexture, sf::Texture& mushroomTexture,
                  sf::Texture& damageMushroomTexture, sf::Texture& centipedeTexture,
                  sf::Texture& headTexture, sf::Texture& playerTexture, sf::Texture& bulletTexture,
                  sf::Font& font, sf::Music& bgMusic);
bool isSpaceKeyPressed(sf::RenderWindow& window);
void initializeGame();
void initializeMushrooms();
void initializeCentipede();
void drawMushrooms(sf::RenderWindow& window, sf::Sprite& mushroomSprite, sf::Sprite& damageMushroomSprite);
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
// Save high scores to file
void saveHighScores();
void loadHighScores();
void updateHighScores(int newScore);
void drawHighScores(sf::RenderWindow& window, sf::Font& font);
// Split the centipede when hit by a bullet
void splitCentipede(int hitSegmentIndex);

int main() {
    // Seed random number generator
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Create window with better size handling
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Centipede", sf::Style::Close);
    window.setSize(sf::Vector2u(640, 640));
    window.setPosition(sf::Vector2i(100, 0));
    
    // Resources
    sf::Music bgMusic;
    sf::Font font;
    sf::Texture backgroundTexture, mushroomTexture, damageMushroomTexture;
    sf::Texture centipedeTexture, headTexture, playerTexture, bulletTexture;
    
    // Load resources
    if (!loadResources(backgroundTexture, mushroomTexture, damageMushroomTexture,
                       centipedeTexture, headTexture, playerTexture, bulletTexture,
                       font, bgMusic)) {
        return -1;
    }
    
    // Create sprites
    sf::Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setColor(sf::Color(255, 255, 255, 255 * 0.40));
    
    sf::Sprite mushroomSprite(mushroomTexture);
    mushroomSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite damageMushroomSprite(damageMushroomTexture);
    damageMushroomSprite.setTextureRect(sf::IntRect(64, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite centipedeSprite(centipedeTexture);
    centipedeSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite headSprite(headTexture);
    headSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite playerSprite(playerTexture);
    playerSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    sf::Sprite bulletSprite(bulletTexture);
    bulletSprite.setTextureRect(sf::IntRect(0, 0, TILE_SIZE, TILE_SIZE));
    
    // Score text
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::Green);
    scoreText.setPosition(10, 10);
    
    // Initialize game
    initializeGame();
    
    // Game timing
    sf::Clock gameClock;
    float deltaTime = 0.0f;
    sf::Clock centipedeClock;
    sf::Clock bulletClock;
    
    loadHighScores();

    // Main game loop
    while (window.isOpen() && !gameOver) {
        // Calculate delta time for smooth movement
        deltaTime = gameClock.restart().asSeconds();
        
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
             if (currentGameState == MENU_STATE || currentGameState == HIGH_SCORE_STATE) {
                handleMenuInput(event, window);
            }
            // Add pause functionality with 'P' key
            if (currentGameState == GAME_STATE && event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::P) {
                gamePaused = !gamePaused;
            }

            if (currentGameState == GAME_STATE && event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::Escape) {
                // Check if score qualifies for high score list
                updateHighScores(score);
                currentGameState = MENU_STATE;
            }
        }
        window.clear();
        window.draw(backgroundSprite);
        
        // Draw different screens based on game state
        switch (currentGameState) {
            case MENU_STATE:
                drawMenu(window, font);
                break;
                
            case HIGH_SCORE_STATE:
                drawHighScores(window, font);
                break;
                
            case GAME_STATE:
                // Calculate delta time
                sf::Time deltaTime = gameClock.restart();
                float dt = deltaTime.asSeconds();  // in seconds

                
                // Skip updates if paused
                if (!gamePaused) {
                    // Game logic (movement, collisions, etc.)
                    handleInput(dt, window);
                    
                    if (centipedeClock.getElapsedTime().asMilliseconds() > 10) {
                        moveCentipede(dt);
                        centipedeClock.restart();
                    }
                    
                    if (bullet[EXISTS]) {
                        moveBullet(dt);
                    }
                    
                    checkCentipedeMushroomCollisions();
                    checkBulletMushroomCollisions();
                    checkBulletCentipedeCollisions();
                    
                    if (checkPlayerCentipedeCollision()) {
                        updateHighScores(score);
                        currentGameState = MENU_STATE;
                    }
                }
                
                // Draw game elements
                drawMushrooms(window, mushroomSprite, damageMushroomSprite);
                drawCentipede(window, centipedeSprite, headSprite);
                drawPlayer(window, playerSprite);
                
                if (bullet[EXISTS]) {
                    drawBullet(window, bulletSprite);
                }
                
                renderScore(window, scoreText);
                
                // Draw pause overlay if paused
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

// Load all game resources
bool loadResources(sf::Texture& backgroundTexture, sf::Texture& mushroomTexture,
                  sf::Texture& damageMushroomTexture, sf::Texture& centipedeTexture,
                  sf::Texture& headTexture, sf::Texture& playerTexture, sf::Texture& bulletTexture,
                  sf::Font& font, sf::Music& bgMusic) {
    
    // Load textures with error handling
    if (!backgroundTexture.loadFromFile("Textures/background1.jpg")) {
        std::cerr << "Failed to load background texture!" << std::endl;
        return false;
    }
    
    if (!mushroomTexture.loadFromFile("Textures/mushroom.png")) {
        std::cerr << "Failed to load mushroom texture!" << std::endl;
        return false;
    }
    
    damageMushroomTexture = mushroomTexture; // Same texture, different rect
    
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
    
    if (!font.loadFromFile("pricedow.ttf")) {
        std::cerr << "Failed to load font!" << std::endl;
        return false;
    }
    
    // Load music
    if (!bgMusic.openFromFile("Music/field_of_hopes.ogg")) {
        std::cerr << "Failed to load background music!" << std::endl;
        return false;
    }
    
    // Start music
    bgMusic.play();
    bgMusic.setVolume(50);
    bgMusic.setLoop(true);
    
    return true;
}

void initializeGame() {
    // Initialize player position
    player[X] = (GRID_COLS / 2) * TILE_SIZE;
    player[Y] = (GRID_ROWS * 3 / 4) * TILE_SIZE;
    
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
}

void initializeMushrooms() {
    for (int i = 0; i < NUM_MUSHROOMS; ++i) {
        mushroomGrid[i][X] = rand() % GRID_COLS * TILE_SIZE;
        mushroomGrid[i][Y] = rand() % (GRID_ROWS - MAX_PLAYER_ROWS) * TILE_SIZE;
        mushroomGrid[i][EXISTS] = true;
        mushroomGrid[i][DAMAGE] = 0;
    }
}

void initializeCentipede() {
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        centipedeGrid[i][X] = (GRID_COLS - i - 1) * TILE_SIZE; // Start from right
        centipedeGrid[i][Y] = 0;
        centipedeGrid[i][EXISTS] = true;
    }
}

void drawMushrooms(sf::RenderWindow& window, sf::Sprite& mushroomSprite, sf::Sprite& damageMushroomSprite) {
    for (int i = 0; i < NUM_MUSHROOMS; ++i) {
        if (mushroomGrid[i][EXISTS]) {
            if (mushroomGrid[i][DAMAGE] == 0) {
                mushroomSprite.setPosition(mushroomGrid[i][X], mushroomGrid[i][Y]);
                window.draw(mushroomSprite);
            } else if (mushroomGrid[i][DAMAGE] == 1) {
                damageMushroomSprite.setPosition(mushroomGrid[i][X], mushroomGrid[i][Y]);
                window.draw(damageMushroomSprite);
            }
        }
    }
}

void drawCentipede(sf::RenderWindow& window, sf::Sprite& centipedeSprite, sf::Sprite& headSprite) {
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        if (centipedeGrid[i][EXISTS]) {
            if (i == CENTIPEDE_LENGTH - 1) { // Head is the last segment
                headSprite.setPosition(centipedeGrid[i][X], centipedeGrid[i][Y]);
                window.draw(headSprite);
            } else {
                centipedeSprite.setPosition(centipedeGrid[i][X], centipedeGrid[i][Y]);
                window.draw(centipedeSprite);
            }
        }
    }
}

void moveCentipede(float deltaTime) {
    int moveStep = 2; // Speed multiplier
    
    if (moveLeft) {
        // Move left
        for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
            if (centipedeGrid[i][EXISTS]) {
                centipedeGrid[i][X] -= moveStep;
            }
        }
    } else {
        // Move right
        for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
            if (centipedeGrid[i][EXISTS]) {
                centipedeGrid[i][X] += moveStep;
            }
        }
    }
    
    // Check boundaries
    bool hitEdge = false;
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        if (centipedeGrid[i][EXISTS]) {
            if (centipedeGrid[i][X] < 0 || centipedeGrid[i][X] >= SCREEN_WIDTH - TILE_SIZE) {
                hitEdge = true;
                break;
            }
        }
    }
    
    // Change direction and move down if hit edge
    if (hitEdge) {
        moveLeft = !moveLeft;
        for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
            if (centipedeGrid[i][EXISTS]) {
                centipedeGrid[i][Y] += TILE_SIZE;
                
                // Ensure centipede stays within bounds
                if (centipedeGrid[i][X] < 0) {
                    centipedeGrid[i][X] = 0;
                }
                if (centipedeGrid[i][X] >= SCREEN_WIDTH - TILE_SIZE) {
                    centipedeGrid[i][X] = SCREEN_WIDTH - TILE_SIZE - 1;
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
                        for (int k = 0; k < CENTIPEDE_LENGTH; ++k) {
                            if (centipedeGrid[k][EXISTS]) {
                                centipedeGrid[k][Y] += TILE_SIZE / 2;
                            }
                        }
                        moveLeft = !moveLeft;
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
                if (mushroomGrid[i][DAMAGE] >= 2) {
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

// Updated function to handle centipede splitting behavior
void checkBulletCentipedeCollisions() {
    if (!bullet[EXISTS]) return;
    
    for (int i = 0; i < CENTIPEDE_LENGTH; ++i) {
        if (centipedeGrid[i][EXISTS]) {
            if (checkCollision(
                bullet[X], bullet[Y], TILE_SIZE, TILE_SIZE,
                centipedeGrid[i][X], centipedeGrid[i][Y], TILE_SIZE, TILE_SIZE)) {
                
                // Split the centipede at this point
                splitCentipede(i);
                
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
    const float bulletSpeed = 600.0f * deltaTime; // Pixels per second
    
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
    const float playerSpeed = 400.0f * deltaTime; // Pixels per second
    
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
    sf::Text titleText, playText, scoresText, exitText;
    
    // Title
    titleText.setFont(font);
    titleText.setString("CENTIPEDE");
    titleText.setCharacterSize(60);
    titleText.setFillColor(sf::Color::Green);
    titleText.setPosition(
        SCREEN_WIDTH / 2 - titleText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 4
    );
    
    // Play option
    playText.setFont(font);
    playText.setString("Play Game");
    playText.setCharacterSize(36);
    playText.setFillColor(selectedMenuItem == MENU_PLAY ? sf::Color::Yellow : sf::Color::White);
    playText.setPosition(
        SCREEN_WIDTH / 2 - playText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2
    );
    
    // High scores option
    scoresText.setFont(font);
    scoresText.setString("High Scores");
    scoresText.setCharacterSize(36);
    scoresText.setFillColor(selectedMenuItem == MENU_HIGH_SCORES ? sf::Color::Yellow : sf::Color::White);
    scoresText.setPosition(
        SCREEN_WIDTH / 2 - scoresText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2 + 60
    );
    
    // Exit option
    exitText.setFont(font);
    exitText.setString("Exit");
    exitText.setCharacterSize(36);
    exitText.setFillColor(selectedMenuItem == MENU_EXIT ? sf::Color::Yellow : sf::Color::White);
    exitText.setPosition(
        SCREEN_WIDTH / 2 - exitText.getGlobalBounds().width / 2,
        SCREEN_HEIGHT / 2 + 120
    );
    
    window.draw(titleText);
    window.draw(playText);
    window.draw(scoresText);
    window.draw(exitText);
}

// Handle menu input
void handleMenuInput(sf::Event& event, sf::RenderWindow& window) {
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
                selectedMenuItem = (selectedMenuItem - 1);
                if (selectedMenuItem < 0) selectedMenuItem = 2;
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
                
            case sf::Keyboard::Escape:
                if (currentGameState == HIGH_SCORE_STATE) {
                    currentGameState = MENU_STATE;
                }
                break;
        }
    }
}
// Save high scores to file
void saveHighScores() {
    std::ofstream file(HIGH_SCORE_FILE);
    if (file.is_open()) {
        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            file << highScores[i] << std::endl;
        }
        file.close();
    }
}

// Load high scores from file
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

// Update high scores with new score
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

// Draw high scores screen
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
// Split the centipede when hit by a bullet
void splitCentipede(int hitSegmentIndex) {
    // Store position of hit segment
    int hitX = centipedeGrid[hitSegmentIndex][X];
    int hitY = centipedeGrid[hitSegmentIndex][Y];
    
    // Create mushroom at hit position
    bool mushroomCreated = false;
    for (int i = 0; i < NUM_MUSHROOMS; i++) {
        if (!mushroomGrid[i][EXISTS]) {
            mushroomGrid[i][X] = hitX;
            mushroomGrid[i][Y] = hitY;
            mushroomGrid[i][EXISTS] = true;
            mushroomGrid[i][DAMAGE] = 0;
            mushroomCreated = true;
            break;
        }
    }
    
    // If it's the head (last segment), award more points
    if (hitSegmentIndex == CENTIPEDE_LENGTH - 1) {
        score += 100;
    } else {
        score += 10;
    }
    
    // Mark hit segment as non-existent
    centipedeGrid[hitSegmentIndex][EXISTS] = false;
    
    // Check if there are segments to the left and right
    bool hasLeftSegments = false;
    bool hasRightSegments = false;
    
    for (int i = 0; i < hitSegmentIndex; i++) {
        if (centipedeGrid[i][EXISTS]) {
            hasLeftSegments = true;
            break;
        }
    }
    
    for (int i = hitSegmentIndex + 1; i < CENTIPEDE_LENGTH; i++) {
        if (centipedeGrid[i][EXISTS]) {
            hasRightSegments = true;
            break;
        }
    }
    
    // If there are segments on the left, make them move left
    if (hasLeftSegments) {
        bool leftMoveDown = false;
        
        // Check if left segments need to move down
        for (int i = 0; i < hitSegmentIndex; i++) {
            if (centipedeGrid[i][EXISTS]) {
                if (centipedeGrid[i][X] <= 0) {
                    leftMoveDown = true;
                    break;
                }
            }
        }
        
        // Move left segments down if needed and set direction to left
        if (leftMoveDown) {
            for (int i = 0; i < hitSegmentIndex; i++) {
                if (centipedeGrid[i][EXISTS]) {
                    centipedeGrid[i][Y] += TILE_SIZE;
                }
            }
        }
        
        // Find the new head for left segments
        int newLeftHead = -1;
        for (int i = hitSegmentIndex - 1; i >= 0; i--) {
            if (centipedeGrid[i][EXISTS]) {
                newLeftHead = i;
                break;
            }
        }
    }
    
    // If there are segments on the right, make them move right
    if (hasRightSegments) {
        bool rightMoveDown = false;
        
        // Check if right segments need to move down
        for (int i = hitSegmentIndex + 1; i < CENTIPEDE_LENGTH; i++) {
            if (centipedeGrid[i][EXISTS]) {
                if (centipedeGrid[i][X] >= SCREEN_WIDTH - TILE_SIZE) {
                    rightMoveDown = true;
                    break;
                }
            }
        }
        
        // Move right segments down if needed and set direction to right
        if (rightMoveDown) {
            for (int i = hitSegmentIndex + 1; i < CENTIPEDE_LENGTH; i++) {
                if (centipedeGrid[i][EXISTS]) {
                    centipedeGrid[i][Y] += TILE_SIZE;
                }
            }
        }
    }
}