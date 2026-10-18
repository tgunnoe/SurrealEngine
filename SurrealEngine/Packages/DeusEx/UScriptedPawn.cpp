
#include "Precomp.h"
#include "UScriptedPawn.h"

void UScriptedPawn::AddCarcass(const NameString& CarcassName)
{
	if (NumCarcasses() < 4)
	{
		bool carcassSeen = HaveSeenCarcass(CarcassName);
		if (carcassSeen == false)
		{
			Carcasses()[NumCarcasses()] = CarcassName;
			NumCarcasses() = NumCarcasses() + 1;
		}
	}
}

void UScriptedPawn::ConBindEvents()
{
	DeusExConBindEvents();
}

uint8_t UScriptedPawn::GetAllianceType(const NameString& AllianceName)
{
	auto alliex = AlliancesEx();
	EAllianceType result = EAllianceType::ALLIANCE_Neutral;
	for (int i = 0; i < 16; i++)
	{
		if (alliex[i].AllianceName == AllianceName)
		{
			if ((alliex[i].AllianceLevel < 0.0) || (alliex[i].AllianceAgitation >= 1.0))
			{
				result = EAllianceType::ALLIANCE_Hostile;
			}
			else if (alliex[i].AllianceLevel > 0.0)
			{
				result = EAllianceType::ALLIANCE_Friendly;
			}
			break;
		}
	}

	if (bLikesNeutral() && (result == EAllianceType::ALLIANCE_Neutral))
	{
		result = EAllianceType::ALLIANCE_Friendly;
	}
	if (bReverseAlliances())
	{
		if (result == EAllianceType::ALLIANCE_Friendly)
		{
			return (uint8_t)EAllianceType::ALLIANCE_Hostile;
		}
		if (result == EAllianceType::ALLIANCE_Hostile)
		{
			return (uint8_t)EAllianceType::ALLIANCE_Friendly;
		}
	}
	return (uint8_t)result;
}

uint8_t UScriptedPawn::GetPawnAllianceType(UPawn* QueryPawn)
{
	if (UScriptedPawn* qp = UObject::TryCast<UScriptedPawn>(QueryPawn))
	{
		uint8_t othersAlliance = qp->GetAllianceType(Alliance());
		if (othersAlliance == (uint8_t)EAllianceType::ALLIANCE_Hostile)
		{
			return (uint8_t)EAllianceType::ALLIANCE_Hostile;
		}
	}
	return GetAllianceType(QueryPawn->Alliance());
}

bool UScriptedPawn::HaveSeenCarcass(const NameString& CarcassName)
{
	for (int i = 0; i < NumCarcasses(); i++)
	{
		if (Carcasses()[i] == CarcassName)
		{
			return true;
		}
	}
	return false;
}

namespace
{
	// bDetectable is declared by the Deus Ex Actor and is not part of the engine's property tables.
	// Any other game has no such property, and there everything is detectable.
	bool IsDetectable(UPawn* pawn)
	{
		const NameString name = "bDetectable";
		if (!pawn->HasProperty(name))
			return true;
		return pawn->BoolValue(pawn->GetPropertyDataOffset(name)).Get();
	}
}

bool UScriptedPawn::IsValidEnemy(UPawn* TestEnemy, std::optional<bool> bCheckAlliance)
{
	// The scripts call this with the flag left out to mean "and hostile to me" (LoudNoiseScore and SetEnemy
	// rely on it to cull friends); only an explicit false skips the alliance.
	bool checkAlliance = bCheckAlliance.value_or(true);

	bool candidateExists = TestEnemy != nullptr;
	bool isSelf = TestEnemy == this;
	bool alive = candidateExists && TestEnemy->Health() > 0 && !TestEnemy->bDeleteMe();
	bool detectable = candidateExists && IsDetectable(TestEnemy);
	bool hostile = candidateExists && checkAlliance && GetPawnAllianceType(TestEnemy) == (uint8_t)EAllianceType::ALLIANCE_Hostile;

	return EnemyValidity(candidateExists, isSelf, alive, detectable, checkAlliance, hostile);
}
