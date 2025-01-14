﻿#include "../../Hooks/hooks.h"
#include "AnimationFix.h"
#include "../Rage/cresolver.h"
#include "../Features.h"
Info resolverInfo[64];
History resolverRecord[64];

void UpdatePlayer(IBasePlayer* player)
{
	// make a backup of globals
	const auto backup_frametime = interfaces.global_vars->frametime;
	const auto backup_curtime = interfaces.global_vars->curtime;

	// get player anim state
	auto state = player->GetPlayerAnimState();

	// allow re-animate player in this tick
	if (state->m_last_update_frame == interfaces.global_vars->framecount)
		state->m_last_update_frame -= 1;

	// fixes for networked players
	interfaces.global_vars->frametime = interfaces.global_vars->interval_per_tick;
	interfaces.global_vars->curtime = player->GetSimulationTime();

	// notify the other hooks to instruct animations and pvs fix
	csgo->EnableBones = player->GetClientSideAnims() = true;
	player->UpdateClientSideAnimation();
	csgo->EnableBones = player->GetClientSideAnims() = false;

	player->InvalidatePhysicsRecursive(8);

	// restore globals
	interfaces.global_vars->curtime = backup_curtime;
	interfaces.global_vars->frametime = backup_frametime;
}

void BuildBones(IBasePlayer* player, float sim_time, matrix* mat) {
	auto accessor = player->GetBoneAccessor();
	if (!accessor)
		return;

	// keep track of old occlusion values
	const auto backup_occlusion_flags = player->GetOcclusionFlags();
	const auto backup_occlusion_framecount = player->GetOcclusionFramecount();

	// skip occlusion checks in c_cs_player::setup_bones
	player->GetOcclusionFlags() = 0;
	player->GetOcclusionFramecount() = 0;

	// clear bone accessor
	accessor->m_ReadableBones = accessor->m_WritableBones = 0;

	// invalidate bone cache
	player->InvalidateBoneCache();

	// stop interpolation
	player->GetEffects() |= 0x8;

	// change bone accessor
	const auto backup_bone_array = accessor->get_bone_array_for_write();
	accessor->set_bone_array_for_write(mat);

	// build bones
	player->SetupBones(nullptr, -1, 0x7FF00, interfaces.global_vars->curtime);

	// restore bone accessor
	accessor->set_bone_array_for_write(backup_bone_array);

	// restore original occlusion
	player->GetOcclusionFlags() = backup_occlusion_flags;
	player->GetOcclusionFramecount() = backup_occlusion_framecount;

	// start interpolation again
	player->GetEffects() &= ~0x8;
}

float CAnimationFix::GetBackwardYaw(IBasePlayer* player) {
	return Math::CalculateAngle(csgo->local->GetOrigin(), player->GetOrigin()).y;
}



