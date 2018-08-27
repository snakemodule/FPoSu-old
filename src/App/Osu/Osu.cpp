//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

#include "OsuDatabase.h"
#include "Osu.h"

#include "Engine.h"
#include "Environment.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "ConsoleBox.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "NetworkHandler.h"
#include "SoundEngine.h"
#include "Console.h"
#include "ConVar.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "RenderTarget.h"
#include "Shader.h"

#include "Camera.h"
#include "OpenGLImage.h"

#include "CWindowManager.h"
//#include "DebugMonitor.h"

#include "OsuVR.h"
#include "OsuMainMenu.h"
#include "OsuOptionsMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuModSelector.h"
#include "OsuRankingScreen.h"
#include "OsuKeyBindings.h"
#include "OsuUpdateHandler.h"
#include "OsuNotificationOverlay.h"
#include "OsuTooltipOverlay.h"
#include "OsuGameRules.h"
#include "OsuPauseMenu.h"
#include "OsuScore.h"
#include "OsuSkin.h"
#include "OsuHUD.h"
#include "OsuVRTutorial.h"
#include "OsuChangelog.h"
#include "OsuEditor.h"

#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"

#include "OsuHitObject.h"

void DUMMY_OSU_LETTERBOXING(UString oldValue, UString newValue) {;}
void DUMMY_OSU_VOLUME_MUSIC_ARGS(UString oldValue, UString newValue) {;}
void DUMMY_OSU_MODS(void) {;}

// release configuration
bool Osu::autoUpdater = false;
ConVar osu_version("osu_version", 28.96f);
#ifdef MCENGINE_FEATURE_OPENVR
ConVar osu_release_stream("osu_release_stream", "vr");
#else
ConVar osu_release_stream("osu_release_stream", "desktop");
#endif
ConVar osu_debug("osu_debug", false);

ConVar osu_vr("osu_vr", false);
ConVar osu_vr_tutorial("osu_vr_tutorial", true);

ConVar osu_disable_mousebuttons("osu_disable_mousebuttons", false);
ConVar osu_disable_mousewheel("osu_disable_mousewheel", false);
ConVar osu_confine_cursor_windowed("osu_confine_cursor_windowed", false, DUMMY_OSU_LETTERBOXING);
ConVar osu_confine_cursor_fullscreen("osu_confine_cursor_fullscreen", true, DUMMY_OSU_LETTERBOXING);

ConVar osu_skin("osu_skin", "", DUMMY_OSU_VOLUME_MUSIC_ARGS); // set dynamically below in the constructor
ConVar osu_skin_reload("osu_skin_reload", DUMMY_OSU_MODS);

