//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

#ifndef OSU_H
#define OSU_H

#include "App.h"
#include "MouseListener.h"

#include <list>

#include "VertexArrayObject.h"

#include "Quaternion.h"
#include "OpenGLImage.h"
#include "Image.h"

class CWindowManager;

class OsuVR;
class OsuMainMenu;
class OsuPauseMenu;
class OsuOptionsMenu;
class OsuModSelector;
class OsuSongBrowser2;
class OsuRankingScreen;
class OsuUpdateHandler;
class OsuNotificationOverlay;
class OsuTooltipOverlay;
class OsuBeatmap;
class OsuScreen;
class OsuScore;
class OsuSkin;
class OsuHUD;
class OsuVRTutorial;
class OsuChangelog;
class OsuEditor;

class ConVar;
class Image;
class McFont;
class RenderTarget;
class Camera;

class Osu : public App, public MouseListener
{
public:
	static ConVar *version;
	static ConVar *debug;
	static bool autoUpdater;

	static Vector2 osuBaseResolution;

	static float getImageScaleToFitResolution(Image *img, Vector2 resolution);
	static float getImageScaleToFitResolution(Vector2 size, Vector2 resolution);
	static float getImageScaleToFillResolution(Vector2 size, Vector2 resolution);
	static float getImageScaleToFillResolution(Image *img, Vector2 resolution);
	static float getImageScale(Osu *osu, Vector2 size, float osuSize);
	static float getImageScale(Osu *osu, Image *img, float osuSize);
	static float getUIScale(Osu *osu, float osuResolutionRatio);

	static bool findIgnoreCase(const std::string &haystack, const std::string &needle);

	enum class GAMEMODE
	{
		STD,
		MANIA
	};

public:
	Osu();
	virtual ~Osu();

	virtual void draw(Graphics *g);
	void drawVR(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);
	virtual void onChar(KeyboardEvent &e);

	void onLeftChange(bool down);
	void onMiddleChange(bool down){;}
	void onRightChange(bool down);

	void onWheelVertical(int delta){;}
	void onWheelHorizontal(int delta){;}

	virtual void onResolutionChanged(Vector2 newResolution);
	virtual void onFocusGained();
	virtual void onFocusLost();
	virtual bool onShutdown();

	void onBeforePlayStart();			// called just before OsuBeatmap->play()
	void onPlayStart();					// called when a beatmap has successfully started playing
	void onPlayEnd(bool quit = true);	// called when a beatmap is finished playing (or the player quit)

	void toggleModSelection(bool waitForF1KeyUp = false);
	void toggleSongBrowser();
	void toggleOptionsMenu();
	void toggleRankingScreen();
	void toggleVRTutorial();
	void toggleChangelog();
	void toggleEditor();

	void volumeDown();
	void volumeUp();

	void saveScreenshot();

	void reloadSkin() {onSkinReload();}

	void setGamemode(GAMEMODE gamemode) {m_gamemode = gamemode;}

	inline GAMEMODE getGamemode() const {return m_gamemode;}

	inline Vector2 getScreenSize() const {return g_vInternalResolution;}
	inline int getScreenWidth() const {return (int)g_vInternalResolution.x;}
	inline int getScreenHeight() const {return (int)g_vInternalResolution.y;}

	OsuBeatmap *getSelectedBeatmap();

	inline OsuVR *getVR() {return m_vr;}
	inline OsuSkin *getSkin() {return m_skin;}
	inline OsuHUD *getHUD() {return m_hud;}
	inline OsuNotificationOverlay *getNotificationOverlay() {return m_notificationOverlay;}
	inline OsuTooltipOverlay *getTooltipOverlay() {return m_tooltipOverlay;}
	inline OsuModSelector *getModSelector() {return m_modSelector;}
	inline OsuPauseMenu *getPauseMenu() {return m_pauseMenu;}
	inline OsuRankingScreen *getRankingScreen() {return m_rankingScreen;}
	inline OsuScore *getScore() {return m_score;}
	inline OsuUpdateHandler *getUpdateHandler() {return m_updateHandler;}

