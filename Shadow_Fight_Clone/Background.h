#ifndef BACKGROUND_H
#define BACKGROUND_H

class Renderer;

// Time-of-day / lighting themes
enum class ArenaTheme {
    SUNSET,     // Burning crimson/golden sunset, giant setting sun, warm atmospheric glow
    NIGHT,      // Luminous sapphire midnight, glowing full moon, starry sky
    DAWN,       // Soft pastel morning sunrise, peach/gold sky, morning god-rays
    TWILIGHT    // Mystical purple dusk / eclipse, magenta twilight, cosmic corona
};

// Fighting locations / arenas
enum class ArenaLocation {
    TEMPLE_PEAKS,   // High mountain ridge, 3-tier pagoda, pine tree, stone lanterns
    BAMBOO_SHRINE,  // Dense bamboo grove, grand Torii gate, fire braziers
    CASTLE_ROOF,    // Feudal castle fortress, fluttering clan banners, tiled roof floor
    BLOSSOM_VALLEY  // Gnarled sakura trees, arched bridge, drifting falling petals
};

class Background {
public:
    Background();

    // Randomizes both theme and location for a new fight
    void randomizeThemeAndLocation();
    void setThemeAndLocation(ArenaTheme newTheme, ArenaLocation newLocation);

    ArenaTheme    getTheme() const    { return theme; }
    ArenaLocation getLocation() const { return location; }

    const char* getThemeName() const;
    const char* getLocationName() const;
    const char* getStageTitle() const;

    void update(float deltaTime);
    void draw(const Renderer& renderer) const;

    // Sky zenith color for setting glClearColor
    void getClearColor(float& r, float& g, float& b) const;

private:
    void initLeaves();
    void drawSky(const Renderer& renderer) const;
    void drawCelestial(const Renderer& renderer) const;
    void drawMountains(const Renderer& renderer) const;
    void drawLocationElements(const Renderer& renderer) const;
    void drawMist(const Renderer& renderer) const;
    void drawFloor(const Renderer& renderer) const;

    ArenaTheme    theme;
    ArenaLocation location;
    float         time;

    // Drifting petals / autumn leaves for Blossom Valley
    struct Leaf {
        float x;
        float y;
        float speedX;
        float speedY;
        float size;
        float rotPhase;
    };
    Leaf leaves[24];
};

#endif // BACKGROUND_H