ConVar osu_volume_master("osu_volume_master", 1.0f, DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_volume_music("osu_volume_music", 0.4f, DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_volume_change_interval("osu_volume_change_interval", 0.05f);

ConVar osu_speed_override("osu_speed_override", -1.0f, DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_pitch_override("osu_pitch_override", -1.0f, DUMMY_OSU_VOLUME_MUSIC_ARGS);

ConVar osu_pause_on_focus_loss("osu_pause_on_focus_loss", true);
ConVar osu_quick_retry_delay("osu_quick_retry_delay", 0.27f);

ConVar osu_mods("osu_mods", "", DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_mod_fadingcursor("osu_mod_fadingcursor", false);
ConVar osu_mod_fadingcursor_combo("osu_mod_fadingcursor_combo", 50.0f);
ConVar osu_mod_endless("osu_mod_endless", false);

ConVar osu_letterboxing("osu_letterboxing", true, DUMMY_OSU_LETTERBOXING);
ConVar osu_resolution("osu_resolution", "1280x720", DUMMY_OSU_VOLUME_MUSIC_ARGS);
ConVar osu_resolution_enabled("osu_resolution_enabled", false);

ConVar osu_draw_fps("osu_draw_fps", true);
ConVar osu_hide_cursor_during_gameplay("osu_hide_cursor_during_gameplay", false);

ConVar *Osu::version = &osu_version;
ConVar *Osu::debug = &osu_debug;
Vector2 Osu::g_vInternalResolution;
Vector2 Osu::osuBaseResolution = Vector2(640.0f, 480.0f);

ConVar fposu_mouse_dpi("fposu_mouse_dpi", 1700);
ConVar fposu_mouse_cm_360("fposu_mouse_cm_360", 35.1f);
ConVar fposu_distance("fposu_distance", 0.5f);
ConVar fposu_fov("fposu_fov", 103.0f);
ConVar fposu_cube("fposu_cube", false);
ConVar fposu_curved("fposu_curved", true);

Osu::Osu()
{
	srand(time(NULL));

	// convar refs
	m_osu_folder_ref = convar->getConVarByName("osu_folder");
	m_osu_draw_hud_ref = convar->getConVarByName("osu_draw_hud");
	m_osu_mod_fps_ref = convar->getConVarByName("osu_mod_fps");
	m_osu_mod_minimize_ref = convar->getConVarByName("osu_mod_minimize");
	m_osu_mod_wobble_ref = convar->getConVarByName("osu_mod_wobble");
	m_osu_mod_wobble2_ref = convar->getConVarByName("osu_mod_wobble2");
	m_osu_playfield_rotation = convar->getConVarByName("osu_playfield_rotation");
	m_osu_playfield_stretch_x = convar->getConVarByName("osu_playfield_stretch_x");
	m_osu_playfield_stretch_y = convar->getConVarByName("osu_playfield_stretch_y");

	// engine settings/overrides
	openvr->setDrawCallback( fastdelegate::MakeDelegate(this, &Osu::drawVR) );
	if (openvr->isReady()) // automatically enable VR mode if it was compiled with OpenVR support and is available
		osu_vr.setValue(1.0f);
	env->setWindowTitle("McOsu");
	env->setCursorVisible(false);
	engine->getConsoleBox()->setRequireShiftToActivate(true);
	engine->getSound()->setVolume(osu_volume_master.getFloat());
	engine->getMouse()->addListener(this);
	convar->getConVarByName("name")->setValue("Guest");
	convar->getConVarByName("console_overlay")->setValue(0.0f);
	convar->getConVarByName("vsync")->setValue(0.0f);
	convar->getConVarByName("fps_max")->setValue(420.0f);
	osu_resolution.setValue(UString::format("%ix%i", engine->getScreenWidth(), engine->getScreenHeight()));

	// VR specific settings
	if (isInVRMode())
	{
		osu_skin.setValue("defaultvr");
		convar->getConVarByName("osu_drain_enabled")->setValue(true);
		convar->getConVarByName("osu_draw_hpbar")->setValue(true);
		env->setWindowResizable(true);
	}
	else
	{
		osu_skin.setValue("default");
		env->setWindowResizable(false);
	}

	// generate default osu! appdata user path
	UString userDataPath = env->getUserDataPath();
	if (userDataPath.length() > 1)
	{
		UString defaultOsuFolder = userDataPath;
		defaultOsuFolder.append("\\osu!\\");
		m_osu_folder_ref->setValue(defaultOsuFolder);
	}

	// convar callbacks
	osu_skin.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinChange) );
	osu_skin_reload.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinReload) );

	osu_volume_master.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMasterVolumeChange) );
	osu_volume_music.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMusicVolumeChange) );
	osu_speed_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSpeedChange) );
	osu_pitch_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onPitchChange) );

	m_osu_playfield_rotation->setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );
	m_osu_playfield_stretch_x->setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );
	m_osu_playfield_stretch_y->setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );

	osu_mods.setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) );

	osu_resolution.setCallback( fastdelegate::MakeDelegate(this, &Osu::onInternalResolutionChanged) );
	osu_letterboxing.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingChange) );

	osu_confine_cursor_windowed.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorWindowedChange) );
	osu_confine_cursor_fullscreen.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorFullscreenChange) );

	convar->getConVarByName("osu_playfield_mirror_horizontal")->setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) ); // force a mod update on OsuBeatmap if changed
	convar->getConVarByName("osu_playfield_mirror_vertical")->setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) ); // force a mod update on OsuBeatmap if changed

  	// vars
	m_skin = NULL;
	m_songBrowser2 = NULL;
	m_modSelector = NULL;
	m_updateHandler = NULL;

	m_bF1 = false;
	m_bUIToggleCheck = false;
	m_bTab = false;
	m_bEscape = false;
	m_bKeyboardKey1Down = false;
	m_bKeyboardKey2Down = false;
	m_bMouseKey1Down = false;
	m_bMouseKey2Down = false;
	m_bSkipScheduled = false;
	m_bQuickRetryDown = false;
	m_fQuickRetryTime = 0.0f;
	m_bSeeking = false;
	m_bSeekKey = false;
	m_fQuickSaveTime = 0.0f;

	m_bToggleModSelectionScheduled = false;
	m_bToggleSongBrowserScheduled = false;
	m_bToggleOptionsMenuScheduled = false;
	m_bToggleRankingScreenScheduled = false;
	m_bToggleVRTutorialScheduled = false;
	m_bToggleChangelogScheduled = false;
	m_bToggleEditorScheduled = false;

	m_bModAuto = false;
	m_bModAutopilot = false;
	m_bModRelax = false;
	m_bModSpunout = false;
	m_bModTarget = false;
	m_bModDT = false;
	m_bModNC = false;
	m_bModNF = false;
	m_bModHT = false;
	m_bModDC = false;
	m_bModHD = false;
	m_bModHR = false;
	m_bModEZ = false;
	m_bModSD = false;
	m_bModSS = false;
	m_bModNM = false;

	m_bShouldCursorBeVisible = false;

	m_gamemode = GAMEMODE::STD;
	m_bScheduleEndlessModNextBeatmap = false;

	// debug
	m_windowManager = new CWindowManager();
	/*
	DebugMonitor *dm = new DebugMonitor();
	m_windowManager->addWindow(dm);
	dm->open();
	*/


	// renderer
	g_vInternalResolution = engine->getScreenSize();
	m_frameBuffer = engine->getResourceManager()->createRenderTarget(0, 0, getScreenWidth(), getScreenHeight());
	m_backBuffer = engine->getResourceManager()->createRenderTarget(0, 0, getScreenWidth(), getScreenHeight());
	m_backBuffer = engine->getResourceManager()->createRenderTarget(0, 0, getScreenWidth(), getScreenHeight());

	// load a few select subsystems very early
	m_notificationOverlay = new OsuNotificationOverlay(this);
	m_score = new OsuScore(this);
	m_updateHandler = new OsuUpdateHandler();

	// exec the main config file (this must be right here!)
	Console::execConfigFile(isInVRMode() ? "osuvr" : "osu");

	// update mod settings
	updateMods();

	// load global resources
	m_titleFont = engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_TITLE", 60.0f); // was 40
	m_subTitleFont = engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_SUBTITLE", 21.0f);
	m_songBrowserFont = engine->getResourceManager()->loadFont("SourceSansPro-Regular.otf", "FONT_OSU_SONGBROWSER", 35.0f);
	m_songBrowserFontBold = engine->getResourceManager()->loadFont("SourceSansPro-Bold.otf", "FONT_OSU_SONGBROWSER_BOLD", 30.0f);

	// load skin
	UString skinFolder = m_osu_folder_ref->getString();
	skinFolder.append("Skins/");
	skinFolder.append(osu_skin.getString());
	skinFolder.append("/");
	if (m_skin == NULL) // the skin may already be loaded by Console::execConfigFile() above
		m_skin = new OsuSkin(this, skinFolder);

	// load subsystems, add them to the screens array
	m_tooltipOverlay = new OsuTooltipOverlay(this);
	m_vr = new OsuVR(this);
	m_mainMenu = new OsuMainMenu(this);
	m_optionsMenu = new OsuOptionsMenu(this);
	m_songBrowser2 = new OsuSongBrowser2(this);
	m_modSelector = new OsuModSelector(this);
	m_rankingScreen = new OsuRankingScreen(this);
	m_pauseMenu = new OsuPauseMenu(this);
	m_hud = new OsuHUD(this);
	m_vrTutorial = new OsuVRTutorial(this);
	m_changelog = new OsuChangelog(this);
	m_editor = new OsuEditor(this);

	// the order in this vector will define in which order events are handled/consumed
	m_screens.push_back(m_notificationOverlay);
	m_screens.push_back(m_rankingScreen);
	m_screens.push_back(m_modSelector);
	m_screens.push_back(m_pauseMenu);
	m_screens.push_back(m_hud);
	m_screens.push_back(m_songBrowser2);
	m_screens.push_back(m_optionsMenu);
	m_screens.push_back(m_vrTutorial);
	m_screens.push_back(m_changelog);
	m_screens.push_back(m_editor);
	m_screens.push_back(m_mainMenu);
	m_screens.push_back(m_tooltipOverlay);

	// make primary screen visible
	//m_optionsMenu->setVisible(true);
	//m_modSelector->setVisible(true);
	//m_songBrowser2->setVisible(true);
	//m_pauseMenu->setVisible(true);
	//m_rankingScreen->setVisible(true);
	//m_changelog->setVisible(true);
	//m_editor->setVisible(true);


	if (isInVRMode() && osu_vr_tutorial.getBool())
		m_vrTutorial->setVisible(true);
	else
		m_mainMenu->setVisible(true);

	m_updateHandler->checkForUpdates();


	m_shaderGenericTextured = engine->getResourceManager()->createShader(

					// vertex Shader
					"#version 110\n"
					"uniform mat4 matrix;\n"
					"varying vec2 texCoords;\n"
					"void main()\n"
					"{\n"
					"	texCoords = gl_MultiTexCoord0.xy;\n"
					"	gl_Position = matrix * gl_Vertex;\n"
					"	gl_FrontColor = gl_Color;\n"
					"}\n",

					// fragment Shader
					"#version 110\n"
					"uniform sampler2D mytexture;\n"
					"varying vec2 texCoords;\n"
					"void main()\n"
					"{\n"
					"   gl_FragColor = texture2D(mytexture, texCoords) * gl_Color;\n"
					"}\n"
			);

	/*m_shaderRay = engine->getResourceManager()->createShader(

					// vertex Shader
					"#version 110\n"
					"uniform mat4 matrix;\n"
					"varying vec2 texCoords;\n"
					"void main()\n"
					"{\n"
					"	texCoords = gl_MultiTexCoord0.xy;\n"
					"	gl_Position = matrix * gl_Vertex;\n"
					"	gl_FrontColor = gl_Color;\n"
					"}\n",

					// fragment Shader
					"#version 110\n"
					"float PI = 3.14159265;\n"
					"uniform sampler2D mytexture;\n"
					"varying vec2 texCoords;\n"

					"float fov = 103.0f;\n"
					"float aspect = 1920/1080;\n"

					"vec2 cameraPos = vec2(0,0);\n"

					"float imageAspectRatio = 1920.0f / 1080.0f;\n"
					"float Px = (2 * ((x + 0.5) / imageWidth) - 1) * tan(fov / 2 * PI / 180) * imageAspectRatio;\n"
					"float Py = (1 - 2 * ((y + 0.5) / imageHeight) * tan(fov / 2 * PI / 180);\n"

					"float radius = 2;\n"
					"float height = 1;\n"
					"float screenAngle = 90;\n"
					"uniform vec3 cameraDir;\n"
					"void main()\n"
					"{\n"
					"   gl_FragColor = texture2D(mytexture, texCoords) * gl_Color;\n"
					"}\n"
			);*/

	m_shaderGenericUntextured = engine->getResourceManager()->createShader(

					// vertex Shader
					"#version 110\n"
					"uniform mat4 matrix;\n"
					"void main()\n"
					"{\n"
					"	gl_Position = matrix * gl_Vertex;\n"
					"	gl_FrontColor = gl_Color;\n"
					"}\n",

					// fragment Shader
					"#version 110\n"
					"uniform vec4 color;\n"
					"void main()\n"
					"{\n"
					"   gl_FragColor = color;\n"
					"}\n"
			);



	/*float dist = -fposu_distance.getFloat();
	circumlength = 0;
	VertexPair vp1 = VertexPair(Vector3(-0.5,0.5,dist),Vector3(-0.5,-0.5, dist), 0);
	VertexPair vp2 = VertexPair(Vector3(0.5,0.5, dist),Vector3(0.5,-0.5, dist), 1);
	edgeDistance = Vector3(0,0,0).distance(Vector3(-0.5,0.0,dist));
	meshList.push_back(vp1);
	meshList.push_back(vp2);
	std::list<VertexPair>::iterator begin = meshList.begin();
	std::list<VertexPair>::iterator end = meshList.end();
	--end;
	subdivide(meshList, begin, end, subdivisions);

	//debugLog("circumlength: %f \n", circumlength);

	begin = meshList.begin();
	std::list<VertexPair>::iterator next = ++meshList.begin();
	while(next != meshList.end()) {
		Vector3 topLeft = (*begin).a;
		Vector3 bottomLeft = (*begin).b;
		Vector3 topRight = (*next).a;
		Vector3 bottomRight = (*next).b;
		float leftTC = (*begin).textureCoordinate;
		float rightTC = (*next).textureCoordinate;
		float topTC = 1;
		float bottomTC = 0;

		arrayObject.addVertex(topLeft);
		arrayObject.addTexcoord(leftTC, topTC);
		arrayObject.addVertex(topRight);
		arrayObject.addTexcoord(rightTC, topTC);
		arrayObject.addVertex(bottomLeft);
		arrayObject.addTexcoord(leftTC, bottomTC);

		arrayObject.addVertex(bottomLeft);
		arrayObject.addTexcoord(leftTC, bottomTC);
		arrayObject.addVertex(topRight);
		arrayObject.addTexcoord(rightTC, topTC);
		arrayObject.addVertex(bottomRight);
		arrayObject.addTexcoord(rightTC, bottomTC);

		(*begin).normal = NormalFromTriangle(topLeft, topRight, bottomLeft);


		begin++;
		next++;
	}



	camera = new Camera(Vector3(0,0,0),Vector3(0,0,-1),103.0f);
	camera->setFov(103.0f);
	float aspect = m_backBuffer->getHeight() / m_backBuffer->getWidth();
	modelMatrix = Matrix4().translate(0, 0, 0).scale(1, aspect*(circumlength), 1);//-fposu_distance.getFloat() scale(2, 2*aspect, 1)
	projectionMatrix = Camera::buildMatrixPerspectiveFov(deg2rad(fposu_fov.getFloat()),
				((float) engine->getScreenWidth())/((float) engine->getScreenHeight()),
				0.1f, 10.0f);*/

	angleImage = (OpenGLImage*)engine->getResourceManager()->loadImage("45px.png", "45px", false);
	angleImage->setWrapMode(Graphics::WRAP_MODE::WRAP_MODE_REPEAT);
	angleImage->setFilterMode(Graphics::FILTER_MODE::FILTER_MODE_NONE);
	makeAngleCircle();
	makePlayfield();

	cubeImage = (OpenGLImage*)engine->getResourceManager()->loadImage("grid2.png", "grid", false);
	cubeImage->setFilterMode(Graphics::FILTER_MODE::FILTER_MODE_NONE);

	arrayObjectCube.addVertex(-2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f,  2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f, -2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(1.0f, 1.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex( 2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(1.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f,  2.5f);  arrayObjectCube.addTexcoord(0.0f, 0.0f);
	arrayObjectCube.addVertex(-2.5f,  2.5f, -2.5f);  arrayObjectCube.addTexcoord(0.0f, 1.0f);


	/*
	// DEBUG: immediately start diff of a beatmap
	UString debugFolder = "c:/Program Files (x86)/osu!/Songs/65853 Blue Stahli - Shotgun Senorita (Zardonic Remix)/";
	UString debugDiffFileName = "Blue Stahli - Shotgun Senorita (Zardonic Remix) (Aleks719) [Insane].osu";
	OsuBeatmap *debugBeatmap = new OsuBeatmapStandard(this);
	UString beatmapPath = debugFolder;
	beatmapPath.append(debugDiffFileName);
	OsuBeatmapDifficulty *debugDiff = new OsuBeatmapDifficulty(this, beatmapPath, debugFolder);
	if (!debugDiff->loadMetadataRaw())
		engine->showMessageError("OsuBeatmapDifficulty", "Couldn't debugDiff->loadMetadataRaw()!");
	else
	{
		std::vector<OsuBeatmapDifficulty*> diffs;
		diffs.push_back(debugDiff);
		debugBeatmap->setDifficulties(diffs);

		debugBeatmap->selectDifficulty(debugDiff);
		m_songBrowser2->onDifficultySelected(debugBeatmap, debugDiff, true);
		//convar->getConVarByName("osu_volume_master")->setValue(1.0f);

		// this will leak memory (one OsuBeatmap object and one OsuBeatmapDifficulty object), but who cares (since debug only)
	}
	*/

	// DEBUG: immediately start diff of a beatmap
	/*
	UString debugFolder = "c:/Program Files (x86)/osu!/Songs/407186 S3RL feat Krystal - R4V3 B0Y/";
	UString debugDiffFileName = "S3RL feat Krystal - R4V3 B0Y (Draftnell) [BANGKE's 4K Normal].osu";
	OsuBeatmap *debugBeatmap = new OsuBeatmapMania(this);
	UString beatmapPath = debugFolder;
	beatmapPath.append(debugDiffFileName);
	OsuBeatmapDifficulty *debugDiff = new OsuBeatmapDifficulty(this, beatmapPath, debugFolder);
	if (!debugDiff->loadMetadataRaw())
		engine->showMessageError("OsuBeatmapDifficulty", "Couldn't debugDiff->loadMetadataRaw()!");
	else
	{
		std::vector<OsuBeatmapDifficulty*> diffs;
		diffs.push_back(debugDiff);
		debugBeatmap->setDifficulties(diffs);

		debugBeatmap->selectDifficulty(debugDiff);
		m_songBrowser2->onDifficultySelected(debugBeatmap, debugDiff, true);
		//convar->getConVarByName("osu_volume_master")->setValue(1.0f);

		// this will leak memory (one OsuBeatmap object and one OsuBeatmapDifficulty object), but who cares (since debug only)
	}
	*/
}

Osu::~Osu()
{
	// "leak" OsuUpdateHandler object, but not relevant since shutdown:
	// this is the only way of handling instant user shutdown requests properly, there is no solution for active working threads besides letting the OS kill them when the main threads exits.
	// we must not delete the update handler object, because the thread is potentially still accessing members during shutdown
	m_updateHandler->stop(); // tell it to stop at the next cancellation point, depending on the OS/runtime and engine shutdown time it may get killed before that

	SAFE_DELETE(m_windowManager);

	for (int i=0; i<m_screens.size(); i++)
	{
		SAFE_DELETE(m_screens[i]);
	}

	SAFE_DELETE(m_skin);
	SAFE_DELETE(m_score);
	SAFE_DELETE(m_vr);
}

void Osu::draw(Graphics *g)
{
	if (m_skin == NULL) // sanity check
	{
		g->setColor(0xffff0000);
		g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
		return;
	}

	// draw everything in the correct order
	if (isInPlayMode()) // if we are playing a beatmap
	{
		m_backBuffer->enable();
		getSelectedBeatmap()->draw(g);

		//m_hud->draw(g); moved to bottom

		// quick retry fadeout overlay
		if (m_fQuickRetryTime != 0.0f && m_bQuickRetryDown)
		{
			float alphaPercent = 1.0f - (m_fQuickRetryTime - engine->getTime())/osu_quick_retry_delay.getFloat();
			if (engine->getTime() > m_fQuickRetryTime)
				alphaPercent = 1.0f;

			g->setColor(COLOR((int)(255*alphaPercent), 0, 0, 0));
			g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
		}

		m_pauseMenu->draw(g);
		m_modSelector->draw(g);

		if (osu_draw_fps.getBool())
			m_hud->drawFps(g);

		m_hud->drawVolumeChange(g);

		m_windowManager->draw(g);


		// special cursor handling (fading cursor + invisible cursor mods)
		const bool allowDrawCursor = !osu_hide_cursor_during_gameplay.getBool() || getSelectedBeatmap()->isPaused();
		float fadingCursorAlpha = 1.0f - clamp<float>((float)m_score->getCombo()/osu_mod_fadingcursor_combo.getFloat(), 0.0f, 1.0f);
		if (m_pauseMenu->isVisible() || getSelectedBeatmap()->isContinueScheduled())
			fadingCursorAlpha = 1.0f;
		OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(getSelectedBeatmap());

		//if (allowDrawCursor && beatmapStd != NULL)
		//	m_hud->drawCursor(g, beatmapStd->getCursorPos(), osu_mod_fadingcursor.getBool() ? fadingCursorAlpha : 1.0f);

		m_backBuffer->disable();

		// draw screen
		//projectionMatrix = Camera::buildMatrixPerspectiveFov(deg2rad(fposu_fov.getFloat()),
		//			((float)getScreenWidth())/((float)getScreenHeight()),
		//			0.05f, 50.0f);
		projectionMatrix = Camera::buildMatrixPerspectiveFovHorizontal(deg2rad(fposu_fov.getFloat()), ((float)getScreenHeight()/(float)getScreenWidth()), 0.05f, 50.0f);
		viewMatrix = Camera::buildMatrixLookAt(camera->getPos(), camera->getViewDirection(), camera->getViewUp());
		//viewMatrix = Camera::buildMatrixLookAt(MyCameraPosition, MyCameraPosition+MyCameraDirection, MyCameraUp);
		//  debugLog("%f %f %f \n", MyCameraDirection.x, MyCameraDirection.y, MyCameraDirection.z);
		Matrix4 mvp = projectionMatrix * viewMatrix * modelMatrix;
		Matrix4 vp = projectionMatrix * viewMatrix;
		m_shaderGenericTextured->enable();
		{


			//clownbil cirkel
			/*angleImage->bind();
			{
				g->setColor(0xffffffff);
				g->drawVAO(&arrayObject2);
			}
			angleImage->unbind();*/

			g->setDepthBuffer(true);

			m_shaderGenericTextured->setUniformMatrix4fv("matrix", mvp);

			m_backBuffer->bind();
			{
				g->setColor(0xffffffff);
				g->drawVAO(&arrayObject);
			}
			m_backBuffer->unbind();

			if(fposu_cube.getBool())
			{
				m_shaderGenericTextured->setUniformMatrix4fv("matrix", vp);
				cubeImage->bind();
				{
					g->setColor(0xffffffff);
					g->drawVAO(&arrayObjectCube);
				}
				cubeImage->unbind();
			}
			g->setDepthBuffer(false);
		}
		m_shaderGenericTextured->disable();

		/*int face=0;
		m_shaderGenericUntextured->enable();
		{
			m_shaderGenericUntextured->setUniformMatrix4fv("matrix", mvp);


			m_backBuffer->bind();
			std::list<VertexPair>::iterator begin = meshList.begin();
			std::list<VertexPair>::iterator next = ++meshList.begin();
			while(next != meshList.end()) {
				if(face == currentIntersectingFace){
					m_shaderGenericUntextured->setUniform4f("color", 1.0f, 0.0f, 0.0f, 1.0f);
					//debugLog("face:%i\n", face);
				}
				else {
					m_shaderGenericUntextured->setUniform4f("color", 1.0f, 1.0f, 0.0f, 1.0f);
				}
				Vector3 topLeft = (*begin).a;
				Vector3 bottomLeft = (*begin).b;
				Vector3 topRight = (*next).a;
				Vector3 bottomRight = (*next).b;
				float leftTC = (*begin).textureCoordinate;
				float rightTC = (*next).textureCoordinate;
				float topTC = 1;
				float bottomTC = 0;

				VertexArrayObject temp;
				temp.addVertex(topLeft);
				temp.addTexcoord(leftTC, topTC);
				temp.addVertex(topRight);
				temp.addTexcoord(rightTC, topTC);
				temp.addVertex(bottomLeft);
				temp.addTexcoord(leftTC, bottomTC);

				temp.addVertex(bottomLeft);
				temp.addTexcoord(leftTC, bottomTC);
				temp.addVertex(topRight);
				temp.addTexcoord(rightTC, topTC);
				temp.addVertex(bottomRight);
				temp.addTexcoord(rightTC, bottomTC);

				g->drawVAO(&temp);
				begin++;
				next++;
				face++;
			}
			m_backBuffer->unbind();
		}
		m_shaderGenericUntextured->disable();*/


		if (allowDrawCursor && beatmapStd != NULL)
			m_hud->drawCursor(g, Vector2((float)getScreenWidth(), (float)getScreenHeight())/2, osu_mod_fadingcursor.getBool() ? fadingCursorAlpha : 1.0f);
		//engine->getScreenSize()/2;


		m_hud->draw(g);
	}
	else // if we are not playing
	{

		if (m_songBrowser2 != NULL)
			m_songBrowser2->draw(g);

		m_modSelector->draw(g);
		m_mainMenu->draw(g);
		m_vrTutorial->draw(g);
		m_changelog->draw(g);
		m_editor->draw(g);
		m_optionsMenu->draw(g);
		m_rankingScreen->draw(g);


		if (osu_draw_fps.getBool())
			m_hud->drawFps(g);

		m_hud->drawVolumeChange(g);

		m_windowManager->draw(g);

		if (!isInVRMode() || (m_vr->isVirtualCursorOnScreen() || engine->hasFocus()))
			m_hud->drawCursor(g, engine->getMouse()->getPos());
	}

	m_tooltipOverlay->draw(g);
	m_notificationOverlay->draw(g);
}

void Osu::drawVR(Graphics *g)
{
	Matrix4 mvp = openvr->getCurrentModelViewProjectionMatrix();

	// draw virtual screen + environment
	m_vr->drawVR(g, mvp, m_backBuffer);

	// draw everything in the correct order
	if (isInPlayMode()) // if we are playing a beatmap
	{
		m_vr->drawVRHUD(g, mvp, m_hud);
		m_vr->drawVRBeatmap(g, mvp, getSelectedBeatmap());
	}
	else // not playing
	{
		if (m_optionsMenu->shouldDrawVRDummyHUD())
			m_vr->drawVRHUDDummy(g, mvp, m_hud);

		m_vr->drawVRPlayfieldDummy(g, mvp);
	}
}

void Osu::update()
{
	if (m_skin != NULL)
		m_skin->update();

	m_windowManager->update();

	if (isInPlayMode()) {

		//float aspect = m_backBuffer->getHeight() / m_backBuffer->getWidth();

		camera->rotateX(-engine->getMouse()->getRawDelta().x);
		camera->rotateY(-engine->getMouse()->getRawDelta().y);

		/*float xRotateAngle = -engine->getMouse()->getRawDelta().x;
		float yRotateAngle = engine->getMouse()->getRawDelta().y;
		pitch+=yRotateAngle;
		if (pitch > 89.8f) {
			pitch = 89.8f;
			yRotateAngle = 0;
		}
		else if (pitch < -89.8f)
		{
			pitch = -89.8f;
			yRotateAngle = 0;
		}

		yaw+=xRotateAngle;
		if (yaw > 360.0f)
			yaw = yaw - 360.0f;
		else if (yaw < 0.0f)
			yaw = 360.0f + yaw;


		a.fromAxis(MyCameraLeft, yRotateAngle);
		b.fromAxis(Vector3(0,1,0), xRotateAngle);
		q = a*b;

		Matrix4 mouseRotation = Matrix4().rotateY(xRotateAngle).rotate(yRotateAngle, MyCameraLeft);
		MyCameraDirection = mouseRotation*MyCameraDirection;
		MyCameraLeft = mouseRotation*MyCameraLeft;
		MyCameraUp = mouseRotation*MyCameraUp;




		Vector3 forwarddirection = Vector3(MyCameraDirection.x, 0, MyCameraDirection.z).normalize();
		Vector3 cumulativeDirection;
		if(!(movingBackward && movingForward)) {
			if (movingBackward)
				cumulativeDirection += forwarddirection;
			if (movingForward)
				cumulativeDirection -= forwarddirection;
		}
		if (!(movingLeft && movingRight)) {
			if (movingLeft)
				cumulativeDirection -= MyCameraLeft;
			if (movingRight)
				cumulativeDirection += MyCameraLeft;
		}
		cumulativeDirection.normalize();
		MyCameraPosition += cumulativeDirection*-5.5*(float)engine->getFrameTime();
		*/

		//float virtualScreenPlaneIntersectionDistance = 0.0f;
		std::list<VertexPair>::iterator begin = meshList.begin();
		std::list<VertexPair>::iterator next = ++meshList.begin();
		int face = 0;
		//debugLog("enter loop \n");
		while(!screenIntersection && next != meshList.end())
		{

			Vector4 topLeft = (modelMatrix * Vector4(	(*begin).a.x, 	(*begin).a.y, 	(*begin).a.z, 1));
			Vector4 right = (modelMatrix * Vector4(		(*next).a.x, 	(*next).a.y, 	(*next).a.z, 1));
			Vector4 down = (modelMatrix * Vector4(		(*begin).b.x, 	(*begin).b.y, 	(*begin).b.z, 1));
			Vector3 normal = (modelMatrix * (*begin).normal).normalize();

			//after model transformation
			Vector3 TopLeft = Vector3(topLeft.x, topLeft.y, topLeft.z);
			Vector3 Right = Vector3(right.x, right.y, right.z);
			Vector3 Down = Vector3(down.x, down.y, down.z);

			Vector3 calculatedNormal = (Right-TopLeft).cross(Down-TopLeft);

			float denominator = calculatedNormal.dot(camera->getViewDirection());
			float numerator = -calculatedNormal.dot(camera->getPos()-TopLeft);
			if(denominator == 0.0f) {
				begin++;
				next++;
				face++;
				continue;
			}

			float t = numerator/denominator;
			Vector3 intersectionPoint = camera->getPos() + camera->getViewDirection()*t;

			if(abs(calculatedNormal.dot(intersectionPoint-TopLeft)) < 1e-6f) {
				float  u = (intersectionPoint-TopLeft).dot(Right-TopLeft);
				float v = (intersectionPoint-TopLeft).dot(Down-TopLeft);
				if(u >= 0 && u <= (Right-TopLeft).dot(Right-TopLeft)) { //*(Right-TopLeft).length()
					if(v >= 0 && v <= (Down-TopLeft).dot(Down-TopLeft)) {		//*(Down-TopLeft).length()
						float rightLength = (Right-TopLeft).length();
						float downLength = (Down-TopLeft).length();
						float x=u/(rightLength*rightLength);
						float y=v/(downLength*downLength);
						//debugLog("v\n");
						float distancePerFace = engine->getScreenWidth()/pow(2,subdivisions);
						float distanceInFace = distancePerFace*x;
						Vector2 newMousePos = Vector2((distancePerFace*face)+distanceInFace,
													  y*engine->getScreenHeight());
						engine->getMouse()->setPos(newMousePos);
					}
				}
			}
			begin++;
			next++;
			face++;
		}
		screenIntersection = false;
	}

	for (int i=0; i<m_screens.size(); i++)
	{
			m_screens[i]->update();
	}


	// main beatmap update
	m_bSeeking = false;
	if (isInPlayMode())
	{
		getSelectedBeatmap()->update();

		// scrubbing/seeking
		if ((engine->getKeyboard()->isControlDown() && engine->getKeyboard()->isAltDown()) || m_bSeekKey)
		{
			m_bSeeking = true;
			const float percent = clamp<float>(engine->getMouse()->getPos().x/getScreenWidth(), 0.0f, 1.0f);
			if (engine->getMouse()->isLeftDown())
				getSelectedBeatmap()->seekPercentPlayable(percent);
			if (engine->getMouse()->isRightDown())
				m_fQuickSaveTime = clamp<float>((float)((getSelectedBeatmap()->getStartTimePlayable()+getSelectedBeatmap()->getLengthPlayable())*percent) / (float)getSelectedBeatmap()->getLength(), 0.0f, 1.0f);
		}

		// skip button clicking
		if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused() && !m_bSeeking)
		{
			// TODO: make this on click only, and not if held too. this can also cause earrape while scrubbing
			bool isAnyOsuKeyDown = (m_bKeyboardKey1Down || m_bKeyboardKey2Down || m_bMouseKey1Down || m_bMouseKey2Down);
			bool isAnyVRKeyDown = isInVRMode() && !m_vr->isUIActive() && (openvr->getLeftController()->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD) || openvr->getRightController()->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD)
												|| openvr->getLeftController()->getTrigger() > 0.95f || openvr->getRightController()->getTrigger() > 0.95f);
			if (engine->getMouse()->isLeftDown() || isAnyOsuKeyDown || isAnyVRKeyDown)
			{
				if (m_hud->getSkipClickRect().contains(engine->getMouse()->getPos()) || isAnyVRKeyDown)
					m_bSkipScheduled = true;
			}
		}

		// skipping
		if (m_bSkipScheduled)
		{
			if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused())
				getSelectedBeatmap()->skipEmptySection();

			m_bSkipScheduled = false;
		}

		// quick retry timer
		if (m_bQuickRetryDown && m_fQuickRetryTime != 0.0f && engine->getTime() > m_fQuickRetryTime)
		{
			m_fQuickRetryTime = 0.0f;

			getSelectedBeatmap()->restart(true);
			getSelectedBeatmap()->update();
			m_pauseMenu->setVisible(false);
		}
	}

	// asynchronous ui toggles
	// TODO: this is cancer, why did I even write this section
	if (m_bToggleModSelectionScheduled)
	{
		m_bToggleModSelectionScheduled = false;

		if (!isInPlayMode())
		{
			if (m_songBrowser2 != NULL)
				m_songBrowser2->setVisible(m_modSelector->isVisible());
		}

		m_modSelector->setVisible(!m_modSelector->isVisible());
	}
	if (m_bToggleSongBrowserScheduled)
	{
		m_bToggleSongBrowserScheduled = false;

		if (m_songBrowser2 != NULL)
			m_songBrowser2->setVisible(!m_songBrowser2->isVisible());

		m_mainMenu->setVisible(!(m_songBrowser2 != NULL && m_songBrowser2->isVisible()));
		updateConfineCursor();
	}
	if (m_bToggleOptionsMenuScheduled)
	{
		m_bToggleOptionsMenuScheduled = false;

		m_optionsMenu->setVisible(!m_optionsMenu->isVisible());
		m_mainMenu->setVisible(!m_optionsMenu->isVisible());
	}
	if (m_bToggleRankingScreenScheduled)
	{
		m_bToggleRankingScreenScheduled = false;

		m_rankingScreen->setVisible(!m_rankingScreen->isVisible());
		if (m_songBrowser2 != NULL)
			m_songBrowser2->setVisible(!m_rankingScreen->isVisible());
	}
	if (m_bToggleVRTutorialScheduled)
	{
		m_bToggleVRTutorialScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_vrTutorial->setVisible(!m_mainMenu->isVisible());
	}
	if (m_bToggleChangelogScheduled)
	{
		m_bToggleChangelogScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_changelog->setVisible(!m_mainMenu->isVisible());
	}
	if (m_bToggleEditorScheduled)
	{
		m_bToggleEditorScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_editor->setVisible(!m_mainMenu->isVisible());
	}

	// handle cursor visibility if outside of internal resolution
	// TODO: not a critical bug, but the real cursor gets visible way too early if sensitivity is > 1.0f, due to this using scaled/offset getMouse()->getPos()
	if (osu_resolution_enabled.getBool())
	{
		McRect internalWindow = McRect(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
		bool cursorVisible = env->isCursorVisible();
		if (!internalWindow.contains(engine->getMouse()->getPos()))
		{
			if (!cursorVisible)
				env->setCursorVisible(true);
		}
		else
		{
			if (cursorVisible != m_bShouldCursorBeVisible)
				env->setCursorVisible(m_bShouldCursorBeVisible);
		}
	}

	// handle mousewheel volume change
	if ((m_songBrowser2 != NULL && (!m_songBrowser2->isVisible() || engine->getKeyboard()->isAltDown())) && !m_optionsMenu->isVisible() && !m_vrTutorial->isVisible() && !m_changelog->isVisible() && (!m_modSelector->isMouseInScrollView() || engine->getKeyboard()->isAltDown()))
	{
		if ((!(isInPlayMode() && !m_pauseMenu->isVisible()) && !m_rankingScreen->isVisible()) || (isInPlayMode() && !osu_disable_mousewheel.getBool()) || engine->getKeyboard()->isAltDown())
		{
			int wheelDelta = engine->getMouse()->getWheelDeltaVertical();
			if (wheelDelta != 0)
			{
				if (wheelDelta > 0)
					volumeUp();
				else
					volumeDown();
			}
		}
	}

	// TODO: shitty override, but it works well enough for now => see OsuVR.cpp
	// it's a bit of a hack, because using cursor visibility to work around SetCursorPos() affecting the windows cursor in the Mouse class
	if (isInVRMode() && !env->isCursorVisible())
		env->setCursorVisible(true);

	// endless mod
	if (m_bScheduleEndlessModNextBeatmap)
	{
		m_bScheduleEndlessModNextBeatmap = false;
		m_songBrowser2->playNextRandomBeatmap();
	}
}

