#include "../Features.h"
#include "../../render.h"
#include "../../includes\imgui/imgui.h"
#include "../../includes\imgui/imgui_internal.h"
#include "../../includes\imgui/imgui_impl_dx9.h"
#include "../../GUI/gui.h"

bool GetBox(IBasePlayer* entity, int& x, int& y, int& w, int& h, Vector origin)
{
	if (entity->IsPlayer()) {
		Vector top, down, s[2];

		Vector adjust = Vector(0, 0, -16) * entity->GetDuckAmount();

		down = origin - Vector(0, 0, 1);
		top = down + Vector(0, 0, 72) + adjust;

		if (Math::WorldToScreen2(top, s[1]) && Math::WorldToScreen2(down, s[0]))
		{
			Vector delta = s[1] - s[0];

			h = fabsf(delta.y);
			w = h / 2.0f;

			x = s[1].x - (w / 2);
			y = s[1].y;

			return true;
		}
	}
	else
	{
		Vector top, down, s[2];

		down = entity->GetAbsOrigin();
		top = down + Vector(0, 0, 2.25f);

		if (Math::WorldToScreen2(top, s[1]) && Math::WorldToScreen2(down, s[0]))
		{
			Vector delta = s[1] - s[0];

			h = fabsf(delta.y);
			w = h * 2;

			x = s[1].x - (w / 2);
			y = s[1].y;

			return true;
		}

		return false;
	}
	return false;
}

void CVisuals::DrawAngleLine(Vector origin, float angle, color_t color)
{
	Vector src, dst, sc1, sc2, forward;

	src = origin;
	Math::AngleVectors(Vector(0, angle, 0), &forward);
	if (Math::WorldToScreen(src, sc1) && Math::WorldToScreen(src + (forward * 40), sc2))
	{
		//Drawing::DrawLine(sc1.x, sc1.y, sc2.x, sc2.y, color);
		g_Render->DrawLine(sc1.x, sc1.y, sc2.x, sc2.y, color, 1.f);
	}
}

static void drawProjectileTrajectory(const Trail& config, const std::vector<std::pair<float, Vector>>& trajectory) noexcept
{
	
}

void RenderEntityBox(const ProjectileInfo_t& projectileData, const char* name) {
	Vector origin2d;
	if (Math::WorldToScreen(projectileData.origin, origin2d)) {
		if (!strcmp(name, str("Smoke"))) {
			if (projectileData.time_to_die + 0.5f <= 0.f) {
				g_Render->DrawString(origin2d.x, origin2d.y, color_t(255, 255, 255, 255),
				 render::centered_x | render::centered_y, fonts::esp_icons_big, str("k"));
			}
			else {

			

				g_Render->DrawString(origin2d.x, origin2d.y, color_t(255, 255, 255, 255),
					render::centered_x | render::centered_y, fonts::esp_icons_big, str("k"), time);
			}
		}
		else if (!strcmp(name, str("Molotov"))) {
			

			g_Render->DrawString(origin2d.x, origin2d.y, color_t(255, 255, 255, 255),
				render::centered_x | render::centered_y, fonts::esp_icons_big, str("l"));
		}
		else if (!strcmp(name, str("HE Grenade"))) {
			

			g_Render->DrawString(origin2d.x, origin2d.y, color_t(255, 255,255, 255),
				render::centered_x | render::centered_y, fonts::esp_icons_big, str("j"));
		}
		else if (!strcmp(name, str("Flashbang"))) {
	

			g_Render->DrawString(origin2d.x, origin2d.y, color_t(255, 255, 255, 255),
				render::centered_x | render::centered_y, fonts::esp_icons_big, str("i"));
		}

	}
}

void renderProjectileEsp(const ProjectileInfo_t& projectileData, const char* name)
{
	const auto& config = Trail{
		.color = [&]() -> color_t {
			if (projectileData.thrownByLocalPlayer)
				return vars.visuals.world.projectiles.colors[2];
			else if (projectileData.thrownByEnemy)
				return vars.visuals.world.projectiles.colors[1];
			else
				return vars.visuals.world.projectiles.colors[0];
		}(),
		.type = Trail::Type::Line,
		.time = 1.5f,
	};

	if ([projectileData]() {
		if (projectileData.thrownByLocalPlayer && vars.visuals.world.projectiles.filter & 4)
			return true;

			if (projectileData.thrownByEnemy && vars.visuals.world.projectiles.filter & 2)
				return true;

			if ((!projectileData.thrownByEnemy && !projectileData.thrownByLocalPlayer) &&
				vars.visuals.world.projectiles.filter & 1)
				return true;

			return false;
		}()) {

		if (vars.visuals.world.projectiles.trajectories)
			drawProjectileTrajectory(config, projectileData.trajectory);

		if (!projectileData.exploded)
			RenderEntityBox(projectileData, name);
	}
}

void CVisuals::DrawWatermark() {

}

class c_dynamic_list {
private:
	const float animtime = 0.2f;
	struct c_iterator {
		float time;
		bool in;
		std::string name;
		std::string type;
	};
	std::map<uint32_t, c_iterator> elements;
public:
	int last_size = 0;
	void add(std::string name, std::string type, int idx = -1) {
		elements[idx] = c_iterator{ csgo->get_absolute_time(), true, name, type };
	}
	void remove(std::string name, std::string type, int idx = -1) {
		elements[idx] = c_iterator{ csgo->get_absolute_time(), false, name, type };
	}
	void render(const Vector2D& pos) {
		int cursor = 0;
		auto easeOutQuad = [](float x) {
			return 1 - (1 - x) * (1 - x);
		};

		for (auto& el : elements) {
			float time_difference = csgo->get_absolute_time() - el.second.time;
			float current_animation = std::clamp(time_difference / animtime, 0.f, 1.f);

			if (el.second.in)
				current_animation = 1.f - current_animation;

			if (current_animation <= 0.f)
				continue;

			std::string t;

	


			g_Render->DrawString(pos.x + 3.f, pos.y + 38.f + cursor,
				style.get_color(c_style::text_color).manage_alpha(255.f * easeOutQuad(current_animation)),
				render::centered_y, fonts::esp_logs,
				str("%s"), (el.second.name).c_str());


			g_Render->DrawString(pos.x + 9.f + 109.5f, pos.y + 38.f + cursor,
				style.get_color(c_style::text_color).manage_alpha(255.f * easeOutQuad(current_animation)),
				render::centered_y, fonts::esp_logs,
				str("%s"), (el.second.type).c_str());

			cursor += 16 * easeOutQuad(current_animation);
		}
		last_size = cursor;
	}
};

c_dynamic_list bind_list;

std::string GetTypeofBind(int type) {
	switch (type)
	{
	case 0: return str("[ off ]");
	case 1: return str("[ hold ]");
	case 2: return str("[ toggle ]");
	case 3: return str("[ release ]");
	default: return str("[ on ]");
	}
}

std::string GetBindName(int id) {
	switch (id)
	{
	case bind_override_dmg:    return str("Override damage");
	case bind_force_safepoint: return str("Force safepoints");
	case bind_baim:            return str("Body aim");
	case bind_double_tap:      return str("Double-tap");
	case bind_hide_shots:      return str("Hide-shots");
	case bind_aa_inverter:     return str("Inverter");
	case bind_manual_left:     return str("Manual Left");
	case bind_manual_right:    return str("Manual Right");
	case bind_manual_back:     return str("Manual Back");
	case bind_manual_forward:  return str("Manual Forward");
	case bind_fake_duck:       return str("Fake Duck");
	case bind_slow_walk:       return str("Slow Walk");
	case bind_peek_assist:     return str("Peek assist");
	default: return str("?");
	}
}

