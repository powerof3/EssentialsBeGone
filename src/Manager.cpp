#include "Manager.h"

void Manager::Register()
{
	if (auto scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton()) {
		scriptEventSource->AddEventSink(this);
	}
}

void Manager::RequestAPI()
{
	_nnd = static_cast<NND_API::IVNND2*>(NND_API::RequestPluginAPI());
	if (_nnd) {
		logger::info("NND API requested successfully");
	} else {
		logger::error("Failed to request NND API");
	}
}

void Manager::LoadSettings()
{
	constexpr auto path = "Data/SKSE/Plugins/po3_EssentialsBeGone.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, generalNPCState, "Settings", "iGeneralNPCState", ";Generic NPC state.\n;0 - Normal (no change), 1 - Protected (can only be killed by the player), 2 - Vunerable (can be killed by anyone).");
	ini::get_value(ini, sideQuestNPCState, "Settings", "iSideQuestNPCState", ";Sidequest NPC state (important quest NPCs will always be protected).\n;0 - Normal, 1 - Protected, 2 - Vunerable.");
	ini::get_value(ini, followerNPCState, "Settings", "iFollowerNPCState", ";Follower NPC state.\n;0 - Normal, 1 - Protected, 2 - Vunerable.");
	ini::get_value(ini, enableMessageBoxVIP, "Settings", "bShowMessageboxVIP", ";Show a Morrowind style message box when important quest NPCs are killed. If false, a notification will display instead.");
	ini::get_value(ini, enableMessageBoxSideQuest, "Settings", "bShowMessageboxSideQuest", ";Show a Morrowind style message box when side quest NPCs are killed. If false, a notification will display instead.");
	ini::get_value(ini, enableCameraShake, "Settings", "bEnableCameraShake", ";Shake camera when important NPCs are killed.");
	ini::get_value(ini, npcExclusions, "Settings", "sNPCExcludeList", ";List of NPCs to exclude (NPC1EditorID,NPC2EditorID).", ",");
	ini::get_value(ini, messageVIP, "Messages", "sMessageboxVIP", ";Message that triggers for important NPCs.");
	ini::get_value(ini, messageSideQuest, "Messages", "sMessageboxSideQuest", ";Message that triggers for side quest NPCs.");
	ini::get_value(ini, notificationVIP, "Messages", "sNotificationVIP", ";Notification that triggers for important NPCs.");
	ini::get_value(ini, notificationSideQuest, "Messages", "sNotificationSideQuest", ";Notification that triggers for side quest NPCs.");

	(void)ini.SaveFile(path);
}

void Manager::DisableEssentialStatus(RE::Actor* a_actor, RE::TESNPC* a_npc)
{
	if (!a_actor || !a_npc) {
		return;
	}

	if (std::find(npcExclusions.begin(), npcExclusions.end(), editorID::get_editorID(a_npc)) != npcExclusions.end()) {
		return;
	}

	bool essential = a_actor->IsEssential();
	bool playerTeammate = a_actor->IsPlayerTeammate();

	if (essential || a_actor->IsProtected()) {
		if (playerTeammate) {
			DisableEssentialStatusActor(followerNPCState, a_actor);
		} else {
			DisableEssentialStatusActor(generalNPCState, a_actor);
		}
	}

	bool baseEssential = a_npc->IsEssential();

	if (baseEssential || a_npc->IsProtected()) {
		if (playerTeammate) {
			DisableEssentialStatusNPC(followerNPCState, a_npc);
		} else {
			DisableEssentialStatusNPC(generalNPCState, a_npc);
		}
	}

	auto xAliases = a_actor->extraList.GetByType<RE::ExtraAliasInstanceArray>();
	if (xAliases) {
		RE::BSReadLockGuard locker(xAliases->lock);
		for (auto& aliasData : xAliases->aliases) {
			if (aliasData) {
				auto quest = aliasData->quest;
				auto alias = const_cast<RE::BGSBaseAlias*>(aliasData->alias);
				if (quest && alias && quest->GetType() != RE::QUEST_DATA::Type::kNone && (alias->IsEssential() || essential || baseEssential)) {
					alias->SetEssential(false);
					alias->SetProtected(true);

					bool isVunerable = playerTeammate ? (followerNPCState == NPC_STATE::kVunerable) : (generalNPCState == NPC_STATE::kVunerable);

					if (isVunerable && sideQuestNPCState == NPC_STATE::kVunerable) {
						switch (quest->GetType()) {
						case RE::QUEST_DATA::Type::kMiscellaneous:
						case RE::QUEST_DATA::Type::kSideQuest:
							alias->SetProtected(false);
							break;
						default:
							break;
						}
					}

					questNPCs.insert({ a_actor->GetFormID(), quest->GetType() });
				}
			}
		}
	}
}