void Osu::updateMods()
{
	m_bModAuto = osu_mods.getString().find("auto") != -1;
	m_bModAutopilot = osu_mods.getString().find("autopilot") != -1;
	m_bModRelax = osu_mods.getString().find("relax") != -1;
	m_bModSpunout = osu_mods.getString().find("spunout") != -1;
	m_bModTarget = osu_mods.getString().find("practicetarget") != -1;
	m_bModDT = osu_mods.getString().find("dt") != -1;
	m_bModNC = osu_mods.getString().find("nc") != -1;
	m_bModNF = osu_mods.getString().find("nf") != -1;
	m_bModHT = osu_mods.getString().find("ht") != -1;
	m_bModDC = osu_mods.getString().find("dc") != -1;
	m_bModHD = osu_mods.getString().find("hd") != -1;
	m_bModHR = osu_mods.getString().find("hr") != -1;
	m_bModEZ = osu_mods.getString().find("ez") != -1;
	m_bModSD = osu_mods.getString().find("sd") != -1;
	m_bModSS = osu_mods.getString().find("ss") != -1;
	m_bModNM = osu_mods.getString().find("nm") != -1;

	// static overrides
	onSpeedChange("", osu_speed_override.getString());
	onPitchChange("", osu_pitch_override.getString());

	// autopilot overrides auto
	if (m_bModAutopilot)
		m_bModAuto = false;

	// handle auto/pilot cursor visibility
	if (!m_bModAuto && !m_bModAutopilot)
	{
		env->setCursorVisible(false);
		m_bShouldCursorBeVisible = false;
	}
	else if (isInPlayMode())
	{
		env->setCursorVisible(true);
		m_bShouldCursorBeVisible = true;
	}

	// notify the possibly running beatmap of mod changes, for e.g. recalculating stacks dynamically if HR is toggled
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->onModUpdate();
}

