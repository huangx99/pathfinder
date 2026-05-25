#include "procedural/ObjectArtCatalog.h"
#include "procedural/ProceduralArt.h"

namespace pf::art {

ax::Node* createWorldObjectIcon(const std::string& key, float size) {
    if (key == "red_berry") return createRedBerry(size);
    if (key == "decayed_red_berry") return createDecayedRedBerry(size);
    if (key == "bitter_leaf") return createBitterLeaf(size);
    if (key == "wood") return createWood(size);
    if (key == "dry_grass") return createDryGrass(size);
    if (key == "loose_stone") return createLooseStone(size);
    if (key == "stone_flake") return createStoneFlake(size);
    if (key == "flint") return createFlint(size);
    if (key == "clay") return createClay(size);
    if (key == "reed") return createReed(size);
    if (key == "vine") return createVine(size);
    if (key == "mushroom") return createMushroom(size);
    if (key == "poison_mushroom") return createPoisonMushroom(size);
    if (key == "axe") return createAxe(size);
    if (key == "torch") return createTorch(size);
    if (key == "camp_fire") return createCampFire(size);
    if (key == "berry_bush") return createBerryBush(size);
    if (key == "young_tree") return createTree(size);
    if (key == "loose_stone_node") return createStonePile(size);
    if (key == "beast_shadow") return createWolf(size);
    return createLooseStone(size);
}

ax::Node* createWorldAgentIcon(const std::string& key, float size) {
    if (key == "companion") return createCompanion(size);
    if (key == "beast_shadow") return createWolf(size);
    return createCompanion(size);
}

} // namespace pf::art
