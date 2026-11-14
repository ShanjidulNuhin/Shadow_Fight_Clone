#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"
#include "SoundManager.h"
#include <cmath>
#include <algorithm>

namespace {
    const float CROUCH_HEIGHT_RATIO = 0.60f;
    const float CROUCH_SPEED_RATIO  = 0.45f;
    const float RUN_SPEED_RATIO     = 1.55f;
    const float ROLL_DURATION       = 0.36f;
    const float ROLL_SPEED          = 470.0f;

    const int   MAX_HEALTH = 100;

    const float MAX_ENERGY   = 100.0f;
    const float ENERGY_REGEN = 10.0f;

    const float KNOCKBACK_DECEL = 1600.0f;

    const float BLOCK_KNOCKBACK_SPEED = 160.0f;
    const float BLOCK_FLASH_TIME      = 0.15f;

    const float RECOVERY_INVULNERABILITY = 0.5f;
    const float BLINK_INTERVAL           = 0.07f;

    const float SKY_SPEAR_OFFSETS[10] = {
        -32.0f, 24.0f, -12.0f, 38.0f, -26.0f,
         14.0f, -40.0f, 30.0f, -18.0f,  0.0f
    };

    struct Vec2 {
        float x;
        float y;
    };

    void drawLightningBolt(const Renderer& renderer, float x1, float y1, float x2, float y2,
                           float seed, float amp, float r, float g, float b, float width) {
        const int segs = 5;
        float px = x1;
        float py = y1;
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        float nx = (len > 1.0f) ? (-dy / len) : 0.0f;
        float ny = (len > 1.0f) ? ( dx / len) : 1.0f;

        for (int i = 1; i <= segs; ++i) {
            float t = (float)i / (float)segs;
            float jitter = (i == segs) ? 0.0f : std::sin(seed * 37.0f + i * 2.3f) * amp;
            float qx = x1 + dx * t + nx * jitter;
            float qy = y1 + dy * t + ny * jitter;
            renderer.drawLine(px, py, qx, qy, r, g, b, 0.95f, width);
            px = qx;
            py = qy;
        }
    }

    void drawOpenFan(const Renderer& renderer, Vec2 hand, float dirX, float dirY, float radius,
                     float spreadRad, float bodyR, float bodyG, float bodyB,
                     float accR, float accG, float accB) {
        float baseAngle = std::atan2(dirY, dirX);
        const int ribs = 5;
        for (int i = 0; i < ribs; ++i) {
            float f0 = -0.5f + (float)i / (float)ribs;
            float f1 = -0.5f + (float)(i + 1) / (float)ribs;
            float a0 = baseAngle + f0 * spreadRad;
            float a1 = baseAngle + f1 * spreadRad;
            float x0 = hand.x + std::cos(a0) * radius;
            float y0 = hand.y + std::sin(a0) * radius;
            float x1 = hand.x + std::cos(a1) * radius;
            float y1 = hand.y + std::sin(a1) * radius;
            // Authentic vibrant scarlet silk fan leaf with alternating fold depth
            float sR = (i % 2 == 0) ? 0.90f : 0.74f;
            float sG = (i % 2 == 0) ? 0.18f : 0.12f;
            float sB = (i % 2 == 0) ? 0.30f : 0.20f;
            renderer.drawTriangle(hand.x, hand.y, x0, y0, x1, y1, sR, sG, sB);
            // Polished gold-lacquered bamboo rib
            renderer.drawLine(hand.x, hand.y, x0, y0, 0.96f, 0.80f, 0.25f, 0.95f, 1.8f);
        }
        float aEnd = baseAngle + 0.5f * spreadRad;
        renderer.drawLine(hand.x, hand.y,
                          hand.x + std::cos(aEnd) * radius,
                          hand.y + std::sin(aEnd) * radius,
                          0.96f, 0.80f, 0.25f, 0.95f, 2.0f);
        (void)bodyR; (void)bodyG; (void)bodyB; (void)accR; (void)accG; (void)accB;
    }

    void drawWeapon(const Renderer& renderer, const Fighter& fighter, WeaponType weapon,
                    Vec2 handF, Vec2 elbowF, Vec2 handB, Vec2 chest,
                    float dir, FighterState state, AttackPhase phase,
                    float stateTime, float actionProgress,
                    float bodyR, float bodyG, float bodyB,
                    float accR, float accG, float accB,
                    bool hideFan1, bool hideFan2, bool hideMainSpear) {
        float steelR = 0.90f;
        float steelG = 0.94f;
        float steelB = 1.0f;
        (void)bodyR; (void)bodyG; (void)bodyB;

        bool isComboStrike = (state == FighterState::PUNCH || state == FighterState::COMBO2 || state == FighterState::COMBO3);
        bool activeSwing = (isComboStrike || state == FighterState::SPECIAL) && (phase == AttackPhase::ACTIVE);

        float dx = handF.x - elbowF.x;
        float dy = handF.y - elbowF.y;
        float len = std::sqrt(dx * dx + dy * dy);
        float ux = (len > 1.0f) ? (dx / len) : dir;
        float uy = (len > 1.0f) ? (dy / len) : -0.35f;
        if (state == FighterState::IDLE || state == FighterState::WALK) {
            ux = dir * 0.78f;
            uy = -0.62f;
        }
        float nx = -uy;
        float ny =  ux;

        switch (weapon) {
            case WeaponType::NONE_UNARMED: {
                if (activeSwing) {
                    renderer.drawLine(elbowF.x, elbowF.y, handF.x + ux * 22.0f, handF.y + uy * 22.0f,
                                      accR, accG, accB, 0.75f, 3.5f);
                }
                if (state == FighterState::SPECIAL) {
                    float pulse = 18.0f + std::sin(stateTime * 35.0f) * 5.0f;
                    renderer.drawCircle(handF.x, handF.y, pulse, 1.0f, 0.65f, 0.20f, 0.55f, 16);
                    renderer.drawCircle(handF.x, handF.y, pulse * 0.55f, 1.0f, 0.95f, 0.55f, 0.90f, 12);
                }
                break;
            }

            case WeaponType::SPIKED_CHAIN: {
                if (state == FighterState::CHAIN_GRAB && phase != AttackPhase::RECOVERY) {
                    float ext = (phase == AttackPhase::STARTUP) ? actionProgress * 70.0f : (120.0f + std::sin(actionProgress * 1.57f) * 185.0f);
                    float tipX = handF.x + dir * ext;
                    float tipY = handF.y + std::sin(stateTime * 28.0f) * 6.0f;
                    int links = 14;
                    float px = handF.x, py = handF.y;
                    for (int i = 1; i <= links; ++i) {
                        float t = (float)i / (float)links;
                        float lx = handF.x + (tipX - handF.x) * t;
                        float ly = handF.y + (tipY - handF.y) * t + std::sin(t * 9.0f + stateTime * 20.0f) * 4.0f;
                        renderer.drawLine(px, py, lx, ly, 0.82f, 0.86f, 0.92f, 0.98f, 3.6f);
                        if (i % 2 == 0) renderer.drawCircle(lx, ly, 3.0f, steelR, steelG, steelB, 0.95f, 6);
                        px = lx; py = ly;
                    }
                    renderer.drawCircle(tipX, tipY, 8.5f, 0.85f, 0.88f, 0.95f, 1.0f, 12);
                    renderer.drawTriangle(tipX, tipY + 6.0f, tipX, tipY - 6.0f, tipX + dir * 18.0f, tipY, steelR, steelG, steelB);
                    renderer.drawTriangle(tipX - 4.0f, tipY + 5.0f, tipX + 4.0f, tipY + 5.0f, tipX, tipY + 16.0f, 0.70f, 0.75f, 0.85f);
                    renderer.drawTriangle(tipX - 4.0f, tipY - 5.0f, tipX + 4.0f, tipY - 5.0f, tipX, tipY - 16.0f, 0.70f, 0.75f, 0.85f);
                } else if (state == FighterState::SPECIAL) {
                    float ext = (phase == AttackPhase::STARTUP) ? 45.0f : (phase == AttackPhase::ACTIVE ? 215.0f : 70.0f);
                    float tipX = handF.x + dir * ext;
                    float tipY = handF.y + 4.0f;
                    renderer.drawLine(handF.x, handF.y, tipX, tipY, 0.82f, 0.86f, 0.92f, 1.0f, 3.8f);
                    drawLightningBolt(renderer, handF.x, handF.y, tipX, tipY, stateTime, 12.0f, 0.35f, 0.92f, 1.0f, 2.8f);
                    drawLightningBolt(renderer, handF.x, handF.y, tipX, tipY, stateTime + 1.7f, 8.0f, 0.95f, 1.0f, 1.0f, 1.6f);
                    renderer.drawCircle(tipX, tipY, 9.5f, 0.35f, 0.90f, 1.0f, 0.95f, 12);
                } else {
                    float chainLen = activeSwing ? 94.0f : 56.0f;
                    if (!activeSwing) {
                        ux = dir * 0.62f;
                        uy = -0.78f;
                    }
                    float tipX = handF.x + ux * chainLen;
                    float tipY = handF.y + uy * chainLen;
                    // Forged steel chain links with razor spikes
                    for (int i = 0; i < 7; ++i) {
                        float t0 = (float)i / 7.0f;
                        float t1 = (float)(i + 1) / 7.0f;
                        float ax = handF.x + ux * chainLen * t0;
                        float ay = handF.y + uy * chainLen * t0;
                        float bx = handF.x + ux * chainLen * t1;
                        float by = handF.y + uy * chainLen * t1;
                        renderer.drawLine(ax, ay, bx, by, 0.80f, 0.84f, 0.90f, 1.0f, (i % 2 == 0) ? 4.2f : 2.8f);
                        if (i >= 2) {
                            float spX = bx - uy * 6.5f * dir;
                            float spY = by + ux * 6.5f * dir;
                            renderer.drawTriangle(ax, ay, bx, by, spX, spY, steelR, steelG, steelB);
                        }
                    }
                    renderer.drawCircle(tipX, tipY, 7.2f, steelR, steelG, steelB, 1.0f, 10);
                    renderer.drawTriangle(tipX - 3.0f, tipY + 4.0f, tipX + 3.0f, tipY + 4.0f,
                                          tipX + ux * 13.0f, tipY + uy * 13.0f, steelR, steelG, steelB);
                    if (activeSwing) {
                        renderer.drawLine(handF.x, handF.y, tipX + ux * 20.0f, tipY + uy * 20.0f, accR, accG, accB, 0.70f, 3.2f);
                    }
                }
                break;
            }

            case WeaponType::WAR_BOW: {
                // Quiver on back in rich brown leather
                renderer.drawQuad(chest.x - dir * 10.0f, chest.y + 10.0f,
                                  chest.x - dir * 4.0f,  chest.y + 8.0f,
                                  chest.x - dir * 12.0f, chest.y - 16.0f,
                                  chest.x - dir * 18.0f, chest.y - 14.0f,
                                  0.42f, 0.26f, 0.16f);
                // Quiver strap
                renderer.drawLine(chest.x - dir * 10.0f, chest.y + 10.0f, chest.x + dir * 6.0f, chest.y - 10.0f,
                                  0.32f, 0.18f, 0.12f, 0.95f, 2.0f);
                // Arrows visible in quiver with white feather fletchings
                renderer.drawLine(chest.x - dir * 9.0f, chest.y + 10.0f, chest.x - dir * 13.0f, chest.y + 22.0f, 0.95f, 0.95f, 0.98f, 1.0f, 2.4f);
                renderer.drawLine(chest.x - dir * 6.0f, chest.y + 9.0f,  chest.x - dir * 8.0f,  chest.y + 21.0f, 0.95f, 0.95f, 0.98f, 1.0f, 2.2f);

                float bowX = handF.x + dir * 3.0f;
                float bowY = handF.y;
                float topX = bowX - dir * 10.0f, topY = bowY + 32.0f;
                float midTopX = bowX + dir * 7.0f, midTopY = bowY + 16.0f;
                float midBotX = bowX + dir * 7.0f, midBotY = bowY - 16.0f;
                float botX = bowX - dir * 10.0f, botY = bowY - 32.0f;

                // Seasoned yew wood recurve bow limbs with brass tips
                renderer.drawLine(topX, topY, midTopX, midTopY, 0.58f, 0.35f, 0.18f, 1.0f, 3.6f);
                renderer.drawLine(midTopX, midTopY, midBotX, midBotY, 0.48f, 0.28f, 0.14f, 1.0f, 4.0f);
                renderer.drawLine(midBotX, midBotY, botX, botY, 0.58f, 0.35f, 0.18f, 1.0f, 3.6f);
                // Brass nocks
                renderer.drawCircle(topX, topY, 2.5f, 0.92f, 0.78f, 0.25f, 1.0f, 6);
                renderer.drawCircle(botX, botY, 2.5f, 0.92f, 0.78f, 0.25f, 1.0f, 6);

                bool drawingBow = (state == FighterState::COMBO3) || (state == FighterState::SPECIAL && stateTime >= 0.34f && stateTime < 0.60f);
                float stringPullX = drawingBow ? (handB.x) : (topX);
                float stringPullY = bowY;
                // Silver tension bowstring
                renderer.drawLine(topX, topY, stringPullX, stringPullY, 0.90f, 0.92f, 0.96f, 0.90f, 1.4f);
                renderer.drawLine(stringPullX, stringPullY, botX, botY, 0.90f, 0.92f, 0.96f, 0.90f, 1.4f);
                if (drawingBow) {
                    // War arrow nocked & drawn
                    renderer.drawLine(stringPullX, stringPullY, bowX + dir * 32.0f, bowY, 0.65f, 0.45f, 0.25f, 1.0f, 2.6f);
                    renderer.drawTriangle(bowX + dir * 28.0f, bowY + 4.0f,
                                          bowX + dir * 28.0f, bowY - 4.0f,
                                          bowX + dir * 40.0f, bowY, steelR, steelG, steelB);
                }
                if (activeSwing && state != FighterState::SPECIAL) {
                    renderer.drawLine(handF.x, handF.y, handF.x + ux * 28.0f, handF.y + uy * 28.0f, accR, accG, accB, 0.70f, 3.0f);
                }
                break;
            }

            case WeaponType::DUAL_FANS: {
                if (!hideFan1) {
                    float fRadius = (activeSwing || state == FighterState::IDLE) ? 24.0f : 19.0f;
                    float fSpread = activeSwing ? 1.50f : 1.15f;
                    drawOpenFan(renderer, handF, ux, uy, fRadius, fSpread, 0.9f, 0.2f, 0.3f, accR, accG, accB);
                    if (activeSwing) {
                        renderer.drawLine(handF.x, handF.y, handF.x + ux * 28.0f, handF.y + uy * 28.0f, accR, accG, accB, 0.75f, 3.0f);
                    }
                }
                if (!hideFan2) {
                    float bx = -dir * 0.45f;
                    float by = -0.88f;
                    drawOpenFan(renderer, handB, bx, by, 19.0f, 1.05f, 0.9f, 0.2f, 0.3f, accR * 0.8f, accG * 0.8f, accB * 0.8f);
                }
                break;
            }

            case WeaponType::WAR_SPEAR: {
                if (hideMainSpear) break;

                if (state == FighterState::BLOCK) {
                    ux = 0.08f * dir;
                    uy = 0.99f;
                } else if (state == FighterState::SPECIAL) {
                    if (stateTime < 1.38f) {
                        ux = 0.0f;
                        uy = 1.0f;
                    } else {
                        ux = dir;
                        uy = 0.03f;
                    }
                } else if (state == FighterState::COMBO2 && phase == AttackPhase::ACTIVE) {
                    ux = dir * 0.78f;
                    uy = -0.62f + actionProgress * 0.9f;
                    float l = std::sqrt(ux * ux + uy * uy);
                    ux /= l; uy /= l;
                } else if (isComboStrike && phase == AttackPhase::ACTIVE) {
                    ux = dir;
                    uy = 0.04f;
                } else if (state == FighterState::IDLE || state == FighterState::WALK) {
                    ux = dir * 0.88f;
                    uy = -0.47f; // Downward-forward warrior spear angle
                }
                nx = -uy; ny = ux;

                float anchorX = (state == FighterState::SPECIAL && stateTime < 1.38f) ? handF.x : (handF.x + handB.x) * 0.5f + dir * 10.0f;
                float anchorY = (state == FighterState::SPECIAL && stateTime < 1.38f) ? handF.y : (handF.y + handB.y) * 0.5f;
                if (state == FighterState::BLOCK) { anchorX = handF.x + dir * 4.0f; anchorY = chest.y - 6.0f; }

                float tailX = anchorX - ux * 54.0f;
                float tailY = anchorY - uy * 54.0f;
                float headBaseX = anchorX + ux * 58.0f;
                float headBaseY = anchorY + uy * 58.0f;
                float tipX = anchorX + ux * 84.0f;
                float tipY = anchorY + uy * 84.0f;

                // Dark polished hardwood spear shaft
                renderer.drawLine(tailX, tailY, headBaseX, headBaseY, 0.48f, 0.28f, 0.15f, 1.0f, 4.6f);
                // Brass collar at spearhead base
                renderer.drawCircle(headBaseX, headBaseY, 4.2f, 0.92f, 0.78f, 0.25f, 1.0f, 8);
                // Crimson silk horsehair tassel
                renderer.drawTriangle(headBaseX, headBaseY,
                                      headBaseX - nx * 4.5f - ux * 8.0f, headBaseY - ny * 4.5f - uy * 8.0f,
                                      headBaseX + nx * 4.5f - ux * 8.0f, headBaseY + ny * 4.5f - uy * 8.0f,
                                      0.92f, 0.18f, 0.22f);
                // Forged mirror steel blade
                renderer.drawTriangle(headBaseX + nx * 5.8f, headBaseY + ny * 5.8f,
                                      headBaseX - nx * 5.8f, headBaseY - ny * 5.8f,
                                      tipX, tipY, steelR, steelG, steelB);
                renderer.drawLine(headBaseX, headBaseY, tipX, tipY, 1.0f, 1.0f, 1.0f, 0.95f, 1.8f);

                if (state == FighterState::SPECIAL && stateTime < 1.38f) {
                    float glow = 16.0f + std::sin(stateTime * 30.0f) * 5.0f;
                    renderer.drawCircle(tipX, tipY, glow, accR, accG, accB, 0.60f, 16);
                    renderer.drawCircle(tipX, tipY, glow * 0.45f, 1.0f, 1.0f, 0.85f, 0.95f, 10);
                } else if (activeSwing) {
                    renderer.drawLine(headBaseX, headBaseY, tipX + ux * 32.0f, tipY + uy * 32.0f, accR, accG, accB, 0.78f, 4.0f);
                }
                break;
            }
        }
        (void)fighter;
    }

