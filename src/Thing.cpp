#include "Thing.h"
#include "Game.h"
#include "Qor/Sprite.h"
using namespace std;
using namespace glm;

const std::vector<std::string> Thing :: s_TypeNames({
    "",
    
    // monsters
    "mouse",
    "snail",
    
    // items
    "battery",
    "heart",
    "star",
    "key",
    
    // objects
    "spring",
    "door",
});

Thing :: Thing(
    const std::shared_ptr<Meta>& config,
    MapTile* placeholder,
    Game* game,
    TileMap* map,
    BasicPartitioner* partitioner,
    Cache<Resource, std::string>* resources
):
    Node(config),
    m_pPlaceholder(placeholder),
    m_pPartitioner(partitioner),
    m_pGame(game),
    m_pMap(map),
    m_pResources(resources),
    m_Identity(config->at<string>("name","")),
    m_ThingID(get_id(config))
{
}

std::shared_ptr<Thing> Thing :: find_thing(Node* n)
{
    shared_ptr<Thing> thing;
    thing = dynamic_pointer_cast<Thing>(n->as_node());
    if(not thing){
        thing = dynamic_pointer_cast<Thing>(n->parent()->as_node());
        if(not thing)
            thing = dynamic_pointer_cast<Thing>(n->parent()->parent()->as_node());
    }
    return thing;
}

void Thing :: init_thing()
{
    assert(m_pPartitioner);

    m_Box = m_pPlaceholder->box();

    m_pPartitioner->register_object(shared_from_this(), Game::THING);

    if(is_monster()) {
        TRY(m_pConfig->merge(make_shared<Meta>(
            m_pResources->transform(m_Identity+".json")
        )));
        
        auto mask = m_pConfig->meta("mask");
        m_Box = Box(
            vec3(mask->at<double>(0), mask->at<double>(1), -0.5f),
            vec3(mask->at<double>(2), mask->at<double>(3), 0.5f)
        );
        
        m_HP = m_pConfig->at<int>("hp",5);
        //LOGf("hp: %s", m_HP);
        m_pSprite = make_shared<Sprite>(
            m_pResources->transform(m_Identity+".json"),
            m_pResources
        );
        add(m_pSprite);
        m_pSprite->set_state(0);
        if(m_pPlaceholder->tile_layer()->depth() || m_pConfig->has("depth"))
            m_pSprite->mesh()->set_geometry(m_pMap->tilted_tile_geometry());
        collapse(); // detach from placeholder
        rescale(glm::vec3(1.0f));
        position(m_pPlaceholder->position(Space::WORLD)); // inherit placeholder pos
        // adding a sprite will spawn its center on 0,0...
        // so we offset
        move(glm::vec3(
            m_pSprite->origin().x * m_pSprite->size().x,
            m_pSprite->origin().y * m_pSprite->size().y,
            0.0f
        ));
        //m_pPlaceholder->detach(); // don't want to invalidate iterator
        m_pPlaceholder->mesh()->visible(false); // remove placeholder
        m_pSprite->mesh()->config()->set<string>("id", m_Identity);
        m_pSprite->mesh()->config()->set<Thing*>("thing", this);
        m_pSprite->mesh()->set_box(m_Box);
        m_pPartitioner->register_object(m_pSprite->mesh(), Game::THING);
        m_Solid = true;

        velocity(vec3(-10.0f, 0.0f, 0.0f));

        //on_lazy_tick.connect([t]{
        //    move(vec3(10.0f * t.s(), 0.0f, 0.0f));
        //});
    } else if(m_ThingID == Thing::STAR) {
        auto l = make_shared<Light>();
        string type = config()->at<string>("type");
        //if(type == "gold"){
            l->ambient(Color::white());
            l->diffuse(Color::white());
        //}else if(type == "silver"){
        //    l->ambient(Color::gray());
        //    l->diffuse(Color::gray());
        //}else if(type == "bronze"){
        //    l->ambient(Color("8c7853"));
        //    l->diffuse(Color("8c7853"));
        //}
        //l->diffuse(Color::yellow());
        l->specular(Color::black());
        l->dist(50.0f);
        l->move(glm::vec3(glm::vec3(0.5f, 0.5f, 0.0f)));
        stick(l);
    }
    else if(m_ThingID == Thing::BATTERY) {
        auto l = make_shared<Light>();
        l->ambient(Color::green());
        l->diffuse(Color::green());
        l->specular(Color::black());
        l->dist(50.0f);
        l->move(glm::vec3(glm::vec3(0.5f, 0.5f, 0.0f)));
        stick(l);
    }
    else if(m_ThingID == Thing::HEART) {
        auto l = make_shared<Light>();
        l->ambient(Color::red());
        l->diffuse(Color::red());
        l->specular(Color::black());
        l->dist(50.0f);
        l->move(glm::vec3(glm::vec3(0.5f, 0.5f, 0.0f)));
        stick(l);
    }
}

void Thing :: sound(const std::string& fn)
{
    Sound::play(this, fn, m_pResources);
}

void Thing :: setup_player(const std::shared_ptr<Sprite>& player)
{
}

void Thing :: setup_map(const std::shared_ptr<TileMap>& map)
{
}

void Thing :: setup_other(const std::shared_ptr<Thing>& thing)
{
}

unsigned Thing :: get_id(const std::shared_ptr<Meta>& config)
{
    string name = config->at<string>("name","");
    
    if(name.empty())
        return INVALID_THING;
    auto itr = std::find(ENTIRE(s_TypeNames),name);
    if(itr == s_TypeNames.end())
        return INVALID_THING;
    
    return std::distance(s_TypeNames.begin(), itr);
}