	inline RenderTarget *getFrameBuffer() {return m_frameBuffer;}
	inline McFont *getTitleFont() {return m_titleFont;}
	inline McFont *getSubTitleFont() {return m_subTitleFont;}
	inline McFont *getSongBrowserFont() {return m_songBrowserFont;}
	inline McFont *getSongBrowserFontBold() {return m_songBrowserFontBold;}

	float getDifficultyMultiplier();
	float getCSDifficultyMultiplier();
	float getScoreMultiplier();
	float getRawSpeedMultiplier();	// without override
	float getSpeedMultiplier();		// with override
	float getPitchMultiplier();

	inline bool getModAuto() const {return m_bModAuto;}
	inline bool getModAutopilot() const {return m_bModAutopilot;}
	inline bool getModRelax() const {return m_bModRelax;}
	inline bool getModSpunout() const {return m_bModSpunout;}
	inline bool getModTarget() const {return m_bModTarget;}
	inline bool getModDT() const {return m_bModDT;}
	inline bool getModNC() const {return m_bModNC;}
	inline bool getModNF() const {return m_bModNF;}
	inline bool getModHT() const {return m_bModHT;}
	inline bool getModDC() const {return m_bModDC;}
	inline bool getModHD() const {return m_bModHD;}
	inline bool getModHR() const {return m_bModHR;}
	inline bool getModEZ() const {return m_bModEZ;}
	inline bool getModSD() const {return m_bModSD;}
	inline bool getModSS() const {return m_bModSS;}
	inline bool getModNM() const {return m_bModNM;}

	bool isInPlayMode();
	bool isNotInPlayModeOrPaused();
	bool isInVRMode();

	inline bool isSeeking() const {return m_bSeeking;}
	inline float getQuickSaveTime() const {return m_fQuickSaveTime;}

	bool shouldFallBackToLegacySliderRenderer(); // certain mods or actions require OsuSliders to render dynamically (e.g. wobble or the CS override slider)

	void updateMods();
	void updateConfineCursor();

	inline RenderTarget* getBackBuffer() const {return m_backBuffer;}
	inline RenderTarget* getframeBuffer() const {return m_frameBuffer;}

private:
	static Vector2 g_vInternalResolution;

	void updateModsForConVarTemplate(UString oldValue, UString newValue) {updateMods();}

	// callbacks
	void onInternalResolutionChanged(UString oldValue, UString args);

	void onSkinReload();
	void onSkinChange(UString oldValue, UString newValue);

	void onMasterVolumeChange(UString oldValue, UString newValue);
	void onMusicVolumeChange(UString oldValue, UString newValue);
	void onSpeedChange(UString oldValue, UString newValue);
	void onPitchChange(UString oldValue, UString newValue);

	void onPlayfieldChange(UString oldValue, UString newValue);

	void onLetterboxingChange(UString oldValue, UString newValue);

	void onConfineCursorWindowedChange(UString oldValue, UString newValue);
	void onConfineCursorFullscreenChange(UString oldValue, UString newValue);

	void onKey1Change(bool pressed, bool mouse);
	void onKey2Change(bool pressed, bool mouse);

	struct VertexPair {
		Vector3 a;
		Vector3 b;
		float textureCoordinate;
		Vector3 normal;
		VertexPair(Vector3 a, Vector3 b, float tc) : a(a), b(b), textureCoordinate(tc) {};
	};

	std::list<VertexPair> meshList;

	void makePlayfield();
	void makeAngleCircle();
	OpenGLImage* angleImage;
	OpenGLImage* cubeImage;
	void subdivide(std::list<VertexPair> meshList, std::list<VertexPair>::iterator begin, std::list<VertexPair>::iterator end, int n, float edgeDistance);
	Vector3 NormalFromTriangle(Vector3 p1, Vector3 p2, Vector3 p3);
	inline bool isZero(float x)
	{
	  const double epsilon = 1e-5 ;
	  return std::abs(x-0) <= epsilon * std::abs(x);
	  // see Knuth section 4.2.2 pages 217-218
	}