    // =========================================================================
    // ANATOMICAL REAL-HUMAN SILHOUETTE BUILDERS (MALE MUSCLE, HEAVY WARRIOR & FEMALE FIGURE)
    // =========================================================================
    struct BodyBuild {
        bool  isFemale;
        bool  isHeavyWarrior;
        float shoulderOut;
        float shoulderBackOut;
        float chestFront;
        float chestBack;
        float waistFront;
        float waistBack;
        float hipFront;
        float hipBack;
        float neckBotR;
        float neckTopR;
        float deltoidR;
        float bicepHipR;
        float bicepMidR;
        float elbowR;
        float foreTopR;
        float foreMidR;
        float wristR;
        float handScale;
        float thighTopR;
        float thighMidR;
        float kneeR;
        float calfTopR;
        float calfMidR;
        float ankleR;
        float bootCuffR;
        float craniumR;
        float faceScale;
    };

    BodyBuild getBodyBuild(CharacterId charId, bool isFemale) {
        if (isFemale) {
            return {
                true, false,
                10.5f, 10.0f,   // Soft feminine shoulder span
                19.2f, 12.2f,   // Real curved bust projection & upper back
                9.2f,  8.8f,    // Trim feminine hourglass waist
                14.8f, 18.2f,   // Full curved hips & glutes
                4.7f,  4.2f,    // Graceful neck
                6.5f,           // Soft deltoid
                6.2f,  5.5f,  4.3f,   // Slender sculpted upper arm
                4.4f,  4.7f,  3.3f,   // Tapered forearm
                0.84f,                // Feminine hand
                13.8f, 12.2f, 7.4f,   // Full shapely thigh tapering to knee
                7.0f,  8.2f,  4.4f,   // Toned calf & slender ankle
                4.9f,                 // Sleek boot cuff
                10.0f, 0.90f          // Refined cranium & facial profile
            };
        }

        if (charId == CharacterId::SPEAR_WARRIOR) {
            // TAKEDA: Heavy armored & heavily muscled powerhouse spear warrior
            return {
                false, true,
                18.5f, 17.5f,   // Massive broad shoulders
                27.0f, 25.0f,   // Thick barrel chest & wide lats
                17.2f, 16.2f,   // Thick powerful waist & core
                21.0f, 20.5f,   // Heavy hips & war plates
                9.4f,  8.2f,    // Bull neck & thick traps
                15.2f,          // Huge muscular deltoid / war pauldron
                12.2f, 11.4f, 8.2f,   // Heavy muscular bicep & tricep
                8.4f,  9.0f,  5.6f,   // Thick powerful forearm
                1.22f,                // Big warrior gauntlet/fist
                18.2f, 17.0f, 11.6f,  // Tree-trunk quad & hamstring
                11.2f, 12.4f, 7.2f,   // Massive calf muscle
                8.4f,                 // Heavy greave cuff
                12.0f, 1.08f          // Strong jaw & larger warrior cranium
            };
        }

        // Strong muscular male build (KAI & KURO, Kuro slightly broader/armored)
        float bulk = (charId == CharacterId::CHAIN_WARRIOR) ? 1.07f : 1.00f;
        return {
            false, false,
            14.8f * bulk, 14.0f * bulk,   // Broad muscular shoulders
            21.8f * bulk, 19.8f * bulk,   // Strong chest/pecs & V-taper lats
            13.2f * bulk, 12.6f * bulk,   // Athletic defined waist
            16.8f * bulk, 16.4f * bulk,   // Solid hips
            7.8f  * bulk, 6.8f  * bulk,   // Strong neck
            11.8f * bulk,                 // Rounded muscular deltoid
            9.8f  * bulk, 9.1f  * bulk, 6.6f * bulk,   // Sculpted bicep/tricep
            7.0f  * bulk, 7.5f  * bulk, 4.8f * bulk,   // Strong forearm
            1.05f * bulk,                              // Strong male fist/hand
            15.2f * bulk, 14.0f * bulk, 9.5f * bulk,   // Muscular quads
            9.2f  * bulk, 9.9f  * bulk, 5.8f * bulk,   // Athletic calves
            6.8f  * bulk,                              // Cinched warrior boot cuff
            11.2f, 1.02f                               // Strong masculine jawline
        };
    }

    // =========================================================================
    // REAL HUMAN CHARACTER APPEARANCE, CLOTHING, FACIAL FEATURES & SKIN TONES
    // =========================================================================
    struct CharacterAppearance {
        // Human Skin Tone
        float skinR, skinG, skinB;
        float skinShadowR, skinShadowG, skinShadowB;
        bool  isFemale;
        bool  isHeavyWarrior;

        // Eyes & Facial Features
        float eyeWhiteR, eyeWhiteG, eyeWhiteB;
        float irisR, irisG, irisB;
        float pupilR, pupilG, pupilB;
        float browR, browG, browB;
        bool  hasLips;
        float lipR, lipG, lipB;
        bool  hasNinjaMask;
        float maskR, maskG, maskB;
        bool  hasKabuto;
        float helmR, helmG, helmB;
        float crestR, crestG, crestB;

        // Hair & Headbands / Accessories
        float hairR, hairG, hairB;
        float ribbonR, ribbonG, ribbonB;
        float pinR, pinG, pinB;

        // Torso / Clothing
        bool  bareChest;
        float chestR, chestG, chestB;
        float chestShadowR, chestShadowG, chestShadowB;
        float chestTrimR, chestTrimG, chestTrimB;
        float sashR, sashG, sashB;
        float sashTrimR, sashTrimG, sashTrimB;

        // Arms & Hands
        bool  bareUpperArm;
        float sleeveR, sleeveG, sleeveB;
        bool  hasBracer;
        float bracerR, bracerG, bracerB;
        bool  hasHandWraps;
        float wrapR, wrapG, wrapB;
        float gloveR, gloveG, gloveB;
        bool  hasGauntlet;
        float gauntletR, gauntletG, gauntletB;

        // Legs & Footwear
        float pantsR, pantsG, pantsB;
        float pantsShadowR, pantsShadowG, pantsShadowB;
        bool  hasGreaves;
        float greaveR, greaveG, greaveB;
        float bootR, bootG, bootB;
        float bootSoleR, bootSoleG, bootSoleB;
        float bootTrimR, bootTrimG, bootTrimB;
    };

    CharacterAppearance getCharacterAppearance(CharacterId charId) {
        CharacterAppearance a = {};
        a.eyeWhiteR = 0.98f; a.eyeWhiteG = 0.98f; a.eyeWhiteB = 1.0f;
        a.pupilR = 0.04f; a.pupilG = 0.04f; a.pupilB = 0.05f;

        switch (charId) {
            case CharacterId::BRAWLER: { // Kai - Agile Martial Monk / Street Brawler
                a.isFemale = false; a.isHeavyWarrior = false;
                a.skinR = 0.88f; a.skinG = 0.69f; a.skinB = 0.53f;
                a.skinShadowR = 0.74f; a.skinShadowG = 0.55f; a.skinShadowB = 0.40f;
                a.irisR = 0.28f; a.irisG = 0.17f; a.irisB = 0.10f;
                a.browR = 0.14f; a.browG = 0.12f; a.browB = 0.10f;
                a.hasLips = false; a.lipR = 0.78f; a.lipG = 0.52f; a.lipB = 0.44f;
                a.hasNinjaMask = false; a.hasKabuto = false;

                a.hairR = 0.12f; a.hairG = 0.12f; a.hairB = 0.14f;
                a.ribbonR = 0.18f; a.ribbonG = 0.20f; a.ribbonB = 0.25f;

                a.bareChest = true; // Bare muscular chest and six-pack abs!
                a.chestR = a.skinR; a.chestG = a.skinG; a.chestB = a.skinB;
                a.chestShadowR = a.skinShadowR; a.chestShadowG = a.skinShadowG; a.chestShadowB = a.skinShadowB;
                a.chestTrimR = 0.20f; a.chestTrimG = 0.85f; a.chestTrimB = 0.95f;

                a.sashR = 0.15f; a.sashG = 0.82f; a.sashB = 0.95f;
                a.sashTrimR = 0.40f; a.sashTrimG = 0.92f; a.sashTrimB = 1.0f;

                a.bareUpperArm = true;
                a.sleeveR = a.skinR; a.sleeveG = a.skinG; a.sleeveB = a.skinB;
                a.hasBracer = false;
                a.hasHandWraps = true;
                a.wrapR = 0.94f; a.wrapG = 0.94f; a.wrapB = 0.96f;
                a.gloveR = a.wrapR; a.gloveG = a.wrapG; a.gloveB = a.wrapB;
                a.hasGauntlet = false;

                a.pantsR = 0.20f; a.pantsG = 0.22f; a.pantsB = 0.28f;
                a.pantsShadowR = 0.14f; a.pantsShadowG = 0.15f; a.pantsShadowB = 0.20f;
                a.hasGreaves = false;

                a.bootR = 0.12f; a.bootG = 0.12f; a.bootB = 0.14f;
                a.bootSoleR = 0.88f; a.bootSoleG = 0.88f; a.bootSoleB = 0.90f;
                a.bootTrimR = a.wrapR; a.bootTrimG = a.wrapG; a.bootTrimB = a.wrapB;
                break;
            }

            case CharacterId::CHAIN_WARRIOR: { // Kuro - Spiked Chain Ninja
                a.isFemale = false; a.isHeavyWarrior = false;
                a.skinR = 0.84f; a.skinG = 0.69f; a.skinB = 0.59f;
                a.skinShadowR = 0.68f; a.skinShadowG = 0.53f; a.skinShadowB = 0.43f;
                a.irisR = 0.45f; a.irisG = 0.85f; a.irisB = 1.0f; // Piercing cyan ninja eyes
                a.browR = 0.10f; a.browG = 0.10f; a.browB = 0.12f;
                a.hasLips = false; a.hasNinjaMask = true;
                a.maskR = 0.16f; a.maskG = 0.17f; a.maskB = 0.22f;
                a.hasKabuto = false;

                a.hairR = 0.14f; a.hairG = 0.15f; a.hairB = 0.19f;
                a.ribbonR = 0.52f; a.ribbonG = 0.58f; a.ribbonB = 0.68f;

                a.bareChest = false;
                a.chestR = 0.18f; a.chestG = 0.20f; a.chestB = 0.26f;
                a.chestShadowR = 0.12f; a.chestShadowG = 0.13f; a.chestShadowB = 0.18f;
                a.chestTrimR = 0.48f; a.chestTrimG = 0.54f; a.chestTrimB = 0.64f; // Steel chest plate

                a.sashR = 0.22f; a.sashG = 0.22f; a.sashB = 0.26f;
                a.sashTrimR = 0.80f; a.sashTrimG = 0.84f; a.sashTrimB = 0.90f;

                a.bareUpperArm = false;
                a.sleeveR = 0.18f; a.sleeveG = 0.20f; a.sleeveB = 0.26f;
                a.hasBracer = false; a.hasHandWraps = false;
                a.gloveR = 0.16f; a.gloveG = 0.17f; a.gloveB = 0.20f;
                a.hasGauntlet = true;
                a.gauntletR = 0.50f; a.gauntletG = 0.56f; a.gauntletB = 0.66f;

                a.pantsR = 0.16f; a.pantsG = 0.17f; a.pantsB = 0.22f;
                a.pantsShadowR = 0.11f; a.pantsShadowG = 0.12f; a.pantsShadowB = 0.16f;
                a.hasGreaves = false;

                a.bootR = 0.14f; a.bootG = 0.14f; a.bootB = 0.18f;
                a.bootSoleR = 0.28f; a.bootSoleG = 0.28f; a.bootSoleB = 0.32f;
                a.bootTrimR = 0.32f; a.bootTrimG = 0.34f; a.bootTrimB = 0.40f;
                break;
            }

            case CharacterId::ARCHER: { // Akari - Female Kyudo Archer
                a.isFemale = true; a.isHeavyWarrior = false;
                a.skinR = 0.93f; a.skinG = 0.79f; a.skinB = 0.69f;
                a.skinShadowR = 0.80f; a.skinShadowG = 0.65f; a.skinShadowB = 0.55f;
                a.irisR = 0.28f; a.irisG = 0.58f; a.irisB = 0.36f; // Hazel-emerald eyes
                a.browR = 0.22f; a.browG = 0.15f; a.browB = 0.10f;
                a.hasLips = true;
                a.lipR = 0.90f; a.lipG = 0.40f; a.lipB = 0.46f; // Soft rosy feminine lips
                a.hasNinjaMask = false; a.hasKabuto = false;

                a.hairR = 0.24f; a.hairG = 0.14f; a.hairB = 0.08f; // Dark chestnut brunette
                a.ribbonR = 0.92f; a.ribbonG = 0.22f; a.ribbonB = 0.22f; // Crimson ribbon

                a.bareChest = false;
                a.chestR = 0.22f; a.chestG = 0.44f; a.chestB = 0.28f; // Forest green leather tunic
                a.chestShadowR = 0.16f; a.chestShadowG = 0.30f; a.chestShadowB = 0.18f;
                a.chestTrimR = 0.90f; a.chestTrimG = 0.72f; a.chestTrimB = 0.25f; // Golden-amber trim

                a.sashR = 0.85f; a.sashG = 0.65f; a.sashB = 0.20f;
                a.sashTrimR = 0.95f; a.sashTrimG = 0.85f; a.sashTrimB = 0.30f;

                a.bareUpperArm = true; // Slender fair feminine arms
                a.sleeveR = a.skinR; a.sleeveG = a.skinG; a.sleeveB = a.skinB;
                a.hasBracer = true; // Brown leather archer's armguard
                a.bracerR = 0.42f; a.bracerG = 0.26f; a.bracerB = 0.16f;
                a.hasHandWraps = false;
                a.gloveR = a.skinR; a.gloveG = a.skinG; a.gloveB = a.skinB;
                a.hasGauntlet = false;

                a.pantsR = 0.18f; a.pantsG = 0.17f; a.pantsB = 0.21f; // Fitted dark hunting leggings
                a.pantsShadowR = 0.12f; a.pantsShadowG = 0.12f; a.pantsShadowB = 0.15f;
                a.hasGreaves = false;

                a.bootR = 0.38f; a.bootG = 0.24f; a.bootB = 0.16f; // Tall leather ranger boots
                a.bootSoleR = 0.18f; a.bootSoleG = 0.14f; a.bootSoleB = 0.10f;
                a.bootTrimR = 0.88f; a.bootTrimG = 0.72f; a.bootTrimB = 0.30f;
                break;
            }

            case CharacterId::FAN_DANCER: { // Mei-Ling - Female Fan Dancer
                a.isFemale = true; a.isHeavyWarrior = false;
                a.skinR = 0.96f; a.skinG = 0.82f; a.skinB = 0.75f;
                a.skinShadowR = 0.85f; a.skinShadowG = 0.70f; a.skinShadowB = 0.62f;
                a.irisR = 0.22f; a.irisG = 0.16f; a.irisB = 0.16f; // Almond dark eyes
                a.browR = 0.14f; a.browG = 0.12f; a.browB = 0.14f;
                a.hasLips = true;
                a.lipR = 0.94f; a.lipG = 0.22f; a.lipB = 0.36f; // Vibrant rose-crimson lips
                a.hasNinjaMask = false; a.hasKabuto = false;

                a.hairR = 0.10f; a.hairG = 0.10f; a.hairB = 0.12f; // Jet black twin odango buns
                a.pinR = 0.98f; a.pinG = 0.82f; a.pinB = 0.25f; // Gold floral hairpins
                a.ribbonR = 0.85f; a.ribbonG = 0.22f; a.ribbonB = 0.35f;

                a.bareChest = false;
                a.chestR = 0.84f; a.chestG = 0.18f; a.chestB = 0.32f; // Imperial rose cheongsam
                a.chestShadowR = 0.65f; a.chestShadowG = 0.12f; a.chestShadowB = 0.22f;
                a.chestTrimR = 0.98f; a.chestTrimG = 0.84f; a.chestTrimB = 0.28f; // Gold lotus embroidery trim

                a.sashR = 0.95f; a.sashG = 0.78f; a.sashB = 0.22f;
                a.sashTrimR = 0.98f; a.sashTrimG = 0.88f; a.sashTrimB = 0.40f;

                a.bareUpperArm = true;
                a.sleeveR = a.skinR; a.sleeveG = a.skinG; a.sleeveB = a.skinB;
                a.hasBracer = false; a.hasHandWraps = false;
                a.gloveR = a.skinR; a.gloveG = a.skinG; a.gloveB = a.skinB;
                a.hasGauntlet = false;

                a.pantsR = 0.18f; a.pantsG = 0.15f; a.pantsB = 0.20f; // Midnight silk leggings
                a.pantsShadowR = 0.12f; a.pantsShadowG = 0.10f; a.pantsShadowB = 0.14f;
                a.hasGreaves = false;

                a.bootR = 0.72f; a.bootG = 0.16f; a.bootB = 0.28f; // Rose silk martial boots
                a.bootSoleR = 0.25f; a.bootSoleG = 0.15f; a.bootSoleB = 0.18f;
                a.bootTrimR = 0.98f; a.bootTrimG = 0.82f; a.bootTrimB = 0.25f;
                break;
            }

            case CharacterId::SPEAR_WARRIOR: { // Takeda - Colossus Spear Samurai
                a.isFemale = false; a.isHeavyWarrior = true;
                a.skinR = 0.80f; a.skinG = 0.64f; a.skinB = 0.50f;
                a.skinShadowR = 0.66f; a.skinShadowG = 0.50f; a.skinShadowB = 0.38f;
                a.irisR = 0.24f; a.irisG = 0.17f; a.irisB = 0.12f;
                a.browR = 0.15f; a.browG = 0.12f; a.browB = 0.10f;
                a.hasLips = false; a.hasNinjaMask = false;
                a.hasKabuto = true;
                a.helmR = 0.20f; a.helmG = 0.20f; a.helmB = 0.24f; // Iron kabuto helmet
                a.crestR = 0.96f; a.crestG = 0.80f; a.crestB = 0.22f; // Golden crescent moon

                a.hairR = 0.12f; a.hairG = 0.12f; a.hairB = 0.14f;

                a.bareChest = false;
                a.chestR = 0.50f; a.chestG = 0.14f; a.chestB = 0.14f; // Lacquered crimson cuirass
                a.chestShadowR = 0.35f; a.chestShadowG = 0.10f; a.chestShadowB = 0.10f;
                a.chestTrimR = 0.94f; a.chestTrimG = 0.78f; a.chestTrimB = 0.22f; // Gold armor lacing bands

                a.sashR = 0.45f; a.sashG = 0.12f; a.sashB = 0.12f;
                a.sashTrimR = 0.94f; a.sashTrimG = 0.78f; a.sashTrimB = 0.22f;

                a.bareUpperArm = false;
                a.sleeveR = 0.18f; a.sleeveG = 0.20f; a.sleeveB = 0.26f;
                a.hasBracer = false; a.hasHandWraps = false;
                a.hasGauntlet = true;
                a.gauntletR = 0.30f; a.gauntletG = 0.32f; a.gauntletB = 0.36f; // Iron kote
                a.gloveR = a.gauntletR; a.gloveG = a.gauntletG; a.gloveB = a.gauntletB;

                a.pantsR = 0.18f; a.pantsG = 0.20f; a.pantsB = 0.26f; // Samurai hakama
                a.pantsShadowR = 0.12f; a.pantsShadowG = 0.14f; a.pantsShadowB = 0.18f;
                a.hasGreaves = true;
                a.greaveR = 0.30f; a.greaveG = 0.32f; a.greaveB = 0.36f; // Iron suneate greaves

                a.bootR = 0.24f; a.bootG = 0.25f; a.bootB = 0.28f;
                a.bootSoleR = 0.15f; a.bootSoleG = 0.16f; a.bootSoleB = 0.18f;
                a.bootTrimR = 0.94f; a.bootTrimG = 0.78f; a.bootTrimB = 0.22f;
                break;
            }
            case CharacterId::COUNT: break;
        }
        return a;
    }