void CAnimationFix::Resolve(IBasePlayer* player)
{
	if (player->GetTeam() == csgo->local->GetTeam() || !player->isAlive() || !player || player->IsDormant())
		return;


	if (player->GetPlayerInfo().fakeplayer)
		return;

	auto Layers = player->GetAnimOverlays();
	auto AnimState = player->GetPlayerAnimState();
#pragma region LambdaFunction

	static auto GetMaxDesyncDelta = [](IBasePlayer* ent) -> float {
		uintptr_t animstate = uintptr_t(ent->GetPlayerAnimState());


		float Duck = *(float*)(animstate + 0xA4);
		float speedfraction = std::fmax(0, std::fmin(*reinterpret_cast<float*>(animstate + 0xF8), 1));

		float speedfactor = std::fmax(0, std::fmin(1, *reinterpret_cast<float*> (animstate + 0xFC)));

		float U1 = ((*reinterpret_cast<float*> (animstate + 0x11C) * -0.30000001) - 0.19999999) * speedfraction;
		float U2 = U1 + 1.1f;


		if (Duck > 0) {

			U2 += ((Duck * speedfactor) * (0.5 - U2));

		}
		else
			U2 += ((Duck * speedfactor) * (0.5 - 0.58));

		return( *(float*)(animstate + 0x334) * U2 );
	};

	static auto GetDesyncDelta = [](IBasePlayer* player) -> float {
		float ret = 0.f;
		auto animstate = player->GetPlayerAnimState();
		auto fr = max(0.f, min(animstate->m_speed_as_portion_of_walk_top_speed, 1.f));
		auto fa = max(0.f, min(animstate->m_speed_as_portion_of_crouch_top_speed, 1.f));

		ret = ((animstate->m_walk_run_transition * -0.30000001f) - 0.19999999f) * fr + 1.f;

		if (animstate->m_anim_duck_amount > 0.0f)
			ret += ((animstate->m_anim_duck_amount * fa) * (0.5f - ret));

		ret *= animstate->m_aim_yaw_max;
		return ret;
	};
	static auto GetFixedDesyncDelta = [](IBasePlayer* ent, float recur) -> float {
		float del = GetMaxDesyncDelta(ent);
		del *= (recur / 59.f);
		return del;
	};

	static auto StaticSideDetection = [](IBasePlayer* ent, float Relative) -> int {
		Vector src3D, dst3D, forward, right, up, src, dst;
		float back_two, right_two, left_two;
		trace_t tr;
		Ray_t ray, ray2, ray3, ray4, ray5;
		CTraceFilter filter;

		Math::AngleVectors(Vector(0, Relative, 0), &forward, &right, &up);

		filter.pSkip = ent;
		src3D = ent->GetEyePosition();
		dst3D = src3D + (forward * 384);

		ray.Init(src3D, dst3D);
		interfaces.trace->TraceRay(ray, MASK_SHOT, &filter, &tr);
		back_two = (tr.endpos - tr.startpos).Length();

		ray2.Init(src3D + right * 35, dst3D + right * 35);
		interfaces.trace->TraceRay(ray2, MASK_SHOT, &filter, &tr);
		right_two = (tr.endpos - tr.startpos).Length();

		ray3.Init(src3D - right * 35, dst3D - right * 35);
		interfaces.trace->TraceRay(ray3, MASK_SHOT, &filter, &tr);
		left_two = (tr.endpos - tr.startpos).Length();

		if (left_two > right_two) {
			return -1;
		}
		else if (right_two > left_two) {
			return 1;
		}
		else
			return -1;
	};



	static auto UpdateSide = [](IBasePlayer* ent, float Relative, CAnimationLayer* layers) -> int {
		if (ent->GetVelocity().Length2D() > 0.f) {
			if (layers[6].m_flPlaybackRate <= 0.03f) {
				return 1;
			}
			else {
				return -1;
			}
		}
		else {
			return StaticSideDetection(ent, Relative); //layer 6 cant be used unlelss moving :<
		}
	};
	static auto UpdateDesyncType = [](IBasePlayer* player, CAnimationLayer* layers) -> DesyncType {
		/*if (vars.ragebot.resolver == 1) {
			return SMART;
		}
		else if (vars.ragebot.resolver == 2) {
			return BUILDSERVERABSYAW;
		}*/
		if (vars.ragebot.resolver != 0)
		{
			switch (vars.ragebot.resolver)
			{
			case 0:
				break;
			case 1:
				return ANIMSTATE;
				break;
			case 2:
				return SMART;
				break;
			case 3:
				return BUILDSERVERABSYAW;
			}
		}
		if (!(player->GetFlags() & FL_ONGROUND))
		{
			return INAIR;
		
		}
		if (layers[3].m_flWeight == 0.0f && layers[3].m_flCycle == 0.0f) {
			resolverRecord[player->EntIndex()].BreakLBY = true;
			return LBYREVERSE;
			
		}


		if (player->GetVelocity().Length2D() >= 5.f && player->GetVelocity().Length2D() < 95.f)
		{
			resolverRecord[player->EntIndex()].BreakLBY = false;
			return LBYREVERSE;
		}

		if (player->GetVelocity().Length2D() >= 95.f && player->GetVelocity().Length2D() < INT_MAX)
		{
			resolverRecord[player->EntIndex()].BreakLBY = false;
			return MOVING;
			 
		}

		if (player->GetVelocity().Length2D() < 5.f) {
			resolverRecord[player->EntIndex()].BreakLBY = true;
			return LBY;
			 
		}
		return NONE;
	};

	static auto GetSmoothedVelocity = [](float min_delta, Vector a, Vector b) -> Vector {
		Vector delta = a - b;
		float Length = delta.Length();

		if (Length <= min_delta)
		{
			Vector result;

			if (-min_delta <= Length)
				return a;
			else
			{
				float FLRadius = 1.0f / (Length + FLT_EPSILON);
				return b - ((delta * FLRadius) * min_delta);
			}
		}
		else
		{
			float FLRadius = 1.0f / (Length + FLT_EPSILON);
			return b + ((delta * FLRadius) * min_delta);
		}
	};

	static auto AngleMod = [](float a) -> float
	{
		return (360.f / 65536) * ((int)(a * (65536.f / 360.0f)) & 65535);
	};
	static auto ApproachAngle = [](float target, float value, float CorrectedSpeed) -> float
	{
		target = AngleMod(target);
		value = AngleMod(value);

		float delta = target - value;

		// Speed is assumed to be positive
		if (CorrectedSpeed < 0)
			CorrectedSpeed = -CorrectedSpeed;

		if (delta < -180)
			delta += 360;
		else if (delta > 180)
			delta -= 360;

		if (delta > CorrectedSpeed)
			value += CorrectedSpeed;
		else if (delta < -CorrectedSpeed)
			value -= CorrectedSpeed;
		else
			value = target;

		return value;
	};

	static auto ResolveFromAnimstate = [](IBasePlayer* m_player, float MaxRotationLBY) -> float
	{
		//setup important vars
		Vector VelocityVec = m_player->GetVelocity();
		auto AnimState = m_player->GetPlayerAnimState();
		float EyeYaw = MaxRotationLBY;
		float ReturnFinalGoalFeetYaw = 0.f;

		//get imoprtant delta
		float PDelta = Math::AngleDiff(EyeYaw, ReturnFinalGoalFeetYaw);

	

	
		//setup velocity
		if (VelocityVec.LengthSqr() > std::powf(1.2f * 260.0f, 2.f))
		{
			Vector NormalizedVelocity = VelocityVec.Normalized();
			VelocityVec = NormalizedVelocity * (1.2f * 260.0f);
		}

		float ChokedTime = AnimState->m_last_update_increment;
		float DuckAmmount = std::clamp(m_player->GetDuckAmount() + AnimState->m_duck_additional, 0.0f, 1.0f);
		float AnimDuck = AnimState->m_anim_duck_amount;
		float ChokedTime6 = ChokedTime * 6.0f;
		float PDuckAmmount;


		if ((DuckAmmount - AnimDuck) <= ChokedTime6) {
			if (-ChokedTime6 <= (DuckAmmount - AnimDuck))
				PDuckAmmount = DuckAmmount;
			else
				PDuckAmmount = AnimDuck - ChokedTime6;
		}
		else {
			PDuckAmmount = AnimDuck + ChokedTime6;
		}

		float FinalDuckAmmount = std::clamp(PDuckAmmount, 0.0f, 1.0f);
		Vector PAnimVel = GetSmoothedVelocity(ChokedTime * 2000.0f, VelocityVec, m_player->GetVelocity());
		float CorrectedSpeed = std::fminf(PAnimVel.Length(), 260.0f);

		float MaxMovementSpeed = 260.0f;

		//fix desync based on weapon speed and cur speed
		IBaseCombatWeapon	* Weap = m_player->GetWeapon();

		if (Weap && Weap->GetCSWpnData())
			MaxMovementSpeed = std::fmaxf(Weap->GetCSWpnData()->m_flMaxSpeedAlt, 0.001f);

		float Run = CorrectedSpeed / (MaxMovementSpeed * 0.520f);
		float Duck = CorrectedSpeed / (MaxMovementSpeed * 0.340f);

		Run = std::clamp(Run, 0.0f, 1.0f);
		//add ground fraction to improve resolve on running targets
		float FixedYawD = (((AnimState->m_walk_run_transition * -0.30000001) - 0.19999999) * Run) + 1.0f;
		//fix duck and fix desync on duck
		if (FinalDuckAmmount > 0.0f)
		{
			float Duck = std::clamp(Duck, 0.0f, 1.0f);
			FixedYawD += (FinalDuckAmmount * Duck) * (0.5f - FixedYawD);
		}

		const float MaxDesyncL = -58.f;
		const float MaxDesyncR = 58.f;

		float LMYaw = MaxDesyncL * FixedYawD;
		float LRYaw = MaxDesyncR * FixedYawD;
		//setup max desync
		if (PDelta <= LRYaw)
		{
			if (LMYaw > PDelta)
				ReturnFinalGoalFeetYaw = fabs(LMYaw) + EyeYaw;
		}
		else
		{
			ReturnFinalGoalFeetYaw = EyeYaw - fabs(LRYaw);
		}

		ReturnFinalGoalFeetYaw = Math::NormalizeYaw(ReturnFinalGoalFeetYaw);
		//do some valve shit here
		if (CorrectedSpeed > 0.1f || fabs(VelocityVec.z) > 100.0f)
		{
			ReturnFinalGoalFeetYaw = ApproachAngle(
				EyeYaw,
				ReturnFinalGoalFeetYaw,
				((AnimState->m_walk_run_transition * 20.0f) + 30.0f)
				* ChokedTime);
		}
		else
		{
			ReturnFinalGoalFeetYaw = ApproachAngle(
				m_player->GetLBY(),
				ReturnFinalGoalFeetYaw,
				ChokedTime * 100.0f);
		}

		return ReturnFinalGoalFeetYaw;
	};

	static auto HasShot = [](float SimTime, float ShotTime)
	{
		return ShotTime == SimTime;
	};

#pragma endregion

	auto i = player->EntIndex();
	resolverInfo[i].CurrentMiss = csgo->actual_misses[i];
	resolverInfo[i].Relative = GetBackwardYaw(player);
	resolverInfo[i].Side = UpdateSide(player, resolverInfo[i].Relative,Layers);
	resolverInfo[i].DesyncType = UpdateDesyncType(player, Layers);
	resolverInfo[i].FixedLowerBodyYaw = Math::NormalizeYaw(remainderf(AnimState->m_abs_yaw, 360.f));

	if (HasShot(player->GetSimulationTime(), player->GetWeapon()->LastShotTime()))
		return; // ez onshot fix $$$

	float EyeYaw = remainderf(AnimState->m_eye_yaw, 360.f);
	float Desync = 0.f;
	switch (resolverInfo[i].DesyncType) {
	case MOVING:
		if (player->GetVelocity().Length2D() >= 155.f) { //why add velo check when velo fixed in setup velo also in build server goalfeetyaw :think:
	//		resolverInfo[i].ResolvedLowerBodyAngle = EyeYaw + 25.8f * resolverInfo[i].Side;

			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 25.8f);
		}
		else {
			//		resolverInfo[i].ResolvedLowerBodyAngle = EyeYaw + 36.f * resolverInfo[i].Side;
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 36.f);
		}
		break;
	case LBYREVERSE:
	
		Desync = GetDesyncDelta(player);
		if (!Desync) {
			Desync = GetMaxDesyncDelta(player);
		}
		//	resolverInfo[i].ResolvedLowerBodyAngle = EyeYaw + Desync * resolverInfo[i].Side;
		resolverInfo[i].DesyncDelta = Desync;
		break;
	case LBY:
		Desync = GetDesyncDelta(player);
		if (!Desync) {
			Desync = GetMaxDesyncDelta(player);
		}
		//	resolverInfo[i].ResolvedLowerBodyAngle = EyeYaw + Desync * resolverInfo[i].Side;
		resolverInfo[i].DesyncDelta = Desync;
		break;
	case INAIR:
		if (player->GetVelocity().Length2D() >= 30.f)
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 15.f);
		else
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 5.f);
		break;
	case SLOWWALK:
		if (player->GetVelocity().Length2D() >= 49.f) {
			//	resolverInfo[i].ResolvedLowerBodyAngle = EyeYaw + 28.f * resolverInfo[i].Side;
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 28.f);
		}
		else {
			//	resolverInfo[i].ResolvedLowerBodyAngle = EyeYaw + 45.f * resolverInfo[i].Side;
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 43.f);
		}
		break;
	case LOWDELTABRUTEFORCE: //this is important later
		switch (resolverInfo[i].CurrentMiss % 4)
		{
		case 0:
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 10.f);
			break;
		case 1:
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 20.f);
			break;
		case 2:
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 15.f);
			break;
		case 3:
			resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 30.f);
			break;
		}
		break;
	case SMART:
		if (!ResolveState[i].LastHit) {
			if (!(player->GetFlags() & FL_ONGROUND))
			{
				resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 10.f);
			}
			if (player->GetVelocity().Length2D() >= 40.f)
			{
				resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 38.f);
			}
			if (resolverRecord[i].BreakLBY)
			{
				resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 56.5f);
			}
			else
				resolverInfo[i].DesyncDelta = GetFixedDesyncDelta(player, 48.f);
		}
		else
		{
			resolverInfo[i].DesyncDelta = ResolveState[i].LastDelta;
		}
		break;
	case ANIMSTATE:
		GetFixedDesyncDelta(player, ResolveFromAnimstate(player, GetMaxDesyncDelta(player)));
		break;
	}
	if (g_Binds[bind_force_safepoint].active) {
	
		Desync = GetMaxDesyncDelta(player);
		Desync *= 0.5f;
		resolverInfo[i].DesyncDelta = Desync;
	}
	resolverInfo[i].ResolvedLowerBodyAngle = EyeYaw + resolverInfo[i].DesyncDelta * resolverInfo[i].Side;
	resolverInfo[i].ResolvedLowerBodyAngle -= (resolverInfo[i].DesyncDelta - (((resolverInfo[i].CurrentMiss % 5) / 4) * resolverInfo[i].DesyncDelta)) * resolverInfo[i].Side;
	resolverInfo[i].ResolvedLowerBodyAngle = Math::NormalizeYaw(resolverInfo[i].ResolvedLowerBodyAngle);

	if (resolverInfo[i].DesyncType == BUILDSERVERABSYAW) {
		if(player->GetVelocity().Length2D() <= 6.f)
			resolverInfo[i].ResolvedLowerBodyAngle = ResolveFromAnimstate(player, EyeYaw + AnimState->m_aim_yaw_max);
		else
			resolverInfo[i].ResolvedLowerBodyAngle = ResolveFromAnimstate(player, EyeYaw + AnimState->m_aim_yaw_max * resolverInfo[i].Side); //note to rxvan u can remove side detection here cuz u might miss wrong sided due to it !!
		
	
			
	
		
	}
}