void Osu::onKeyDown(KeyboardEvent &key)
{
	// global hotkeys

	// special hotkeys
	// reload & recompile shaders
	if (engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_R)
	{
		Shader *sliderShader = engine->getResourceManager()->getShader("slider");
		Shader *sliderShaderVR = engine->getResourceManager()->getShader("sliderVR");

		if (sliderShader != NULL)
			sliderShader->reload();
		if (sliderShaderVR != NULL)
			sliderShaderVR->reload();

		key.consume();
	}

	// reload skin
	if (engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_S)
	{
		onSkinReload();
		m_notificationOverlay->addNotification("Skin reloaded!");
		key.consume();
	}

	// fullscreen toggle
	if (engine->getKeyboard()->isAltDown() && key == KEY_ENTER)
	{
		engine->toggleFullscreen();
		key.consume();
	}

	// arrow keys volume (alt)
	if (engine->getKeyboard()->isAltDown() && key == (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt())
	{
		volumeUp();
		key.consume();
	}
	if (engine->getKeyboard()->isAltDown() && key == (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt())
	{
		volumeDown();
		key.consume();
	}

	// disable mouse buttons hotkey
	if (key == (KEYCODE)OsuKeyBindings::DISABLE_MOUSE_BUTTONS.getInt())
	{
		if (osu_disable_mousebuttons.getBool())
		{
			osu_disable_mousebuttons.setValue(0.0f);
			m_notificationOverlay->addNotification("Mouse buttons are enabled.");
		}
		else
		{
			osu_disable_mousebuttons.setValue(1.0f);
			m_notificationOverlay->addNotification("Mouse buttons are disabled.");
		}
	}

	// screenshots
	if (key == (KEYCODE)OsuKeyBindings::SAVE_SCREENSHOT.getInt())
		saveScreenshot();

	// boss key (minimize)
	if (key == (KEYCODE)OsuKeyBindings::BOSS_KEY.getInt())
		engine->getEnvironment()->minimize();

	// local hotkeys

	// while playing
	if (isInPlayMode())
	{
		// while playing and not paused
		if (!getSelectedBeatmap()->isPaused())
		{
			if (key == (KEYCODE)OsuKeyBindings::MOVE_FORWARD.getInt()) {
				movingForward = true;
				key.consume();
			} else if (key == (KEYCODE)OsuKeyBindings::MOVE_BACKWARD.getInt()) {
				movingBackward = true;
				key.consume();
			} else if (key == (KEYCODE)OsuKeyBindings::MOVE_LEFT.getInt()) {
				movingLeft = true;
				key.consume();
			} else if (key == (KEYCODE)OsuKeyBindings::MOVE_RIGHT.getInt()) {
				movingRight = true;
				key.consume();
			}

			getSelectedBeatmap()->onKeyDown(key);

			if (!m_bKeyboardKey1Down && key == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt())
			{
				m_bKeyboardKey1Down = true;
				onKey1Change(true, false);
				key.consume();
			}

			if (!m_bKeyboardKey2Down && key == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt())
			{
				m_bKeyboardKey2Down = true;
				onKey2Change(true, false);
				key.consume();
			}

			// handle skipping
			if (key == KEY_ENTER || key == (KEYCODE)OsuKeyBindings::SKIP_CUTSCENE.getInt())
				m_bSkipScheduled = true;

			// toggle ui
			if (key == KEY_SHIFT)
			{
				if (m_bTab && !m_bUIToggleCheck)
				{
					m_bUIToggleCheck = true;
					m_osu_draw_hud_ref->setValue(!m_osu_draw_hud_ref->getBool());

					key.consume();
				}
			}
			if (key == KEY_TAB)
			{
				m_bTab = true;
				if (engine->getKeyboard()->isShiftDown() && !m_bUIToggleCheck)
				{
					m_bUIToggleCheck = true;
					m_osu_draw_hud_ref->setValue(!m_osu_draw_hud_ref->getBool());

					key.consume();
				}
			}

			// allow live mod changing while playing
			if (!key.isConsumed() && key == KEY_F1 && !m_bF1 && !getSelectedBeatmap()->hasFailed()) // only if not failed though
			{
				m_bF1 = true;
				toggleModSelection(true);
			}

			// quick save/load
			if (key == (KEYCODE)OsuKeyBindings::QUICK_SAVE.getInt())
				m_fQuickSaveTime = getSelectedBeatmap()->getPercentFinished();
			if (key == (KEYCODE)OsuKeyBindings::QUICK_LOAD.getInt())
				getSelectedBeatmap()->seekPercent(m_fQuickSaveTime);
		}

		// while paused or maybe not paused

		// handle quick restart
		if (((key == (KEYCODE)OsuKeyBindings::QUICK_RETRY.getInt() || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown() && key == KEY_R)) && !m_bQuickRetryDown))
		{
			m_bQuickRetryDown = true;
			m_fQuickRetryTime = engine->getTime() + osu_quick_retry_delay.getFloat();
		}

		// handle seeking
		if (key == (KEYCODE)OsuKeyBindings::SEEK_TIME.getInt())
			m_bSeekKey = true;
	}

	// forward to all subsystem, if not already consumed
	for (int i=0; i<m_screens.size(); i++)
	{
		if (key.isConsumed())
			break;

		m_screens[i]->onKeyDown(key);
	}

	// special handling, after subsystems, if still not consumed
	if (!key.isConsumed())
	{
		// if playing
		if (isInPlayMode())
		{
			// toggle pause menu
			if ((key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE) && !m_bEscape)
			{
				if (!getSelectedBeatmap()->hasFailed() || !m_pauseMenu->isVisible()) // you can open the pause menu while the failing animation is happening, but never close it then
				{
					m_bEscape = true;
					getSelectedBeatmap()->pause();
					m_pauseMenu->setVisible(getSelectedBeatmap()->isPaused());

					key.consume();
				}
				else // pressing escape while in failed pause menu
				{
					getSelectedBeatmap()->stop(true);
				}
			}

			// local offset
			if (key == (KEYCODE)OsuKeyBindings::INCREASE_LOCAL_OFFSET.getInt())
			{
				long offsetAdd = engine->getKeyboard()->isAltDown() ? 1 : 5;
				getSelectedBeatmap()->getSelectedDifficulty()->localoffset += offsetAdd;
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));
			}
			if (key == (KEYCODE)OsuKeyBindings::DECREASE_LOCAL_OFFSET.getInt())
			{
				long offsetAdd = -(engine->getKeyboard()->isAltDown() ? 1 : 5);
				getSelectedBeatmap()->getSelectedDifficulty()->localoffset += offsetAdd;
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));
			}

			// mania scroll speed
			/*
			if (key == (KEYCODE)OsuKeyBindings::INCREASE_SPEED.getInt())
			{
				ConVar *maniaSpeed = convar->getConVarByName("osu_mania_speed");
				maniaSpeed->setValue(clamp<float>(std::round((maniaSpeed->getFloat() + 0.05f) * 100.0f) / 100.0f, 0.05f, 10.0f));
				m_notificationOverlay->addNotification(UString::format("osu!mania speed set to %gx (fixed)", maniaSpeed->getFloat()));
			}
			if (key == (KEYCODE)OsuKeyBindings::DECREASE_SPEED.getInt())
			{
				ConVar *maniaSpeed = convar->getConVarByName("osu_mania_speed");
				maniaSpeed->setValue(clamp<float>(std::round((maniaSpeed->getFloat() - 0.05f) * 100.0f) / 100.0f, 0.05f, 10.0f));
				m_notificationOverlay->addNotification(UString::format("osu!mania speed set to %gx (fixed)", maniaSpeed->getFloat()));
			}
			*/
		}

		// if playing or not playing

		// volume
		if (key == (KEYCODE)OsuKeyBindings::INCREASE_VOLUME.getInt())
			volumeUp();
		if (key == (KEYCODE)OsuKeyBindings::DECREASE_VOLUME.getInt())
			volumeDown();
	}
}