    void drawHumanBoot(const Renderer& renderer, Vec2 ankle, Vec2 toe, float dir, const BodyBuild& b,
                       const CharacterAppearance& app, bool isBack, float hitFlash) {
        float r = isBack ? (app.bootR * 0.84f) : app.bootR;
        float g = isBack ? (app.bootG * 0.84f) : app.bootG;
        float bCol = isBack ? (app.bootB * 0.84f) : app.bootB;
        if (hitFlash > 0.0f) {
            r = r * (1.0f - hitFlash) + 0.95f * hitFlash;
            g = g * (1.0f - hitFlash) + 0.20f * hitFlash;
            bCol = bCol * (1.0f - hitFlash) + 0.20f * hitFlash;
        }

        float heelBack = b.isFemale ? 5.2f : (b.isHeavyWarrior ? 8.6f : 7.4f);
        float soleDrop = b.isFemale ? 3.5f : (b.isHeavyWarrior ? 5.0f : 4.2f);
        float frontTop = b.isFemale ? 7.6f : (b.isHeavyWarrior ? 11.8f : 10.0f);
        float archBack = b.isFemale ? 4.2f : (b.isHeavyWarrior ? 7.2f : 5.8f);
        float soleY    = std::min(ankle.y, toe.y) - soleDrop;

        float xs[6] = {
            ankle.x - dir * heelBack,
            ankle.x - dir * (heelBack + 1.5f),
            ankle.x + dir * frontTop,
            toe.x + dir * (b.isHeavyWarrior ? 4.2f : 2.8f),
            toe.x,
            ankle.x + dir * archBack
        };
        float ys[6] = {
            ankle.y + (b.isFemale ? 1.8f : 2.4f),
            soleY,
            soleY,
            soleY + 1.4f,
            toe.y + (b.isFemale ? 3.2f : (b.isHeavyWarrior ? 5.2f : 4.2f)),
            ankle.y + 1.5f
        };
        renderer.drawPolygon(xs, ys, 6, r, g, bCol);

        // Boot sole tread line
        float sR = app.bootSoleR, sG = app.bootSoleG, sB = app.bootSoleB;
        renderer.drawLine(ankle.x - dir * (heelBack + 1.5f), soleY + 0.6f,
                          toe.x + dir * (b.isHeavyWarrior ? 4.2f : 2.8f), soleY + 0.6f,
                          sR, sG, sB, 0.95f, b.isHeavyWarrior ? 2.8f : 2.0f);

        // Boot cuff / trim / cloth wrap
        float cR = app.hasHandWraps ? app.wrapR : app.bootTrimR;
        float cG = app.hasHandWraps ? app.wrapG : app.bootTrimG;
        float cB = app.hasHandWraps ? app.wrapB : app.bootTrimB;
        renderer.drawCircle(ankle.x, ankle.y + (b.isFemale ? 4.5f : 6.0f), b.bootCuffR * 0.92f, cR, cG, cB, 1.0f, 12);
    }

    void drawHumanLeg(const Renderer& renderer, Vec2 hip, Vec2 knee, Vec2 ankle, Vec2 toe,
                      float dir, const BodyBuild& b, const CharacterAppearance& app,
                      bool isBack, float hitFlash) {
        Vec2 midThigh = {
            (hip.x + knee.x) * 0.5f + dir * (b.isFemale ? 2.4f : 2.0f),
            (hip.y + knee.y) * 0.5f + 1.0f
        };
        Vec2 midCalf = {
            (knee.x + ankle.x) * 0.5f - dir * (b.isFemale ? 2.6f : 3.2f),
            (knee.y + ankle.y) * 0.5f + 5.0f
        };

        float pR = isBack ? app.pantsShadowR : app.pantsR;
        float pG = isBack ? app.pantsShadowG : app.pantsG;
        float pB = isBack ? app.pantsShadowB : app.pantsB;
        if (hitFlash > 0.0f) {
            pR = pR * (1.0f - hitFlash) + 0.95f * hitFlash;
            pG = pG * (1.0f - hitFlash) + 0.20f * hitFlash;
            pB = pB * (1.0f - hitFlash) + 0.20f * hitFlash;
        }

        // Sculpted human thigh (Pants / Leggings)
        renderer.drawSculptedLimb(hip.x, hip.y, midThigh.x, midThigh.y, knee.x, knee.y,
                                  b.thighTopR, b.thighMidR, b.kneeR, pR, pG, pB);

        // Sculpted human calf (Hakama / Leggings / Shin Wraps / Greaves)
        float calfR = pR, calfG = pG, calfB = pB;
        if (app.hasGreaves) {
            calfR = isBack ? (app.greaveR * 0.85f) : app.greaveR;
            calfG = isBack ? (app.greaveG * 0.85f) : app.greaveG;
            calfB = isBack ? (app.greaveB * 0.85f) : app.greaveB;
        } else if (app.hasHandWraps) {
            calfR = isBack ? (app.wrapR * 0.88f) : app.wrapR;
            calfG = isBack ? (app.wrapG * 0.88f) : app.wrapG;
            calfB = isBack ? (app.wrapB * 0.88f) : app.wrapB;
        }
        renderer.drawSculptedLimb(knee.x, knee.y, midCalf.x, midCalf.y, ankle.x, ankle.y,
                                  b.calfTopR, b.calfMidR, b.ankleR, calfR, calfG, calfB);

        // Greave gold rib or cloth wrap bands
        if (app.hasGreaves) {
            renderer.drawLine(knee.x + dir * 2.0f, knee.y - 2.0f, ankle.x + dir * 2.0f, ankle.y + 4.0f,
                              0.94f, 0.78f, 0.22f, 0.90f, 1.8f);
        } else if (app.hasHandWraps) {
            renderer.drawLine(knee.x - dir * 2.0f, knee.y - 6.0f, ankle.x + dir * 2.0f, ankle.y + 6.0f,
                              0.78f, 0.78f, 0.82f, 0.85f, 1.2f);
        }

        drawHumanBoot(renderer, ankle, toe, dir, b, app, isBack, hitFlash);
    }

    void drawHumanHand(const Renderer& renderer, Vec2 elbow, Vec2 hand, float dir, const BodyBuild& b,
                       const CharacterAppearance& app, bool isBack, float hitFlash) {
        float dx = hand.x - elbow.x;
        float dy = hand.y - elbow.y;
        float len = std::sqrt(dx * dx + dy * dy);
        float ux = (len > 1.0f) ? (dx / len) : dir;
        float uy = (len > 1.0f) ? (dy / len) : 0.0f;
        float nx = -uy;
        float ny =  ux;

        float s = b.handScale;
        float xs[5] = {
            hand.x - nx * 4.2f * s,
            hand.x + ux * 6.8f * s - nx * 3.8f * s,
            hand.x + ux * 8.6f * s + nx * 1.6f * s,
            hand.x + ux * 4.8f * s + nx * 5.2f * s,
            hand.x + nx * 4.4f * s
        };
        float ys[5] = {
            hand.y - ny * 4.2f * s,
            hand.y + uy * 6.8f * s - ny * 3.8f * s,
            hand.y + uy * 8.6f * s + ny * 1.6f * s,
            hand.y + uy * 4.8f * s + ny * 5.2f * s,
            hand.y + ny * 4.4f * s
        };

        float r = app.skinR, g = app.skinG, bCol = app.skinB;
        if (app.hasHandWraps) {
            r = app.wrapR; g = app.wrapG; bCol = app.wrapB;
        } else if (app.hasNinjaMask) {
            r = app.gloveR; g = app.gloveG; bCol = app.gloveB;
        } else if (app.hasKabuto) {
            r = app.gauntletR; g = app.gauntletG; bCol = app.gauntletB;
        }
        if (isBack) { r *= 0.85f; g *= 0.85f; bCol *= 0.85f; }
        if (hitFlash > 0.0f) {
            r = r * (1.0f - hitFlash) + 0.95f * hitFlash;
            g = g * (1.0f - hitFlash) + 0.20f * hitFlash;
            bCol = bCol * (1.0f - hitFlash) + 0.20f * hitFlash;
        }
        renderer.drawPolygon(xs, ys, 5, r, g, bCol);
    }

    void drawHumanArm(const Renderer& renderer, Vec2 shoulder, Vec2 elbow, Vec2 hand,
                      float dir, const BodyBuild& b, const CharacterAppearance& app,
                      bool isBack, float hitFlash) {
        Vec2 midUpper = {
            (shoulder.x + elbow.x) * 0.5f + dir * (b.isFemale ? 0.8f : 2.2f),
            (shoulder.y + elbow.y) * 0.5f + 2.0f
        };
        Vec2 midFore = {
            (elbow.x + hand.x) * 0.5f + dir * (b.isFemale ? 0.4f : 1.0f),
            (elbow.y + hand.y) * 0.5f + 1.5f
        };

        // Upper arm (Athletic Skin or Cloth Sleeve)
        float uR, uG, uB;
        if (app.bareUpperArm) {
            uR = isBack ? app.skinShadowR : app.skinR;
            uG = isBack ? app.skinShadowG : app.skinG;
            uB = isBack ? app.skinShadowB : app.skinB;
        } else {
            uR = isBack ? (app.sleeveR * 0.85f) : app.sleeveR;
            uG = isBack ? (app.sleeveG * 0.85f) : app.sleeveG;
            uB = isBack ? (app.sleeveB * 0.85f) : app.sleeveB;
        }
        if (hitFlash > 0.0f) {
            uR = uR * (1.0f - hitFlash) + 0.95f * hitFlash;
            uG = uG * (1.0f - hitFlash) + 0.20f * hitFlash;
            uB = uB * (1.0f - hitFlash) + 0.20f * hitFlash;
        }
        renderer.drawSculptedLimb(shoulder.x, shoulder.y, midUpper.x, midUpper.y, elbow.x, elbow.y,
                                  b.bicepHipR, b.bicepMidR, b.elbowR, uR, uG, uB);

        // Forearm (Skin / Bracer / Wraps / Gauntlet)
        float fR, fG, fB;
        if (app.hasHandWraps) {
            fR = isBack ? (app.wrapR * 0.88f) : app.wrapR;
            fG = isBack ? (app.wrapG * 0.88f) : app.wrapG;
            fB = isBack ? (app.wrapB * 0.88f) : app.wrapB;
        } else if (app.hasBracer) {
            fR = isBack ? (app.bracerR * 0.85f) : app.bracerR;
            fG = isBack ? (app.bracerG * 0.85f) : app.bracerG;
            fB = isBack ? (app.bracerB * 0.85f) : app.bracerB;
        } else if (app.hasGauntlet) {
            fR = isBack ? (app.gauntletR * 0.85f) : app.gauntletR;
            fG = isBack ? (app.gauntletG * 0.85f) : app.gauntletG;
            fB = isBack ? (app.gauntletB * 0.85f) : app.gauntletB;
        } else {
            fR = isBack ? app.skinShadowR : app.skinR;
            fG = isBack ? app.skinShadowG : app.skinG;
            fB = isBack ? app.skinShadowB : app.skinB;
        }
        if (hitFlash > 0.0f) {
            fR = fR * (1.0f - hitFlash) + 0.95f * hitFlash;
            fG = fG * (1.0f - hitFlash) + 0.20f * hitFlash;
            fB = fB * (1.0f - hitFlash) + 0.20f * hitFlash;
        }
        renderer.drawSculptedLimb(elbow.x, elbow.y, midFore.x, midFore.y, hand.x, hand.y,
                                  b.foreTopR, b.foreMidR, b.wristR, fR, fG, fB);

        // Details on forearm:
        if (app.isFemale && !app.hasBracer) {
            renderer.drawCircle(hand.x - dir * 2.0f, hand.y, b.wristR * 1.15f, app.ribbonR, app.ribbonG, app.ribbonB, 1.0f, 10);
        } else if (app.hasBracer) {
            renderer.drawLine(midFore.x, midFore.y + 2.0f, midFore.x, midFore.y - 2.0f, 0.88f, 0.72f, 0.30f, 0.90f, 1.5f);
        } else if (app.hasGauntlet) {
            renderer.drawCircle(midFore.x, midFore.y, 2.0f, 0.92f, 0.78f, 0.25f, 1.0f, 6);
        }

        drawHumanHand(renderer, elbow, hand, dir, b, app, isBack, hitFlash);
    }