void DrawIndicators() {
	if (!vars.visuals.indicators)
		return;

	Vector2D size = Vector2D(200.f, 25.f);
	static Vector2D position = Vector2D(240.f, 300.f);

	static std::vector<c_bind> old_binds;
	if (old_binds.empty()) {
		old_binds.resize(bind_max);
		memcpy(&*old_binds.begin(), g_Binds, sizeof(g_Binds));
		for (int i = 0; i < bind_max; ++i) {
			if (i == bind_third_person)
				continue;

			if (old_binds[i].active)
				bind_list.add(std::string(GetBindName(i)), GetTypeofBind(g_Binds[i].type), i);
		}
	}

	for (int i = 0; i < bind_max; ++i) {
		if (i == bind_third_person)
			continue;

		if (!old_binds[i].active && g_Binds[i].active)
			bind_list.remove(std::string(GetBindName(i)), GetTypeofBind(g_Binds[i].type), i);
		else if (old_binds[i].active && !g_Binds[i].active)
			bind_list.add(std::string(GetBindName(i)), GetTypeofBind(g_Binds[i].type), i);
	}
	if (bind_list.last_size > 0) {
		g_Render->FilledRect(position.x, position.y, + size.x - 30.f, size.y, color_t(7, 2, 31, 255));

		g_Render->DrawString(position.x + 14, position.y + (size.y / 2.f), style.get_color(c_style::text_color), render::centered_y, fonts::keybindsBig, str("KEYBINDS"));
	}
	bind_list.render(position);

	memcpy(&*old_binds.begin(), g_Binds, sizeof(g_Binds));
}

void CVisuals::DrawLocalVisuals()
{
	static auto CLLagComp = interfaces.cvars->FindVar("cl_lagcompensation");

	static bool LastConnected = false;

	if (LastConnected != (interfaces.engine->IsConnected() && interfaces.engine->IsInGame())) {
		LastConnected = (interfaces.engine->IsConnected() && interfaces.engine->IsInGame());
		memset(csgo->maxmisses, 0, sizeof(csgo->maxmisses));
	}
	
	if (vars.visuals.watermark)
		DrawWatermark();

	if (!csgo->is_connected)
		return;


	if (vars.ragebot.AntiDefensive) {
		if (csgo->weapon) {
		
			CLLagComp->SetValue(0);
			csgo->DisableLagComp = true;
			
		}
		else {
			CLLagComp->SetValue(1);
			csgo->DisableLagComp = false;
		}

	}
	else {
		CLLagComp->SetValue(1);
		csgo->DisableLagComp = false;
	}

	DrawIndicators();
	//g_GrenadePrediction->Paint();
	if (csgo->local->isAlive() && csgo->local) {
		if (csgo->send_packet)
			csgo->LegSwitch = !csgo->LegSwitch;
		
		if (!csgo->LocalGrenadePrediction.Valid) {
			
	
			auto flags_backup = g_Render->_drawList->Flags;
			g_Render->_drawList->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines;
			Vector Screen;
			Vector Last;
			//std::vector<ImVec2> pts;
			if (!csgo->LocalGrenadePrediction.Finished) {
				csgo->LocalGrenadePrediction.Finished = true;
				trace_t tr;
				Ray_t ray;
				ray.Init(csgo->LocalGrenadePrediction.EndPos + Vector(0, 0, 50), csgo->LocalGrenadePrediction.EndPos - Vector(0, 0, 1000));
				static CTraceFilterWorldAndPropsOnly t;
				interfaces.trace->TraceRay(ray, MASK_SOLID, &t, &tr);
				csgo->LocalGrenadePrediction.EndPos = tr.endpos;
				for (auto& Cur : csgo->LocalGrenadePrediction.Path) {

					if (!Last.IsZero()) {

						static std::string model_name = str("sprites/purplelaser1.vmt");


						BeamInfo_t beamInfo;
						beamInfo.m_nType = TE_BEAMPOINTS;
						beamInfo.m_pszModelName = model_name.c_str();
						beamInfo.m_nModelIndex = -1;
						beamInfo.m_flHaloScale = 0.0f;
						beamInfo.m_flLife = 3.f; //duration of tracers
						beamInfo.m_flWidth = 14; //start width
						beamInfo.m_flEndWidth = 14; //end width
						beamInfo.m_flFadeLength = 0.0f;
						beamInfo.m_flAmplitude = 2.0f;
						beamInfo.m_flBrightness =255.f;
						beamInfo.m_flSpeed = 0.0000000007f;
						beamInfo.m_nStartFrame = 0;
						beamInfo.m_flFrameRate = 0.f;
						beamInfo.m_flRed = vars.visuals.nadepred_color.get_red();
						beamInfo.m_flGreen = vars.visuals.nadepred_color.get_green();
						beamInfo.m_flBlue = vars.visuals.nadepred_color.get_blue();
						beamInfo.m_nSegments = 2;
						beamInfo.m_bRenderable = true;
						beamInfo.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
						beamInfo.m_vecStart = Last;
						beamInfo.m_vecEnd = Cur;

						Beam_t* beam = interfaces.beams->CreateBeamPoints(beamInfo);
						if (beam)
							interfaces.beams->DrawBeam(beam);


					}
					Last = Cur;

				}
			}
			Vector origin2d;
			if (interfaces.global_vars->realtime < csgo->LocalGrenadePrediction.ThrowTime + 3.0f) {
				for (auto& bounce : csgo->LocalGrenadePrediction.Bounce) {
					if (Math::WorldToScreen(bounce, origin2d)) {
						g_Render->GradientCircle(origin2d, 10.f, color_t(vars.visuals.nadepred_color[2], vars.visuals.nadepred_color[1],
							vars.visuals.nadepred_color[0], 0), color_t(vars.visuals.nadepred_color[2], vars.visuals.nadepred_color[1],
								vars.visuals.nadepred_color[0], vars.visuals.nadepred_color[3]));
					}
				}

				if (Math::WorldToScreen(csgo->LocalGrenadePrediction.EndPos, origin2d)) {
					g_Render->GradientCircle(origin2d, 17.f, color_t(vars.visuals.nadepred_color[2], vars.visuals.nadepred_color[1],
						vars.visuals.nadepred_color[0], 0), color_t(vars.visuals.nadepred_color[2], vars.visuals.nadepred_color[1],
							vars.visuals.nadepred_color[0], vars.visuals.nadepred_color[3]));
					//g_Render->CircleFilled(Screen.x, Screen.y, 40, color_t(255, 255, 255, 255), 40);
				}
			}
		
			g_Render->_drawList->Flags = flags_backup;
		}
			if (csgo->LocalGrenadePrediction.Valid) {
				auto flags_backup = g_Render->_drawList->Flags;
				g_Render->_drawList->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines;
				Vector Screen;
				std::vector<ImVec2> pts;
				for (auto& Cur : csgo->LocalGrenadePrediction.Path) {
					if (Math::WorldToScreen(Cur, Screen)) {
						pts.push_back(ImVec2(Screen.x, Screen.y));
					}
				}
				if (!pts.empty())
					g_Render->_drawList->AddPolyline(pts.data(), pts.size(), vars.visuals.nadepred_color.u32(), false, 3.f);

				for (auto& bounce : csgo->LocalGrenadePrediction.Bounce) {
					if (Math::WorldToScreen(bounce, Screen)) {
						g_Render->GradientCircle(Screen, 7.f, color_t(170,170,255, 0), color_t(170,170,255, 255));
					}
				}

				if (Math::WorldToScreen(csgo->LocalGrenadePrediction.EndPos, Screen)) {
					g_Render->GradientCircle(Screen, 15.f, color_t(vars.visuals.nadepred_color[2], vars.visuals.nadepred_color[1],
						vars.visuals.nadepred_color[0], 0), color_t(vars.visuals.nadepred_color[2], vars.visuals.nadepred_color[1],
							vars.visuals.nadepred_color[0], vars.visuals.nadepred_color[3]));
					//g_Render->CircleFilled(Screen.x, Screen.y, 40, color_t(255, 255, 255, 255), 40);
				}
				g_Render->_drawList->Flags = flags_backup;
			}
		
	}
	if ((vars.visuals.remove & 8) && csgo->is_local_alive && csgo->scoped) {
		g_Render->DrawLine(csgo->w / 2, 0, csgo->w / 2, csgo->h, color_t(0, 0, 0, 255));
		g_Render->DrawLine(0, csgo->h / 2, csgo->w, csgo->h / 2, color_t(0, 0, 0, 255));
	}

	auto flags_backup = g_Render->_drawList->Flags;
	g_Render->_drawList->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedLines;
	{
		g_AutoPeek->Draw();
	}

	/*if (csgo->is_local_alive && !PredictedOrigin.IsZero()) {
		Vector scrOrigin, scrPredOrigin;
		if (Math::WorldToScreen(csgo->eyepos, scrOrigin)
			&& Math::WorldToScreen(PredictedOrigin, scrPredOrigin)) {
			g_Render->DrawLine(scrOrigin.x, scrOrigin.y,
				scrPredOrigin.x, scrPredOrigin.y, color_t(235, 52, 52, 255),
				2.f);
			g_Render->Circle(scrPredOrigin.x, scrPredOrigin.y, 6.f, color_t(0, 0, 0, 255), 100);
			g_Render->CircleFilled(scrPredOrigin.x, scrPredOrigin.y, 5.f, color_t(255, 255, 255, 255), 100);
		}
	}*/

	g_Render->_drawList->Flags = flags_backup;
}

