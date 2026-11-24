#include "graphics/VfxSystem.h"
#include <iostream>

void VfxSystem::registerDef(const VfxDefinition& def) {
    m_defs[def.id] = def;
}

void VfxSystem::init() {
    if (m_initialized) return;
    m_initialized = true;

    auto loadTextureSafe = [](const std::string& path) -> std::shared_ptr<sf::Texture> {
        std::string resolved = resolveAssetPath(path);
        auto tex = std::make_shared<sf::Texture>();
        if (tex->loadFromFile(resolved)) {
            return tex;
        }
        std::cerr << "[VfxSystem] Внимание: не удалось загрузить текстуру VFX: " << path 
                  << " (разрешено как: " << resolved << ")" << std::endl;
        return nullptr;
    };

    // 1. Impacts: ground-slam
    {
        VfxDefinition d;
        d.id = "ground-slam";
        d.texturePath = "assets/sprites/vfx/impacts/ground-slam.png";
        d.frameW = 288;
        d.frameH = 208;
        d.totalFrames = 97;
        d.columns = 9;
        d.pivot = {144.0f, 164.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 2. Nature: earth-spike
    {
        VfxDefinition d;
        d.id = "earth-spike";
        d.texturePath = "assets/sprites/vfx/nature/earth-spike.png";
        d.frameW = 272;
        d.frameH = 150;
        d.totalFrames = 103;
        d.columns = 8;
        d.pivot = {136.0f, 132.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 3. Nature: quake
    {
        VfxDefinition d;
        d.id = "quake";
        d.texturePath = "assets/sprites/vfx/nature/quake.png";
        d.frameW = 288;
        d.frameH = 104;
        d.totalFrames = 94;
        d.columns = 6;
        d.pivot = {144.0f, 88.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 4. Melee: heavy-cleave
    {
        VfxDefinition d;
        d.id = "heavy-cleave";
        d.texturePath = "assets/sprites/vfx/melee/heavy-cleave.png";
        d.frameW = 208;
        d.frameH = 116;
        d.totalFrames = 58;
        d.columns = 6;
        d.pivot = {68.0f, 96.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 5. Melee: hammer-smash
    {
        VfxDefinition d;
        d.id = "hammer-smash";
        d.texturePath = "assets/sprites/vfx/melee/hammer-smash.png";
        d.frameW = 208;
        d.frameH = 112;
        d.totalFrames = 61;
        d.columns = 6;
        d.pivot = {80.0f, 92.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 6. Melee: spin-slash
    {
        VfxDefinition d;
        d.id = "spin-slash";
        d.texturePath = "assets/sprites/vfx/melee/spin-slash.png";
        d.frameW = 208;
        d.frameH = 104;
        d.totalFrames = 25;
        d.columns = 4;
        d.pivot = {104.0f, 84.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 7. Melee: sword-slash
    {
        VfxDefinition d;
        d.id = "sword-slash";
        d.texturePath = "assets/sprites/vfx/melee/sword-slash.png";
        d.frameW = 136;
        d.frameH = 104;
        d.totalFrames = 24;
        d.columns = 5;
        d.pivot = {44.0f, 92.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 8. Explosions: dirt-explosion
    {
        VfxDefinition d;
        d.id = "dirt-explosion";
        d.texturePath = "assets/sprites/vfx/explosions/dirt-explosion.png";
        d.frameW = 260;
        d.frameH = 240;
        d.totalFrames = 109;
        d.columns = 11;
        d.pivot = {130.0f, 206.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 9. Impacts: heavy-landing
    {
        VfxDefinition d;
        d.id = "heavy-landing";
        d.texturePath = "assets/sprites/vfx/impacts/heavy-landing.png";
        d.frameW = 176;
        d.frameH = 80;
        d.totalFrames = 49;
        d.columns = 5;
        d.pivot = {88.0f, 60.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }

    // 10. Impacts: hit-spark
    {
        VfxDefinition d;
        d.id = "hit-spark";
        d.texturePath = "assets/sprites/vfx/impacts/hit-spark.png";
        d.frameW = 136;
        d.frameH = 80;
        d.totalFrames = 16;
        d.columns = 4;
        d.pivot = {44.0f, 64.0f};
        d.fps = 30.0f;
        d.texture = loadTextureSafe(d.texturePath);
        registerDef(d);
    }
}

void VfxSystem::spawn(const std::string& name, const sf::Vector2f& pos,
                      const sf::Vector2f& scale, float rotation, const sf::Color& color) {
    if (!m_initialized) {
        init();
    }
    auto it = m_defs.find(name);
    if (it == m_defs.end()) {
        std::cerr << "[VfxSystem] Неизвестный эффект: " << name << std::endl;
        return;
    }

    VfxInstance inst;
    inst.name = name;
    inst.position = pos;
    inst.scale = scale;
    inst.rotation = rotation;
    inst.color = color;
    inst.timer = 0.0f;
    inst.currentFrame = 0;
    inst.active = true;
    inst.def = &it->second;

    m_instances.push_back(inst);
}

void VfxSystem::spawnGroundSlam(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("ground-slam", pos, {scaleX, scaleY});
}

void VfxSystem::spawnEarthSpike(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("earth-spike", pos, {scaleX, scaleY});
}

void VfxSystem::spawnQuake(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("quake", pos, {scaleX, scaleY});
}

void VfxSystem::spawnHeavyCleave(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("heavy-cleave", pos, {scaleX, scaleY});
}

void VfxSystem::spawnHammerSmash(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("hammer-smash", pos, {scaleX, scaleY});
}

void VfxSystem::spawnSpinSlash(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("spin-slash", pos, {scaleX, scaleY});
}

void VfxSystem::spawnSwordSlash(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("sword-slash", pos, {scaleX, scaleY});
}

void VfxSystem::spawnDirtExplosion(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("dirt-explosion", pos, {scaleX, scaleY});
}

void VfxSystem::spawnHeavyLanding(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("heavy-landing", pos, {scaleX, scaleY});
}

void VfxSystem::spawnHitSpark(const sf::Vector2f& pos, float scaleX, float scaleY) {
    spawn("hit-spark", pos, {scaleX, scaleY});
}

void VfxSystem::update(float dt) {
    for (auto& inst : m_instances) {
        if (inst.active) {
            inst.update(dt);
        }
    }
    m_instances.erase(
        std::remove_if(m_instances.begin(), m_instances.end(),
                       [](const VfxInstance& inst) { return !inst.active; }),
        m_instances.end()
    );
}

void VfxSystem::draw(sf::RenderTarget& target) {
    for (const auto& inst : m_instances) {
        if (inst.active) {
            inst.draw(target);
        }
    }
}

void VfxSystem::clear() {
    m_instances.clear();
}

size_t VfxSystem::getActiveCount() const {
    size_t count = 0;
    for (const auto& inst : m_instances) {
        if (inst.active) count++;
    }
    return count;
}