void Osu::onKeyUp(KeyboardEvent &key)
{
	if (isInPlayMode())
	{
		if (!getSelectedBeatmap()->isPaused())
		{
			getSelectedBeatmap()->onKeyUp(key);
		}

		if (key == (KEYCODE)OsuKeyBindings::MOVE_FORWARD.getInt()) {
			movingForward = false;
			key.consume();
		} else if (key == (KEYCODE)OsuKeyBindings::MOVE_BACKWARD.getInt()) {
			movingBackward = false;
			key.consume();
		} else if (key == (KEYCODE)OsuKeyBindings::MOVE_LEFT.getInt()) {
			movingLeft = false;
			key.consume();
		} else if (key == (KEYCODE)OsuKeyBindings::MOVE_RIGHT.getInt()) {
			movingRight = false;
			key.consume();
		}
	}

	// clicks
	if (key == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt() && m_bKeyboardKey1Down)
	{
		m_bKeyboardKey1Down = false;
		if (isInPlayMode())
			onKey1Change(false, false);
	}
	if (key == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt() && m_bKeyboardKey2Down)
	{
		m_bKeyboardKey2Down = false;
		if (isInPlayMode())
			onKey2Change(false, false);
	}

	// forward to all subsystems, if not consumed
	for (int i=0; i<m_screens.size(); i++)
	{
		if (key.isConsumed())
			break;

		m_screens[i]->onKeyUp(key);
	}

	// misc hotkeys release
	if (key == KEY_F1)
		m_bF1 = false;
	if (key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE)
		m_bEscape = false;
	if (key == KEY_SHIFT)
		m_bUIToggleCheck = false;
	if (key == KEY_TAB)
	{
		m_bTab = false;
		m_bUIToggleCheck = false;
	}
	if (key == (KEYCODE)OsuKeyBindings::QUICK_RETRY.getInt() || key == KEY_R)
		m_bQuickRetryDown = false;
	if (key == (KEYCODE)OsuKeyBindings::SEEK_TIME.getInt())
		m_bSeekKey = false;
}

