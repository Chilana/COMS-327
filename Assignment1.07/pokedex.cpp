#include <cstdio>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include<sstream>

#include "pokedex.h"
using namespace std;
//std::getline(istream&, std::string &, char dlim);


/*

class pokemon{
    public:
        string type;
        int id, species_id, height, weight, base_experiecne, order, is_default;
        pokemon(int id, string identifier, int species_id, 
        int height, int weight, int base_experience, int order, int is_default){
            this->id = id;
            this->type = identifier;
            this->species_id = species_id;
            this->height = height;
            this->weight = weight;
            this->base_experiecne = base_experiecne;
            this->order = order;
            this->is_default = is_default;
        }

        void display(){
            cout << id << " " <<  type << " " << species_id << " " << height << " " << weight<< " " << base_experiecne<< " " << order << " " << is_default << endl;
        }
};

class Moves{
    public:
        string iden;
        int id, generation_id, type_id,  power, pp, accuracy, priority,  target_id,
        damage_class_id,  effect_id,  effect_chance,  contest_type_id,  contest_effect_id,  super_contest_effect_id;
        Moves(int id, string identifier, int generation_id,
        int type_id, int power,int pp,int accuracy,int priority, int target_id,
        int damage_class_id, int effect_id, int effect_chance, int contest_type_id, int contest_effect_id, int super_contest_effect_id){
            this->id = id;
            iden = identifier; 
            this->generation_id = generation_id;
            this->type_id = type_id;
            this->power = power;
            this->pp = pp;
            this->accuracy = accuracy;
            this->priority = priority;
            this->target_id = target_id;
            this->damage_class_id = damage_class_id;
            this->effect_id = effect_id;
            this->effect_chance = effect_chance;
            this->contest_type_id = contest_type_id;
            this->contest_effect_id = contest_effect_id;
            this->super_contest_effect_id = super_contest_effect_id;
            

        }
        void display(){
            cout << id << " " << iden << " " << generation_id << " " << type_id << " " << 
            power << " " << pp << " " << accuracy << " " << priority 
            << " " << target_id << " " << damage_class_id << " " << effect_id << " " << 
            effect_chance << " " << contest_type_id << " " << contest_effect_id << " " <<
             super_contest_effect_id << endl; 
        }
};

class pokemove{
public:
    int pokemon_id, versionGroup, moveId, pokemonMoveMethod, level, order;
    pokemove(int pokemon_id, int versionGroup, int moveId, int pokemonMoveMethod,
    int level, int order){

    this->pokemon_id = pokemon_id;
    this->versionGroup = versionGroup;
    this->moveId = moveId;
    this->pokemonMoveMethod = pokemonMoveMethod;
    this->level = level;
    this->order = order;
    
    }
    void display(){
        cout << pokemon_id << " " << versionGroup << " " << moveId << " " << pokemonMoveMethod << " " << level << " " << order << endl;
    }

}; 

class pokespecies{
public:
    string name;
    int id, generationID, evolves, evoChain, color, shape, habit;
    pokespecies(int id, string name, int generationID, int evolves, int evoChain, int color, int shape, int habit){
        this->id = id;
        this->generationID = generationID;
        this->evolves = evolves;
        this->evoChain = evoChain;
        this->color = color;
        this->shape = shape;
        this->habit = habit;
    }

    void display(){
        cout << id << " " << generationID << " " << evolves << " " << evoChain << " " << color << " " << shape << " " << habit << endl;
    }

};

class experience{
public:
    int growthrate, level, exper;
    experience(int growthRate, int level, int exper){
        this->growthrate = growthRate;
        this->level = level;
        this->exper = exper;
    }
    void display(){
        cout << growthrate << " "  << level << " " << exper << endl;
    }

};

class typeName{
public:
    int id, localLangID;
    string name;
    typeName(int id, int localLangID, string name){
        this->id = id;
        this->localLangID = localLangID;
        this->name = name;
    }
    void display(){
        if(localLangID == 9){
            cout << id << " "  << localLangID << " " << name << endl;       
        } 
    }
};

class pokestats{
public:
    int id, statID, base, effort;
    pokestats(int id, int statID, int base, int effort){
        this->id = id;
        this->statID = statID;
        this->base = base;
        this->effort = effort;
    }
    void display(){
     cout << id << " " << statID << " " << base << " " << effort << endl;
    }
};

class stats{
public:
    string iden;
    int id, damageID, battle, gameID;
    stats(int id, int damageID, string idenity, int battle, int gameID){
        this->id = id;
        this->damageID = damageID;
        this->battle = battle;
        this->gameID = gameID;
        iden = idenity;
    }
    void display(){
     cout << id << " "  << damageID << " " << iden << " " << battle << " " << gameID << endl;

    }
};

class pokeTypes{
public:
    int id, typeID, slot;
    pokeTypes(int id, int typeID, int slot){
        this->id = id;
        this->typeID = typeID;
        this->slot = slot;
    }
    void display(){
        cout << id << " "  << typeID << " " << slot << endl;
    }
};
*/
vector<pokemon> pokeVec;
vector<Moves> moveVec;
vector<pokemove> pokemoveVec;
vector<pokespecies> speciesVec;
vector<experience> experienceVec;
vector<pokestats> pokestatVec;
vector<typeName> typeVec;
vector<stats> statVec;
vector<pokeTypes> pokeTypesVec;

