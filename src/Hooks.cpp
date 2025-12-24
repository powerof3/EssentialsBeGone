#include "Hooks.h"

#include "Manager.h"

namespace Hooks
{
	struct Load3D
	{
		static RE::NiAVObject* thunk(RE::Character* a_this, bool a_backgroundLoading)
		{
			auto node = func(a_this, a_backgroundLoading);
			if (node) {
				Manager::GetSingleton()->DisableEssentialStatus(a_this, a_this->GetActorBase());
			}
			return node;
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static constexpr std::size_t                   idx{ 0x6A };

		static void Install()
		{
			stl::write_vfunc<RE::Character, Load3D>();
			logger::info("Hooked Character::Load3D"sv);
		}
	};

	void Install()
	{
		Load3D::Install();
	}
}