void animation::build_inversed_bones(IBasePlayer* player) {
	auto idx = player->GetIndex();
	static float spawntime[65] = { 0.f };
	static CBaseHandle* selfhandle[65] = { nullptr };

	if (spawntime[idx] == 0.f)
		spawntime[idx] = player->GetSpawnTime();


	auto alloc = g_Animfix->IS_Animstate[idx] == nullptr;
	auto change = !alloc && selfhandle[idx] != &player->GetRefEHandle();
	auto reset = !alloc && !change && player->GetSpawnTime() != spawntime[idx];

	if (change) {
		memset(&g_Animfix->IS_Animstate[idx], 0, sizeof(g_Animfix->IS_Animstate[idx]));
		selfhandle[idx] = (CBaseHandle*)&player->GetRefEHandle();
	}
	if (reset) {
		player->ResetAnimationState(g_Animfix->IS_Animstate[idx]);
		spawntime[idx] = player->GetSpawnTime();
	}

	if (alloc || change) {
		g_Animfix->IS_Animstate[idx] = reinterpret_cast<CCSGOPlayerAnimState*>(interfaces.memalloc->Alloc(sizeof(CCSGOPlayerAnimState)));
		if (g_Animfix->IS_Animstate[idx])
			player->CreateAnimationState(g_Animfix->IS_Animstate[idx]);
	}

	if (!g_Animfix->IS_Animstate[idx])
		return;

	if (player->GetFlags() & FL_ONGROUND) {
		g_Animfix->IS_Animstate[idx]->m_on_ground = true;
		g_Animfix->IS_Animstate[idx]->m_landing = false;
	}
	g_Animfix->IS_Animstate[idx]->m_duration_in_air = 0.f;

	g_Animfix->Resolve(player);

	g_Animfix->IS_Animstate[idx]->m_abs_yaw = resolverInfo[idx].ResolvedLowerBodyAngle;

	//const float angle = g_Resolver->GetAngle(player);
	//const float delta = player->GetDSYDelta();
	//const float low_delta = delta * 0.5f;

	//float desync_angle = delta;

	//bool resolver_disabled = ResolverMode[idx] == str("Disabled") || ResolverMode[idx].find('d') != -1 || g_Resolver->ResolverInfo[idx].Index == 0;

	//if (ResolverMode[idx].find('l') != -1) // low delta
	//	desync_angle = low_delta; // half of max desync delta

	//if (!resolver_disabled)
	//	g_Animfix->IS_Animstate[idx]->m_abs_yaw = Math::NormalizeYaw(Math::NormalizeYaw(angle + (desync_angle * -g_Resolver->ResolverInfo[idx].Index)));

	player->UpdateAnimationState(g_Animfix->IS_Animstate[idx], player->GetEyeAngles());
	BuildBones(player, sim_time, inversed_bones);
}