void Osu::onChar(KeyboardEvent &e)
{
	for (int i=0; i<m_screens.size(); i++)
	{
		if (e.isConsumed())
			break;

		m_screens[i]->onChar(e);
	}
}

void Osu::onLeftChange(bool down)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool())
		return;

	if (!m_bMouseKey1Down && down)
	{
		m_bMouseKey1Down = true;
		onKey1Change(true, true);
	}
	else if (m_bMouseKey1Down)
	{
		m_bMouseKey1Down = false;
		onKey1Change(false, true);
	}
}

void Osu::onRightChange(bool down)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool())
		return;

	if (!m_bMouseKey2Down && down)
	{
		m_bMouseKey2Down = true;
		onKey2Change(true, true);
	}
	else if (m_bMouseKey2Down)
	{
		m_bMouseKey2Down = false;
		onKey2Change(false, true);
	}
}

void Osu::toggleModSelection(bool waitForF1KeyUp)
{
	m_bToggleModSelectionScheduled = true;
	m_modSelector->setWaitForF1KeyUp(waitForF1KeyUp);
}

void Osu::toggleSongBrowser()
{
	m_bToggleSongBrowserScheduled = true;
}

void Osu::toggleOptionsMenu()
{
	m_bToggleOptionsMenuScheduled = true;
}

void Osu::toggleRankingScreen()
{
	m_bToggleRankingScreenScheduled = true;
}

void Osu::toggleVRTutorial()
{
	m_bToggleVRTutorialScheduled = true;
}

void Osu::toggleChangelog()
{
	m_bToggleChangelogScheduled = true;
}

void Osu::toggleEditor()
{
	m_bToggleEditorScheduled = true;
}

void Osu::volumeDown()
{
	float newVolume = clamp<float>(osu_volume_master.getFloat() - osu_volume_change_interval.getFloat(), 0.0f, 1.0f);
	osu_volume_master.setValue(newVolume);
	m_hud->animateVolumeChange();
}

void Osu::volumeUp()
{
	float newVolume = clamp<float>(osu_volume_master.getFloat() + osu_volume_change_interval.getFloat(), 0.0f, 1.0f);
	osu_volume_master.setValue(newVolume);
	m_hud->animateVolumeChange();
}

void Osu::saveScreenshot()
{
	engine->getSound()->play(m_skin->getShutter());
	int screenshotNumber = 0;
	while (env->fileExists(UString::format("screenshots/screenshot%i.png", screenshotNumber)))
	{
		screenshotNumber++;
	}
	std::vector<unsigned char> pixels = engine->getGraphics()->getScreenshot();
	Image::saveToImage(&pixels[0], engine->getGraphics()->getResolution().x, engine->getGraphics()->getResolution().y, UString::format("screenshots/screenshot%i.png", screenshotNumber));
}



void Osu::onBeforePlayStart()
{
	debugLog("Osu::onBeforePlayStart()\n");

	engine->getSound()->play(m_skin->getMenuHit());

	makePlayfield();
	updateMods();
}

void Osu::onPlayStart()
{
	debugLog("Osu::onPlayStart()\n");

	if (m_bModAuto || m_bModAutopilot)
	{
		env->setCursorVisible(true);
		m_bShouldCursorBeVisible = true;
	}

	if (getSelectedBeatmap()->getSelectedDifficulty()->localoffset != 0)
		m_notificationOverlay->addNotification(UString::format("Using local beatmap offset (%ld ms)", getSelectedBeatmap()->getSelectedDifficulty()->localoffset));

	m_fQuickSaveTime = 0.0f; // reset

	updateConfineCursor();
}

void Osu::onPlayEnd(bool quit)
{
	debugLog("Osu::onPlayEnd()\n");

	if (!quit)
	{
		if (!osu_mod_endless.getBool())
		{
			m_rankingScreen->setScore(m_score);
			m_rankingScreen->setBeatmapInfo(getSelectedBeatmap(), getSelectedBeatmap()->getSelectedDifficulty());
			engine->getSound()->play(m_skin->getApplause());
		}
		else
		{
			m_bScheduleEndlessModNextBeatmap = true;
			return; // nothing more to do here
		}
	}

	m_mainMenu->setVisible(false);
	m_modSelector->setVisible(false);
	m_pauseMenu->setVisible(false);

	env->setCursorVisible(false);
	m_bShouldCursorBeVisible = false;

	if (m_songBrowser2 != NULL)
		m_songBrowser2->onPlayEnd(quit);

	if (quit)
		toggleSongBrowser();
	else
		toggleRankingScreen();

	updateConfineCursor();
}


OsuBeatmap *Osu::getSelectedBeatmap()
{
	if (m_songBrowser2 != NULL)
		return m_songBrowser2->getSelectedBeatmap();
	return NULL;
}

float Osu::getDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.4f;
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getCSDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.3f; // different!
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getScoreMultiplier()
{
	float multiplier = 1.0f;

	if (m_bModEZ/* || m_bModNF*/) // TODO: commented until proper drain is implemented
		multiplier *= 0.5f;
	if (m_bModHT || m_bModDC)
		multiplier *= 0.3f;
	if (m_bModHR)
		multiplier *= 1.06f;
	if (m_bModDT || m_bModNC)
		multiplier *= 1.12f;
	if (m_bModHD)
		multiplier *= 1.06f;
	if (m_bModSpunout)
		multiplier *= 0.9f;

	return multiplier;
}

float Osu::getRawSpeedMultiplier()
{
	float speedMultiplier = 1.0f;

	if (m_bModDT || m_bModNC || m_bModHT || m_bModDC)
	{
		if (m_bModDT || m_bModNC)
			speedMultiplier = 1.5f;
		else
			speedMultiplier = 0.75f;
	}

	return speedMultiplier;
}