void Manager::DisableEssentialStatusActor(NPC_STATE a_state, RE::Actor* a_actor) const
{
	switch (a_state) {
	case NPC_STATE::kDisabled:
		break;
	case NPC_STATE::kProtected:
		{
			a_actor->boolFlags.reset(RE::Actor::BOOL_FLAGS::kEssential);
			a_actor->boolFlags.set(RE::Actor::BOOL_FLAGS::kProtected);
		}
		break;
	case NPC_STATE::kVunerable:
		{
			a_actor->boolFlags.reset(RE::Actor::BOOL_FLAGS::kEssential);
			a_actor->boolFlags.reset(RE::Actor::BOOL_FLAGS::kProtected);
		}
		break;
	default:
		std::unreachable();
	}
}

void Manager::DisableEssentialStatusNPC(NPC_STATE a_state, RE::TESNPC* a_npc) const
{
	switch (a_state) {
	case NPC_STATE::kDisabled:
		break;
	case NPC_STATE::kProtected:
		{
			a_npc->actorData.actorBaseFlags.set(RE::ACTOR_BASE_DATA::Flag::kProtected);
			a_npc->actorData.actorBaseFlags.reset(RE::ACTOR_BASE_DATA::Flag::kEssential);
		}
		break;
	case NPC_STATE::kVunerable:
		{
			a_npc->actorData.actorBaseFlags.reset(RE::ACTOR_BASE_DATA::Flag::kEssential);
			a_npc->actorData.actorBaseFlags.reset(RE::ACTOR_BASE_DATA::Flag::kProtected);
		}
		break;
	default:
		std::unreachable();
	}
}

std::string Manager::GetActorName(const RE::TESObjectREFRPtr& a_actor) const
{
	if (_nnd) {
		if (auto actor = a_actor->As<RE::Actor>()) {
			_nnd->RevealName(actor);
			if (auto name = _nnd->GetName(actor, NND_API::NameContext::kCrosshair); !name.empty()) {
				return { name.data(), name.size() };
			}
		}
	}

	return a_actor->GetDisplayFullName();
}

void Manager::ShowMessage(bool a_showMessage, const std::string& a_message, const std::string& a_notification, const RE::TESObjectREFRPtr& a_actor) const
{
	const auto get_message = [this](const std::string& a_template, const RE::TESObjectREFRPtr& actor) {
		std::string result = a_template;
		string::replace_all(result, "[npc]", GetActorName(actor));
		return result;
	};

	if (a_showMessage) {
		RE::DebugMessageBox(get_message(a_message, a_actor).c_str());
	} else {
		RE::SendHUDMessage::ShowHUDMessage(get_message(a_notification, a_actor).c_str());
	}
}

RE::BSEventNotifyControl Manager::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
{
	if (!a_event || !a_event->actorDying) {
		return RE::BSEventNotifyControl::kContinue;
	}

	const auto& actor = a_event->actorDying;

	if (auto result = questNPCs.find(actor->GetFormID()); result != questNPCs.end()) {
		if (!a_event->dead) {
			RE::PlaySound("AMBRumbleShakeGreybeardsSD");
			if (enableCameraShake) {
				switch (result->second) {
				case RE::QUEST_DATA::Type::kMiscellaneous:
				case RE::QUEST_DATA::Type::kSideQuest:
					RE::ShakeCamera(0.125f, actor->GetPosition(), 2.0f);
					break;
				default:
					RE::ShakeCamera(0.25f, actor->GetPosition(), 2.0f);
					break;
				}
			}
		} else {
			switch (result->second) {
			case RE::QUEST_DATA::Type::kMiscellaneous:
			case RE::QUEST_DATA::Type::kSideQuest:
				ShowMessage(enableMessageBoxSideQuest, messageSideQuest, notificationSideQuest, actor);
				break;
			default:
				ShowMessage(enableMessageBoxVIP, messageVIP, notificationVIP, actor);
				break;
			}
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}