    void drawHumanTorso(const Renderer& renderer, Vec2 chest, Vec2 waist, Vec2 hips,
                        Vec2 shoulderF, Vec2 shoulderB, Vec2 neck, float dir, const BodyBuild& b,
                        const CharacterAppearance& app, CharacterId charId,
                        FighterState state, float stateTime, float hitFlash) {
        if (b.isFemale) {
            // Real-life woman hourglass figure: smooth upper chest -> curved bust -> under-bust -> narrow waist -> curved glutes & hips
            float upperChestY = chest.y + 3.0f;
            float bustPeakY   = chest.y - 8.5f;
            float underBustY  = chest.y - 18.0f;
            float ribMidY     = (chest.y + waist.y) * 0.50f;
            float hipY        = hips.y - 3.0f;

            float xs[10] = {
                chest.x - dir * b.chestBack,        // upper back / shoulder blade
                chest.x + dir * 11.2f,              // collarbone / upper sternum
                chest.x + dir * b.chestFront,       // full rounded feminine bust contour
                chest.x + dir * 11.6f,              // smooth curve under bust
                waist.x + dir * b.waistFront,       // narrow shapely front waist
                hips.x  + dir * b.hipFront,         // curved front hip
                hips.x  - dir * b.hipBack,          // rounded feminine hip / glute curve
                waist.x - dir * b.waistBack,        // hollow lumbar curve
                chest.x - dir * (b.chestBack * 0.92f),
                chest.x - dir * b.chestBack
            };
            float ys[10] = {
                upperChestY,
                upperChestY,
                bustPeakY,
                underBustY,
                waist.y,
                hipY,
                hipY,
                waist.y,
                ribMidY,
                upperChestY - 4.0f
            };

            float cR = app.chestR, cG = app.chestG, cB = app.chestB;
            if (hitFlash > 0.0f) {
                cR = cR * (1.0f - hitFlash) + 0.95f * hitFlash;
                cG = cG * (1.0f - hitFlash) + 0.20f * hitFlash;
                cB = cB * (1.0f - hitFlash) + 0.20f * hitFlash;
            }

            // Tunic / Dress polygon
            renderer.drawPolygon(xs, ys, 10, cR, cG, cB);

            // Rounded feminine bust projection
            renderer.drawCircle(chest.x + dir * 11.5f, chest.y - 9.0f, 7.6f, cR, cG, cB, 1.0f, 16);

            // Hips & glutes in fitted leggings
            float pR = app.pantsR, pG = app.pantsG, pB = app.pantsB;
            if (hitFlash > 0.0f) {
                pR = pR * (1.0f - hitFlash) + 0.95f * hitFlash;
                pG = pG * (1.0f - hitFlash) + 0.20f * hitFlash;
                pB = pB * (1.0f - hitFlash) + 0.20f * hitFlash;
            }
            renderer.drawCircle(hips.x - dir * 8.0f, hips.y - 1.5f, 10.6f, pR, pG, pB, 1.0f, 16);
            renderer.drawCircle(hips.x + dir * 5.0f, hips.y - 2.0f, 9.6f, pR, pG, pB, 1.0f, 16);

            // Graceful feminine neck & exposed collarbone skin
            float skR = app.skinR, skG = app.skinG, skB = app.skinB;
            if (hitFlash > 0.0f) {
                skR = skR * (1.0f - hitFlash) + 0.95f * hitFlash;
                skG = skG * (1.0f - hitFlash) + 0.20f * hitFlash;
                skB = skB * (1.0f - hitFlash) + 0.20f * hitFlash;
            }
            renderer.drawLimb(neck.x, neck.y - 5.0f, neck.x + dir * 1.5f, neck.y + 10.0f, b.neckBotR, b.neckTopR, skR, skG, skB);
            renderer.drawTriangle(neck.x - dir * 3.5f, upperChestY + 1.0f,
                                  neck.x + dir * 4.5f, upperChestY + 1.0f,
                                  chest.x + dir * 5.5f, upperChestY - 6.5f,
                                  skR, skG, skB);

            // Golden decorative embroidery trim / leather piping
            renderer.drawLine(neck.x - dir * 4.0f, upperChestY + 1.0f, chest.x + dir * 5.5f, upperChestY - 6.5f,
                              app.chestTrimR, app.chestTrimG, app.chestTrimB, 0.95f, 1.8f);
            renderer.drawLine(chest.x + dir * 5.5f, upperChestY - 6.5f, chest.x + dir * 11.2f, upperChestY,
                              app.chestTrimR, app.chestTrimG, app.chestTrimB, 0.95f, 1.8f);
            renderer.drawLine(chest.x + dir * 2.0f, underBustY, waist.x + dir * b.waistFront, waist.y,
                              app.chestTrimR, app.chestTrimG, app.chestTrimB, 0.85f, 1.6f);

            // Feminine shoulders in soft skin tone
            renderer.drawCircle(shoulderF.x, shoulderF.y, b.deltoidR, skR, skG, skB, 1.0f, 14);
            renderer.drawCircle(shoulderB.x, shoulderB.y, b.deltoidR * 0.92f, app.skinShadowR, app.skinShadowG, app.skinShadowB, 1.0f, 14);
            renderer.drawTriangle(shoulderB.x, shoulderB.y + 1.5f,
                                  shoulderF.x, shoulderF.y + 1.5f,
                                  neck.x, neck.y + 2.0f, skR, skG, skB);

            // Waist sash & fluttering martial ribbon tails
            renderer.drawRect(waist.x - dir * b.waistBack, waist.y - 2.5f, (b.waistBack + b.waistFront) * 1.05f, 5.0f,
                              app.sashR, app.sashG, app.sashB);
            renderer.drawLine(waist.x - dir * b.waistBack, waist.y + 2.5f, waist.x + dir * b.waistFront, waist.y + 2.5f,
                              app.sashTrimR, app.sashTrimG, app.sashTrimB, 0.95f, 1.2f);
            if (state != FighterState::DEAD) {
                float sway = std::sin(stateTime * 8.5f) * 5.0f;
                renderer.drawQuad(waist.x - dir * 6.0f, waist.y + 2.0f,
                                  waist.x - dir * 9.0f, waist.y - 3.0f,
                                  hips.x - dir * (22.0f + sway), hips.y - 22.0f,
                                  hips.x - dir * (16.0f + sway), hips.y - 19.0f,
                                  app.sashR, app.sashG, app.sashB);
            }

        } else {
            // Real muscular man V-taper torso: broad pecs & lats -> strong ribcage -> athletic waist -> muscular hips
            float pecY  = chest.y - 9.0f;
            float ribY  = (chest.y + waist.y) * 0.48f;
            float hipY  = hips.y - 4.0f;
            float xs[10] = {
                chest.x - dir * b.chestBack,            // broad back latissimus
                chest.x + dir * (b.chestFront * 0.86f), // upper chest
                chest.x + dir * b.chestFront,           // full muscular pectoral projection
                chest.x + dir * (b.chestFront * 0.82f), // lower pecs
                waist.x + dir * b.waistFront,           // strong obliques / front waist
                hips.x  + dir * b.hipFront,             // front hip
                hips.x  - dir * b.hipBack,              // back hip / glutes
                waist.x - dir * b.waistBack,            // lower back
                chest.x - dir * (b.chestBack * 0.90f),  // mid lats
                chest.x - dir * b.chestBack
            };
            float ys[10] = {
                chest.y + 5.0f,
                chest.y + 5.0f,
                pecY,
                ribY + 6.0f,
                waist.y,
                hipY,
                hipY,
                waist.y,
                ribY,
                pecY
            };

            if (app.bareChest) {
                // KAI: BARE MUSCULAR CHEST & SIX-PACK ABS
                float skR = app.skinR, skG = app.skinG, skB = app.skinB;
                if (hitFlash > 0.0f) {
                    skR = skR * (1.0f - hitFlash) + 0.95f * hitFlash;
                    skG = skG * (1.0f - hitFlash) + 0.20f * hitFlash;
                    skB = skB * (1.0f - hitFlash) + 0.20f * hitFlash;
                }
                renderer.drawPolygon(xs, ys, 10, skR, skG, skB);
                renderer.drawCircle(chest.x + dir * (b.chestFront * 0.35f), pecY + 1.0f, b.chestFront * 0.52f, skR, skG, skB, 1.0f, 16);

                // Pectoral muscle separation & lower pec lines
                float shR = app.skinShadowR, shG = app.skinShadowG, shB = app.skinShadowB;
                renderer.drawLine(chest.x - dir * 2.0f, pecY + 6.0f, chest.x + dir * (b.chestFront * 0.80f), pecY - 2.0f,
                                  shR, shG, shB, 0.85f, 1.8f);
                renderer.drawLine(chest.x + dir * 4.0f, pecY + 8.0f, chest.x + dir * 4.0f, ribY + 4.0f,
                                  shR, shG, shB, 0.80f, 1.5f);

                // Abdominal six-pack grooves
                for (int ab = 0; ab < 3; ++ab) {
                    float abY = ribY + 2.0f - (float)ab * 8.0f;
                    renderer.drawLine(waist.x - dir * 4.0f, abY, waist.x + dir * (b.waistFront * 0.75f), abY,
                                      shR, shG, shB, 0.75f, 1.4f);
                }

                // Muscular neck & strong traps
                renderer.drawLimb(neck.x, neck.y - 6.0f, neck.x + dir * 1.5f, neck.y + 11.0f, b.neckBotR, b.neckTopR, skR, skG, skB);
                renderer.drawCircle(shoulderF.x, shoulderF.y + 1.5f, b.deltoidR, skR, skG, skB, 1.0f, 18);
                renderer.drawCircle(shoulderB.x, shoulderB.y + 1.5f, b.deltoidR * 0.94f, shR, shG, shB, 1.0f, 18);

                // Vibrant Cyan Obi Sash & Gi Pants at hips
                renderer.drawCircle(hips.x, hips.y - 2.0f, b.hipFront * 0.72f, app.pantsR, app.pantsG, app.pantsB, 1.0f, 16);
                renderer.drawRect(waist.x - dir * (b.waistBack + 1.0f), waist.y - 3.0f, (b.waistBack + b.waistFront + 2.0f), 7.0f,
                                  app.sashR, app.sashG, app.sashB);
                renderer.drawLine(waist.x - dir * (b.waistBack + 1.0f), waist.y + 4.0f, waist.x + dir * (b.waistFront + 1.0f), waist.y + 4.0f,
                                  app.sashTrimR, app.sashTrimG, app.sashTrimB, 0.95f, 1.4f);

                // Fluttering sash knot tails
                if (state != FighterState::DEAD) {
                    float w = std::sin(stateTime * 7.5f) * 4.0f;
                    renderer.drawQuad(waist.x + dir * (b.waistFront - 2.0f), waist.y + 2.0f,
                                      waist.x + dir * (b.waistFront + 3.0f), waist.y - 1.0f,
                                      waist.x + dir * (b.waistFront + 16.0f + w), waist.y - 16.0f,
                                      waist.x + dir * (b.waistFront + 11.0f + w), waist.y - 18.0f,
                                      app.sashR, app.sashG, app.sashB);
                }

            } else if (charId == CharacterId::CHAIN_WARRIOR) {
                // KURO: SHINOBI VEST WITH FORGED STEEL CHESTPLATE
                float vR = app.chestR, vG = app.chestG, vB = app.chestB;
                if (hitFlash > 0.0f) {
                    vR = vR * (1.0f - hitFlash) + 0.95f * hitFlash;
                    vG = vG * (1.0f - hitFlash) + 0.20f * hitFlash;
                    vB = vB * (1.0f - hitFlash) + 0.20f * hitFlash;
                }
                renderer.drawPolygon(xs, ys, 10, vR, vG, vB);
                renderer.drawCircle(chest.x + dir * (b.chestFront * 0.35f), pecY + 1.0f, b.chestFront * 0.52f, vR, vG, vB, 1.0f, 16);

                // Steel chest protector plate across upper chest
                renderer.drawQuad(chest.x - dir * 10.0f, pecY + 10.0f,
                                  chest.x + dir * (b.chestFront * 0.92f), pecY + 8.0f,
                                  chest.x + dir * (b.chestFront * 0.75f), ribY + 6.0f,
                                  chest.x - dir * 8.0f, ribY + 8.0f,
                                  app.chestTrimR, app.chestTrimG, app.chestTrimB);
                renderer.drawLine(chest.x - dir * 8.0f, pecY + 9.0f, chest.x + dir * (b.chestFront * 0.88f), pecY + 7.0f,
                                  0.85f, 0.90f, 0.98f, 0.90f, 1.6f);

                // Diagonal leather harness straps
                renderer.drawLine(shoulderB.x, shoulderB.y + 2.0f, waist.x + dir * 6.0f, waist.y,
                                  0.35f, 0.22f, 0.15f, 0.95f, 2.4f);

                // Ninja cowl neck
                renderer.drawLimb(neck.x, neck.y - 6.0f, neck.x + dir * 1.5f, neck.y + 11.0f, b.neckBotR, b.neckTopR,
                                  app.maskR, app.maskG, app.maskB);
                renderer.drawCircle(shoulderF.x, shoulderF.y + 1.5f, b.deltoidR, vR, vG, vB, 1.0f, 18);
                renderer.drawCircleOutline(shoulderF.x, shoulderF.y + 1.5f, b.deltoidR, app.chestTrimR, app.chestTrimG, app.chestTrimB, 0.90f, 1.8f);
                renderer.drawCircle(shoulderB.x, shoulderB.y + 1.5f, b.deltoidR * 0.94f, app.chestShadowR, app.chestShadowG, app.chestShadowB, 1.0f, 18);

                // Utility belt with silver buckle
                renderer.drawRect(waist.x - dir * (b.waistBack + 1.0f), waist.y - 3.0f, (b.waistBack + b.waistFront + 2.0f), 6.5f,
                                  app.sashR, app.sashG, app.sashB);
                renderer.drawRect(waist.x + dir * 2.0f, waist.y - 3.0f, 5.0f, 6.5f, 0.80f, 0.84f, 0.90f);
                renderer.drawCircle(hips.x, hips.y - 2.0f, b.hipFront * 0.72f, app.pantsR, app.pantsG, app.pantsB, 1.0f, 16);

            } else {
                // TAKEDA: HEAVY LACQUERED CRIMSON SAMURAI DŌ CUIRASS & GOLD LAMES
                float aR = app.chestR, aG = app.chestG, aB = app.chestB;
                if (hitFlash > 0.0f) {
                    aR = aR * (1.0f - hitFlash) + 0.95f * hitFlash;
                    aG = aG * (1.0f - hitFlash) + 0.20f * hitFlash;
                    aB = aB * (1.0f - hitFlash) + 0.20f * hitFlash;
                }
                renderer.drawPolygon(xs, ys, 10, aR, aG, aB);
                renderer.drawCircle(chest.x + dir * (b.chestFront * 0.35f), pecY + 1.0f, b.chestFront * 0.52f, aR, aG, aB, 1.0f, 16);

                // Horizontal gold armor lacing bands
                for (int lame = 0; lame < 4; ++lame) {
                    float lameY = pecY + 6.0f - (float)lame * 9.5f;
                    renderer.drawLine(chest.x - dir * (b.chestBack * 0.85f), lameY,
                                      chest.x + dir * (b.chestFront * 0.85f), lameY,
                                      app.chestTrimR, app.chestTrimG, app.chestTrimB, 0.95f, 2.2f);
                }

                // Armored neck throat guard
                renderer.drawLimb(neck.x, neck.y - 6.0f, neck.x + dir * 1.5f, neck.y + 11.0f, b.neckBotR, b.neckTopR,
                                  0.25f, 0.25f, 0.28f);

                // Layered sodé samurai shoulder armor pauldrons
                renderer.drawCircle(shoulderF.x, shoulderF.y + 2.0f, b.deltoidR * 1.05f, aR, aG, aB, 1.0f, 18);
                renderer.drawCircleOutline(shoulderF.x, shoulderF.y + 2.0f, b.deltoidR * 1.05f,
                                           app.chestTrimR, app.chestTrimG, app.chestTrimB, 0.95f, 2.2f);
                renderer.drawCircle(shoulderB.x, shoulderB.y + 2.0f, b.deltoidR * 0.98f, app.chestShadowR, app.chestShadowG, app.chestShadowB, 1.0f, 18);

                // Armored kusazuri skirt plates over hakama pants
                renderer.drawCircle(hips.x, hips.y - 2.0f, b.hipFront * 0.72f, app.pantsR, app.pantsG, app.pantsB, 1.0f, 16);
                if (state != FighterState::DEAD) {
                    float flairOut = 25.0f;
                    float tassetDrop = 22.0f;
                    renderer.drawQuad(waist.x - dir * (b.waistBack + 2.0f), waist.y + 2.0f,
                                      waist.x + dir * (b.waistFront + 2.0f), waist.y + 2.0f,
                                      hips.x + dir * flairOut, hips.y - tassetDrop,
                                      hips.x - dir * (flairOut + 2.0f), hips.y - (tassetDrop + 2.0f),
                                      aR, aG, aB);
                    renderer.drawLine(hips.x - dir * (flairOut + 2.0f), hips.y - (tassetDrop + 2.0f),
                                      hips.x + dir * flairOut, hips.y - tassetDrop,
                                      app.chestTrimR, app.chestTrimG, app.chestTrimB, 0.95f, 2.0f);
                }
            }
        }
    }