std::string str_toupper(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return toupper(c); }
	);
	return s;
}

void CVisuals::ResetInfo()
{
	for (auto i = 0; i < interfaces.ent_list->GetHighestEntityIndex(); i++)
	{
		IBasePlayer* entity = interfaces.ent_list->GetClientEntity(i);
		if (i < 64)
			player_info[i].Reset();

	}
}

void renderWeaponBox(const WeaponData_t& weaponData) noexcept
{
	Vector screen;
	if (!Math::WorldToScreen(weaponData.origin, screen))
		return;

	g_Render->DrawString(screen.x, screen.y, vars.visuals.world.weapons.color,
	 render::centered_x | render::centered_y, fonts::esp_info, weaponData.name.c_str());
}

void renderInferno(const InfernoInfo_t& info) {
	Vector origin2d;
	if (!Math::WorldToScreen(info.entity_origin + Vector(0,0,70.f), origin2d))
		return;

	static auto world_circle = [](Vector location, float radius) {
		static constexpr float Step = PI * 2.0f / 60;
		std::vector<ImVec2> points;
		for (float lat = 0.f; lat <= PI * 2.0f; lat += Step)
		{
			const auto& point3d = Vector(sin(lat), cos(lat), 0.f) * radius;
			Vector point2d;
			if (Math::WorldToScreen(location + point3d, point2d))
				points.push_back(ImVec2(point2d.x, point2d.y));
		}
		g_Render->_drawList->AddPolyline(points.data(), points.size(), color_t(170, 170, 255, 180).u32(), true, 1.f);
	};




	static auto polyobject = [](vector<Vector> Points,int Count,float Rad, Vector cen) {
		Vector Temp;
		Vector w2s;
		std::vector<ImVec2> points;


		float Step = PI * 2.0f / Count;
		float temp;
		for (float lat = 0.f; lat <= PI * 2.0f; lat += Step)
		{
			temp = INT_MAX;
			const auto& TB = Vector(sin(lat), cos(lat), 0.f) * Rad;
			for (auto& Point : Points) {
				auto l = (Point - (cen + TB)).LengthSqr();
				if (l < temp) {
					temp = l;
					Temp = Point;
				}
			}
			if (Math::WorldToScreen(Temp, w2s))
				points.push_back(ImVec2(w2s.x, w2s.y));

		}

		//g_Render->_drawList->AddConvexPolyFilled(points.data(), points.size(), color_t(170, 170, 255, 40).u32());
		g_Render->_drawList->AddPolyline(points.data(), points.size(), color_t(170, 170, 255, 150).u32(), true, 2.f);
	};


	auto PPoint = info.PPoints;
	//g_Render->Render3DCircle(info.origin, info.range, color_t(255, 170, 170, 0), color_t(255, 170, 170, 169));
	polyobject(PPoint, info.PPoints.size(), info.range, info.origin);

	//g_Render->Render3DCircle(info.origin, info.range, color_t(255, 170, 170, 0), color_t(255, 170, 170, 99));
	/*
	int ct = 0;
	for (auto& PT : info.PPoints) {
		world_circle(PT,info.PRads[ct]);
		ct++;
	}*/

	

	g_Render->GradientCircle(Vector2D(origin2d.x, origin2d.y), 39, color_t(255,170,170, 0), color_t(255,170,170, 255));
	g_Render->CircleFilled(origin2d.x, origin2d.y, 26 - 7, color_t(13, 13, 13, 255), 35);
	g_Render->PArc(origin2d.x, origin2d.y, 26 - 5, 1.5f * PI, ((1.5f * PI) - ((1.5f * PI + 1.57) * (info.time_to_die / 7.03125f))), 4.f, color_t(170,170,255, 255));
	g_Render->DrawString(origin2d.x + 1, origin2d.y, color_t(170,170,255,255), render::centered_x | render::centered_y, fonts::esp_icons_big, str("l"));
}

void renderBomb(const BombInfo_t& info) {
	if (!csgo->is_local_alive)
		return;

	if (!vars.visuals.world.weapons.planted_bomb)
		return;

	constexpr int radius = 40;
	const float offset = 155;

	static float pulse = 0.f;
	static bool b_switch = false;

	if (b_switch) {
		if (pulse >= 0.f)
			pulse += animation_speed / 4.f;
		if (pulse >= 1.f)
			b_switch = false;
	}
	else {
		if (pulse <= 1.f)
			pulse -= animation_speed / 4.f;
		if (pulse <= 0.f)
			b_switch = true;
	}
	pulse = clamp(pulse, 0.f, 1.f);

	

	Vector origin;
	if (!Math::WorldToScreen(info.origin, origin))
		return;

	auto bomb_str = [&]() ->std::string {
		if (info.bomb_defused)
			return str("defused");
		else if (info.is_defusing)
			return str("defusing");
		else
			return str("planted");
	};

	auto bomb_clr = [&]() {
		if (info.bomb_defused)
			return color_t(0, 255, 0, 255);
		else if (info.is_defusing)
			return color_t(84, 227, 255, 155 + (100 * pulse));
		else
			return color_t(255, 0, 0, 255);
	};

	g_Render->DrawString(origin.x, origin.y, bomb_clr(),
	render::centered_x | render::centered_y, fonts::esp_logs,
		bomb_str().c_str());
}

