// Pack Medic — plugin RE_Kenshi.
// Quando o médico (tarefa Médico), o reparador de robôs ou quem põe talas fica sem kit e alguém do grupo
// precisa de cuidado, o mod passa um kit do animal de carga mais próximo pra ele.
//
// Hooks:
//   GameWorld::mainLoop_GPUSensitiveStuff -> confere o grupo a cada @interval ms (thread principal)

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/CharStats.h>
#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/PlayerInterface.h>

#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <Windows.h>

namespace
{
	const char* const VERSION = "1.1.0";

	bool debug = false;
	float maxRange = 1000.0f; // @range: distância máxima médico ↔ animal (unidades do mundo)
	DWORD interval = 1000;    // @interval: ms entre conferências
	DWORD lastCheck = 0;

	// Como medir se alguém do grupo precisa do kit
	enum NeedKind
	{
		NEED_FIRSTAID,   // scoreFirstAidNeed(false)
		NEED_ROBOTAID,   // scoreFirstAidNeed(true)
		NEED_SPLINT      // scoreJuryRigNeed(habilidade de medicina de quem põe a tala)
	};

	// Tarefa permanente -> função do kit que ela consome. `alt`: outra tarefa que também vale.
	// Talas: o jogo não deixa "Aplicar talas" virar tarefa fixa pelo menu, então vale o Médico.
	struct Need
	{
		TaskType job;
		TaskType alt;
		ItemFunction kit;
		NeedKind kind;
		const char* label;
	};
	const Need NEEDS[] = {
		{ JOB_MEDIC, JOB_MEDIC, ITEM_FIRSTAID, NEED_FIRSTAID, "first aid kit" },
		{ JOB_REPAIR_ROBOT, JOB_REPAIR_ROBOT, ITEM_ROBOTREPAIR, NEED_ROBOTAID, "robot repair kit" },
		{ JOB_MEDIC, SPLINT_JOB, ITEM_MEDRIGGING, NEED_SPLINT, "splint kit" },
	};
	const int NEED_COUNT = sizeof(NEEDS) / sizeof(NEEDS[0]);