void animation::build_unresolved_bones(IBasePlayer* player)
{
	auto idx = player->GetIndex();

	static float spawntime[65] = { 0.f };
	static CBaseHandle* selfhandle[65] = { nullptr };

	if (spawntime[idx] == 0.f)
		spawntime[idx] = player->GetSpawnTime();


	auto alloc = g_Animfix->US_Animstate[idx] == nullptr;
	auto change = !alloc && selfhandle[idx] != &player->GetRefEHandle();
	auto reset = !alloc && !change && player->GetSpawnTime() != spawntime[idx];

	if (change) {
		memset(&g_Animfix->US_Animstate[idx], 0, sizeof(g_Animfix->US_Animstate[idx]));
		selfhandle[idx] = (CBaseHandle*)&player->GetRefEHandle();
	}
	if (reset) {
		player->ResetAnimationState(g_Animfix->US_Animstate[idx]);
		spawntime[idx] = player->GetSpawnTime();
	}

	if (alloc || change) {
		g_Animfix->US_Animstate[idx] = reinterpret_cast<CCSGOPlayerAnimState*>(interfaces.memalloc->Alloc(sizeof(CCSGOPlayerAnimState)));
		if (g_Animfix->US_Animstate[idx])
			player->CreateAnimationState(g_Animfix->US_Animstate[idx]);
	}

	if (!g_Animfix->US_Animstate[idx])
		return;

	if (player->GetFlags() & FL_ONGROUND) {
		g_Animfix->US_Animstate[idx]->m_on_ground = true;
		g_Animfix->US_Animstate[idx]->m_landing = false;
	}
	g_Animfix->US_Animstate[idx]->m_duration_in_air = 0.f;

	const float delta = player->GetDSYDelta();

	 g_Animfix->Resolve(player);
	g_Animfix->US_Animstate[idx]->m_abs_yaw = resolverInfo[idx].ResolvedLowerBodyAngle;

	//const float low_delta = delta * 0.5f;

	//float desync_angle = delta;

	//bool resolver_disabled = ResolverMode[idx] == str("Disabled") || ResolverMode[idx].find('d') != -1 || g_Resolver->ResolverInfo[idx].Index == 0;

	//if (ResolverMode[idx].find('l') != -1) // low delta
	//	desync_angle = low_delta; // half of max desync delta

	//if (resolver_disabled)
	//	g_Animfix->US_Animstate[idx]->m_abs_yaw = Math::NormalizeYaw(player->GetEyeAngles().y + desync_angle); // setup inversed side
	//else
	//	g_Animfix->US_Animstate[idx]->m_abs_yaw = Math::NormalizeYaw(player->GetEyeAngles().y);

	player->UpdateAnimationState(g_Animfix->US_Animstate[idx], player->GetEyeAngles());
	BuildBones(player, sim_time, unresolved_bones);
}