void CVisuals::Draw()
{
	csgo->mtx.lock();
	if (!csgo->is_connected) {
		csgo->mtx.unlock();
		return;
	}
	auto flags_backup = g_Render->_drawList->Flags;
	g_Render->_drawList->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines;

	if (vars.visuals.world.projectiles.enable) {
		for (const auto& projectile : ProjectileInfo) {
			if (!projectile.name.empty())
				renderProjectileEsp(projectile, projectile.name.c_str());
		}
	}

	if (vars.visuals.world.weapons.enabled) {
		for (const auto& weapon : WeaponData)
			renderWeaponBox(weapon);
	}

	for (const auto& c4_info : BombInfo)
		renderBomb(c4_info);

	for (const auto& info : InfernoInfo)
		renderInferno(info);

	g_Render->_drawList->Flags = flags_backup;

	static float oof_pulsating = 0.f;
	static bool b_switch = false;

	if (b_switch) {
		if (oof_pulsating >= 0.f)
			oof_pulsating += animation_speed / 4.f;
		if (oof_pulsating >= 1.f)
			b_switch = false;
	}
	else {
		if (oof_pulsating <= 1.f)
			oof_pulsating -= animation_speed / 4.f;
		if (oof_pulsating <= 0.f)
			b_switch = true;
	}
	oof_pulsating = clamp(oof_pulsating, 0.f, 1.f);

	auto easeOutQuad = [](float x) {
		return 1 - (1 - x) * (1 - x);
	};

	if (vars.visuals.enable)
	{
		for (int i = 0; i < 64; ++i)
		{
			auto& info = player_info[i];
			if (!info.player)
				continue;
			int alpha = vars.visuals.dormant ? info.alpha : 255;
			alpha = std::clamp(alpha, 0, 255);

			if (csgo->is_local_alive && info.offscreen && vars.visuals.out_of_fov && info.is_valid)
			{
				float width = 10.f;
				Vector viewangles;
				interfaces.engine->GetViewAngles(viewangles);

				const auto& rot = DEG2RAD(viewangles.y - Math::CalculateAngle(csgo->origin, info.origin).y - 90.f);
				auto radius = 50 + vars.visuals.out_of_fov_distance;
				auto size = vars.visuals.out_of_fov_size;
				auto center = ImVec2(csgo->w / 2.f, csgo->h / 2.f);

				auto pos = ImVec2(center.x + radius * cosf(rot) * (2 * (0.5f + 10 * 0.5f * 0.01f)), center.y + radius * sinf(rot));
				auto line = pos - center;

				auto arrowBase = pos - (line * (size / (2 * (tanf(PI / 4) / 2) * line.length())));
				auto normal = ImVec2(-line.y, line.x);
				auto left = arrowBase + normal * (size / (2 * line.length()));
				auto right = arrowBase + normal * (-size / (2 * line.length()));

				auto clr = info.dormant ? color_t(200, 200, 200, alpha)
					: vars.visuals.out_of_fov_color;
				auto flags_backup = g_Render->_drawList->Flags;
				g_Render->_drawList->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedLines;
				g_Render->_drawList->AddTriangleFilled(left, right, pos,
					clr.manage_alpha((vars.visuals.out_of_fov_color.get_alpha() * 0.55f) + (70.f * oof_pulsating)).u32());
				g_Render->_drawList->Flags = flags_backup;
			}

			if (alpha <= 0
				|| info.offscreen
				|| !info.is_valid)
				continue;


			if (vars.visuals.zeus_warning) {
				if (info.zeuser_stages != none) {
					color_t warning_clr = [&]() {
						switch (info.zeuser_stages) {
						case good:
							return color_t(0, 255, 0, 155 + (100 * oof_pulsating));
							break;
						case warning:
							return color_t(255, 208, 55, 155 + (100 * oof_pulsating));
							break;
						case fatal:
							return color_t(255, 0, 0, 155 + (100 * oof_pulsating));
							break;
						}
					}();

					int add = vars.visuals.name ? 47 : 35;

					auto flags_backup = g_Render->_drawList->Flags;
					g_Render->_drawList->Flags |= ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines;

					constexpr int radius = 26;
					g_Render->CircleFilled(info.box.x + info.box.w / 2, info.box.y - add, radius, color_t(40, 40, 40, 155 + (100 * oof_pulsating)), 50);
					g_Render->CircleFilled(info.box.x + info.box.w / 2, info.box.y - add, radius - 6, color_t(25, 25, 25, 255), 50);

					g_Render->_drawList->PathArcTo(ImVec2(info.box.x + info.box.w / 2, info.box.y - add), radius - 3, DEG2RAD(0.f), DEG2RAD(360.f), 32);
					g_Render->_drawList->PathStroke(warning_clr.u32(), false, 4.f);

					g_Render->_drawList->Flags = flags_backup;

					g_Render->DrawString(info.box.x + info.box.w / 2, info.box.y - add, color_t(
						255,
						info.zeuser_stages == fatal ? 0 : 255,
						info.zeuser_stages == fatal ? 0 : 255,
						255 * oof_pulsating),
						render::centered_x | render::centered_y,
						fonts::esp_icons_big, str("h"));
				}
			}

			{
				int box_alpha = info.dormant && vars.visuals.dormant ? alpha : vars.visuals.box_color[3];
				auto clr = info.dormant ? color_t(255, 255, 255, box_alpha * 0.9f) : color_t(
					vars.visuals.box_color[0],
					vars.visuals.box_color[1],
					vars.visuals.box_color[2],
					box_alpha * 0.49f);

				if (!info.dormant) {
					if (vars.visuals.box && info.esp_offsets[3] < 1.f)
						info.esp_offsets[3] += animation_speed * 1.5f;
					else if (!vars.visuals.box && info.esp_offsets[3] > 0.f)
						info.esp_offsets[3] -= animation_speed * 1.5f;
					info.esp_offsets[3] = clamp(info.esp_offsets[3], 0.f, 1.f);
				}
				else {
					info.esp_offsets[3] = vars.visuals.box ? 1.f : 0.f;
				}

				float box_x = info.box.x;
				float box_y = info.box.y;

				float box_w = info.box.w;
				float box_h = info.box.h;

				float multiplier = (1.f - easeOutQuad(info.esp_offsets[3]));
				float multiplier_b = 10.f;

				g_Render->Rect(
					box_x + (box_w * (0.2f * multiplier)),
					box_y + (box_h * (0.2f * multiplier)),
					box_w - ((box_w * 0.5f) * multiplier),
					box_h - ((box_h * 0.5f) * multiplier),
					clr.manage_alpha(clr.get_alpha() * easeOutQuad(info.esp_offsets[3])), 1.f);

				g_Render->Rect(
					box_x + (box_w * (0.2f * multiplier)) - 1,
					box_y + (box_h * (0.2f * multiplier)) - 1,
					box_w - ((box_w * 0.5f) * multiplier) + 2,
					box_h - ((box_h * 0.5f) * multiplier) + 2,
					color_t(10, 10, 10, clr.get_alpha() * easeOutQuad(info.esp_offsets[3])), 1.f);

				g_Render->Rect(
					box_x + (box_w * (0.2f * multiplier)) + 1,
					box_y + (box_h * (0.2f * multiplier)) + 1,
					box_w - ((box_w * 0.5f) * multiplier) - 2,
					box_h - ((box_h * 0.5f) * multiplier) - 2,
					color_t(10, 10, 10, clr.get_alpha() * easeOutQuad(info.esp_offsets[3])), 1.f);
			}

			{
				if (!info.dormant) {
					if (vars.visuals.name && info.esp_offsets[4] < 1.f)
						info.esp_offsets[4] += animation_speed * 1.5f;
					else if (!vars.visuals.name && info.esp_offsets[4] > 0.f)
						info.esp_offsets[4] -= animation_speed * 1.5f;
					info.esp_offsets[4] = clamp(info.esp_offsets[4], 0.f, 1.f);
				}
				else {
					info.esp_offsets[4] = vars.visuals.name ? 1.f : 0.f;
				}

				ImGui::PushFont(fonts::esp_name);
				auto text_size = ImGui::CalcTextSize(info.name.c_str());

				int r = vars.visuals.name_color[0],
					g = vars.visuals.name_color[1],
					b = vars.visuals.name_color[2],
					a = vars.visuals.name_color[3] * (alpha / 255.f);

				auto clr = info.dormant ? color_t(255, 255, 255, alpha) : color_t(r, g, b, a * easeOutQuad(info.esp_offsets[4]));

				g_Render->DrawString(info.box.x + info.box.w / 2 - text_size.x / 2, info.box.y - 15, clr,
					render::none, fonts::esp_name, info.name.c_str());
				ImGui::PopFont();
			}

			if (!info.dormant) {
				auto clr = color_t(
					vars.visuals.skeleton_color[0],
					vars.visuals.skeleton_color[1],
					vars.visuals.skeleton_color[2],
					vars.visuals.skeleton_color[3]);

				if (vars.visuals.shot_multipoint && csgo->is_local_alive) {

					//if (info.AnimInfo.points.size() > 0) {
					//	Vector p1 = info.AnimInfo.points[0];
					//	Vector origin2d = info.AnimInfo.points[1];

					//	int add = 5;

					//	Vector w1, w2;
					//	if (Math::WorldToScreen(p1, w1)
					//		&& Math::WorldToScreen(origin2d, w2)) {
					//		g_Render->FilledRect(w1.x - 4 + add, w1.y - 4 + add, 6 + add, 6 + add, color_t(0, 0, 0, 255));
					//		g_Render->FilledRect(w1.x - 3 + add, w1.y - 3 + add, 4 + add, 4 + add, color_t(255, 0, 0, 255));

					//		g_Render->FilledRect(w2.x - 4 + add, w2.y - 4, 6 + add, 6 +add, color_t(0, 0, 0, 255));
					//		g_Render->FilledRect(w2.x - 3 + add, w2.y - 3, 4 + add, 4 +add, color_t(0, 255, 0, 255));
					//	}
					//}

					for (const auto& p : info.AnimInfo.points) {
						Vector world;
						if (Math::WorldToScreen(p, world)) {
							g_Render->CircleFilled(world.x, world.y, 3.f, color_t(0, 0, 0, alpha), 100);
							g_Render->CircleFilled(world.x, world.y, 2.f, color_t(255, 255, 255, alpha), 100);
						}
					}
				}

				if (info.hdr && vars.visuals.skeleton) {
					for (int i = 0; i < 128; i++) {
						Vector sParent{}, sChild{};

						if (Math::WorldToScreen(info.bone_pos_parent[i], sParent) && Math::WorldToScreen(info.bone_pos_child[i], sChild))
							g_Render->DrawLine(sParent[0], sParent[1], sChild[0], sChild[1], clr, 2.f);
					}
				}
			}

			{
				if (!info.dormant) {
					if (vars.visuals.healthbar && info.esp_offsets[2] < 1.f)
						info.esp_offsets[2] += animation_speed * 1.5f;
					else if (!vars.visuals.healthbar && info.esp_offsets[2] > 0.f)
						info.esp_offsets[2] -= animation_speed * 1.5f;
					info.esp_offsets[2] = clamp(info.esp_offsets[2], 0.f, 1.f);
				}
				else {
					info.esp_offsets[2] = vars.visuals.healthbar ? 1.f : 0.f;
				}

				if (info.esp_offsets[2] > 0.f) {
					// to-do: ������� ������������� �����, ����� �� �����������
					// � �� ������� �� ����
					int health = clamp(info.hp, 0, 100);
					int health_height = info.box.h - (info.box.h * health) / 100;

					// ������� � �� �������
					// �������� �� ��� ������� ����� = �� ������ ������
					float health_multiplier = 12.f / 360.f;
					health_multiplier *= std::ceil(health / 10.f) - 1;
					color_t health_clr =
						info.dormant ?
						color_t(200, 200, 200, alpha)
						: vars.visuals.override_hp ? color_t(
							vars.visuals.hp_color[0],
							vars.visuals.hp_color[1],
							vars.visuals.hp_color[2],
							vars.visuals.hp_color[3] * (alpha / 255.f))
						: color_t::hsb(health_multiplier, 1, 1);

					float cur_h = (info.box.h + 2) * easeOutQuad(info.esp_offsets[2]);

					g_Render->FilledRect(info.box.x - 6, info.box.y - 1, 4,
						cur_h,
						color_t(50, 50, 50, (alpha * 0.49f) * easeOutQuad(info.esp_offsets[2])), 1.5f);

					g_Render->FilledRect(info.box.x - 5,
						info.box.y + health_height,
						2,
						(info.box.h - health_height) * easeOutQuad(info.esp_offsets[2]),
						health_clr.manage_alpha(health_clr.get_alpha() * easeOutQuad(info.esp_offsets[2])), 1.5f);

					if (health < 100) {
						std::string health_str = std::to_string(health);

						ImGui::PushFont(fonts::esp_info);
						const auto& health_width = ImGui::CalcTextSize(health_str.c_str());
						ImGui::PopFont();

						g_Render->DrawString(info.box.x - health_width.x + 1, info.box.y + health_height - 2,
							color_t(255, 255, 255, alpha * easeOutQuad(info.esp_offsets[2])), render::centered_y, fonts::esp_info,
							health_str.c_str());
					}
				}
			}

			if (!info.dormant) {

				bool valid_ammo_bar = vars.visuals.ammo && !info.misc_weapon;

				if (valid_ammo_bar && info.esp_offsets[0] < 1.f)
					info.esp_offsets[0] += animation_speed * 1.5f;
				else if (!valid_ammo_bar && info.esp_offsets[0] > 0.f)
					info.esp_offsets[0] -= animation_speed * 1.5f;
				info.esp_offsets[0] = clamp(info.esp_offsets[0], 0.f, 1.f);

				float animated_mod = easeOutQuad(info.esp_offsets[0]);
				if (info.esp_offsets[0] > 0.f) {

					auto ammo = info.ammo;
					auto max_ammo = info.max_ammo;
					{
						auto clr = color_t(
							vars.visuals.ammo_color[0],
							vars.visuals.ammo_color[1],
							vars.visuals.ammo_color[2],
							vars.visuals.ammo_color[3] * ((alpha / 255.f) * info.esp_offsets[0]));

						int hp_percent = info.box.w - (int)((info.box.w * ammo) / 100);

						int width = (info.box.w * (ammo / float(max_ammo)));

						char ammostr[10];
						sprintf_s(ammostr, str("%d"), ammo);

						ImGui::PushFont(fonts::esp_info);

						const auto text_size = ImGui::CalcTextSize(ammostr);

						g_Render->FilledRect(info.box.x - 1, info.box.y + 2 + info.box.h, (info.box.w + 2) * animated_mod, 4, color_t(50, 50, 50, (alpha * 0.49f) * info.esp_offsets[0]));
						g_Render->FilledRect(info.box.x, info.box.y + 3 + info.box.h, (width)*animated_mod, 2, clr);

						if (ammo < max_ammo)
							g_Render->DrawString((info.box.x + width - text_size.x) + 1, info.box.y + info.box.h + 3,
								color_t(255, 255, 255, alpha * animated_mod),  render::centered_y, fonts::esp_info,
								ammostr);

						ImGui::PopFont();
					}
				}

				{
					if (!info.dormant) {
						if (vars.visuals.weapon && info.esp_offsets[1] < 1.f)
							info.esp_offsets[1] += animation_speed * 1.5f;
						else if (!vars.visuals.weapon && info.esp_offsets[1] > 0.f)
							info.esp_offsets[1] -= animation_speed * 1.5f;
						info.esp_offsets[1] = clamp(info.esp_offsets[1], 0.f, 1.f);
					}
					else
						info.esp_offsets[1] = vars.visuals.weapon ? 1.f : 0.f;

					if (info.esp_offsets[1] > 0.f) {
						ImGui::PushFont(fonts::esp_info);
						std::string t = info.weapon_name;
						g_Render->DrawString(info.box.x + info.box.w / 2, info.box.y + 1 + info.box.h + (6.f * animated_mod),
							vars.visuals.weapon_color.manage_alpha(vars.visuals.weapon_color.get_alpha() * easeOutQuad(info.esp_offsets[1])),
							render::centered_x, fonts::esp_name, t.c_str());

						ImGui::PopFont();
					}
				}

			}

			if (info.weapon)
			{
				auto clr = info.dormant ? color_t(200, 200, 200, alpha)
					: vars.visuals.flags_color;

				auto& flags_info = FlagsInfo[i];
				flags_info.clear();

				bool have_armor = (vars.visuals.flags & 1) && info.have_armor;
				bool is_scoped = (vars.visuals.flags & 2) && info.scope;
				bool is_flashed = (vars.visuals.flags & 4) && info.flash;
				bool have_kit = (vars.visuals.flags & 8) && info.have_kit;
				bool fake_ducking = (vars.visuals.flags & 16) && info.fake_duck;
				bool draw_distance = (vars.visuals.flags & 32) && info.player_distance > 0.f;
				bool draw_last_place = (vars.visuals.flags & 64);
				bool draw_resolver = (vars.visuals.flags * 128) && resolverInfo[info.player->EntIndex()].DesyncType != NONE;

				std::string dist_to_target = std::to_string(info.player_distance) + str("u");

				flags_info.emplace_back(Flags_t(info.have_helmet ? str("HK") : str("K"),
					have_armor, clr, true));

				flags_info.emplace_back(Flags_t(str("Kit"),
					have_kit, clr, true));

				flags_info.emplace_back(Flags_t(str("Flashed"),
					is_flashed, clr, true));

				flags_info.emplace_back(Flags_t(str("Scoped"),
					is_scoped, clr));

				flags_info.emplace_back(Flags_t(str("Fake duck"),
					fake_ducking, clr));

				flags_info.emplace_back(Flags_t(dist_to_target,
					draw_distance, clr));

				flags_info.emplace_back(Flags_t(info.last_place,
					draw_last_place, clr));

				float step = 0.f;

				for (int j = 0; j < flags_info.size(); j++) {

					auto& current_flags = flags_info[j];
					if (current_flags.toggled && info.modifier[j] < 1.f)
						info.modifier[j] += animation_speed * 1.5f;
					else if (!current_flags.toggled && info.modifier[j] > 0.f)
						info.modifier[j] -= animation_speed * 1.5f;
					info.modifier[j] = clamp(info.modifier[j], 0.f, 1.f);

					float cur_alpha = clamp(
						current_flags.clr.get_alpha() * easeOutQuad(info.modifier[j]),
						0.f, 255.f);

					float add = current_flags.icon ? 19.f : 10.f;

					float cur_step = clamp(
						add * easeOutQuad(info.modifier[j]),
						0.f, add);

					if (cur_alpha > 0.f) {
						g_Render->DrawString(info.box.x + info.box.w + 3,
							(info.box.y - 2) + step, current_flags.clr.manage_alpha(cur_alpha), render::none, 
							fonts::esp_info, current_flags.name.c_str());

						step += cur_step;
					}
				}
			}
		}
	}

	csgo->mtx.unlock();
}