	std::string modDir()
	{
		HMODULE hm = NULL;
		char path[MAX_PATH] = { 0 };
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCSTR)&modDir, &hm);
		GetModuleFileNameA(hm, path, MAX_PATH);
		std::string s(path);
		size_t slash = s.find_last_of("\\/");
		return slash == std::string::npos ? "" : s.substr(0, slash + 1);
	}

	std::string trim(const std::string& s)
	{
		size_t a = s.find_first_not_of(" \t\r\n");
		if (a == std::string::npos)
			return "";
		size_t b = s.find_last_not_of(" \t\r\n");
		return s.substr(a, b - a + 1);
	}

	// settings.txt: linhas "@chave = valor"; o resto é comentário
	std::map<std::string, std::string> loadSettings(const std::string& path)
	{
		std::map<std::string, std::string> out;
		std::ifstream f(path.c_str());
		std::string line;
		while (std::getline(f, line))
		{
			line = trim(line);
			size_t eq = line.find('=');
			if (line.empty() || line[0] != '@' || eq == std::string::npos)
				continue;
			out[trim(line.substr(1, eq - 1))] = trim(line.substr(eq + 1));
		}
		return out;
	}

	void dbg(const std::string& msg)
	{
		if (debug)
			DebugLog("PackMedic: " + msg);
	}

	bool awake(Character* c)
	{
		return c && !c->isDead() && !c->medical.unconcious;
	}

	bool hasJob(Character* c, TaskType job)
	{
		int n = c->getPermajobCount();
		for (int i = 0; i < n; ++i)
			if (c->getPermajob(i) == job)
				return true;
		return false;
	}

	// Melhor kit com essa função no inventário ou nas mochilas dentro dele. `owner` recebe o
	// inventário onde o kit está (é dele que o kit tem que sair).
	Item* findKit(Inventory* inv, ItemFunction fn, Inventory*& owner, int depth)
	{
		if (!inv || depth > 2)
			return NULL;
		Item* best = inv->getBestItemWithFunction(fn);
		if (best)
		{
			owner = inv;
			return best;
		}
		const lektor<Item*>& items = inv->getAllItems();
		for (size_t i = 0; i < items.size(); ++i)
		{
			Item* it = items[i];
			if (!it || it->getItemType() != CONTAINER)
				continue;
			Item* inner = findKit(static_cast<ContainerItem*>(it)->getInventory(), fn, owner, depth + 1);
			if (inner)
				return inner;
		}
		return NULL;
	}

	// A mochila equipada não aparece em getAllItems() do dono (fica num compartimento de equipamento)
	Item* findKitOn(Character* c, ItemFunction fn, Inventory*& owner)
	{
		Item* kit = findKit(c->getInventory(), fn, owner, 0);
		if (kit)
			return kit;
		ContainerItem* pack = c->hasABackpackOn();
		return pack ? findKit(pack->getInventory(), fn, owner, 1) : NULL;
	}

	// @debug: lista o que o animal carrega (tipo e função de cada item), uma vez por animal
	std::set<std::string> dumped;
	void dumpInventory(Inventory* inv, const std::string& indent, int depth)
	{
		if (!inv || depth > 2)
			return;
		const lektor<Item*>& items = inv->getAllItems();
		for (size_t i = 0; i < items.size(); ++i)
		{
			Item* it = items[i];
			if (!it)
				continue;
			std::ostringstream o;
			o << indent << it->getName() << " (type " << (int)it->getItemType() << ", function "
				<< (int)it->itemFunction << ", x" << it->quantity << ")";
			dbg(o.str());
			if (it->getItemType() == CONTAINER)
				dumpInventory(static_cast<ContainerItem*>(it)->getInventory(), indent + "  ", depth + 1);
		}
	}

	float needScore(Character* patient, const Need& need, Character* medic)
	{
		if (need.kind == NEED_SPLINT)
		{
			CharStats* st = medic->getStats();
			return patient->medical.scoreJuryRigNeed(st ? st->medic : 0.0f);
		}
		return patient->medical.scoreFirstAidNeed(need.kind == NEED_ROBOTAID);
	}

	bool someoneNeeds(lektor<Character*>& squad, const Need& need, Character* medic)
	{
		for (size_t i = 0; i < squad.size(); ++i)
		{
			Character* c = squad[i];
			if (!c || c->isDead())
				continue;
			float score = needScore(c, need, medic);
			if (debug && need.kind == NEED_SPLINT && score != 0.0f)
			{
				std::ostringstream o;
				o << "  " << c->getName() << " splint need " << score;
				dbg(o.str());
			}
			if (score > 0.0f)
				return true;
		}
		return false;
	}

	// Passa um kit do animal pro médico. Se não couber, devolve ao animal.
	bool transfer(Item* kit, Inventory* from, Character* medic)
	{
		Inventory* to = medic->getInventory();
		if (!to || !to->hasRoomForItem(kit->data))
		{
			dbg("  no room for " + kit->getName() + " in " + medic->getName() + "'s inventory");
			return false;
		}
		Item* moved = from->removeItemDontDestroy_returnsItem(kit, 1, true);
		if (!moved)
			return false;
		if (!to->addItem(moved, moved->quantity, false, false))
		{
			from->addItem(moved, moved->quantity, true, false);
			dbg("  addItem failed, kit returned to the animal");
			return false;
		}
		return true;
	}

	void supply(Character* medic, const Need& need, lektor<Character*>& squad)
	{
		Inventory* own = NULL;
		Item* ownKit = findKitOn(medic, need.kit, own);
		if (debug && need.kind == NEED_SPLINT)
		{
			CharStats* st = medic->getStats();
			std::ostringstream o;
			o << medic->getName() << " (splints), medic skill " << (st ? st->medic : 0.0f)
				<< (ownKit ? ", carries " + ownKit->getName() : ", no splint kit");
			dbg(o.str());
		}
		if (ownKit)
			return;
		if (!someoneNeeds(squad, need, medic))
		{
			if (need.kind == NEED_SPLINT)
				dbg("  nobody needs a splint");
			return;
		}

		Ogre::Vector3 pos = medic->getPosition();
		Character* bestAnimal = NULL;
		Item* bestKit = NULL;
		Inventory* bestOwner = NULL;
		float bestDist = 0.0f;
		for (size_t i = 0; i < squad.size(); ++i)
		{
			Character* a = squad[i];
			if (!a || a == medic || a->isDead() || !a->isAnimal())
				continue;
			if (debug && dumped.insert(a->getName()).second)
			{
				dbg("  inventory of " + a->getName() + ":");
				dumpInventory(a->getInventory(), "    ", 0);
				lektor<InventorySection*>& sections = a->getInventory()->getAllSections();
				for (size_t k = 0; k < sections.size(); ++k)
				{
					std::ostringstream o;
					o << "    section '" << sections[k]->name << "': " << sections[k]->getItems().size() << " items";
					dbg(o.str());
				}
				ContainerItem* pack = a->hasABackpackOn();
				dbg(pack ? "    backpack " + pack->getName() + ":" : "    no backpack");
				if (pack)
					dumpInventory(pack->getInventory(), "      ", 1);
			}
			float d = pos.distance(a->getPosition());
			Inventory* owner = NULL;
			Item* kit = findKitOn(a, need.kit, owner);
			std::ostringstream o;
			o << "  " << a->getName() << " at " << (int)d << (kit ? ": has " + kit->getName() : ": no kit");
			dbg(o.str());
			if (!kit || d > maxRange || (bestAnimal && d >= bestDist))
				continue;
			bestAnimal = a;
			bestKit = kit;
			bestOwner = owner;
			bestDist = d;
		}
		if (!bestAnimal)
			return;

		std::string kitName = bestKit->getName();
		if (transfer(bestKit, bestOwner, medic))
		{
			std::ostringstream o;
			o << medic->getName() << " took " << kitName << " (" << need.label << ") from "
				<< bestAnimal->getName() << " (" << (int)bestDist << " away)";
			DebugLog("PackMedic: " + o.str());
		}
	}

	// @debug: lista as tarefas permanentes de cada personagem quando mudam (número do TaskType)
	std::map<std::string, std::string> lastJobs;
	void dumpJobs(Character* c)
	{
		std::ostringstream o;
		int n = c->getPermajobCount();
		for (int i = 0; i < n; ++i)
			o << (i ? ", " : "") << (int)c->getPermajob(i);
		std::string jobs = o.str();
		std::string& last = lastJobs[c->getName()];
		if (last == jobs)
			return;
		last = jobs;
		std::ostringstream m;
		m << c->getName() << " jobs: [" << jobs << "] (medic " << (int)JOB_MEDIC << ", robot "
			<< (int)JOB_REPAIR_ROBOT << ", splint " << (int)SPLINT_JOB << ")";
		dbg(m.str());
	}

	void check()
	{
		if (!ou || !ou->player)
			return;
		lektor<Character*>& squad = ou->player->playerCharacters;
		for (size_t i = 0; i < squad.size(); ++i)
		{
			Character* c = squad[i];
			if (!awake(c) || c->isAnimal())
				continue;
			if (debug)
				dumpJobs(c);
			for (int n = 0; n < NEED_COUNT; ++n)
				if (hasJob(c, NEEDS[n].job) || hasJob(c, NEEDS[n].alt))
					supply(c, NEEDS[n], squad);
		}
	}

	void (*mainLoop_orig)(GameWorld*, float) = NULL;
	void mainLoop_hook(GameWorld* self, float time)
	{
		mainLoop_orig(self, time);
		DWORD now = GetTickCount();
		if (now - lastCheck < interval)
			return;
		lastCheck = now;
		check();
	}
}

// Tem que ser macro: GetRealAddress precisa receber &Classe::funcao direto no ponto de uso.
#define HOOK(target, detour, original) \
	if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(target), (void*)(detour), (void**)(original))) \
		ErrorLog("PackMedic: failed to hook " #target)

__declspec(dllexport) void startPlugin()
{
	std::map<std::string, std::string> s = loadSettings(modDir() + "settings.txt");
	debug = s["debug"] == "1";
	if (!s["range"].empty())
		maxRange = (float)atof(s["range"].c_str());
	if (!s["interval"].empty())
		interval = (DWORD)atoi(s["interval"].c_str());

	std::ostringstream o;
	o << "PackMedic v" << VERSION << ": range " << maxRange << ", interval " << interval << " ms"
		<< (debug ? ", debug on" : "");
	DebugLog(o.str());

	HOOK(&GameWorld::_NV_mainLoop_GPUSensitiveStuff, &mainLoop_hook, &mainLoop_orig);
}
