<div align="center">

# 🥷 Shadow Fight Clone

### A 2D silhouette fighting game built from scratch with **C++** and **OpenGL (freeglut)**

![C++](https://img.shields.io/badge/C++-Game_Logic-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-freeglut-5586A4?style=for-the-badge&logo=opengl&logoColor=white)
![Code::Blocks](https://img.shields.io/badge/IDE-Code::Blocks-3A8F3A?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![Status](https://img.shields.io/badge/Status-Playable-success?style=for-the-badge)

</div>

---

## 📸 Screenshots

<table>
  <tr>
    <td align="center" width="50%"><b>Main Menu</b><br><br>
      <img src="https://github.com/user-attachments/assets/50f6a044-45ef-4265-b788-af96e012d1bf" alt="Main Menu" width="100%">
    </td>
    <td align="center" width="50%"><b>Gameplay</b><br><br>
      <img src="https://github.com/user-attachments/assets/7c84b09f-cad5-493d-9f0b-8458323fec0e" alt="Gameplay" width="100%">
    </td>
  </tr>
  <tr>
    <td align="center" width="50%"><b>Special Attack</b><br><br>
      <img src="https://github.com/user-attachments/assets/d49c1a2d-1eb2-4315-b2f7-9106ab1fa541" alt="Special Attack" width="100%">
    </td>
    <td align="center" width="50%"><b>End Game</b><br><br>
      <img src="https://github.com/user-attachments/assets/d1ed80b2-38d2-4d42-9041-ae2d12ec2c4f" alt="End Game" width="100%">
    </td>
  </tr>
  <tr>
    <td align="center" colspan="2"><b>Match Result</b><br><br>
      <img src="https://github.com/user-attachments/assets/45279dae-e211-4970-865e-f717ef37f59f" alt="Match Result" width="50%">
    </td>
  </tr>
</table>

---

## 📖 About The Project

**Shadow Fight Clone** is a 2D fighting game inspired by the silhouette art style of *Shadow Fight*. You play as a **shadow ninja** and fight against an **armored spearman** controlled by an AI opponent, in a best-of-three match set in Japanese-style arenas.

The whole game, including the characters, the animated backgrounds, the hit sparks and the UI, is drawn using only **OpenGL primitives**. No sprites or textures are used.

This project was built to learn and practice:

- Real-time game loop design and delta-time physics
- Object-oriented game architecture in C++
- Frame-data based combat (startup / active / recovery)
- Simple AI with difficulty levels
- Computer graphics with OpenGL

---

## ✨ Features

### ⚔️ Combat System
- **Three attacks**: fast Punch, unblockable Kick and the energy-based **Special Dragon Strike**
- **Frame-data combat**: every attack has *startup → active → recovery* phases
- **Hitboxes & hurtboxes** with knockback and hit-stun
- **Blocking and crouching** to defend against attacks
- **Energy bar** that regenerates over time and is spent on special moves

### 🤖 Enemy AI
- AI states: `IDLE`, `APPROACH`, `RETREAT`, `DEFEND`, `JUMP`, `ATTACK`, `HIT`, `DEAD`
- Reacts to your attacks by blocking or dodging
- Keeps a randomised preferred distance from the player
- **3 difficulty modes** that change reaction time, aggression and damage:

| Mode | Enemy Behaviour | Enemy Damage |
|:---:|---|:---:|
| 🟢 **Easy** | Slow reactions, rarely blocks | ×0.65 |
| 🟡 **Normal** | Balanced blocks and dodges | ×1.00 |
| 🔴 **Hard** | Fast, aggressive, rarely hesitates | ×1.50 |

### 🎨 Visuals
- Hand-drawn **silhouette fighters**: a ninja with a glowing cyan headband vs. an armored spearman
- **4 lighting themes**: Sunset, Night, Dawn, Twilight
- **4 arenas**: Temple Peaks, Bamboo Shrine, Castle Roof, Blossom Valley
- A random theme and arena is picked for each fight
- **Hit sparks**, dust puffs, shockwave rings and **screen shake**
- Screen vignette and animated backgrounds

### 🕹️ Game Flow
- Full menu system: **Main Menu → Options → Controls / Game Mode**
- **Best of 3 rounds** with round-intro and K.O. screens
- Pause menu with in-fight settings
- Mouse and keyboard menu navigation
- Debug hitbox view for development

---

## 🎮 Controls

| Key | Action | Notes |
|:---:|---|---|
| `A` / `D` | Move left / right | |
| `W` or `Space` | Jump | Also dodges unblockable kicks |
| `S` | Crouch | Duck under high attacks |
| `J` | Punch | Fast, blockable |
| `K` | Kick | Slower, stronger, **unblockable** |
| `L` | Special Dragon Strike | Costs **50 energy**, heavy damage |
| `I` | Block | Hold to guard |
| `P` / `Esc` | Pause | Opens the in-fight menu |
| `R` | Restart | After a match ends |
| `H` | Toggle hitbox debug view | Developer tool |
| `T` | Restore health and energy | Developer / test tool |

### Attack Stats

| Attack | Damage | Reach | Startup | Hit-stun | Energy | Blockable |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 👊 Punch | 10 | 85 | 0.08s | 0.30s | 0 | ✅ |
| 🦵 Kick | 15 | 115 | 0.30s | 0.40s | 0 | ❌ |
| 🐉 Special | 30 | 145 | 0.25s | 0.60s | 50 | ✅ |

---

## 🚀 Getting Started

### Prerequisites
- A C++ compiler (**MinGW / GCC** recommended)
- **Code::Blocks** IDE (the project file is included)
- **freeglut** and OpenGL libraries (`freeglut`, `opengl32`, `glu32`)

### Option 1: Just play (Windows)
1. Go to the **[Releases](../../releases)** page (or the `Shadow_Fight_Clone/bin/Debug/` folder)
2. Download and run `Shadow_Fight_Clone.exe`
3. Keep `freeglut.dll` in the same folder as the `.exe`

### Option 2: Build from source with Code::Blocks
```bash
# 1. Clone the repository
git clone https://github.com/ShanjidulNuhin/Shadow_Fight_Clone.git

# 2. Open the project file in Code::Blocks
Shadow_Fight_Clone/Shadow_Fight_Clone.cbp

# 3. Make sure freeglut is set up in your compiler settings, then
#    press Build and Run (F9)
```

### Option 3: Build from the command line (MinGW)
```bash
cd Shadow_Fight_Clone/Shadow_Fight_Clone
g++ *.cpp -o Shadow_Fight_Clone.exe -lfreeglut -lopengl32 -lglu32 -O2
```

> 💡 If the build fails with a missing `GL/glut.h` error, install freeglut and add its `include` and `lib` folders to your compiler search directories.

---

## 🗂️ Project Structure

```
Shadow_Fight_Clone/
├── main.cpp              # Entry point, GLUT setup and callbacks
├── Game.h / .cpp         # Game states, menus, rounds, main loop logic
├── Config.h              # World size, physics constants, round rules
│
├── Fighter.h / .cpp      # Base fighter: physics, states, drawing
├── Player.h / .cpp       # Keyboard-controlled ninja
├── Enemy.h / .cpp        # AI-controlled armored spearman
├── AttackData.h          # Frame data for Punch / Kick / Special
├── CombatSystem.h / .cpp # Hit detection and damage resolution
├── Collision.h / .cpp    # Rectangle collision helpers
├── Animation.h / .cpp    # Animation helpers
│
├── Renderer.h / .cpp     # Low-level OpenGL drawing helpers
├── Background.h / .cpp   # Arenas and lighting themes
├── Effects.h / .cpp      # Sparks, dust, shockwaves, screen shake
├── UI.h / .cpp           # HUD, menus, dialogs
├── InputManager.h / .cpp # Keyboard state tracking
│
└── Shadow_Fight_Clone.cbp  # Code::Blocks project file
```

---

## 🧠 How It Works

```
 ┌─────────────┐   input    ┌──────────┐
 │ InputManager│──────────▶│  Player  │──┐
 └─────────────┘            └──────────┘  │      ┌──────────────┐
                                          ├────▶│ CombatSystem │──▶ damage, knockback
 ┌─────────────┐  watches   ┌──────────┐  │      └──────────────┘
 │   Player    │──────────▶│  Enemy   │──┘             │
 └─────────────┘            │   (AI)   │                ▼
                            └──────────┘        ┌──────────────┐
                                                │ Effects / UI │
                                                └──────────────┘
```

1. **Game loop**: GLUT's idle callback computes `deltaTime`, then `Game::update()` and `Game::draw()` run every frame.
2. **Fighter state machine**: `IDLE → WALK → JUMP/FALL → CROUCH → BLOCK → PUNCH/KICK/SPECIAL → HIT → DEAD`.
3. **Attack phases**: `STARTUP → ACTIVE → RECOVERY`. Only the *active* phase can hurt, so timing and spacing matter.
4. **Combat**: `CombatSystem` checks if an active hitbox overlaps the opponent's hurtbox, then applies damage, knockback and hit-stun, or blocks the hit.
5. **AI**: the enemy chooses between approaching, retreating, defending, jumping and attacking based on distance and the player's actions.

---

## 🛠️ Built With

| Technology | Purpose |
|---|---|
| **C++** | Core game logic (OOP) |
| **OpenGL** | All 2D rendering |
| **freeglut** | Window, input and main loop |
| **Code::Blocks + MinGW** | IDE and compiler |

---

## 🗺️ Roadmap

- [x] Player vs AI combat
- [x] 3 difficulty modes
- [x] Multiple arenas and lighting themes
- [x] Menus, pause and best-of-3 rounds
- [x] 🔊 Sound effects
- [x] 🥋 Combo system
- [x] 🧙 More characters and weapons
- [x] 🏆 High-score and stats tracking
- [ ] 🎮 Gamepad support
- [ ] 👥 Local 2-player mode

---

## 🤝 Contributing

Contributions, ideas and bug reports are welcome!

1. Fork the project
2. Create your branch: `git checkout -b feature/AmazingFeature`
3. Commit your changes: `git commit -m "Add AmazingFeature"`
4. Push to the branch: `git push origin feature/AmazingFeature`
5. Open a Pull Request

---

## 👨‍💻 Author

**Shanjidul Nuhin**
CSE Student, American International University-Bangladesh (AIUB)

[![GitHub](https://img.shields.io/badge/GitHub-ShanjidulNuhin-181717?style=for-the-badge&logo=github)](https://github.com/ShanjidulNuhin)

---

## 🙏 Acknowledgements

- Inspired by the art style of the *Shadow Fight* series (this is a fan-made educational project, not affiliated with Nekki)
- [freeglut](https://freeglut.sourceforge.net/) for the windowing toolkit
- Built as a learning project at AIUB (edit this line to name your course, if you like)

---

<div align="center">

### ⭐ If you enjoyed the game, please give the repo a star!

</div>