void CVisuals::StoreOtherInfo()
{
	csgo->mtx.lock();

	WeaponData.clear();
	InfernoInfo.clear();
	BombInfo.clear();

	if (!csgo->is_connected) {
		ProjectileInfo.clear();
		csgo->mtx.unlock();
		return;
	}

	for (auto i = 0; i < interfaces.ent_list->GetHighestEntityIndex(); i++)
	{
		IBasePlayer* entity = interfaces.ent_list->GetClientEntity(i);

		if (i < 64) {

		}
		else {
			if (!entity || entity->IsPlayer()
				|| entity->IsDormant()) {
				continue;
			}
			if (entity->IsWeapon()
				|| entity->GetClientClass()->m_ClassID == g_ClassID->CC4) {
				if (((IBaseCombatWeapon*)entity)->Owner() == -1)
					WeaponData.emplace_back(entity);
			}
			else {
				auto class_id = entity->GetClientClass()->m_ClassID;
				if (class_id == g_ClassID->CPlantedC4) {
					BombInfo_t info;
					info.origin = entity->GetAbsOrigin();
					info.time = entity->GetTimerLength();
					info.blow = entity->GetC4Blow();
					info.blow_time = entity->GetC4Blow() - interfaces.global_vars->curtime;
					info.defuse_cooldown = entity->GetDefuseCooldown();
					info.is_defusing = entity->GetDefuser() != nullptr;
					info.bomb_defused = entity->IsBombDefused();
					info.defuse_time = entity->GetDefuseCooldown() - interfaces.global_vars->curtime;
					info.bomb_ticking = entity->GetBombTicking();
					BombInfo.emplace_back(info);
				}
				else {
					if (class_id == g_ClassID->CInferno) {
						auto& inferno = InfernoInfo.emplace_back();
						const auto& inferno_origin = entity->GetOrigin();
						inferno.time_to_die = (((*(float*)(uintptr_t(entity) + 0x20)) + 7.03125f) - interfaces.global_vars->curtime);

						bool* m_bFireIsBurning = entity->m_bFireIsBurning(); //0xE94
						int* m_fireXDelta = entity->m_fireXDelta(); //0x9E4
						int* m_fireYDelta = entity->m_fireYDelta(); //0xB74
						int* m_fireZDelta = entity->m_fireZDelta(); //0xD04
						int m_fireCount = entity->m_fireCount();  //0x13A8
						inferno.entity_origin = inferno_origin;
						inferno.PPoints.push_back(inferno_origin);
						inferno.range = 0.f;
						Vector average_vector = Vector(0, 0, 0);
						for (int i = 0; i <= m_fireCount; i++) {
	
							Vector fire_origin = Vector(m_fireXDelta[i], m_fireYDelta[i], m_fireZDelta[i]);
					
							float delta = fire_origin.LengthSqr();
							if (delta > inferno.range)
								inferno.range = delta;

							average_vector += fire_origin;
							if (fire_origin == Vector(0, 0, 0))
								continue;

							Vector fire_origin2 = Vector(m_fireXDelta[i], m_fireYDelta[i], m_fireZDelta[i]);
							Vector POrigin = inferno_origin + (fire_origin2 * 1.3f);
					
							inferno.PPoints.push_back(POrigin);
						}

						inferno.range = sqrtf(inferno.range) + 14.5f;

						if (m_fireCount <= 1)
							inferno.origin = inferno_origin;
						else
							inferno.origin = (average_vector / m_fireCount) + inferno_origin;

					
					}
					else {
						switch (class_id) {
						case (int)Z::ClassId::BaseCSGrenadeProjectile:
							if (entity->GrenadeExploded()) {
								if (const auto it = std::find(ProjectileInfo.begin(), ProjectileInfo.end(), entity->GetRefEHandle()); it != ProjectileInfo.end())
									it->exploded = true;
								break;
							}
							[[fallthrough]];
						case (int)Z::ClassId::BreachChargeProjectile:
						case (int)Z::ClassId::BumpMineProjectile:
						case (int)Z::ClassId::DecoyProjectile:
						case (int)Z::ClassId::MolotovProjectile:
						case (int)Z::ClassId::SensorGrenadeProjectile:
						case (int)Z::ClassId::SmokeGrenadeProjectile:
						case (int)Z::ClassId::SnowballProjectile:
							if (const auto it = std::find(ProjectileInfo.begin(), ProjectileInfo.end(), entity->GetRefEHandle()); it != ProjectileInfo.end())
								it->update(entity);
							else
								ProjectileInfo.emplace_back(entity);
							break;
						}
					}
				}
			}
		}
	}

	std::sort(WeaponData.begin(), WeaponData.end());

	for (auto it = ProjectileInfo.begin(); it != ProjectileInfo.end();) {
		if (!interfaces.ent_list->GetClientEntityFromHandle(it->handle)) {
			it->exploded = true;

			if (it->trajectory.size() < 1 || it->trajectory[it->trajectory.size() - 1].first + 60.0f < csgo->get_absolute_time()) {
				it = ProjectileInfo.erase(it);
				continue;
			}
		}
		++it;
	}

	csgo->mtx.unlock();
}