    void drawHumanHead(const Renderer& renderer, Vec2 headCenter, Vec2 chest,
                       float dir, const BodyBuild& b, const CharacterAppearance& app,
                       CharacterId charId, FighterState state, float stateTime, float hitFlash) {
        float fs = b.faceScale;

        // 1. Cranium Base
        float crR = app.hasKabuto ? app.helmR : (app.hasNinjaMask ? app.maskR : app.hairR);
        float crG = app.hasKabuto ? app.helmG : (app.hasNinjaMask ? app.maskG : app.hairG);
        float crB = app.hasKabuto ? app.helmB : (app.hasNinjaMask ? app.maskB : app.hairB);
        renderer.drawCircle(headCenter.x - dir * 1.0f, headCenter.y + 1.0f, b.craniumR, crR, crG, crB, 1.0f, 20);

        // 2. Sculpted Human Facial Profile (Forehead, Brow, Sculpted Nose, Lips, Chin)
        float fxs[5] = {
            headCenter.x + dir * 3.0f * fs,
            headCenter.x + dir * 12.2f * fs,
            headCenter.x + dir * 11.4f * fs,
            headCenter.x + dir * 9.0f * fs,
            headCenter.x - dir * 1.0f * fs
        };
        float fys[5] = {
            headCenter.y + 8.6f * fs,
            headCenter.y + 1.6f * fs,
            headCenter.y - 4.5f * fs,
            headCenter.y - 10.6f * fs,
            headCenter.y - 8.6f * fs
        };

        float skR = app.skinR, skG = app.skinG, skB = app.skinB;
        if (hitFlash > 0.0f) {
            skR = skR * (1.0f - hitFlash) + 0.95f * hitFlash;
            skG = skG * (1.0f - hitFlash) + 0.20f * hitFlash;
            skB = skB * (1.0f - hitFlash) + 0.20f * hitFlash;
        }
        renderer.drawPolygon(fxs, fys, 5, skR, skG, skB);

        // 3. Ninja Mask covering lower face for Kuro
        if (app.hasNinjaMask) {
            float mxs[4] = {
                headCenter.x + dir * 2.0f * fs,
                headCenter.x + dir * 12.0f * fs,
                headCenter.x + dir * 9.0f * fs,
                headCenter.x - dir * 1.0f * fs
            };
            float mys[4] = {
                headCenter.y - 1.0f * fs,
                headCenter.y - 2.5f * fs,
                headCenter.y - 10.6f * fs,
                headCenter.y - 8.6f * fs
            };
            renderer.drawPolygon(mxs, mys, 4, app.maskR, app.maskG, app.maskB);
        }

        // 4. Lifelike Human Eyes (White Sclera + Colored Iris + Dark Pupil + Catchlight Glint)
        float eyeX = headCenter.x + dir * 6.2f * fs;
        float eyeY = headCenter.y + 1.6f * fs;
        renderer.drawCircle(eyeX, eyeY, 2.5f * fs, app.eyeWhiteR, app.eyeWhiteG, app.eyeWhiteB, 1.0f, 10);
        renderer.drawCircle(eyeX + dir * 0.7f, eyeY, 1.6f * fs, app.irisR, app.irisG, app.irisB, 1.0f, 8);
        renderer.drawCircle(eyeX + dir * 0.8f, eyeY, 0.9f * fs, app.pupilR, app.pupilG, app.pupilB, 1.0f, 6);
        renderer.drawCircle(eyeX + dir * 0.5f, eyeY + 0.6f * fs, 0.5f * fs, 1.0f, 1.0f, 1.0f, 0.95f, 4);

        // 5. Defined Martial / Feminine Eyebrow
        renderer.drawLine(eyeX - dir * 2.2f * fs, eyeY + 3.2f * fs,
                          eyeX + dir * 3.5f * fs, eyeY + 2.8f * fs,
                          app.browR, app.browG, app.browB, 0.95f, 1.5f);

        // 6. Realistic Lips for Female Characters (Akari & Mei-Ling)
        if (app.hasLips) {
            float lipX = headCenter.x + dir * 10.6f * fs;
            float lipY = headCenter.y - 5.5f * fs;
            renderer.drawCircle(lipX, lipY + 0.8f, 1.6f * fs, app.lipR, app.lipG, app.lipB, 1.0f, 8);
            renderer.drawCircle(lipX - dir * 0.8f, lipY - 1.0f, 1.8f * fs, app.lipR * 0.92f, app.lipG * 0.92f, app.lipB * 0.92f, 1.0f, 8);
            renderer.drawLine(lipX - dir * 2.2f * fs, lipY, lipX + dir * 1.5f * fs, lipY,
                              app.lipR * 0.65f, app.lipG * 0.65f, app.lipB * 0.65f, 0.90f, 1.0f);
        } else if (charId == CharacterId::SPEAR_WARRIOR) {
            renderer.drawLine(headCenter.x + dir * 3.0f * fs, headCenter.y - 7.5f * fs,
                              headCenter.x + dir * 9.0f * fs, headCenter.y - 10.0f * fs,
                              0.52f, 0.40f, 0.30f, 0.75f, 1.8f);
        }

        // 7. Distinctive Hair, Headgear, and Accessories
        if (charId == CharacterId::BRAWLER) {
            // KAI: Topknot hair + Knotted martial headband with fluttering tails
            renderer.drawCircle(headCenter.x - dir * 2.0f, headCenter.y + 10.5f, 5.0f, app.hairR, app.hairG, app.hairB);
            renderer.drawLine(headCenter.x - dir * 8.0f, headCenter.y + 4.5f,
                              headCenter.x + dir * 8.5f, headCenter.y + 4.5f,
                              app.ribbonR, app.ribbonG, app.ribbonB, 1.0f, 3.2f);
            if (state != FighterState::DEAD) {
                float w1 = std::sin(stateTime * 10.0f) * 4.5f;
                float w2 = std::cos(stateTime * 8.0f)  * 5.0f;
                renderer.drawCircle(headCenter.x - dir * 11.5f, headCenter.y + 3.0f, 4.0f, app.ribbonR, app.ribbonG, app.ribbonB);
                renderer.drawQuad(headCenter.x - dir * 11.5f, headCenter.y + 5.0f,
                                  headCenter.x - dir * 11.5f, headCenter.y + 1.0f,
                                  headCenter.x - dir * 33.0f, headCenter.y - 4.0f + w1,
                                  headCenter.x - dir * 33.0f, headCenter.y + 1.0f + w1,
                                  app.ribbonR, app.ribbonG, app.ribbonB);
                renderer.drawQuad(headCenter.x - dir * 11.5f, headCenter.y + 3.0f,
                                  headCenter.x - dir * 11.5f, headCenter.y - 1.0f,
                                  headCenter.x - dir * 39.0f, headCenter.y - 12.0f + w2,
                                  headCenter.x - dir * 39.0f, headCenter.y - 7.0f  + w2,
                                  app.ribbonR * 0.85f, app.ribbonG * 0.85f, app.ribbonB * 0.85f);
            }

        } else if (charId == CharacterId::CHAIN_WARRIOR) {
            // KURO: Horned demon crest + steel forehead plate + strapped back hilt
            renderer.drawTriangle(headCenter.x + dir * 2.0f, headCenter.y + 8.0f,
                                  headCenter.x + dir * 7.8f, headCenter.y + 8.0f,
                                  headCenter.x + dir * 10.4f, headCenter.y + 25.0f,
                                  0.55f, 0.62f, 0.72f);
            renderer.drawTriangle(headCenter.x - dir * 2.5f, headCenter.y + 8.0f,
                                  headCenter.x + dir * 2.5f, headCenter.y + 8.0f,
                                  headCenter.x + dir * 4.2f, headCenter.y + 22.0f,
                                  0.42f, 0.48f, 0.58f);
            renderer.drawQuad(headCenter.x - dir * 4.0f, headCenter.y + 2.0f,
                              headCenter.x - dir * 14.5f, headCenter.y - 11.5f,
                              headCenter.x - dir * 10.5f, headCenter.y - 14.5f,
                              headCenter.x + dir * 1.0f, headCenter.y - 5.0f,
                              app.maskR, app.maskG, app.maskB);
            if (state != FighterState::DEAD) {
                renderer.drawQuad(chest.x - dir * 11.0f, chest.y + 4.0f,
                                  chest.x - dir * 5.0f, chest.y + 2.0f,
                                  chest.x - dir * 19.0f, chest.y + 28.0f,
                                  chest.x - dir * 25.0f, chest.y + 25.0f,
                                  0.35f, 0.25f, 0.18f);
            }

        } else if (charId == CharacterId::ARCHER) {
            // AKARI: High tied sweeping brunette ponytail with fluttering crimson ribbon
            renderer.drawCircle(headCenter.x - dir * 4.0f, headCenter.y + 6.0f, 6.5f, app.hairR, app.hairG, app.hairB);
            if (state != FighterState::DEAD) {
                float hairWave = std::sin(stateTime * 9.5f) * 4.8f;
                renderer.drawCircle(headCenter.x - dir * 10.5f, headCenter.y + 6.0f, 4.4f, app.ribbonR, app.ribbonG, app.ribbonB);
                renderer.drawQuad(headCenter.x - dir * 10.5f, headCenter.y + 8.5f,
                                  headCenter.x - dir * 10.5f, headCenter.y + 2.5f,
                                  headCenter.x - dir * 35.0f, headCenter.y - 16.0f + hairWave,
                                  headCenter.x - dir * 31.0f, headCenter.y - 5.0f + hairWave,
                                  app.hairR, app.hairG, app.hairB);
            }

        } else if (charId == CharacterId::FAN_DANCER) {
            // MEI-LING: Twin odango chignon buns with golden floral hairpins & dangling beads
            renderer.drawCircle(headCenter.x - dir * 7.5f, headCenter.y + 10.5f, 5.2f, app.hairR, app.hairG, app.hairB);
            renderer.drawCircle(headCenter.x + dir * 1.5f, headCenter.y + 11.5f, 4.6f, app.hairR, app.hairG, app.hairB);
            renderer.drawLine(headCenter.x - dir * 15.0f, headCenter.y + 17.5f,
                              headCenter.x + dir * 3.0f,  headCenter.y + 7.5f,
                              app.pinR, app.pinG, app.pinB, 1.0f, 2.2f);
            renderer.drawLine(headCenter.x - dir * 3.0f,  headCenter.y + 19.0f,
                              headCenter.x + dir * 10.5f, headCenter.y + 8.5f,
                              app.pinR, app.pinG, app.pinB, 1.0f, 2.2f);
            renderer.drawCircle(headCenter.x - dir * 15.0f, headCenter.y + 17.5f, 2.0f, 0.92f, 0.22f, 0.32f, 1.0f, 6);
            renderer.drawCircle(headCenter.x + dir * 10.5f, headCenter.y + 8.5f, 2.0f, 0.92f, 0.22f, 0.32f, 1.0f, 6);

        } else {
            // TAKEDA: Imperial Kabuto samurai helmet + sweeping golden crescent moon crest + neck shikoro
            renderer.drawTriangle(headCenter.x - dir * 15.0f, headCenter.y + 6.5f,
                                  headCenter.x + dir * 13.0f, headCenter.y + 6.5f,
                                  headCenter.x + dir * 2.0f,  headCenter.y + 27.5f,
                                  app.helmR, app.helmG, app.helmB);
            renderer.drawTriangle(headCenter.x + dir * 1.0f, headCenter.y + 16.0f,
                                  headCenter.x + dir * 7.0f, headCenter.y + 16.0f,
                                  headCenter.x + dir * 9.0f, headCenter.y + 34.0f,
                                  app.crestR, app.crestG, app.crestB);
            renderer.drawTriangle(headCenter.x - dir * 4.0f, headCenter.y + 16.0f,
                                  headCenter.x + dir * 2.0f, headCenter.y + 16.0f,
                                  headCenter.x - dir * 3.0f, headCenter.y + 32.0f,
                                  app.crestR * 0.85f, app.crestG * 0.85f, app.crestB * 0.85f);
            renderer.drawQuad(headCenter.x - dir * 3.0f, headCenter.y + 2.0f,
                              headCenter.x - dir * 18.0f, headCenter.y - 14.0f,
                              headCenter.x - dir * 13.0f, headCenter.y - 17.0f,
                              headCenter.x + dir * 1.0f, headCenter.y - 6.0f,
                              0.48f, 0.14f, 0.14f);
        }
    }
}

Fighter::Fighter(float startX, float startY, float bodyWidth, float bodyHeight)
    : characterId(CharacterId::BRAWLER), role(FighterRole::NINJA), damageMultiplier(1.0f),
      x(startX), y(startY), width(bodyWidth), height(bodyHeight),
      standingHeight(bodyHeight), crouchHeight(bodyHeight * CROUCH_HEIGHT_RATIO),
      moveSpeed(260.0f), jumpSpeed(Config::JUMP_SPEED), energyRegen(ENERGY_REGEN),
      velocityY(0.0f), airMomentumX(0.0f), knockbackVelocityX(0.0f),
      rollVelocityX(0.0f), rollTimer(0.0f),
      moveDirection(0.0f), facingRight(true),
      onGround(false), crouching(false), blocking(false), running(false),
      state(FighterState::IDLE), stateTime(0.0f), hitStunDuration(0.0f),
      blockFlashTimer(0.0f), invulnerableTimer(0.0f), electrocuteTimer(0.0f),
      chainGrabCooldown(0.0f), comboQueued(false),
      specialShotFired(false), fanSpecialStage(0), specialSubTimer(0.0f),
      fan1Fired(false), fan2Fired(false), kneeImpactPending(false), kneeImpactDone(false),
      skySpearsSpawned(0), activeSkySpears(0), finalSpearThrown(false),
      targetHintX(400.0f), targetHintY(Config::GROUND_HEIGHT),
      colorR(0.08f), colorG(0.09f), colorB(0.12f),
      accentR(0.25f), accentG(0.85f), accentB(0.95f),
      health(MAX_HEALTH), maxHealth(MAX_HEALTH),
      energy(MAX_ENERGY), maxEnergy(MAX_ENERGY), attackHasHit(false) {
}

const AttackData* Fighter::attackDataFor(FighterState s) const {
    const CharacterProfile& p = getProfile();
    switch (s) {
        case FighterState::PUNCH:      return &p.comboAttacks[0];
        case FighterState::COMBO2:     return &p.comboAttacks[1];
        case FighterState::COMBO3:     return &p.comboAttacks[2];
        case FighterState::KICK:       return &p.kickAttack;
        case FighterState::JUMP_KICK:  return &p.jumpKickAttack;
        case FighterState::CHAIN_GRAB: return &p.grabAttack;
        case FighterState::SPECIAL:    return &p.specialAttack;
        default:                       return nullptr;
    }
}

void Fighter::keepInsideWorld() {
    if (x < 0.0f) x = 0.0f;
    if (x + width > Config::WORLD_WIDTH) x = Config::WORLD_WIDTH - width;
}

void Fighter::moveHorizontal(float direction, float deltaTime) {
    if (!canAct() || blocking) {
        direction = 0.0f;
        running = false;
    }
    moveDirection = direction;

    if (!onGround && direction != 0.0f) {
        float airFactor = running ? 1.15f : 0.85f;
        airMomentumX = direction * moveSpeed * airFactor;
    }

    float speedMult = crouching ? CROUCH_SPEED_RATIO : (running ? RUN_SPEED_RATIO : 1.0f);
    float speed = moveSpeed * speedMult;
    x += direction * speed * deltaTime;

    if (direction > 0.0f) facingRight = true;
    else if (direction < 0.0f) facingRight = false;

    keepInsideWorld();
}

void Fighter::jump() {
    if (onGround && canAct()) {
        velocityY = jumpSpeed;
        onGround = false;
        float airFactor = running ? 1.15f : 0.85f;
        airMomentumX = moveDirection * moveSpeed * airFactor;
        SoundManager::play(SoundEffect::JUMP);
    }
}

void Fighter::setCrouching(bool wantsToCrouch) {
    if (!canAct()) wantsToCrouch = false;

    if (wantsToCrouch && onGround) {
        crouching = true;
        running = false;
        height = crouchHeight;
    } else {
        crouching = false;
        if (state != FighterState::ROLL) {
            height = standingHeight;
        }
    }
}

void Fighter::setBlocking(bool wantsToBlock) {
    if (!canAct() || crouching || !onGround) wantsToBlock = false;
    blocking = wantsToBlock;
    if (blocking) running = false;
}

void Fighter::setRunning(bool wantsToRun) {
    if (!canAct() || crouching || blocking || moveDirection == 0.0f) {
        wantsToRun = false;
    }
    running = wantsToRun;
}

bool Fighter::isRunning() const {
    return running;
}

void Fighter::startRoll(float rollDir) {
    const CharacterProfile& prof = getProfile();
    if (!prof.hasAcrobaticKit) return;
    if (!canAct() || !onGround) return;

    float dir = (rollDir != 0.0f) ? (rollDir > 0.0f ? 1.0f : -1.0f) : (facingRight ? 1.0f : -1.0f);
    facingRight = (dir > 0.0f);
    state = FighterState::ROLL;
    stateTime = 0.0f;
    rollTimer = ROLL_DURATION;
    rollVelocityX = dir * (prof.isFemale ? ROLL_SPEED * 1.18f : ROLL_SPEED);
    crouching = false;
    blocking = false;
    height = crouchHeight * 0.72f;
    SoundManager::play(SoundEffect::PUNCH_WHOOSH);
}

void Fighter::startAttack(FighterState attackState) {
    const AttackData* data = attackDataFor(attackState);
    if (data == nullptr) return;

    if (attackState == FighterState::SPECIAL) {
        if (energy < data->energyCost) return;
        energy -= data->energyCost;
        SoundManager::play(SoundEffect::SPECIAL_CHARGE);

        specialShotFired  = false;
        fanSpecialStage   = 0;
        specialSubTimer   = 0.0f;
        fan1Fired         = false;
        fan2Fired         = false;
        kneeImpactPending = false;
        kneeImpactDone    = false;
        skySpearsSpawned  = 0;
        activeSkySpears   = 0;
        finalSpearThrown  = false;

        float dir = facingRight ? 1.0f : -1.0f;
        if (characterId == CharacterId::BRAWLER) {
            knockbackVelocityX = dir * 360.0f;
        } else if (characterId == CharacterId::ARCHER) {
            // Back-roll to gain distance before firing the power arrow
            rollVelocityX = -dir * 480.0f;
            rollTimer = 0.34f;
        }
    }

    state = attackState;
    stateTime = 0.0f;
    attackHasHit = false;
    comboQueued = false;
    crouching = false;
    blocking = false;
    height = standingHeight;
}

void Fighter::startPunch() {
    if ((state == FighterState::PUNCH || state == FighterState::COMBO2) && stateTime > 0.03f) {
        comboQueued = true;
        return;
    }
    if (!canAct() || !onGround) return;
    startAttack(FighterState::PUNCH);
}

void Fighter::startKick() {
    if (!canAct()) return;
    if (onGround) {
        startAttack(FighterState::KICK);
    } else if (getProfile().hasAcrobaticKit) {
        // Flying jump kick with higher damage!
        if (std::fabs(airMomentumX) < 40.0f) {
            airMomentumX = (facingRight ? 1.0f : -1.0f) * moveSpeed * 0.85f;
        }
        startAttack(FighterState::JUMP_KICK);
        SoundManager::play(SoundEffect::KICK_WHOOSH);
    }
}

void Fighter::startChainGrab() {
    if (!getProfile().hasChainPull) return;
    if (!canAct() || !onGround) return;
    if (chainGrabCooldown > 0.0f) return;

    chainGrabCooldown = 3.2f;
    startAttack(FighterState::CHAIN_GRAB);
    SoundManager::play(SoundEffect::SPECIAL_WHOOSH);
}

void Fighter::startSpecial() {
    if (!canAct() || !onGround) return;
    startAttack(FighterState::SPECIAL);
}

void Fighter::applyPhysics(float deltaTime) {
    if (!onGround) {
        bool wasAirborne = (y > Config::GROUND_HEIGHT + 1.0f);
        velocityY += Config::GRAVITY * deltaTime;
        y += velocityY * deltaTime;

        if (state == FighterState::JUMP_KICK) {
            x += airMomentumX * deltaTime;
            keepInsideWorld();
        }

        if (y <= Config::GROUND_HEIGHT) {
            y = Config::GROUND_HEIGHT;
            if (wasAirborne && velocityY < -120.0f) {
                SoundManager::play(SoundEffect::LAND);
            }
            velocityY = 0.0f;
            airMomentumX = 0.0f;
            onGround = true;
        }
    }

    if (rollTimer > 0.0f) {
        rollTimer -= deltaTime;
        x += rollVelocityX * deltaTime;
        keepInsideWorld();
        if (rollTimer <= 0.0f) {
            rollVelocityX = 0.0f;
        }
    }

    if (knockbackVelocityX != 0.0f) {
        x += knockbackVelocityX * deltaTime;
        keepInsideWorld();

        float sign = (knockbackVelocityX > 0.0f) ? 1.0f : -1.0f;
        float reduction = KNOCKBACK_DECEL * deltaTime;
        if (std::fabs(knockbackVelocityX) <= reduction) {
            knockbackVelocityX = 0.0f;
        } else {
            knockbackVelocityX -= sign * reduction;
        }
    }
}