float Osu::getSpeedMultiplier()
{
	float speedMultiplier = getRawSpeedMultiplier();

	if (osu_speed_override.getFloat() >= 0.0f)
		return osu_speed_override.getFloat();

	return speedMultiplier;
}

float Osu::getPitchMultiplier()
{
	float pitchMultiplier = 1.0f;

	if (m_bModDC)
		pitchMultiplier = 0.92f;

	if (m_bModNC)
		pitchMultiplier = 1.1166f;

	if (osu_pitch_override.getFloat() > 0.0f)
		return osu_pitch_override.getFloat();

	return pitchMultiplier;
}

bool Osu::isInPlayMode()
{
	return (m_songBrowser2 != NULL && m_songBrowser2->hasSelectedAndIsPlaying());
}

bool Osu::isNotInPlayModeOrPaused()
{
	return !isInPlayMode() || (getSelectedBeatmap() != NULL && getSelectedBeatmap()->isPaused());
}

bool Osu::isInVRMode()
{
	return osu_vr.getBool() && openvr->isReady();
}

bool Osu::shouldFallBackToLegacySliderRenderer()
{
	return m_osu_mod_wobble_ref->getBool() || m_osu_mod_wobble2_ref->getBool() || m_osu_mod_minimize_ref->getBool() || m_modSelector->isCSOverrideSliderActive()/* || (m_osu_playfield_rotation->getFloat() < -0.01f || m_osu_playfield_rotation->getFloat() > 0.01f)*/;
}



void Osu::onResolutionChanged(Vector2 newResolution)
{
	if (engine->isMinimized()) // ignore if minimized
		return;

	if (!osu_resolution_enabled.getBool())
		g_vInternalResolution = newResolution;
	else if (!engine->isMinimized()) // if we just got minimized, ignore the resolution change (for the internal stuff)
	{
		// clamp upwards to internal resolution (osu_resolution)
		if (g_vInternalResolution.x < m_vInternalResolution.x)
			g_vInternalResolution.x = m_vInternalResolution.x;
		if (g_vInternalResolution.y < m_vInternalResolution.y)
			g_vInternalResolution.y = m_vInternalResolution.y;

		// clamp downwards to engine resolution
		if (newResolution.x < g_vInternalResolution.x)
			g_vInternalResolution.x = newResolution.x;
		if (newResolution.y < g_vInternalResolution.y)
			g_vInternalResolution.y = newResolution.y;

		// disable internal resolution on specific conditions
		bool windowsBorderlessHackCondition = (env->getOS() == Environment::OS::OS_WINDOWS && env->isFullscreen() && env->isFullscreenWindowedBorderless() && (int)g_vInternalResolution.y == (int)env->getNativeScreenSize().y); // HACKHACK
		if (((int)g_vInternalResolution.x == engine->getScreenWidth() && (int)g_vInternalResolution.y == engine->getScreenHeight()) || !env->isFullscreen() || windowsBorderlessHackCondition)
		{
			debugLog("Internal resolution == Engine resolution || !Fullscreen, disabling resampler (%i, %i)\n", (int)(g_vInternalResolution == engine->getScreenSize()), (int)(!env->isFullscreen()));
			osu_resolution_enabled.setValue(0.0f);
			g_vInternalResolution = engine->getScreenSize();
		}
	}

	// interfaces
	for (int i=0; i<m_screens.size(); i++)
	{
		m_screens[i]->onResolutionChange(g_vInternalResolution);
	}

	// rendertargets
	m_frameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	m_backBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	m_backBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);

	// mouse scaling & offset
	// TODO: rethink scale logic
	if (osu_resolution_enabled.getBool())
	{
		if (osu_letterboxing.getBool())
		{
			engine->getMouse()->setOffset(-Vector2(engine->getScreenWidth()/2 - g_vInternalResolution.x/2, engine->getScreenHeight()/2 - g_vInternalResolution.y/2));
			engine->getMouse()->setScale(Vector2(1,1));
		}
		else
		{
			engine->getMouse()->setOffset(Vector2(0,0));
			engine->getMouse()->setScale(Vector2(1,1));
		}
	}
	else
	{
		engine->getMouse()->setOffset(Vector2(0,0));
		engine->getMouse()->setScale(Vector2(1,1));
	}

	// cursor clipping
	updateConfineCursor();
}

void Osu::onInternalResolutionChanged(UString oldValue, UString args)
{
	if (args.length() < 7)
		return;

	std::vector<UString> resolution = args.split("x");
	if (resolution.size() != 2)
		debugLog("Error: Invalid parameter count for command 'osu_resolution'! (Usage: e.g. \"osu_resolution 1280x720\")");
	else
	{
		int width = resolution[0].toFloat();
		int height = resolution[1].toFloat();

		if (width < 300 || height < 240)
			debugLog("Error: Invalid values for resolution for command 'osu_resolution'!");
		else
		{
			Vector2 newInternalResolution = Vector2(width, height);

			// clamp requested internal resolution to current renderer resolution
			// however, this could happen while we are transitioning into fullscreen. therefore only clamp when not in fullscreen or not in fullscreen transition
			bool isTransitioningIntoFullscreenHack = engine->getGraphics()->getResolution().x < env->getNativeScreenSize().x || engine->getGraphics()->getResolution().y < env->getNativeScreenSize().y;
			if (!env->isFullscreen() || !isTransitioningIntoFullscreenHack)
			{
				if (newInternalResolution.x > engine->getGraphics()->getResolution().x)
					newInternalResolution.x = engine->getGraphics()->getResolution().x;
				if (newInternalResolution.y > engine->getGraphics()->getResolution().y)
					newInternalResolution.y = engine->getGraphics()->getResolution().y;
			}

			// enable and store, then force onResolutionChanged()
			osu_resolution_enabled.setValue(1.0f);
			g_vInternalResolution = newInternalResolution;
			m_vInternalResolution = newInternalResolution;
			onResolutionChanged(newInternalResolution);
		}
	}
}

void Osu::onFocusGained()
{
	// cursor clipping
	updateConfineCursor();
}

void Osu::onFocusLost()
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_pause_on_focus_loss.getBool())
	{
		getSelectedBeatmap()->pause(false);
		m_pauseMenu->setVisible(true);
		m_modSelector->setVisible(false);
	}

	// release cursor clip
	env->setCursorClip(false, McRect());
}

bool Osu::onShutdown()
{
	// the only time where a shutdown could be problematic is while an update is being installed, so we block it here
	return m_updateHandler == NULL || m_updateHandler->getStatus() != OsuUpdateHandler::STATUS::STATUS_INSTALLING_UPDATE;
}

void Osu::onSkinReload()
{
	onSkinChange("", osu_skin.getString());
}

void Osu::onSkinChange(UString oldValue, UString newValue)
{
	if (newValue.length() > 1)
	{
		SAFE_DELETE(m_skin);

		UString skinFolder = m_osu_folder_ref->getString();
		skinFolder.append("Skins/");
		skinFolder.append(newValue);
		skinFolder.append("/");
		m_skin = new OsuSkin(this, skinFolder);

		// force
		onResolutionChanged(g_vInternalResolution);
	}
}

void Osu::onMasterVolumeChange(UString oldValue, UString newValue)
{
	float newVolume = newValue.toFloat();
	engine->getSound()->setVolume(newVolume);
}

void Osu::onMusicVolumeChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->setVolume(newValue.toFloat());
}

void Osu::onSpeedChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float speed = newValue.toFloat();
		getSelectedBeatmap()->setSpeed(speed >= 0.0f ? speed : getSpeedMultiplier());
	}
}

void Osu::onPitchChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float pitch = newValue.toFloat();
		getSelectedBeatmap()->setPitch(pitch > 0.0f ? pitch : getPitchMultiplier());
	}
}

void Osu::onPlayfieldChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->onModUpdate();
}

void Osu::onLetterboxingChange(UString oldValue, UString newValue)
{
	if (osu_resolution_enabled.getBool())
	{
		bool oldVal = oldValue.toFloat() > 0.0f;
		bool newVal = newValue.toFloat() > 0.0f;

		if (oldVal != newVal)
			Osu::onResolutionChanged(g_vInternalResolution);
	}
}

void Osu::updateConfineCursor()
{
	if (isInVRMode())
		return;

	if ((osu_confine_cursor_fullscreen.getBool() && env->isFullscreen()) || (osu_confine_cursor_windowed.getBool() && !env->isFullscreen()) || (isInPlayMode() && !getSelectedBeatmap()->isPaused() && !getModAuto() && !getModAutopilot()))
		env->setCursorClip(true, McRect());
	else
		env->setCursorClip(false, McRect());
}

void Osu::onConfineCursorWindowedChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onConfineCursorFullscreenChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onKey1Change(bool pressed, bool mouse)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused())
	{
		if (!(mouse && osu_disable_mousebuttons.getBool()))
		{
			// quickfix
			if (osu_disable_mousebuttons.getBool())
				m_bMouseKey1Down = false;

			if (pressed && !(m_bKeyboardKey1Down && m_bMouseKey1Down))
				getSelectedBeatmap()->keyPressed1();
			else if (!m_bKeyboardKey1Down && !m_bMouseKey1Down)
				getSelectedBeatmap()->keyReleased1();
		}
	}

	// TODO: put the animations inside OsuBeatmap::keyPressed/released
	bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());

	if (doAnimate)
	{
		if (pressed && !(m_bKeyboardKey1Down && m_bMouseKey1Down))
			m_hud->animateCursorExpand();
		else if (!m_bKeyboardKey1Down && !m_bMouseKey1Down && !m_bKeyboardKey2Down && !m_bMouseKey2Down)
			m_hud->animateCursorShrink();
	}
}

