#pragma once

#include "NND_API.h"

class Manager :
	public REX::Singleton<Manager>,
	public RE::BSTEventSink<RE::TESDeathEvent>
{
public:
	enum class NPC_STATE
	{
		kDisabled,
		kProtected,
		kVunerable
	};

	void Register();
	void RequestAPI();
	void LoadSettings();
	void DisableEssentialStatus(RE::Actor* a_actor, RE::TESNPC* a_npc);

private:
	void        DisableEssentialStatusActor(NPC_STATE a_state, RE::Actor* a_actor) const;
	void        DisableEssentialStatusNPC(NPC_STATE a_state, RE::TESNPC* a_npc) const;
	std::string GetActorName(const RE::TESObjectREFRPtr& a_actor) const;
	void        ShowMessage(bool a_showMessage, const std::string& a_message, const std::string& a_notification, const RE::TESObjectREFRPtr& a_actor) const;

	RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>* a_eventSource) override;

	// members
	NND_API::IVNND2*                                     _nnd{ nullptr };
	NPC_STATE                                            generalNPCState{ NPC_STATE::kProtected };
	NPC_STATE                                            followerNPCState{ NPC_STATE::kDisabled };
	NPC_STATE                                            sideQuestNPCState{ NPC_STATE::kProtected };
	bool                                                 enableMessageBoxVIP{ true };
	bool                                                 enableMessageBoxSideQuest{ true };
	bool                                                 enableCameraShake{ true };
	std::string                                          messageVIP{ "With [npc]'s death, the thread of prophecy is severed. Restore a saved game to restore the weave of fate, or persist in the doomed world you have created." };
	std::string                                          messageSideQuest{ "With [npc]'s death, the thread of prophecy is damaged. Restore a saved game to repair the weave of fate, or persist in the doomed world you have created." };
	std::string                                          notificationVIP{ "Your actions have broken the thread of prophecy..." };
	std::string                                          notificationSideQuest{ "Your actions have caused a shift throughout the weave of fate..." };
	std::unordered_map<RE::FormID, RE::QUEST_DATA::Type> questNPCs;
	std::vector<std::string>                             npcExclusions{};
};