void Thing :: cb_to_player(Node* player_node, Node* thing_node)
{
    auto thing = (Thing*)thing_node;
    if(thing->id() == Thing::STAR){
        if(thing->placeholder()->visible()){
            thing->sound("pickup2.wav");
            thing->placeholder()->visible(false);
            thing->m_ResetCon = thing->game()->on_reset.connect([thing]{
                thing->placeholder()->visible(true);
            });
        }
    }else if(thing->id() == Thing::HEART){
        if(thing->placeholder()->visible()){
            thing->sound("pickup.wav");
            thing->placeholder()->visible(false);
            thing->m_ResetCon = thing->game()->on_reset.connect([thing]{
                thing->placeholder()->visible(true);
            });
        }
    }else if(thing->id() == Thing::BATTERY){
        if(thing->placeholder()->visible()){
            thing->sound("pickup.wav");
            thing->placeholder()->visible(false);
            thing->m_ResetCon = thing->game()->on_reset.connect([thing]{
                thing->placeholder()->visible(true);
            });
        }
    }else if(thing->id() == Thing::SPRING){
        if(thing->hook_type<Sound>().empty())
            thing->sound("spring.wav");
        auto player = player_node->parent();// mask -> mesh -> sprite
        auto vel = player->velocity();
        player->velocity(glm::vec3(0.0f,
            vel.y > 250.0f ? -vel.y : -250.0f,
            0.0f)
        );
    }
    //else if(thing->id() == Thing::SNAIL){
    //    auto player = player_node->parent();// mask -> mesh -> sprite
    //    player->velocity(vec3(0.0f));
    //}
}

void Thing :: cb_to_static(Node* thing_node, Node* static_node)
{
    auto thing = thing_node->config()->at<Thing*>("thing",nullptr);
    if(not thing)
        return;
    if(thing->is_monster())
    {
        if(thing->num_snapshots())
        {
            //unsigned r = thing_node->world_box().classify(static_node->world_box());
            //if(r==0)
            thing->velocity(-thing->velocity());
            auto vx = thing->velocity().x;
            if(vx > K_EPSILON)
                thing->sprite()->set_state("right");
            else if(vx < K_EPSILON)
                thing->sprite()->set_state("left");
            //if(r & kit::bit(3) && (not(r&kit::bit(1)) || (not(r&kit::bit(4))))){
            //    thing->restore_snapshot(0);
            //    thing->velocity(vec3(-abs(thing->velocity().x), 0.0f, 0.0f));
            //    thing->clear_snapshots();
            //    thing->snapshot();
            //}
            //if(r & kit::bit(0) && (not(r&kit::bit(1)) || not(r&kit::bit(4)))){
            //    thing->restore_snapshot(0);
            //    thing->velocity(vec3(abs(thing->velocity().x), 0.0f, 0.0f));
            //    thing->clear_snapshots();
            //    thing->snapshot();
            //}
            ////}
            ////else if(r & 2){
            ////    thing->restore_snapshot(0);
            ////    thing->velocity(vec3(-abs(thing->velocity().x), 0.0f, 0.0f));
            ////}
            //LOG(to_string(r));
        }
        
        //auto vel = thing->velocity();
        //thing->velocity(-vel);
        //if(vel.x < K_EPSILON)
        //    thing->sprite()->set_state("right");
        //else if(vel.x > K_EPSILON)
        //    thing->sprite()->set_state("left");
    }
}

void Thing :: cb_to_bullet(Node* thing_node, Node* bullet)
{
    auto thing = thing_node->config()->at<Thing*>("thing",nullptr);
    if(not thing)
        return;
    if(thing->is_monster() && thing->alive() && not bullet->detaching())
    {
        thing->sound("damage.wav");
        if(thing->damage(bullet->config()->at("damage",1))){
            for(int i=0;i<20;++i)
                thing->gib(bullet);
            bullet->safe_detach();
        }
    }
}

bool Thing :: damage(int dmg)
{
    if(m_HP <= 0 || dmg < 0)
        return false;
    m_HP = std::max(m_HP-dmg, 0);
    if(m_HP <= 0){
        m_Dying = true;
        velocity(glm::vec3(0.0f));
    }
    return true;
}

void Thing :: logic_self(Freq::Time t)
{
    clear_snapshots();
    snapshot();

    if(not alive())
        detach();
}

void Thing :: lazy_logic_self(Freq::Time t)
{
    Node::lazy_logic_self(t);
    m_pPlaceholder->logic_self(t);
}

void Thing :: gib(Node* bullet)
{
    auto gib = make_shared<Sprite>(m_pResources->transform("blood.json"), m_pResources);
    gib->set_state(0);
    auto dir = Angle::degrees(1.0f * (std::rand() % 360)).vector();
    add(gib);
    gib->move(glm::vec3(std::rand() % 32 - 16.0f, std::rand() % 32 - 16.0f, 2.0f));
    gib->velocity(glm::vec3(dir, 0.0f) * 100.0f);
    gib->acceleration(glm::vec3(0.0f, 500.0f, 0.0f));
    gib->scale(std::rand() % 100 / 100.0 * 2.0f);
    auto life = make_shared<float>(0.25f * (std::rand() % 4));
    auto gibptr = gib.get();
    gib->on_tick.connect([gibptr, life](Freq::Time t){
        *life -= t.s();
        if(*life < 0.0f)
            gibptr->detach();
    });
}