BaseData::BaseData(IBasePlayer* entity) noexcept
{
	distanceToLocal = entity->GetAbsOrigin().DistTo(csgo->local->GetOrigin());

	if (entity->IsPlayer()) {
		const auto collideable = entity->GetCollideable();
		obbMins = collideable->OBBMins();
		obbMaxs = collideable->OBBMaxs();
	}
	else if (const auto model = entity->GetModel()) {
		obbMins = model->mins;
		obbMaxs = model->maxs;
	}

	box.x = 0;
	box.y = 0;
	box.w = 0;
	box.h = 0;
	//coordinateFrame = entity->GetrgflCoordinateFrame();
}

ProjectileInfo_t::ProjectileInfo_t(IBasePlayer* projectile) noexcept : BaseData{ projectile }
{
	name = [](IBasePlayer* projectile) -> std::string {
		switch (projectile->GetClientClass()->m_ClassID) {
		case (int)Z::ClassId::BaseCSGrenadeProjectile:
			if (const auto model = projectile->GetModel(); model && strstr(model->name, str("flashbang")))
				return str("Flashbang");
			else
				return str("HE Grenade");
		case (int)Z::ClassId::BreachChargeProjectile: return str("Breach Charge");
		case (int)Z::ClassId::BumpMineProjectile: return str("Bump Mine");
		case (int)Z::ClassId::DecoyProjectile: return str("Decoy Grenade");
		case (int)Z::ClassId::MolotovProjectile: return str("Molotov");
		case (int)Z::ClassId::SensorGrenadeProjectile: return str("TA Grenade");
		case (int)Z::ClassId::SmokeGrenadeProjectile: return str("Smoke");
		case (int)Z::ClassId::SnowballProjectile: return str("Snowball");
		default: assert(false); return str("unknown");
		}
	}(projectile);

	if (const auto thrower = interfaces.ent_list->GetClientEntityFromHandle(projectile->Thrower()); thrower && csgo->local) {
		if (thrower == csgo->local)
			thrownByLocalPlayer = true;
		else
			thrownByEnemy = thrower->GetTeam() != csgo->local->GetTeam();
	}

	handle = projectile->GetRefEHandle();
}