void Osu::onKey2Change(bool pressed, bool mouse)
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused())
	{
		if (!(mouse && osu_disable_mousebuttons.getBool()))
		{
			// quickfix
			if (osu_disable_mousebuttons.getBool())
				m_bMouseKey2Down = false;

			if (pressed && !(m_bKeyboardKey2Down && m_bMouseKey2Down))
				getSelectedBeatmap()->keyPressed2();
			else if (!m_bKeyboardKey2Down && !m_bMouseKey2Down)
				getSelectedBeatmap()->keyReleased2();
		}
	}

	// TODO: put the animations inside OsuBeatmap::keyPressed/released
	bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());

	if (doAnimate)
	{
		if (pressed && !(m_bKeyboardKey2Down && m_bMouseKey2Down))
			m_hud->animateCursorExpand();
		else if (!m_bKeyboardKey2Down && !m_bMouseKey2Down && !m_bKeyboardKey1Down && !m_bMouseKey1Down)
			m_hud->animateCursorShrink();
	}
}



float Osu::getImageScaleToFitResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x > resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFitResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFitResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScaleToFillResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x < resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFillResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFillResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScale(Osu *osu, Vector2 size, float osuSize)
{
	int swidth = osu->getScreenWidth();
	int sheight = osu->getScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuSize*xMultiplier;
	const float yDiameter = osuSize*yMultiplier;

	return xDiameter/size.x > yDiameter/size.y ? xDiameter/size.x : yDiameter/size.y;
}

float Osu::getImageScale(Osu *osu, Image *img, float osuSize)
{
	return getImageScale(osu, Vector2(img->getWidth(), img->getHeight()), osuSize);
}

float Osu::getUIScale(Osu *osu, float osuResolutionRatio)
{
	int swidth = osu->getScreenWidth();
	int sheight = osu->getScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuResolutionRatio*xMultiplier;
	const float yDiameter = osuResolutionRatio*yMultiplier;

	return xDiameter > yDiameter ? xDiameter : yDiameter;
}

bool Osu::findIgnoreCase(const std::string &haystack, const std::string &needle)
{
	auto it = std::search(
	    haystack.begin(), haystack.end(),
		needle.begin(),   needle.end(),
	    [](char ch1, char ch2)
		{return std::tolower(ch1) == std::tolower(ch2);}
	);

	return (it != haystack.end());
}

void Osu::subdivide(std::list<VertexPair> meshList, const std::list<VertexPair>::iterator begin, const std::list<VertexPair>::iterator end, int n, float edgeDistance) {
	Vector3 a = Vector3((*begin).a.x, 0, (*begin).a.z);
	Vector3 b = Vector3((*end).a.x, 0, (*end).a.z);
	Vector3 middlePoint = Vector3(lerp<float>(a.x, b.x, 0.5),
								  lerp<float>(a.y, b.y, 0.5),
								  lerp<float>(a.z, b.z, 0.5));

	if(fposu_curved.getBool())
		middlePoint.setLength(edgeDistance);

	Vector3 top, bottom;
	top = bottom = middlePoint;
	top.y = (*begin).a.y;
	bottom.y = (*begin).b.y;
	float tc = lerp<float>((*begin).textureCoordinate, (*end).textureCoordinate, 0.5);
	VertexPair newVP = VertexPair(top, bottom, tc);
	//debugLog("New VP: %f \n", newVP.a.distance(Vector3(0,0,0)));
	//std::cout << "New VP: " << newVP.a.distance(Vector3(0,0,0)) <<"\n";
	std::list<VertexPair>::iterator newPos = meshList.insert(end,newVP);
	if(n > 1) {
		subdivide(meshList, begin, newPos, n-1, edgeDistance);
		subdivide(meshList, newPos, end, n-1, edgeDistance);
	} else {
		circumlength += (*begin).a.distance(newVP.a);
		circumlength += newVP.a.distance((*end).a);
	}

}

Vector3 Osu::NormalFromTriangle(Vector3 p1, Vector3 p2, Vector3 p3) {
	Vector3 U = p2-p1;
	Vector3 V = p3-p1;

	Vector3 Normal;
	Normal.x = (U.y * V.z) - (U.z * V.y);
	Normal.y = (U.z * V.x) - (U.x * V.z);
	Normal.z = (U.x * V.y) - (U.y * V.x);

	return Normal.normalize();
}


void Osu::makePlayfield() {
	float dist = -fposu_distance.getFloat();
	circumlength = 0;
	//VertexPair vp1 = VertexPair(Vector3(-0.5,0.5,dist),Vector3(-0.5,-0.5, dist), 0);
	//VertexPair vp2 = VertexPair(Vector3(0.5,0.5, dist),Vector3(0.5,-0.5, dist), 1);
	VertexPair vp1 = VertexPair(Vector3(-0.5,0.5,dist),Vector3(-0.5,-0.5, dist), 0);
	VertexPair vp2 = VertexPair(Vector3(0.5,0.5, dist),Vector3(0.5,-0.5, dist), 1);
	float edgeDistance = Vector3(0,0,0).distance(Vector3(-0.5,0.0,dist));
	meshList.clear();
	meshList.push_back(vp1);
	meshList.push_back(vp2);
	std::list<VertexPair>::iterator begin = meshList.begin();
	std::list<VertexPair>::iterator end = meshList.end();
	--end;
	subdivide(meshList, begin, end, subdivisions, edgeDistance);

	//debugLog("circumlength: %f \n", circumlength);

	arrayObject.clear();
	begin = meshList.begin();
	std::list<VertexPair>::iterator next = ++meshList.begin();
	while(next != meshList.end()) {
		Vector3 topLeft = (*begin).a;
		Vector3 bottomLeft = (*begin).b;
		Vector3 topRight = (*next).a;
		Vector3 bottomRight = (*next).b;
		float leftTC = (*begin).textureCoordinate;
		float rightTC = (*next).textureCoordinate;
		float topTC = 1;
		float bottomTC = 0;

		arrayObject.addVertex(topLeft);
		arrayObject.addTexcoord(leftTC, topTC);
		arrayObject.addVertex(topRight);
		arrayObject.addTexcoord(rightTC, topTC);
		arrayObject.addVertex(bottomLeft);
		arrayObject.addTexcoord(leftTC, bottomTC);

		arrayObject.addVertex(bottomLeft);
		arrayObject.addTexcoord(leftTC, bottomTC);
		arrayObject.addVertex(topRight);
		arrayObject.addTexcoord(rightTC, topTC);
		arrayObject.addVertex(bottomRight);
		arrayObject.addTexcoord(rightTC, bottomTC);

		(*begin).normal = NormalFromTriangle(topLeft, topRight, bottomLeft);


		begin++;
		next++;
	}



	camera = new Camera(Vector3(0,0,0),Vector3(0,0,-1),103.0f);
	//camera->setType(Camera::CAMERA_TYPE_FIRST_PERSON);
	camera->setFov(103.0f);
	float aspect = m_backBuffer->getHeight() / m_backBuffer->getWidth();
	modelMatrix = Matrix4().translate(0, 0, 0).scale(1, aspect*(circumlength), 1);//-fposu_distance.getFloat() scale(2, 2*aspect, 1)
	//projectionMatrix = Camera::buildMatrixPerspectiveFov(deg2rad(fposu_fov.getFloat()),
	//			((float) engine->getScreenWidth())/((float) engine->getScreenHeight()),
	//			0.05f, 100.0f);
}

void Osu::makeAngleCircle() {
	std::list<VertexPair>* vplist = new std::list<VertexPair>();
	VertexPair vp1 = VertexPair(Vector3(-1.0, 0.5, 0.0), Vector3(-1.0,-0.5, 0.0), 0);
	VertexPair vp2 = VertexPair(Vector3( 0.0, 0.5,-1.0), Vector3( 0.0,-0.5,-1.0), 1);
	VertexPair vp3 = VertexPair(Vector3( 1.0, 0.5, 0.0), Vector3( 1.0,-0.5, 0.0), 2);
	VertexPair vp4 = VertexPair(Vector3( 0.0, 0.5, 1.0), Vector3( 0.0,-0.5, 1.0), 3);
	VertexPair vp5 = VertexPair(Vector3(-1.0, 0.5, 0.0), Vector3(-1.0,-0.5, 0.0), 4);
	vplist->push_back(vp1);
	vplist->push_back(vp2);
	vplist->push_back(vp3);
	vplist->push_back(vp4);
	vplist->push_back(vp5);
	float edgeDistance = 1.0;

	std::list<VertexPair>::iterator current = (*vplist).begin();
	std::list<VertexPair>::iterator next = ++(*vplist).begin();
	while(next != vplist->end()) {
		subdivide(*vplist, current, next, subdivisions, edgeDistance);
		current=next;
		++next;
	}

	std::list<VertexPair>::iterator begin = (*vplist).begin();
	next = ++(*vplist).begin();
	while(next != (*vplist).end()) {
		Vector3 topLeft = (*begin).a;
		Vector3 bottomLeft = (*begin).b;
		Vector3 topRight = (*next).a;
		Vector3 bottomRight = (*next).b;
		float leftTC = (*begin).textureCoordinate;
		float rightTC = (*next).textureCoordinate;
		float topTC = 1;
		float bottomTC = 0;

		arrayObject2.addVertex(topLeft);
		arrayObject2.addTexcoord(leftTC, topTC);
		arrayObject2.addVertex(topRight);
		arrayObject2.addTexcoord(rightTC, topTC);
		arrayObject2.addVertex(bottomLeft);
		arrayObject2.addTexcoord(leftTC, bottomTC);

		arrayObject2.addVertex(bottomLeft);
		arrayObject2.addTexcoord(leftTC, bottomTC);
		arrayObject2.addVertex(topRight);
		arrayObject2.addTexcoord(rightTC, topTC);
		arrayObject2.addVertex(bottomRight);
		arrayObject2.addTexcoord(rightTC, bottomTC);

		(*begin).normal = NormalFromTriangle(topLeft, topRight, bottomLeft);


		begin++;
		next++;
	}
	delete(vplist);
}