void Fighter::updateState(float deltaTime) {
    FighterState prevState = state;
    AttackPhase  prevPhase = getAttackPhase();

    stateTime += deltaTime;

    if (blockFlashTimer > 0.0f)   blockFlashTimer -= deltaTime;
    if (invulnerableTimer > 0.0f) invulnerableTimer -= deltaTime;
    if (electrocuteTimer > 0.0f)  electrocuteTimer -= deltaTime;
    if (chainGrabCooldown > 0.0f) chainGrabCooldown -= deltaTime;

    energy += energyRegen * deltaTime;
    if (energy > maxEnergy) energy = maxEnergy;

    if (state == FighterState::DEAD) return;

    if (state == FighterState::ROLL) {
        if (stateTime >= ROLL_DURATION) {
            state = FighterState::IDLE;
            stateTime = 0.0f;
            height = standingHeight;
        }
        return;
    }

    if (state == FighterState::HIT) {
        if (stateTime >= hitStunDuration) {
            state = FighterState::IDLE;
            stateTime = 0.0f;
            invulnerableTimer = RECOVERY_INVULNERABILITY;
        }
        return;
    }

    // Handle multi-stage special animations & projectile spawns
    if (state == FighterState::SPECIAL) {
        float dir = facingRight ? 1.0f : -1.0f;

        if (characterId == CharacterId::ARCHER) {
            // Shrink hurtbox slightly during back-roll phase (0.0 .. 0.34s)
            height = (stateTime < 0.34f) ? (crouchHeight * 0.80f) : standingHeight;
            if (!specialShotFired && stateTime >= 0.52f) {
                specialShotFired = true;
                ProjectileSpawn p;
                p.kind = ProjectileKind::WAR_ARROW;
                p.x = (facingRight ? (x + width) : (x - 42.0f));
                p.y = y + 114.0f;
                p.vx = dir * 860.0f;
                p.vy = 0.0f;
                p.width = 42.0f;
                p.height = 18.0f;
                p.damage = 34;
                p.knockback = 760.0f;
                p.hitStun = 0.62f;
                p.r = accentR; p.g = accentG; p.b = accentB;
                pendingSpawns.push_back(p);
                SoundManager::play(SoundEffect::SPECIAL_WHOOSH);
            }
        } else if (characterId == CharacterId::FAN_DANCER) {
            if (fanSpecialStage == 0) {
                if (!fan1Fired && stateTime >= 0.14f) {
                    fan1Fired = true;
                    ProjectileSpawn p;
                    p.kind = ProjectileKind::CHINESE_FAN;
                    p.x = (facingRight ? (x + width) : (x - 28.0f));
                    p.y = y + 116.0f;
                    p.vx = dir * 680.0f;
                    p.vy = 0.0f;
                    p.width = 28.0f;
                    p.height = 28.0f;
                    p.damage = 5;
                    p.knockback = 130.0f;
                    p.hitStun = 0.25f;
                    p.r = accentR; p.g = accentG; p.b = accentB;
                    pendingSpawns.push_back(p);
                    SoundManager::play(SoundEffect::PUNCH_WHOOSH);
                }
                if (!fan2Fired && stateTime >= 0.36f) {
                    fan2Fired = true;
                    ProjectileSpawn p;
                    p.kind = ProjectileKind::CHINESE_FAN;
                    p.x = (facingRight ? (x + width) : (x - 28.0f));
                    p.y = y + 108.0f;
                    p.vx = dir * 720.0f;
                    p.vy = 0.0f;
                    p.width = 28.0f;
                    p.height = 28.0f;
                    p.damage = 5;
                    p.knockback = 130.0f;
                    p.hitStun = 0.25f;
                    p.r = accentR; p.g = accentG; p.b = accentB;
                    pendingSpawns.push_back(p);
                    SoundManager::play(SoundEffect::PUNCH_WHOOSH);
                }
            } else {
                // Stages 1 & 2 (Rush -> Jump + Head-Grab Knee) are driven by Game::updateFight
                specialSubTimer += deltaTime;
                if (fanSpecialStage == 2 && !kneeImpactDone && specialSubTimer >= 0.18f) {
                    kneeImpactDone = true;
                    kneeImpactPending = true;
                }
                if (fanSpecialStage == 2 && specialSubTimer >= 0.50f && onGround) {
                    state = FighterState::IDLE;
                    stateTime = 0.0f;
                    fanSpecialStage = 0;
                    height = standingHeight;
                }
                return;
            }
        } else if (characterId == CharacterId::SPEAR_WARRIOR) {
            // Stage 1: Drop 10 spears sequentially from the sky onto opponent
            while (skySpearsSpawned < 10 && stateTime >= 0.34f + skySpearsSpawned * 0.085f) {
                ProjectileSpawn p;
                p.kind = ProjectileKind::SKY_SPEAR;
                p.x = targetHintX + SKY_SPEAR_OFFSETS[skySpearsSpawned] - 7.0f;
                p.y = Config::WORLD_HEIGHT - 10.0f;
                p.vx = 0.0f;
                p.vy = -1240.0f;
                p.width = 16.0f;
                p.height = 68.0f;
                p.damage = 3;
                p.knockback = 40.0f;
                p.hitStun = 0.18f;
                p.r = accentR; p.g = accentG; p.b = accentB;
                pendingSpawns.push_back(p);
                skySpearsSpawned++;
                SoundManager::play(SoundEffect::PUNCH_WHOOSH);
            }
            // Stage 2: After 10th sky spear drops, hurl main spear with force!
            if (skySpearsSpawned == 10 && !finalSpearThrown &&
                ((activeSkySpears == 0 && stateTime >= 1.38f) || stateTime >= 1.65f)) {
                finalSpearThrown = true;
                ProjectileSpawn p;
                p.kind = ProjectileKind::THROWN_SPEAR;
                p.x = (facingRight ? (x + width) : (x - 86.0f));
                p.y = y + 106.0f;
                p.vx = dir * 940.0f;
                p.vy = 0.0f;
                p.width = 86.0f;
                p.height = 18.0f;
                p.damage = 16;
                p.knockback = 860.0f;
                p.hitStun = 0.65f;
                p.r = accentR; p.g = accentG; p.b = accentB;
                pendingSpawns.push_back(p);
                SoundManager::play(SoundEffect::SPECIAL_WHOOSH);
            }
        }
    }

    if (isAttacking()) {
        const AttackData* attack = getCurrentAttack();
        if (attack != nullptr) {
            // Chain 3-hit martial combo if J was tapped during Punch or Combo2!
            if ((state == FighterState::PUNCH || state == FighterState::COMBO2) &&
                comboQueued && stateTime >= attack->startup + attack->active) {
                FighterState nextCombo = (state == FighterState::PUNCH) ? FighterState::COMBO2 : FighterState::COMBO3;
                state = nextCombo;
                stateTime = 0.0f;
                attackHasHit = false;
                comboQueued = false;
                pushHorizontally((facingRight ? 1.0f : -1.0f) * 14.0f);
                SoundManager::play(SoundEffect::PUNCH_WHOOSH);
                return;
            }

            if (stateTime >= attack->totalTime()) {
                state = onGround ? FighterState::IDLE : FighterState::FALL;
                stateTime = 0.0f;
                height = standingHeight;
                comboQueued = false;
            } else if (prevPhase == AttackPhase::STARTUP && getAttackPhase() != AttackPhase::STARTUP) {
                if (state == FighterState::PUNCH || state == FighterState::COMBO2 || state == FighterState::COMBO3) {
                    SoundManager::play(SoundEffect::PUNCH_WHOOSH);
                } else if (state == FighterState::KICK || state == FighterState::JUMP_KICK) {
                    SoundManager::play(SoundEffect::KICK_WHOOSH);
                } else if (state == FighterState::SPECIAL) {
                    SoundManager::play(SoundEffect::SPECIAL_WHOOSH);
                }
            }
        }
        return;
    }

    if (!onGround) {
        state = (velocityY >= 0.0f) ? FighterState::JUMP : FighterState::FALL;
        return;
    }

    if (blocking) {
        state = FighterState::BLOCK;
        if (prevState != FighterState::BLOCK) {
            SoundManager::play(SoundEffect::BLOCK_GUARD);
        }
        return;
    }

    if (crouching) {
        state = FighterState::CROUCH;
        return;
    }

    if (std::fabs(moveDirection) > 0.001f) {
        state = FighterState::WALK;
        return;
    }

    state = FighterState::IDLE;
}

float Fighter::pushHorizontally(float dx) {
    float oldX = x;
    x += dx;
    keepInsideWorld();
    return x - oldX;
}

void Fighter::setRole(FighterRole r) {
    role = r;
}

FighterRole Fighter::getRole() const {
    return role;
}

void Fighter::setCharacter(CharacterId id) {
    characterId = id;
    const CharacterProfile& p = Characters::get(id);
    role = (id == CharacterId::BRAWLER) ? FighterRole::NINJA : FighterRole::ARMORED_SPEARMAN;

    width = p.bodyWidth;
    standingHeight = p.bodyHeight;
    crouchHeight = p.bodyHeight * CROUCH_HEIGHT_RATIO;
    height = crouching ? crouchHeight : standingHeight;

    moveSpeed = p.moveSpeed;
    jumpSpeed = p.jumpSpeed;
    energyRegen = p.energyRegen;

    maxHealth = p.maxHealth;
    health = p.maxHealth;

    colorR = p.bodyR;
    colorG = p.bodyG;
    colorB = p.bodyB;
    accentR = p.accentR;
    accentG = p.accentG;
    accentB = p.accentB;

    keepInsideWorld();
}

CharacterId Fighter::getCharacterId() const {
    return characterId;
}

const CharacterProfile& Fighter::getProfile() const {
    return Characters::get(characterId);
}

float Fighter::getSpecialEnergyCost() const {
    return getProfile().specialAttack.energyCost;
}

bool Fighter::canChainGrab() const {
    return getProfile().hasChainPull && chainGrabCooldown <= 0.0f;
}

void Fighter::setDamageMultiplier(float mult) {
    damageMultiplier = mult;
}

float Fighter::getDamageMultiplier() const {
    return damageMultiplier;
}

void Fighter::setColor(float r, float g, float b) {
    colorR = r;
    colorG = g;
    colorB = b;
}

void Fighter::setAccentColor(float r, float g, float b) {
    accentR = r;
    accentG = g;
    accentB = b;
}

void Fighter::setFacingRight(bool faceRight) {
    facingRight = faceRight;
}

Rect Fighter::getHurtbox() const {
    Rect box = { x, y, width, height };
    return box;
}

float Fighter::getCenterX() const {
    return x + width * 0.5f;
}

float Fighter::getDirection() const {
    return facingRight ? 1.0f : -1.0f;
}

bool Fighter::isRolling() const {
    return state == FighterState::ROLL ||
           (state == FighterState::SPECIAL && characterId == CharacterId::ARCHER && stateTime < 0.34f);
}

bool Fighter::isElectrified() const {
    return electrocuteTimer > 0.0f;
}

bool Fighter::getHitbox(Rect& outBox) const {
    if (attackHasHit) return false;

    // Archer, Fan Dancer (fan throw phase), and Spear Warrior deal special hits via projectiles / knee
    if (state == FighterState::SPECIAL &&
        (characterId == CharacterId::ARCHER ||
         characterId == CharacterId::FAN_DANCER ||
         characterId == CharacterId::SPEAR_WARRIOR)) {
        return false;
    }

    const AttackData* attack = getCurrentAttack();
    if (attack == nullptr) return false;
    if (getAttackPhase() != AttackPhase::ACTIVE) return false;

    outBox.width  = attack->reach;
    outBox.height = attack->hitboxHeight;
    outBox.x = facingRight ? (x + width) : (x - attack->reach);
    outBox.y = y + attack->hitboxOffsetY;
    return true;
}

const AttackData* Fighter::getCurrentAttack() const {
    return attackDataFor(state);
}

void Fighter::onAttackConnected() {
    attackHasHit = true;
}

void Fighter::receiveHit(const AttackData& attack, float pushDirection, int customDamage,
                         float customHitStun, float knockbackScale) {
    if (isDead()) return;

    bool attackerIsOnRight = (pushDirection < 0.0f);
    bool facingAttacker = (attackerIsOnRight == facingRight);

    int rawDamage = (customDamage >= 0) ? customDamage : attack.damage;

    if (blocking && facingAttacker && attack.blockable && state != FighterState::ROLL) {
        int chipDamage = (int)std::round(rawDamage * attack.blockChipRatio);
        if (chipDamage < 1) chipDamage = 1;
        takeDamage(chipDamage);

        knockbackVelocityX = pushDirection * BLOCK_KNOCKBACK_SPEED;
        blockFlashTimer = BLOCK_FLASH_TIME;
        return;
    }

    takeDamage(rawDamage);

    if (isDead()) {
        state = FighterState::DEAD;
        stateTime = 0.0f;
        knockbackVelocityX = pushDirection * attack.knockbackSpeed * knockbackScale;
        crouching = false;
        blocking = false;
        height = standingHeight;
        return;
    }

    // If fighter was rolling when struck by a critical, reduce stagger so roll stays agile
    if (isRolling() && customHitStun > 0.0f && customHitStun < 0.30f) {
        knockbackVelocityX = pushDirection * attack.knockbackSpeed * knockbackScale;
        return;
    }

    state = FighterState::HIT;
    stateTime = 0.0f;
    hitStunDuration = (customHitStun > 0.0f) ? customHitStun : attack.hitStun;
    knockbackVelocityX = pushDirection * attack.knockbackSpeed * knockbackScale;

    rollTimer = 0.0f;
    rollVelocityX = 0.0f;
    crouching = false;
    blocking = false;
    height = standingHeight;
}

void Fighter::applyElectrocution(float duration) {
    electrocuteTimer = duration;
}

void Fighter::setTargetPositionHint(float targetCenterX, float targetGroundY) {
    targetHintX = targetCenterX;
    targetHintY = targetGroundY;
}

std::vector<ProjectileSpawn> Fighter::drainProjectileSpawns() {
    std::vector<ProjectileSpawn> out;
    out.swap(pendingSpawns);
    return out;
}

void Fighter::onFanProjectileHit() {
    if (state == FighterState::SPECIAL && characterId == CharacterId::FAN_DANCER && fanSpecialStage == 0) {
        fanSpecialStage = 1;
        specialSubTimer = 0.0f;
    }
}

int Fighter::getFanSpecialStage() const {
    return fanSpecialStage;
}

void Fighter::advanceFanSpecialStage(int stage, float newVelocityY) {
    fanSpecialStage = stage;
    specialSubTimer = 0.0f;
    if (newVelocityY > 0.0f) {
        velocityY = newVelocityY;
        onGround = false;
    }
}

bool Fighter::consumeKneeImpactReady() {
    if (kneeImpactPending) {
        kneeImpactPending = false;
        return true;
    }
    return false;
}

void Fighter::setActiveSkySpearsCount(int count) {
    activeSkySpears = count;
}

void Fighter::takeDamage(int amount) {
    if (health <= 0) return;
    health -= amount;
    if (health <= 0) {
        health = 0;
        state = FighterState::DEAD;
        stateTime = 0.0f;
    }
}

void Fighter::restoreHealth() {
    health = maxHealth;
}

int Fighter::getHealth() const {
    return health;
}

int Fighter::getMaxHealth() const {
    return maxHealth;
}

bool Fighter::isDead() const {
    return health <= 0;
}

void Fighter::restoreEnergy() {
    energy = maxEnergy;
}

float Fighter::getEnergy() const {
    return energy;
}

float Fighter::getMaxEnergy() const {
    return maxEnergy;
}

bool Fighter::isInvulnerable() const {
    return invulnerableTimer > 0.0f;
}

bool Fighter::canAct() const {
    return state != FighterState::HIT &&
           state != FighterState::DEAD &&
           state != FighterState::ROLL &&
           !isAttacking();
}

bool Fighter::isAttacking() const {
    return state == FighterState::PUNCH ||
           state == FighterState::COMBO2 ||
           state == FighterState::COMBO3 ||
           state == FighterState::KICK ||
           state == FighterState::JUMP_KICK ||
           state == FighterState::CHAIN_GRAB ||
           state == FighterState::SPECIAL;
}

AttackPhase Fighter::getAttackPhase() const {
    const AttackData* attack = getCurrentAttack();
    if (attack == nullptr) return AttackPhase::NONE;

    if (stateTime < attack->startup) return AttackPhase::STARTUP;
    if (stateTime < attack->startup + attack->active) return AttackPhase::ACTIVE;
    return AttackPhase::RECOVERY;
}

FighterState Fighter::getState() const {
    return state;
}

const char* Fighter::getStateName() const {
    switch (state) {
        case FighterState::IDLE:       return "IDLE";
        case FighterState::WALK:       return "WALK";
        case FighterState::JUMP:       return "JUMP";
        case FighterState::FALL:       return "FALL";
        case FighterState::CROUCH:     return "CROUCH";
        case FighterState::BLOCK:      return "BLOCK";
        case FighterState::ROLL:       return "ROLL";
        case FighterState::PUNCH:      return "COMBO1";
        case FighterState::COMBO2:     return "COMBO2";
        case FighterState::COMBO3:     return "COMBO3";
        case FighterState::KICK:       return "KICK";
        case FighterState::JUMP_KICK:  return "JUMP_KICK";
        case FighterState::CHAIN_GRAB: return "CHAIN_GRAB";
        case FighterState::SPECIAL:    return "SPECIAL";
        case FighterState::HIT:        return "HIT";
        case FighterState::DEAD:       return "DEAD";
    }
    return "UNKNOWN";
}