void animation::build_server_bones(IBasePlayer* player)
{
	BuildBones(player, sim_time, bones);
}

void CAnimationFix::animation_info::UpdateAnims(animation* record, animation* from)
{
	auto animstate = player->GetPlayerAnimState();
	int idx = player->GetIndex();

	if (record->came_from_dormant <= 2)
		record->came_from_dormant++;

	if (!from)
	{
		record->velocity = player->GetVelocity();
		record->didshot = false;
		record->safepoints = false;
		record->apply(player);

		animstate->m_abs_yaw = resolverInfo[idx].ResolvedLowerBodyAngle;

		return UpdatePlayer(player);
	}
	else
	{
		int ticks_to_simulate = TIME_TO_TICKS(record->sim_time - from->sim_time);

		if (ticks_to_simulate > 31)
			ticks_to_simulate = 1;

		// did the player shoot?
		const float& came_from_dormant_time = record->player->CameFromDormantTime();

		if (!from->didshot)
			record->didshot = record->last_shot_time > from->sim_time && record->last_shot_time <= record->sim_time
			&& record->last_shot_time != came_from_dormant_time && from->last_shot_time != came_from_dormant_time;

		record->resolver_mode = resolverInfo[player->GetIndex()].DesyncType;

		const auto velocity_per_tick = (record->velocity - from->velocity) / ticks_to_simulate;
		const auto duck_amount_per_tick = (record->duck - from->duck) / ticks_to_simulate;

		if (ticks_to_simulate <= 1)
		{
			// set VelocityVec and layers.
			record->velocity = player->GetVelocity();

			// apply record.
			record->apply(player);

			animstate->m_abs_yaw = resolverInfo[idx].ResolvedLowerBodyAngle;

			// run update.
			return UpdatePlayer(player);
		}
		else {

			float land_time = 0.0f;
			bool is_landed = false;
			bool land_in_cycle = false;

			// check if landed in choke cycle
			if (record->layers[4].m_flCycle < 0.5f && (!(record->flags & FL_ONGROUND) || !(from->flags & FL_ONGROUND)))
			{
				land_time = record->sim_time - (record->layers[4].m_flPlaybackRate * record->layers[4].m_flCycle);
				land_in_cycle = land_time >= from->sim_time;
			}

			bool on_ground = record->flags & FL_ONGROUND;

			for (auto i = 1; i <= ticks_to_simulate; i++)
			{
				const auto simulated_time = from->sim_time + TICKS_TO_TIME(i);

				player->GetVelocity() = (velocity_per_tick * i) + from->velocity;
				player->GetDuckAmount() = (duck_amount_per_tick * i) + from->duck;

				const auto ct = interfaces.global_vars->curtime;
				interfaces.global_vars->curtime = simulated_time;

				if (land_in_cycle && !is_landed)
				{
					if (land_time <= simulated_time)
					{
						is_landed = true;
						on_ground = true;
					}
					else
						on_ground = from->flags & FL_ONGROUND;
				}

				if (on_ground)
					player->GetFlagsPtr() |= FL_ONGROUND;
				else
					player->GetFlagsPtr() &= ~FL_ONGROUND;

				// backup simtime.
				const auto backup_simtime = player->GetSimulationTime();

				// set new simtime.
				player->GetSimulationTime() = simulated_time;

				animstate->m_abs_yaw = resolverInfo[idx].ResolvedLowerBodyAngle;

				// run update.
				UpdatePlayer(player);

				// restore old simtime.
				player->GetSimulationTime() = backup_simtime;

				interfaces.global_vars->curtime = ct;
			}
		}
	}
}

void CAnimationFix::FixPvs() {
	for (int i = 1; i <= interfaces.global_vars->maxClients; i++) {
		auto pCurEntity = interfaces.ent_list->GetClientEntity(i);
		if (!pCurEntity
			|| !pCurEntity->IsPlayer()
			|| pCurEntity->EntIndex() == interfaces.engine->GetLocalPlayer())
			continue;

		*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pCurEntity) + 0xA30) = interfaces.global_vars->framecount;
		*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pCurEntity) + 0xA28) = 0;
	}
}