void parsefile(string filestream){
    //look throhugh file need to see whats in them
    string home = "/share/cs327/pokedex/pokedex/data/csv/";
    //string option2 = "/.poke327/pokedex/pokedex/data/csv/";
    //char * $home = "$HOME"
    string option2 = getenv("HOME");
    option2 += "/.poke327/pokedex/pokedex/data/csv/";
    //cout << option2;
    string dir;
    ifstream i;
    i.open(home + filestream + ".csv");
    if(i){
        dir = home + filestream + ".csv";
        i.close();
    }else{
        cerr << "File NOT FOUND" << endl;
        dir = option2 + filestream;
    }
    //}
    //else{
    //    dir = option2 + filestream;
    //}

    
    
    //cout << dir;
    try{
        
        if(filestream == "moves"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string identifier, tempString;
                int id, generation_id, type_id,  power, pp, accuracy, priority,  target_id,
                damage_class_id,  effect_id,  effect_chance,  contest_type_id,  contest_effect_id,  super_contest_effect_id;
                
                stringstream inputString(line);

                getline(inputString, tempString, ',');
                id = atoi(tempString.c_str());
                tempString = "";
                getline(inputString, identifier, ',');
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    generation_id = INT_MAX;
                }
                else{
                    generation_id = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    type_id = INT_MAX;
                }
                else{
                    type_id = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    power = INT_MAX;
                }
                else{
                    power = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    pp = INT_MAX;
                }
                else{
                    pp = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    accuracy = INT_MAX;
                }
                else{
                    accuracy = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    priority = INT_MAX;
                }
                else{
                    priority = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    target_id = INT_MAX;
                }
                else{
                    target_id = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    damage_class_id = INT_MAX;
                }
                else{
                    damage_class_id = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    effect_id = INT_MAX;
                }
                else{
                    effect_id = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    effect_chance = INT_MAX;
                }
                else{
                    effect_chance = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    contest_type_id = INT_MAX;
                }
                else{
                    contest_type_id = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    contest_effect_id = INT_MAX;
                }
                else{
                    contest_effect_id = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    super_contest_effect_id = INT_MAX;
                }
                else{
                    super_contest_effect_id = atoi(tempString.c_str());
                }                
                Moves move(id, identifier, generation_id, type_id,  power, pp, accuracy, priority,  target_id,
                damage_class_id,  effect_id,  effect_chance,  contest_type_id,  contest_effect_id,  super_contest_effect_id);
                
                moveVec.push_back(move);
                
                tempString = "";
                line = "";
            }

            for(auto move: moveVec){
                if(j > 0){
                    move.display();
                }
                j++;
            }
            i.close();
        }
        else if(filestream == "pokemon"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string identifier, tempString;
                int id, species_id, height, weight, base_experiecne, order, is_default;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, identifier);
                getline(inputString, tempString, ',');
                species_id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                height = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                weight = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                base_experiecne = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                order = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                is_default = atoi(tempString.c_str());
                tempString ="";
                pokemon poke(id, identifier, species_id, height, weight, base_experiecne, order, is_default);
                //damage_class_id,  effect_id,  effect_chance);
                
                pokeVec.push_back(poke);
                
                tempString = "";
                line = "";
            }

            for(auto poke: pokeVec){
                if(j > 0){
                    poke.display();
                }
                j++;
            }
            i.close();
        }
        else if(filestream == "pokemon_moves"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string tempString;
                int pokemon_id, versionGroup, moveId, pokemonMoveMethod, level, order;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                pokemon_id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                versionGroup = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                moveId = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                pokemonMoveMethod = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                level = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    order = INT_MAX;
                }
                else{
                    order = atoi(tempString.c_str());
                }
                tempString ="";
                pokemove pokeM(pokemon_id, versionGroup, moveId, pokemonMoveMethod, level, order);
                //damage_class_id,  effect_id,  effect_chance);
                
                pokemoveVec.push_back(pokeM);
                
                tempString = "";
                line = "";
            }

            for(auto pokeM: pokemoveVec){
                if(j > 0){
                    pokeM.display();
                }
                j++;
            }
        }
        else if(filestream == "pokemon_species"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string name, tempString;
                int id, generationID, evolves, evoChain, color, shape, habit, gender, capture, happiness, baby, hatchCounter, 
                        hasGenderDiff, growthRate, formSwitch, lengendary, mythical, order, conqOrder;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, name, ',');
                getline(inputString, tempString, ',');
                generationID = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    evolves = INT_MAX;
                }
                else{
                    evolves = atoi(tempString.c_str());
                }
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    evoChain = INT_MAX;
                }
                else{
                    evoChain = atoi(tempString.c_str());
                }
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    color = INT_MAX;
                }
                else{
                    color = atoi(tempString.c_str());
                }
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    shape = INT_MAX;
                }
                else{
                    shape = atoi(tempString.c_str());
                }
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    habit = INT_MAX;
                }
                else{
                    habit = atoi(tempString.c_str());
                }
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    gender = INT_MAX;
                }
                else{
                    gender = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    capture = INT_MAX;
                }
                else{
                    capture = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    happiness = INT_MAX;
                }
                else{
                    happiness = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    baby = INT_MAX;
                }
                else{
                    baby = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    hatchCounter = INT_MAX;
                }
                else{
                    hatchCounter = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    hasGenderDiff = INT_MAX;
                }
                else{
                    hasGenderDiff = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    growthRate = INT_MAX;
                }
                else{
                    growthRate = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    formSwitch = INT_MAX;
                }
                else{
                    formSwitch = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    lengendary = INT_MAX;
                }
                else{
                    lengendary = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    mythical = INT_MAX;
                }
                else{
                    mythical = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    order = INT_MAX;
                }
                else{
                    order = atoi(tempString.c_str());
                }
                tempString = "";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    conqOrder = INT_MAX;
                }
                else{
                    conqOrder = atoi(tempString.c_str());
                }
                tempString = "";
                pokespecies pokeS(id, name, generationID, evolves, evoChain, color, shape, habit, gender, capture, happiness, baby, hatchCounter, 
                    hasGenderDiff, growthRate, formSwitch, lengendary, mythical, order, conqOrder);
                //damage_class_id,  effect_id,  effect_chance);
                
                speciesVec.push_back(pokeS);
                
                tempString = "";
                line = "";
            }

            for(auto pokeS: speciesVec){
                if(j > 0){
                    pokeS.display();
                }
                j++;
            }
        }
        else if(filestream == "experience"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string tempString;
                int growthrate, level, exper;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                growthrate = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                level = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                exper = atoi(tempString.c_str());
                tempString ="";
                experience xp(growthrate, level, exper);
                //damage_class_id,  effect_id,  effect_chance);
                
                experienceVec.push_back(xp);
                
                tempString = "";
                line = "";
            }

            for(auto xp: experienceVec){
                if(j > 0){
                    xp.display();
                }
                j++;
            }
        }
        else if(filestream == "type_names"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string name, tempString;
                int id, localLangID;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                localLangID = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, name, ',');
                typeName tName(id, localLangID, name);
                //damage_class_id,  effect_id,  effect_chance);
                
                typeVec.push_back(tName);
                
                tempString = "";
                line = "";
            }

            for(auto tName: typeVec){
                if(j > 0){
                    tName.display();
                }
                j++;
            }
        }
        else if(filestream == "pokemon_stats"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string tempString;
                int id, statID, base, effort;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                statID = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                base = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                effort = atoi(tempString.c_str());
                tempString ="";

                pokestats pStats(id, statID, base, effort);
                //damage_class_id,  effect_id,  effect_chance);
                
                pokestatVec.push_back(pStats);
                
                tempString = "";
                line = "";
            }

            for(auto pStats: pokestatVec){
                if(j > 0){
                    pStats.display();
                }
                j++;
            }
            
        }
        else if(filestream == "stats"){
            i.open(dir);
            int j = 0;
            string line = "";
            while(getline(i, line)){
                string iden, tempString;
                int id, damageID, battle, gameID;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                if(tempString.empty()){
                    damageID = INT_MAX;
                }
                else{
                    damageID = atoi(tempString.c_str());
                }
                tempString ="";
                getline(inputString, iden, ',');
                getline(inputString, tempString, ',');
                battle = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                gameID = atoi(tempString.c_str());
                tempString ="";

                stats stat(id, damageID, iden, battle, gameID);
                //damage_class_id,  effect_id,  effect_chance);
                
                statVec.push_back(stat);
                
                tempString = "";
                line = "";
            }

            for(auto stat: statVec){
                if(j > 0){
                    stat.display();
                }
                j++;
            }   
        }
        else if(filestream == "pokemon_types"){
            i.open(dir);
            string line = "";
            int j = 0;
            while(getline(i, line)){
                string tempString;
                int id, typeID, slot;

                stringstream inputString(line);
                getline(inputString, tempString, ',');
                id = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                typeID = atoi(tempString.c_str());
                tempString ="";
                getline(inputString, tempString, ',');
                slot = atoi(tempString.c_str());
                tempString ="";

                pokeTypes pType(id, typeID, slot);
                //damage_class_id,  effect_id,  effect_chance);
                
                pokeTypesVec.push_back(pType);
                
                tempString = "";
                line = "";
            }

            for(auto pType: pokeTypesVec){
                if(j > 0){
                    pType.display();
                }
                j++;
            }   
        }
    }
    catch(...){
        cout << "This does not exist" << endl;
    }
}

void changeMax(string empty, int *a){
    if(empty.empty()){
        *a = INT_MAX;
    }
}
/*int main(){ 
    string filename;

    cin >> filename;

    

    parsefile(filename);

    //cout << "penis";



    return 0;
}*/