	// convar refs
	ConVar *m_osu_folder_ref;
	ConVar *m_osu_draw_hud_ref;
	ConVar *m_osu_mod_fps_ref;
	ConVar *m_osu_mod_wobble_ref;
	ConVar *m_osu_mod_wobble2_ref;
	ConVar *m_osu_mod_minimize_ref;
	ConVar *m_osu_playfield_rotation;
	ConVar *m_osu_playfield_stretch_x;
	ConVar *m_osu_playfield_stretch_y;

	// interfaces
	OsuVR *m_vr;
	OsuMainMenu *m_mainMenu;
	OsuOptionsMenu *m_optionsMenu;
	OsuSongBrowser2 *m_songBrowser2;
	OsuModSelector *m_modSelector;
	OsuRankingScreen *m_rankingScreen;
	OsuPauseMenu *m_pauseMenu;
	OsuSkin *m_skin;
	OsuHUD *m_hud;
	OsuTooltipOverlay *m_tooltipOverlay;
	OsuNotificationOverlay *m_notificationOverlay;
	OsuScore *m_score;
	OsuVRTutorial *m_vrTutorial;
	OsuChangelog *m_changelog;
	OsuEditor *m_editor;
	OsuUpdateHandler *m_updateHandler;

	std::vector<OsuScreen*> m_screens;

	// rendering
	RenderTarget *m_frameBuffer;
	RenderTarget *m_backBuffer;
	//RenderTarget *m_backBuffer;
	Vector2 m_vInternalResolution;

	// mods
	bool m_bModAuto;
	bool m_bModAutopilot;
	bool m_bModRelax;
	bool m_bModSpunout;
	bool m_bModTarget;
	bool m_bModDT;
	bool m_bModNC;
	bool m_bModNF;
	bool m_bModHT;
	bool m_bModDC;
	bool m_bModHD;
	bool m_bModHR;
	bool m_bModEZ;
	bool m_bModSD;
	bool m_bModSS;
	bool m_bModNM;

	// keys
	bool m_bF1;
	bool m_bUIToggleCheck;
	bool m_bTab;
	bool m_bEscape;
	bool m_bKeyboardKey1Down;
	bool m_bKeyboardKey2Down;
	bool m_bMouseKey1Down;
	bool m_bMouseKey2Down;
	bool m_bSkipScheduled;
	bool m_bQuickRetryDown;
	float m_fQuickRetryTime;
	bool m_bSeekKey;
	bool m_bSeeking;
	float m_fQuickSaveTime;

	// async toggles
	// TODO: this way of doing things is bullshit
	bool m_bToggleModSelectionScheduled;
	bool m_bToggleSongBrowserScheduled;
	bool m_bToggleOptionsMenuScheduled;
	bool m_bToggleRankingScreenScheduled;
	bool m_bToggleVRTutorialScheduled;
	bool m_bToggleChangelogScheduled;
	bool m_bToggleEditorScheduled;

	// cursor
	bool m_bShouldCursorBeVisible;

	// global resources
	McFont *m_titleFont;
	McFont *m_subTitleFont;
	McFont *m_songBrowserFont;
	McFont *m_songBrowserFontBold;

	// debugging
	CWindowManager *m_windowManager;

	// custom
	GAMEMODE m_gamemode;
	bool m_bScheduleEndlessModNextBeatmap;

	Shader* m_shaderGenericTextured;
	Shader* m_shaderGenericUntextured;
	Shader* m_shaderRay;
	Matrix4 projectionMatrix;
	Matrix4 viewMatrix;
	Camera* camera;

	Vector3 MyCameraPosition = Vector3(0,0,0);
	Vector3 MyCameraDirection = Vector3(0,0,-1);
	float pitch = 0;
	float yaw = 0;
	Quaternion q;
	Quaternion a;
	Quaternion b;
	Vector3 MyCameraUp = Vector3(0,1,0);
	Vector3 MyCameraLeft = Vector3(-1,0,0);


	Matrix4 modelMatrix;
	bool screenIntersection;
	bool clicked = false;


	float circumlength;
	//float edgeDistance;
	int subdivisions = 4; //4
	int currentIntersectingFace = -1;
	VertexArrayObject arrayObject;
	VertexArrayObject arrayObject2;
	VertexArrayObject arrayObjectCube;

	bool movingForward = false;
	bool movingBackward = false;
	bool movingLeft = false;
	bool movingRight = false;

};

#endif