void ProjectileInfo_t::update(IBasePlayer* projectile) noexcept
{
	static_cast<BaseData&>(*this) = { projectile };
	this->time_to_die = -1.f;
	if (const auto& pos = projectile->GetAbsOrigin(); trajectory.size() < 1 || trajectory[trajectory.size() - 1].second != pos)
		trajectory.emplace_back(csgo->get_absolute_time(), pos);

	if (name == str("Smoke")) {
		this->time_to_die = (TICKS_TO_TIME(projectile->m_nSmokeEffectTickBegin()) + 17.5f) - interfaces.global_vars->curtime;
		this->m_bDidSmokeEffect = projectile->m_bDidSmokeEffect();
	}

	this->origin = projectile->GetAbsOrigin();
	this->entity = projectile;
}

WeaponData_t::WeaponData_t(IBasePlayer* entity) noexcept : BaseData{ entity }
{
	auto weapon = (IBaseCombatWeapon*)entity;
	clip = weapon->GetAmmo(false);
	reserveAmmo = weapon->GetAmmo(true);

	if (const auto weaponInfo = weapon->GetCSWpnData()) {
		group = [](int type, int weaponId) -> std::string {
			switch (type) {
			case (int)Z::WeaponType::SubMachinegun: return str("SMGs");
			case (int)Z::WeaponType::Pistol: return str("Pistols");
			case (int)Z::WeaponType::Rifle: return str("Rifles");
			case (int)Z::WeaponType::SniperRifle: return str("Sniper Rifles");
			case (int)Z::WeaponType::Shotgun: return str("Shotguns");
			case (int)Z::WeaponType::Machinegun: return str("Machineguns");
			case (int)Z::WeaponType::Grenade: return str("Grenades");
			case (int)Z::WeaponType::Melee: return str("Melee");
			default:
				switch (weaponId) {
				case (int)Z::WeaponId::C4:
				case (int)Z::WeaponId::Healthshot:
				case (int)Z::WeaponId::BumpMine:
				case (int)Z::WeaponId::ZoneRepulsor:
				case (int)Z::WeaponId::Shield:
					return str("Other");
				default: return str("All");
				}
			}
		}(weaponInfo->m_iWeaponType, weapon->GetItemDefinitionIndex());
		displayName = name = weapon->GetGunName();
		origin = entity->GetRenderOrigin();
		//displayName = interfaces->localize->findAsUTF8(weaponInfo->name);
	}
}

bool isFakeDucking(IBasePlayer* player) {
	static float storedTick;
	static float crouchedTicks[64];

	if (!player->GetPlayerAnimState())
		return false;

	float m_flDuckAmount = player->GetDuckAmount();
	float m_flDuckSpeed = player->GetDuckSpeed();
	auto m_fFlags = player->GetFlags();
	auto curtime = interfaces.global_vars->curtime;

	if (m_flDuckSpeed != 0.f && m_flDuckAmount != 0.f)
	{
		if (m_flDuckSpeed == 8.f && m_flDuckAmount <= 0.9f && m_flDuckAmount > 0.01f)
		{
			if (storedTick != TIME_TO_TICKS(curtime))
			{
				crouchedTicks[player->EntIndex()] = crouchedTicks[player->EntIndex()] + 1;
				storedTick = TIME_TO_TICKS(curtime);
			}
			return (crouchedTicks[player->EntIndex()] >= 5) && m_fFlags & FL_ONGROUND;
		}
		else
			crouchedTicks[player->EntIndex()] = 0;
	}

	return false;
}

