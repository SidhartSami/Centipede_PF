🔗 Connect
For professional inquiries or collaborations:
LinkedIn – Sidhart Sami
---
# 🕹️ Centipede Game – SFML C++ Implementation

**Author:** Sidhart Sami (https://www.linkedin.com/in/sidhart-sami/)
**Project Title:** Centipede Game – Programming Fundamentals Project  
**Term:** Fall 2023  
**Institution:** FAST National University of Computer and Emerging Sciences (FAST-NUCES), Islamabad Campus  
**Technologies:** C++, SFML (Simple and Fast Multimedia Library)

---

## 🎮 Description

This is a simplified clone of **Centipede**, a classic arcade shooter originally released by Atari in June 1981. The player controls a fighter at the bottom of the screen, defending against an advancing segmented centipede, spiders, ants, and scorpions, navigating through mushrooms while firing lasers.

A playable demo video is available inside the repository.

---

## 📁 Repository Structure
- `Centipede.cpp` – Main game logic
- ` centipede.o` – Compiled object file
- `sfml.app` – Executable
- `Texture/` – Image/textures for game assets
- `Music/` – Background music files
- `highscores.txt` – Score persistence
- `GamePlay.mp4 /` – Gameplay demo video
---
## 🧩 Features

- **Grid-based screen**: Represented using a 2D array.
- **Player fighter**: Controlled via arrow keys within a 5-row high player zone.
- **Enemy Centipede**:
  - Composed of 12 segments in level 1.
  - Splits into multiple heads when hit.
  - Increases speed with each level.
  - Moves left to right, descends on hitting mushrooms or edges.
- **Mushrooms**:
  - Randomly placed (20–30 initially).
  - Require two laser hits to destroy.
- **Poisonous Mushrooms**:
  - Spawned when centipede is killed in player area.
  - Lethal to the player on contact.
- **Scoring**:
  - Destroy Mushroom → 1 Point
  - Centipede Body → 10 Points
  - Centipede Head → 20 Points
- **Level progression**: Increased centipede speed and complexity.
- **Game over**:
  - Player is hit by centipede.
  - Player touches poisonous mushroom.

---

## 🛠️ Build Instructions

### ✅ Requirements
- C++ Compiler (`g++` or `clang++`)
- SFML 2.5+ installed and configured

### 💻 Linux/macOS
```bash
g++ Centipede.cpp -o centipede -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
./centipede
```

© 2023 Sidhart Sami – All Rights Reserved.