// =============================================================================
// CHARACTER-DISTINCT PROCEDURAL SILHOUETTE & WEAPON RENDERER
// =============================================================================
void Fighter::draw(const Renderer& renderer) const {
    Pose pose = Animation::getPose(state, stateTime, getCurrentAttack(), hitStunDuration);
    AttackPhase phase = getAttackPhase();
    const CharacterProfile& prof = getProfile();
    CharacterAppearance app = getCharacterAppearance(characterId);

    float hitFlash = (state == FighterState::HIT) ? 0.40f : 0.0f;
    if (state == FighterState::DEAD) {
        app.skinR *= 0.60f; app.skinG *= 0.60f; app.skinB *= 0.60f;
        app.chestR *= 0.55f; app.chestG *= 0.55f; app.chestB *= 0.55f;
        app.pantsR *= 0.55f; app.pantsG *= 0.55f; app.pantsB *= 0.55f;
    } else if (isInvulnerable() && ((int)(invulnerableTimer / BLINK_INTERVAL) % 2 == 0)) {
        hitFlash = 0.50f;
    }

    float drawW = width  * pose.scaleX;
    float drawH = height * pose.scaleY;
    float drawX = x + (width - drawW) / 2.0f;
    float drawY = y + pose.offsetY;
    float cx    = drawX + drawW * 0.5f;
    float dir   = facingRight ? 1.0f : -1.0f;
    float hScale = standingHeight / 186.0f;
    BodyBuild build = getBodyBuild(characterId, prof.isFemale);

    Vec2 headCenter, neck, chest, waist, hips;
    Vec2 shoulderF, elbowF, handF;
    Vec2 shoulderB, elbowB, handB;
    Vec2 hipF, kneeF, ankleF, toeF;
    Vec2 hipB, kneeB, ankleB, toeB;

    if (state == FighterState::DEAD) {
        float col = pose.collapse;
        float invCol = 1.0f - col;

        headCenter = { cx + dir * (10.0f * invCol + 58.0f * col), drawY + 170.0f * invCol + 12.0f * col };
        neck       = { cx + dir * (8.0f  * invCol + 44.0f * col), drawY + 154.0f * invCol + 10.0f * col };
        chest      = { cx + dir * (6.0f  * invCol + 26.0f * col), drawY + 138.0f * invCol + 10.0f * col };
        waist      = { cx + dir * (3.0f  * invCol + 4.0f  * col), drawY + 104.0f * invCol + 9.0f  * col };
        hips       = { cx - dir * (18.0f * col),                  drawY + 86.0f  * invCol + 9.0f  * col };

        hipF   = { hips.x + dir * 5.0f, hips.y };
        kneeF  = { cx + dir * (26.0f * invCol - 40.0f * col), drawY + 46.0f * invCol + 8.0f * col };
        ankleF = { cx + dir * (28.0f * invCol - 64.0f * col), drawY + 6.0f };
        toeF   = { ankleF.x + dir * (12.0f * invCol - 10.0f * col), ankleF.y - 1.0f };

        hipB   = { hips.x - dir * 5.0f, hips.y };
        kneeB  = { cx - dir * (16.0f * invCol + 34.0f * col), drawY + 46.0f * invCol + 10.0f * col };
        ankleB = { cx - dir * (26.0f * invCol + 56.0f * col), drawY + 6.0f * invCol + 9.0f * col };
        toeB   = { ankleB.x - dir * 10.0f, ankleB.y - 1.0f };

        shoulderF = { chest.x + dir * 6.0f, chest.y };
        elbowF    = { cx + dir * (28.0f * invCol + 16.0f * col), drawY + 112.0f * invCol + 6.0f * col };
        handF     = { cx + dir * (36.0f * invCol + 4.0f  * col), drawY + 132.0f * invCol + 4.0f * col };

        shoulderB = { chest.x - dir * 6.0f, chest.y };
        elbowB    = { cx + dir * (-6.0f * invCol + 34.0f * col), drawY + 108.0f * invCol + 11.0f * col };
        handB     = { cx + dir * (14.0f * invCol + 48.0f * col), drawY + 122.0f * invCol + 11.0f * col };

        if (col >= 0.85f) (void)0;

    } else if (state == FighterState::ROLL ||
               (state == FighterState::SPECIAL && characterId == CharacterId::ARCHER && stateTime < 0.34f)) {
        // Low tucked athletic dodge roll under punches & chain grabs
        float spinAngle = stateTime * 16.5f;
        float tumbleX = std::cos(spinAngle) * 10.0f;
        float tumbleY = std::sin(spinAngle) * 8.0f;

        hips       = { cx - dir * 10.0f, drawY + 26.0f };
        waist      = { cx,               drawY + 38.0f };
        chest      = { cx + dir * 16.0f, drawY + 46.0f };
        neck       = { cx + dir * 24.0f, drawY + 50.0f };
        headCenter = { cx + dir * 32.0f + tumbleX * 0.4f, drawY + 52.0f + tumbleY * 0.4f };

        hipF   = { hips.x + dir * 5.0f, hips.y };
        kneeF  = { cx + dir * 24.0f, drawY + 34.0f };
        ankleF = { cx + dir * 14.0f, drawY + 12.0f };
        toeF   = { cx + dir * 24.0f, drawY + 8.0f };

        hipB   = { hips.x - dir * 5.0f, hips.y };
        kneeB  = { cx - dir * 20.0f, drawY + 28.0f };
        ankleB = { cx - dir * 28.0f, drawY + 10.0f };
        toeB   = { cx - dir * 18.0f, drawY + 6.0f };

        shoulderF = { chest.x + dir * 4.0f, chest.y };
        elbowF    = { cx + dir * 26.0f, drawY + 34.0f };
        handF     = { cx + dir * 36.0f, drawY + 28.0f };

        shoulderB = { chest.x - dir * 4.0f, chest.y };
        elbowB    = { cx + dir * 10.0f, drawY + 30.0f };
        handB     = { cx + dir * 20.0f, drawY + 22.0f };

    } else if (state == FighterState::SPECIAL && characterId == CharacterId::FAN_DANCER && fanSpecialStage == 2) {
        // Mei-Ling leaping, grabbing opponent's head with BOTH hands, launching huge Knee into opponent face!
        hips       = { cx - dir * 6.0f,  drawY + 76.0f };
        waist      = { cx + dir * 2.0f,  drawY + 94.0f };
        chest      = { cx + dir * 12.0f, drawY + 126.0f };
        neck       = { chest.x + dir * 5.0f, chest.y + 15.0f };
        headCenter = { neck.x + dir * 5.0f,  neck.y + 15.0f };

        // High tucked back leg
        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 18.0f, drawY + 46.0f };
        ankleB = { cx - dir * 28.0f, drawY + 22.0f };
        toeB   = { cx - dir * 22.0f, drawY + 14.0f };

        // Driving front KNEE upward into opponent's face!
        hipF   = { hips.x + dir * 8.0f, hips.y };
        kneeF  = { cx + dir * 44.0f, drawY + 118.0f };
        ankleF = { cx + dir * 26.0f, drawY + 84.0f };
        toeF   = { cx + dir * 20.0f, drawY + 72.0f };

        // BOTH arms stretched straight forward grabbing the opponent's head!
        shoulderF = { chest.x + dir * 8.0f, chest.y };
        elbowF    = { cx + dir * 34.0f, drawY + 148.0f };
        handF     = { cx + dir * 56.0f, drawY + 162.0f };

        shoulderB = { chest.x - dir * 6.0f, chest.y };
        elbowB    = { cx + dir * 30.0f, drawY + 142.0f };
        handB     = { cx + dir * 54.0f, drawY + 158.0f };

    } else if (state == FighterState::SPECIAL && characterId == CharacterId::SPEAR_WARRIOR && stateTime < 1.38f) {
        // Takeda raising his Spear high skyward in one hand to call down 10 sky spears!
        hips       = { cx,               drawY + 86.0f };
        waist      = { cx + dir * 2.0f,  drawY + 104.0f };
        chest      = { cx + dir * 5.0f,  drawY + 138.0f };
        neck       = { chest.x + dir * 2.0f, chest.y + 16.0f };
        headCenter = { neck.x + dir * 2.0f,  neck.y + 16.0f };

        hipF   = { hips.x + dir * 8.0f, hips.y };
        kneeF  = { cx + dir * 26.0f, drawY + 46.0f };
        ankleF = { cx + dir * 30.0f, drawY + 6.0f };
        toeF   = { cx + dir * 42.0f, drawY + 4.0f };

        hipB   = { hips.x - dir * 8.0f, hips.y };
        kneeB  = { cx - dir * 18.0f, drawY + 46.0f };
        ankleB = { cx - dir * 28.0f, drawY + 6.0f };
        toeB   = { cx - dir * 16.0f, drawY + 4.0f };

        // Front arm raised straight above head gripping spear skyward
        shoulderF = { chest.x + dir * 8.0f, chest.y };
        elbowF    = { shoulderF.x + dir * 10.0f, chest.y + 24.0f };
        handF     = { shoulderF.x + dir * 14.0f, chest.y + 46.0f };

        shoulderB = { chest.x - dir * 7.0f, chest.y };
        elbowB    = { cx - dir * 18.0f, drawY + 114.0f };
        handB     = { cx - dir * 24.0f, drawY + 102.0f };

    } else if (state == FighterState::WALK ||
               (state == FighterState::SPECIAL && characterId == CharacterId::FAN_DANCER && fanSpecialStage == 1)) {
        float baseWalkFreq = prof.isFemale ? 12.2f : (build.isHeavyWarrior ? 7.8f : 9.5f);
        if (running) baseWalkFreq *= 1.55f;
        float phaseWave = stateTime * ((fanSpecialStage == 1) ? 19.0f : baseWalkFreq);
        float stride = std::sin(phaseWave);
        float sprintLean = running ? 6.5f : 0.0f;
        float strideDist = running ? 30.0f : 23.0f;

        hips       = { cx + dir * (pose.torsoLeanX * 0.6f + sprintLean * 0.4f), drawY + (90.0f + std::fabs(stride) * 3.5f) * hScale };
        waist      = { cx + dir * (pose.torsoLeanX * 1.0f + sprintLean * 0.7f), drawY + (112.0f + std::fabs(stride) * 3.5f) * hScale };
        chest      = { cx + dir * (6.0f + pose.torsoLeanX * 1.5f + sprintLean),  drawY + (142.0f + std::fabs(stride) * 3.5f) * hScale };
        neck       = { chest.x + dir * 3.5f, chest.y + 16.0f };
        headCenter = { neck.x + dir * 2.5f,  neck.y + 16.0f };

        hipF   = { hips.x + dir * (prof.isFemale ? 7.5f : 10.0f), hips.y };
        ankleF = { cx + dir * (28.0f + stride * strideDist), drawY + 6.0f + std::max(0.0f, stride) * 16.0f };
        toeF   = { ankleF.x + dir * 13.0f, ankleF.y - 2.0f };
        kneeF  = { (hipF.x + ankleF.x) * 0.5f + dir * 8.0f, (hipF.y + ankleF.y) * 0.5f + 6.0f };

        hipB   = { hips.x - dir * (prof.isFemale ? 7.5f : 10.0f), hips.y };
        ankleB = { cx + dir * (-26.0f - stride * strideDist), drawY + 6.0f + std::max(0.0f, -stride) * 16.0f };
        toeB   = { ankleB.x + dir * 10.0f, ankleB.y - 2.0f };
        kneeB  = { (hipB.x + ankleB.x) * 0.5f + dir * 5.0f, (hipB.y + ankleB.y) * 0.5f + 6.0f };

        shoulderF = { chest.x + dir * build.shoulderOut, chest.y };
        elbowF    = { cx + dir * (28.0f - stride * 10.0f), drawY + 114.0f * hScale };
        handF     = { cx + dir * (37.0f - stride * 15.0f), drawY + 132.0f * hScale };

        shoulderB = { chest.x - dir * build.shoulderBackOut, chest.y };
        elbowB    = { cx + dir * (-16.0f + stride * 10.0f), drawY + 110.0f * hScale };
        handB     = { cx + dir * (-2.0f + stride * 15.0f),  drawY + 122.0f * hScale };

    } else if (state == FighterState::CROUCH) {
        hips       = { cx - dir * 8.0f,  drawY + 44.0f };
        waist      = { cx,               drawY + 58.0f };
        chest      = { cx + dir * 15.0f, drawY + 78.0f };
        neck       = { cx + dir * 23.0f, drawY + 92.0f };
        headCenter = { cx + dir * 30.0f, drawY + 108.0f };

        hipF   = { hips.x + dir * 8.0f, hips.y };
        kneeF  = { cx + dir * 36.0f, drawY + 28.0f };
        ankleF = { cx + dir * 28.0f, drawY + 6.0f };
        toeF   = { cx + dir * 41.0f, drawY + 4.0f };

        hipB   = { hips.x - dir * 8.0f, hips.y };
        kneeB  = { cx - dir * 20.0f, drawY + 24.0f };
        ankleB = { cx - dir * 30.0f, drawY + 6.0f };
        toeB   = { cx - dir * 20.0f, drawY + 4.0f };

        shoulderF = { chest.x + dir * 6.0f, chest.y };
        elbowF    = { cx + dir * 36.0f, drawY + 56.0f };
        handF     = { cx + dir * 44.0f, drawY + 68.0f };

        shoulderB = { chest.x - dir * 6.0f, chest.y };
        elbowB    = { cx + dir * 14.0f, drawY + 48.0f };
        handB     = { cx + dir * 24.0f, drawY + 56.0f };

    } else if (state == FighterState::JUMP_KICK) {
        // Dramatic airborne Flying Kick!
        hips       = { cx - dir * 6.0f,  drawY + 86.0f };
        waist      = { cx - dir * 4.0f,  drawY + 104.0f };
        chest      = { cx - dir * 2.0f,  drawY + 134.0f };
        neck       = { cx - dir * 3.0f,  drawY + 148.0f };
        headCenter = { cx - dir * 2.0f,  drawY + 164.0f };

        // Tucked back leg
        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 22.0f, drawY + 58.0f };
        ankleB = { cx - dir * 34.0f, drawY + 42.0f };
        toeB   = { cx - dir * 28.0f, drawY + 32.0f };

        // Full extended flying kick forward!
        float ext = (phase == AttackPhase::STARTUP) ? 0.35f : (phase == AttackPhase::ACTIVE ? 1.0f : 0.65f);
        hipF   = { hips.x + dir * 8.0f, hips.y };
        ankleF = { cx + dir * (28.0f + ext * 54.0f), drawY + 74.0f - ext * 10.0f };
        kneeF  = { (hipF.x + ankleF.x) * 0.5f, (hipF.y + ankleF.y) * 0.5f + 4.0f };
        toeF   = { ankleF.x + dir * 11.0f, ankleF.y };

        shoulderF = { chest.x + dir * 7.0f, chest.y };
        elbowF    = { cx + dir * 26.0f, drawY + 142.0f };
        handF     = { cx + dir * 38.0f, drawY + 152.0f };

        shoulderB = { chest.x - dir * 6.0f, chest.y };
        elbowB    = { cx - dir * 24.0f, drawY + 116.0f };
        handB     = { cx - dir * 34.0f, drawY + 102.0f };

    } else if (state == FighterState::JUMP || state == FighterState::FALL) {
        hips       = { cx,               drawY + 84.0f };
        waist      = { cx + dir * 4.0f,  drawY + 100.0f };
        chest      = { cx + dir * 10.0f, drawY + 130.0f };
        neck       = { cx + dir * 14.0f, drawY + 146.0f };
        headCenter = { cx + dir * 18.0f, drawY + 164.0f };

        hipF   = { hips.x + dir * 8.0f, hips.y };
        kneeF  = { cx + dir * 28.0f, drawY + 54.0f };
        ankleF = { cx + dir * 16.0f, drawY + 28.0f };
        toeF   = { cx + dir * 26.0f, drawY + 20.0f };

        hipB   = { hips.x - dir * 8.0f, hips.y };
        kneeB  = { cx - dir * 20.0f, drawY + 44.0f };
        ankleB = { cx - dir * 30.0f, drawY + 18.0f };
        toeB   = { cx - dir * 22.0f, drawY + 10.0f };

        shoulderF = { chest.x + dir * 7.0f, chest.y };
        elbowF    = { cx + dir * 32.0f, drawY + 138.0f };
        handF     = { cx + dir * 44.0f, drawY + 150.0f };

        shoulderB = { chest.x - dir * 6.0f, chest.y };
        elbowB    = { cx - dir * 20.0f, drawY + 112.0f };
        handB     = { cx - dir * 30.0f, drawY + 98.0f };

    } else if (state == FighterState::BLOCK) {
        float b = pose.blockBrace;
        float invB = 1.0f - b;

        hips       = { cx - dir * (5.0f * b),               drawY + 86.0f - 4.0f * b };
        waist      = { cx + dir * (3.0f * invB - 4.0f * b),  drawY + 104.0f - 4.0f * b };
        chest      = { cx + dir * (6.0f * invB),            drawY + 138.0f - 4.0f * b };
        neck       = { cx + dir * (8.0f * invB + 3.0f * b),  drawY + 154.0f - 5.0f * b };
        headCenter = { cx + dir * (10.0f * invB + 4.0f * b), drawY + 170.0f - 5.0f * b };

        hipF   = { hips.x + dir * 8.0f, hips.y };
        kneeF  = { cx + dir * (26.0f * invB + 23.0f * b), drawY + 46.0f - 4.0f * b };
        ankleF = { cx + dir * (28.0f * invB + 26.0f * b), drawY + 6.0f };
        toeF   = { cx + dir * 38.0f, drawY + 4.0f };

        hipB   = { hips.x - dir * 8.0f, hips.y };
        kneeB  = { cx - dir * (16.0f * invB + 19.0f * b), drawY + 46.0f - 4.0f * b };
        ankleB = { cx - dir * (26.0f * invB + 30.0f * b), drawY + 6.0f };
        toeB   = { cx - dir * 18.0f, drawY + 4.0f };

        shoulderF = { chest.x + dir * 7.0f, chest.y };
        elbowF    = { cx + dir * (28.0f * invB + 26.0f * b), drawY + 112.0f * invB + 122.0f * b };
        handF     = { cx + dir * (36.0f * invB + 30.0f * b), drawY + 132.0f * invB + 154.0f * b };

        shoulderB = { chest.x - dir * 6.0f, chest.y };
        elbowB    = { cx + dir * (-6.0f * invB + 18.0f * b), drawY + 108.0f * invB + 118.0f * b };
        handB     = { cx + dir * (14.0f * invB + 24.0f * b), drawY + 122.0f * invB + 148.0f * b };

    } else if (state == FighterState::HIT) {
        hips       = { cx + dir * (pose.hitRecoilX * 0.6f), drawY + 84.0f };
        waist      = { cx + dir * (pose.hitRecoilX * 1.1f), drawY + 102.0f };
        chest      = { cx + dir * (pose.hitRecoilX * 1.8f), drawY + 134.0f };
        neck       = { chest.x - dir * 5.0f, chest.y + 16.0f };
        headCenter = { neck.x - dir * 6.0f,  neck.y + 16.0f };

        hipF   = { hips.x + dir * 8.0f, hips.y };
        kneeF  = { cx + dir * (13.0f + pose.hitRecoilX * 0.4f), drawY + 38.0f };
        ankleF = { cx + dir * 19.0f, drawY + 6.0f };
        toeF   = { cx + dir * 30.0f, drawY + 4.0f };

        hipB   = { hips.x - dir * 8.0f, hips.y };
        kneeB  = { cx - dir * (26.0f - pose.hitRecoilX * 0.4f), drawY + 36.0f };
        ankleB = { cx - dir * (34.0f - pose.hitRecoilX * 0.3f), drawY + 6.0f };
        toeB   = { cx - dir * 24.0f, drawY + 4.0f };

        shoulderF = { chest.x + dir * 5.0f, chest.y };
        elbowF    = { cx + dir * (5.0f + pose.hitRecoilX * 0.6f),  drawY + 126.0f };
        handF     = { cx + dir * (15.0f + pose.hitRecoilX * 0.8f), drawY + 122.0f };

        shoulderB = { chest.x - dir * 6.0f, chest.y };
        elbowB    = { cx - dir * (34.0f - pose.hitRecoilX * 0.8f), drawY + 118.0f };
        handB     = { cx - dir * (46.0f - pose.hitRecoilX * 1.0f), drawY + 102.0f };

    } else if (state == FighterState::PUNCH || state == FighterState::COMBO2 ||
               state == FighterState::COMBO3 || state == FighterState::CHAIN_GRAB) {
        hips       = { cx + dir * (5.0f + pose.torsoLeanX * 0.5f),  drawY + 88.0f * hScale };
        waist      = { cx + dir * (10.0f + pose.torsoLeanX * 1.0f), drawY + 108.0f * hScale };
        chest      = { cx + dir * (18.0f + pose.torsoLeanX * 1.6f), drawY + 140.0f * hScale };
        neck       = { chest.x + dir * 4.5f,                        chest.y + 15.0f * hScale };
        headCenter = { neck.x + dir * 4.5f,                         neck.y + 15.0f * hScale };

        hipF   = { hips.x + dir * (prof.isFemale ? 7.5f : 10.0f), hips.y };
        kneeF  = { cx + dir * 34.0f, drawY + 48.0f * hScale };
        ankleF = { cx + dir * 36.0f, drawY + 6.0f };
        toeF   = { cx + dir * 48.0f, drawY + 4.0f };

        hipB   = { hips.x - dir * (prof.isFemale ? 7.5f : 10.0f), hips.y };
        kneeB  = { cx - dir * 20.0f, drawY + 44.0f * hScale };
        ankleB = { cx - dir * 34.0f, drawY + 6.0f };
        toeB   = { cx - dir * 20.0f, drawY + 4.0f };

        shoulderF = { chest.x + dir * build.shoulderOut, chest.y };
        shoulderB = { chest.x - dir * build.shoulderBackOut, chest.y };

        const AttackData* atk = getCurrentAttack();
        float visualReach   = atk ? std::min(atk->reach, 88.0f) : 80.0f;
        float maxReachHandX = (facingRight ? (drawX + drawW) : drawX) + dir * visualReach * 0.96f;
        float strikeY       = atk ? (drawY + atk->hitboxOffsetY + atk->hitboxHeight * 0.5f) : (drawY + 134.0f);
        if (state == FighterState::COMBO3) strikeY += 12.0f; // High finishing strike/uppercut!

        float readyHandX  = cx + dir * 36.0f;
        float coiledHandX = cx + dir * 8.0f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            handF  = { readyHandX * (1.0f - t) + coiledHandX * t, (drawY + 132.0f) * (1.0f - t) + strikeY * t };
            elbowF = { cx + dir * (28.0f * (1.0f - t) + 14.0f * t), drawY + (112.0f * (1.0f - t) + 124.0f * t) };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t);
            handF  = { coiledHandX * (1.0f - snap) + maxReachHandX * snap, strikeY };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, strikeY - 4.0f * (1.0f - snap) };
        } else {
            float t = pose.actionProgress;
            handF  = { maxReachHandX * (1.0f - t) + readyHandX * t, strikeY * (1.0f - t) + (drawY + 132.0f) * t };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, (strikeY - 4.0f) * (1.0f - t) + (drawY + 112.0f) * t };
        }

        // Combo 2 uses back arm cross hook for Brawler / Fan Dancer; Spear Warrior grips spear with both hands!
        if (prof.spearOnlyStyle) {
            elbowB = { cx + dir * 2.0f, drawY + 118.0f };
            handB  = { handF.x - dir * 34.0f, handF.y - 6.0f };
        } else if (state == FighterState::COMBO2) {
            elbowB = { cx + dir * 18.0f, drawY + 122.0f };
            handB  = { cx + dir * 40.0f, drawY + 138.0f };
        } else {
            elbowB = { cx - dir * 10.0f, drawY + 116.0f };
            handB  = { cx + dir * 5.0f,  drawY + 118.0f };
        }

    } else if (state == FighterState::KICK) {
        hips       = { cx + dir * (pose.torsoLeanX * 0.4f), drawY + 86.0f };
        waist      = { cx + dir * (pose.torsoLeanX * 0.9f), drawY + 104.0f };
        chest      = { cx + dir * (pose.torsoLeanX * 1.6f), drawY + 134.0f };
        neck       = { chest.x - dir * 4.0f,                chest.y + 16.0f };
        headCenter = { neck.x - dir * 4.0f,                 neck.y + 16.0f };

        hipB   = { hips.x - dir * 8.0f, hips.y };
        kneeB  = { cx - dir * 14.0f, drawY + 44.0f };
        ankleB = { cx - dir * 18.0f, drawY + 6.0f };
        toeB   = { cx - dir * 8.0f,  drawY + 4.0f };

        hipF = { hips.x + dir * 8.0f, hips.y };
        const AttackData& kAtk = prof.kickAttack;
        float maxReachFootX = (facingRight ? (drawX + drawW) : drawX) + dir * kAtk.reach * 0.98f;
        float kickY         = drawY + kAtk.hitboxOffsetY + kAtk.hitboxHeight * 0.5f;
        float readyFootX    = cx + dir * 28.0f;
        float chamberFootX  = cx + dir * 16.0f;
        float chamberFootY  = drawY + 70.0f;
        float chamberKneeX  = cx + dir * 28.0f;
        float chamberKneeY  = drawY + 90.0f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            float ease = t * t;
            ankleF = { readyFootX * (1.0f - ease) + chamberFootX * ease, (drawY + 6.0f) * (1.0f - ease) + chamberFootY * ease };
            kneeF  = { (cx + dir * 26.0f) * (1.0f - ease) + chamberKneeX * ease, (drawY + 46.0f) * (1.0f - ease) + chamberKneeY * ease };
            toeF   = { ankleF.x + dir * 10.0f, ankleF.y + 4.0f * ease };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t);
            ankleF = { chamberFootX * (1.0f - snap) + (maxReachFootX - dir * 8.0f) * snap, chamberFootY * (1.0f - snap) + kickY * snap };
            kneeF  = { (hipF.x + ankleF.x) * 0.50f, chamberKneeY * (1.0f - snap) + (kickY + 10.0f) * snap };
            toeF   = { ankleF.x + dir * 10.0f, ankleF.y };
        } else {
            float t = pose.actionProgress;
            ankleF = { (maxReachFootX - dir * 8.0f) * (1.0f - t) + readyFootX * t, kickY * (1.0f - t) + (drawY + 6.0f) * t };
            kneeF  = { (hipF.x + ankleF.x) * 0.50f + dir * 5.0f, (drawY + 60.0f) * (1.0f - t) + (drawY + 46.0f) * t };
            toeF   = { ankleF.x + dir * 10.0f, ankleF.y - 2.0f };
        }

        shoulderF = { chest.x + dir * build.shoulderOut, chest.y };
        elbowF    = { cx + dir * 8.0f,  drawY + 126.0f * hScale };
        handF     = { cx + dir * 20.0f, drawY + 130.0f * hScale };

        shoulderB = { chest.x - dir * build.shoulderBackOut, chest.y };
        elbowB    = { cx - dir * 30.0f, drawY + 112.0f * hScale };
        handB     = { cx - dir * 38.0f, drawY + 100.0f * hScale };

    } else if (state == FighterState::SPECIAL) {
        hips       = { cx + dir * (5.0f + pose.torsoLeanX * 0.45f), drawY + 88.0f * hScale };
        waist      = { cx + dir * (11.0f + pose.torsoLeanX * 0.9f), drawY + 108.0f * hScale };
        chest      = { cx + dir * (19.0f + pose.torsoLeanX * 1.6f), drawY + 140.0f * hScale };
        neck       = { chest.x + dir * 4.5f,                        chest.y + 15.0f * hScale };
        headCenter = { neck.x + dir * 4.5f,                         neck.y + 15.0f * hScale };

        hipF   = { hips.x + dir * (prof.isFemale ? 7.5f : 10.0f), hips.y };
        kneeF  = { cx + dir * 36.0f, drawY + 46.0f * hScale };
        ankleF = { cx + dir * 42.0f, drawY + 6.0f };
        toeF   = { cx + dir * 54.0f, drawY + 4.0f };

        hipB   = { hips.x - dir * (prof.isFemale ? 7.5f : 10.0f), hips.y };
        kneeB  = { cx - dir * 24.0f, drawY + 40.0f * hScale };
        ankleB = { cx - dir * 36.0f, drawY + 6.0f };
        toeB   = { cx - dir * 24.0f, drawY + 4.0f };

        shoulderF = { chest.x + dir * build.shoulderOut, chest.y };
        shoulderB = { chest.x - dir * build.shoulderBackOut, chest.y };

        const AttackData& sAtk = prof.specialAttack;
        float visualReach   = std::min(sAtk.reach, 96.0f);
        float maxReachHandX = (facingRight ? (drawX + drawW) : drawX) + dir * visualReach * 0.98f;
        float specY         = drawY + sAtk.hitboxOffsetY + sAtk.hitboxHeight * 0.5f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            elbowF = { cx + dir * (28.0f * (1.0f - t) + 8.0f * t),  drawY + 112.0f * (1.0f - t) + 120.0f * t };
            handF  = { cx + dir * (36.0f * (1.0f - t) + 16.0f * t), drawY + 132.0f * (1.0f - t) + 126.0f * t };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t);
            handF  = { (cx + dir * 16.0f) * (1.0f - snap) + maxReachHandX * snap, specY };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, specY - 4.0f };
        } else {
            float t = pose.actionProgress;
            handF  = { maxReachHandX * (1.0f - t) + (cx + dir * 36.0f) * t, specY * (1.0f - t) + (drawY + 132.0f) * t };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, specY * (1.0f - t) + (drawY + 112.0f) * t };
        }

        elbowB = { cx - dir * 16.0f, drawY + 112.0f };
        handB  = { cx - dir * 24.0f, drawY + 104.0f };

    } else {
        // Real human grounded martial stance (muscular men & heavy Takeda vs curved agile women)
        float breath = pose.breathY;
        float stanceSpread = prof.isFemale ? 24.0f : (build.isHeavyWarrior ? 34.0f : 29.0f);
        hips       = { cx,                drawY + (92.0f + breath * 0.4f) * hScale };
        waist      = { cx + dir * 3.0f,   drawY + (113.0f + breath * 0.7f) * hScale };
        chest      = { cx + dir * (prof.isFemale ? 5.5f : 7.5f), drawY + (143.0f + breath * 1.1f) * hScale };
        neck       = { chest.x + dir * 1.5f, chest.y + 15.0f * hScale };
        headCenter = { neck.x + dir * 2.0f,  neck.y + 15.0f * hScale };

        hipF   = { hips.x + dir * (prof.isFemale ? 7.5f : (build.isHeavyWarrior ? 11.5f : 9.5f)), hips.y };
        kneeF  = { cx + dir * (stanceSpread - 3.0f), drawY + 52.0f * hScale };
        ankleF = { cx + dir * stanceSpread, drawY + 11.0f };
        toeF   = { ankleF.x + dir * 13.0f, drawY + 3.5f };

        hipB   = { hips.x - dir * (prof.isFemale ? 7.5f : (build.isHeavyWarrior ? 11.5f : 9.5f)), hips.y };
        kneeB  = { cx - dir * (stanceSpread - 5.0f), drawY + 52.0f * hScale };
        ankleB = { cx - dir * (stanceSpread - 1.0f), drawY + 11.0f };
        toeB   = { ankleB.x + dir * 12.0f, drawY + 3.5f };

        bool readyGuard = (characterId == CharacterId::BRAWLER || characterId == CharacterId::ARCHER);
        shoulderF = { chest.x + dir * build.shoulderOut, chest.y };
        elbowF    = readyGuard
                    ? Vec2{ cx + dir * (prof.isFemale ? 24.0f : 31.0f), drawY + (114.0f + breath * 0.9f) * hScale }
                    : Vec2{ cx + dir * (build.isHeavyWarrior ? 32.0f : 26.0f), drawY + (116.0f + breath * 0.9f) * hScale };
        handF     = readyGuard
                    ? Vec2{ cx + dir * (prof.isFemale ? 34.0f : 40.0f), drawY + (129.0f + breath * 1.2f) * hScale }
                    : Vec2{ cx + dir * (build.isHeavyWarrior ? 44.0f : 37.0f), drawY + (96.0f  + breath * 1.1f) * hScale };

        shoulderB = { chest.x - dir * build.shoulderBackOut, chest.y };
        elbowB    = { cx - dir * (prof.isFemale ? 20.0f : (build.isHeavyWarrior ? 30.0f : 25.0f)),
                      drawY + (114.0f + breath * 0.8f) * hScale };
        handB     = readyGuard
                    ? Vec2{ cx + dir * (prof.isFemale ? 13.0f : 17.0f), drawY + (120.0f + breath * 1.0f) * hScale }
                    : Vec2{ cx - dir * (prof.isFemale ? 18.0f : (build.isHeavyWarrior ? 26.0f : 22.0f)),
                            drawY + (95.0f  + breath * 1.0f) * hScale };
    }

    // =========================================================================
    // LAYER 1: BACK LIMBS (Real Sculpted Human Back Leg & Arm)
    // =========================================================================
    drawHumanLeg(renderer, hipB, kneeB, ankleB, toeB, dir, build, app, true, hitFlash);
    drawHumanArm(renderer, shoulderB, elbowB, handB, dir, build, app, true, hitFlash);

    // =========================================================================
    // LAYER 2: REAL MALE / FEMALE TORSO & WAIST CLOTHING / ARMOR
    // =========================================================================
    drawHumanTorso(renderer, chest, waist, hips, shoulderF, shoulderB, neck,
                   dir, build, app, characterId, state, stateTime, hitFlash);

    // =========================================================================
    // LAYER 3: FRONT LIMBS & AUTHENTIC WEAPONS
    // =========================================================================
    drawHumanLeg(renderer, hipF, kneeF, ankleF, toeF, dir, build, app, false, hitFlash);
    drawHumanArm(renderer, shoulderF, elbowF, handF, dir, build, app, false, hitFlash);

    if (state != FighterState::DEAD) {
        bool hideFan1 = (characterId == CharacterId::FAN_DANCER && state == FighterState::SPECIAL && fan1Fired && fanSpecialStage == 0);
        bool hideFan2 = (characterId == CharacterId::FAN_DANCER && state == FighterState::SPECIAL && fan2Fired && fanSpecialStage == 0);
        bool hideSpear = (characterId == CharacterId::SPEAR_WARRIOR && state == FighterState::SPECIAL && finalSpearThrown);

        drawWeapon(renderer, *this, prof.weapon, handF, elbowF, handB, chest, dir,
                   state, phase, stateTime, pose.actionProgress,
                   app.chestR, app.chestG, app.chestB, prof.accentR, prof.accentG, prof.accentB,
                   hideFan1, hideFan2, hideSpear);
    }

    // =========================================================================
    // LAYER 4: REAL HUMAN CRANIUM, JAW PROFILE, EYES, LIPS, HAIR & HELMET
    // =========================================================================
    drawHumanHead(renderer, headCenter, chest, dir, build, app, characterId,
                  state, stateTime, hitFlash);

    // =========================================================================
    // LAYER 5: COMBAT AURAS, STUN STARS & ELECTRIC SHOCK ARCS
    // =========================================================================
    if (state == FighterState::BLOCK) {
        bool flashing = (blockFlashTimer > 0.0f);
        float shieldX = cx + dir * 36.0f;
        float shieldY = drawY + 34.0f;
        float shieldW = flashing ? 20.0f : 12.0f;
        float shieldH = drawH * 0.78f;

        if (flashing) {
            renderer.drawRectAlpha(shieldX - shieldW * 0.5f, shieldY, shieldW, shieldH, 1.0f, 1.0f, 0.70f, 0.85f);
            renderer.drawCircle(shieldX, shieldY + shieldH * 0.5f, 40.0f, 1.0f, 0.95f, 0.5f, 0.30f, 20);
        } else {
            renderer.drawRectAlpha(shieldX - shieldW * 0.5f, shieldY, shieldW, shieldH, 0.30f, 0.65f, 1.0f, 0.65f);
        }
    }

    // 2-Second Chain Pull / Heavy Stun Stars above head
    if (state == FighterState::HIT && hitStunDuration >= 1.0f) {
        for (int s = 0; s < 3; ++s) {
            float ang = stateTime * 7.5f + s * 2.094f;
            float sx = headCenter.x + std::cos(ang) * 18.0f;
            float sy = headCenter.y + 24.0f + std::sin(ang) * 5.0f;
            renderer.drawCircle(sx, sy, 3.6f, 1.0f, 0.90f, 0.25f, 0.95f, 8);
        }
    }

    // Electric Shock arcs coursing across shocked victim
    if (electrocuteTimer > 0.0f && state != FighterState::DEAD) {
        drawLightningBolt(renderer, headCenter.x - 16.0f, headCenter.y + 8.0f, ankleF.x + 10.0f, ankleF.y,
                          electrocuteTimer * 15.0f, 14.0f, 0.35f, 0.92f, 1.0f, 2.5f);
        drawLightningBolt(renderer, shoulderB.x - 10.0f, shoulderB.y, hipF.x + 14.0f, hipF.y - 18.0f,
                          electrocuteTimer * 23.0f, 12.0f, 0.95f, 1.0f, 1.0f, 1.8f);
    }

    if (state == FighterState::SPECIAL && phase == AttackPhase::ACTIVE &&
        (characterId == CharacterId::BRAWLER || characterId == CharacterId::CHAIN_WARRIOR)) {
        renderer.drawCircle(handF.x, handF.y, 30.0f, accentR, accentG, accentB, 0.42f, 18);
        renderer.drawCircle(handF.x, handF.y, 16.0f, 1.0f, 0.92f, 0.45f, 0.80f, 14);
    }
}