void CVisuals::RecordInfo()
{
	csgo->mtx.lock();

	static bool should_reset = false;
	if (!vars.visuals.enable) {
		if (!should_reset) {
			for (int i = 0; i < 64; i++) {
				player_info[i].Reset();
				g_DormantEsp->m_cSoundPlayers[i].reset();
			}
			g_DormantEsp->m_utlvecSoundBuffer.RemoveAll();
			g_DormantEsp->m_utlCurSoundList.RemoveAll();
			should_reset = true;
		}
		csgo->mtx.unlock();
		return;
	}
	else should_reset = false;

	g_DormantEsp->Start();

	static auto hud_ptr = *(DWORD**)(csgo->Utils.FindPatternIDA(GetModuleHandleA(g_Modules[fnva1(hs::client_dll.s().c_str())]().c_str()),
		str("B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08")) + 1);

	static auto find_hud_element =
		reinterpret_cast<DWORD(__thiscall*)(void*, const char*)>(csgo->Utils.FindPatternIDA(GetModuleHandleA(g_Modules[fnva1(hs::client_dll.s().c_str())]().c_str()),
			str("55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28")));

	if (!hud_ptr || !find_hud_element) {
		csgo->mtx.unlock();
		return;
	}
	auto radar_base = find_hud_element(hud_ptr, str("CCSGO_HudRadar"));
	auto hud_radar = (CCSGO_HudRadar*)(radar_base - 0x14);

	for (auto i = 0; i < interfaces.ent_list->GetHighestEntityIndex(); ++i)
	{
		if (i > 64)
			continue;

		IBasePlayer* entity = interfaces.ent_list->GetClientEntity(i);

		if (!vars.visuals.enable)
			continue;

		INFO_t& info = player_info[i];
		if (csgo->local == nullptr
			|| !entity
			|| !entity->IsPlayer()
			|| entity == csgo->local
			|| entity->GetTeam() == csgo->local->GetTeam()
			|| !entity->isAlive()) {
			info.is_valid = false;
			info.offscreen = false;
			continue;
		}

		info.is_valid = true;
		info.player = entity;

		if (vars.visuals.dormant) {
			info.dormant = false;
			auto backup_flags = entity->GetFlags();
			auto& backup_origin = entity->GetRenderOrigin();

			if (entity->IsDormant()) {
				info.dormant = g_DormantEsp->AdjustSound(entity);
				if (!info.dormant) {
					info.is_valid = false;
					continue;
				}
			}
			else
			{
				health[i] = entity->GetHealth();
				g_DormantEsp->m_cSoundPlayers[i].reset(true, entity->GetAbsOrigin(), entity->GetFlags());
			}

			if (radar_base && hud_radar && entity->IsDormant() && entity->GetTeam() != csgo->local->GetTeam() && entity->TargetSpotted())
				health[i] = hud_radar->radar_info[i].health;

			if (!health[i])
			{
				if (entity->IsDormant())
				{
					entity->GetFlagsPtr() = backup_flags;
					entity->SetAbsOrigin(backup_origin);
					info.is_valid = false;
				}

				continue;
			}

			if (info.dormant)
			{
				if ((csgo->LastSeenTime[entity->GetIndex()] + 3.f) <= csgo->get_absolute_time()
					&& info.alpha > 0)
					info.alpha--;

				if (info.alpha <= 1)
					info.is_valid = false;
			}
			else
			{
				info.alpha = 255;
				csgo->LastSeenTime[entity->GetIndex()] = csgo->get_absolute_time();
			}
			info.origin = entity->GetRenderOrigin();
			info.alpha = std::clamp(info.alpha, 0.f, 255.f);
		}
		else {
			bool dormant = entity->IsDormant();
			info.dormant = dormant;

			if (dormant || !entity->isAlive()) {
				info.is_valid = false;
				csgo->LastSeenTime[entity->GetIndex()] = csgo->get_absolute_time();
				continue;
			}

			info.alpha = 255;
			info.origin = entity->GetRenderOrigin();
		}

		if (entity->DormantWrapped())
			info.is_valid = false;

		auto weapon = entity->GetWeapon();
		if (!weapon) {
			info.is_valid = false;
			continue;
		}

		CBaseCSGrenade* pCSGrenade = (CBaseCSGrenade*)weapon;
		if (!pCSGrenade) {
			info.is_valid = false;
			continue;
		}

		Vector screenPos;
		interfaces.debug_overlay->ScreenPosition(entity->GetAbsOrigin(), screenPos);

		info.offscreen = screenPos.x < 0 || screenPos.y < 0 || screenPos.x > csgo->w || screenPos.y > csgo->w;
		if (info.offscreen) {
			continue;
		}

		if (!GetBox(entity, info.box.x, info.box.y, info.box.w, info.box.h, info.origin))
			continue;

		if (csgo->local && csgo->local->isAlive()) {
			info.player_distance = std::floor(csgo->local->GetAbsOrigin().DistTo(entity->GetAbsOrigin())); // calculate distance between local and target

			if (entity->GetWeapon()->IsZeus()) { // have taser, will turn indicator
				info.zeuser_stages = good;
				if (info.player_distance <= 600.f) {// target can move and hit you
					info.zeuser_stages = warning;
					if (info.player_distance <= 250.f) // target is near you so he can kill you with taser
						info.zeuser_stages = fatal;
				}
			}
			else
				info.zeuser_stages = none; // doesn't have taser, don't need to indicate smth
		}
		else {
			info.zeuser_stages = none; // local is dead so we have no reasons to indicate that enemy have zeus
			info.player_distance = -1; // clear last distance to entity
		}

		info.name = entity->GetName();
		info.weapon = weapon;
		info.hp = entity->GetHealth();
		info.ammo = weapon->GetAmmo(false);
		info.max_ammo = weapon->GetAmmo(true);
		info.misc_weapon = weapon->IsMiscWeapon();
		info.is_gun = weapon->IsGun();
		info.scope = entity->IsScoped();
		info.flash = entity->IsFlashed();
		info.have_kit = entity->HaveDefuser();
		info.have_armor = entity->GetArmor() > 0;
		info.have_helmet = entity->HasHelmet();
		info.weapon_name = weapon->GetGunName();
		info.duck = entity->GetDuckAmount();
		info.last_place = entity->GetLastPlace();
		info.fake_duck = isFakeDucking(entity);

		if (!info.AnimInfo.points.empty())
			info.AnimInfo.points.clear();

		if (!info.AnimInfo.hitboxes.empty())
			info.AnimInfo.hitboxes.clear();

		auto record = g_Animfix->get_latest_animation(entity);
		if (record && record->player) {
			memcpy(info.AnimInfo.bones, record->bones, sizeof(matrix) * 128);

			if (vars.visuals.shot_multipoint) {
				auto hitboxes = g_Ragebot->GetHitboxesToScan(record->player);

				info.AnimInfo.hitboxes.clear();
				info.AnimInfo.hitboxes = hitboxes;

				info.AnimInfo.points.clear();
				for (auto i : hitboxes) {
					auto points = g_Ragebot->GetMultipoints(record->player, i, info.AnimInfo.bones);
					for (auto p : points)
						info.AnimInfo.points.push_back(p.first);
				}
			}
		}

		info.hdr = interfaces.models.model_info->GetStudioModel(entity->GetModel());
		if (!info.hdr)
			continue;

		if (vars.visuals.skeleton && record && record->player) {
			for (int j = 0; j < info.hdr->num_bones; j++)
			{
				mstudiobone_t* pBone = info.hdr->GetBone(j);

				if (pBone && (pBone->flags & 0x100) && (pBone->parent != -1))
				{
					info.bone_pos_child[j] = info.player->GetBonePos(info.AnimInfo.bones, j);
					info.bone_pos_parent[j] = info.player->GetBonePos(info.AnimInfo.bones, pBone->parent);
				}
			}
		}
		else {
			for (int i = 0; i < 128; i++) {
				info.bone_pos_child[i].Zero();
				info.bone_pos_parent[i].Zero();
			}
		}
			
	}

	//if (csgo->local && csgo->is_local_alive) {
	//	PredictedOrigin = csgo->eyepos + csgo->vecUnpredictedVel * TICKS_TO_TIME(15);
	//}

	csgo->mtx.unlock();
}