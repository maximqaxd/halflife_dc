// menu.h -- the front-end menu pages and the passcodes they listen for.

// scr_safe_x / scr_safe_y
#ifndef MENU_H
#define MENU_H

#ifdef __cplusplus
extern "C" {
#endif
// Pad buttons the front end should treat as pressed this frame
#define MAX_MENU_BUTTONS	64
extern int		joymenubuttons[];

// Draw the menu instead of the 3D view this frame
extern int		gfDrawMenu;

// The cheat page hands the game side a pointer to each toggle it puts up, so
// the world can be kept in step with what the page shows.
void	Host_BindCheatFly( int* pValue );
void	Host_BindCheatNoclip( int* pValue );
void	Host_BindCheatNotarget( int* pValue );
void	Host_BindCheatGod( int* pValue );
void	Host_BindCheatSlomo( int* pValue );
void	Host_BindCheatAmmo( int* pValue );
void	Host_BindCheatWeapons( int* pValue );
void	Host_BindCheatEverything( int* pValue );
void	Host_BindCheatHealth( int* pValue );
void	Host_BindCheatGravity( int* pValue );
void	Host_BindCheatAllies( int* pValue );
#ifdef __cplusplus
}
#endif

// The console background, borrowed by a page that comes up mid-game. dc_draw.c
// is built as C++, so this keeps C++ linkage.
extern qpic_t*	conback;

#define MAX_MENU_ITEMS		90
#define MAX_MENU_TEXTURES	32
#define MAX_MENU_NAME		68
#define MAX_MENU_COMMAND	472
#define MAX_MENU_COMMAND_TEXT	68
#define MAX_MENU_PRESET_LABEL	32
#define MAX_MENU_PRESET_DESCRIPTION	64
#define MAX_MENU_PRESET_COMMAND	32

class CMenu;
class CMenuItemBase;

// One entry a page can put on itself. The label and description are language
// tags looked up when the item is drawn; the command is what running the item
// pushes into the console.
typedef struct menuitemdef_s
{
	int			id;
	char*		pszLabel;
	char*		pszCommand;
	char*		pszDescription;
} menuitemdef_t;

// One page of the front end, as the script describes it. items ends at MI_END.
typedef struct menupage_s
{
	char*		pszName;
	char*		pszCommand;
	int			fullscreen;
	int			items[MAX_MENU_ITEMS];
} menupage_t;

#define MAX_MENU_PAGES		29
#define MAX_MENU_ITEMDEFS	94
#define MI_END			0xfe

extern menuitemdef_t	g_MenuItems[];
extern menupage_t		g_MenuPages[];

// A three-button passcode. The cheats unlock a gameplay toggle, the level
// codes warp to a chapter; the name is the language tag the codes page shows.
typedef struct menucode_s
{
	int			button1;
	int			button2;
	int			button3;
	char*		pszName;
	byte		enabled;
	int			result;
} menucode_t;

extern menucode_t g_MenuCodes[];

// The two credit rolls; the second one is what the button code unlocks
extern char*  g_pszCredits[];
extern char*  g_pszCredits2[];

// One value an option can be set to, and where the option draws it.
typedef struct menuvalue_s
{
	char*		pszText;
	int			x;
	int			y;
	float		flScale;
	float		flAspect;
} menuvalue_t;

#define MAX_OPTION_VALUES	50

// One option on a settings page: the values it steps through, which one is
// current, and the help line it shows. pfnBind hands the game side a pointer
// to the option's value so it can keep the world in step with it.
typedef struct menuoption_s
{
	int			id;
	char*		pszValues[MAX_OPTION_VALUES];
	int			nValues;
	int			iValue;
	char*		pszDescription;
	void		(*pfnBind)( int* pValue );
} menuoption_t;

#define MAX_MENU_OPTIONS	22

// A slider: how many notches it has and which one it starts on.
typedef struct menuslider_s
{
	int			id;
	char*		pszLabel;
	int			nSteps;
	int			iValue;
	char*		pszDescription;
} menuslider_t;

#define MAX_MENU_SLIDERS	5

extern menuoption_t		g_MenuOptions[];
extern menuslider_t		g_MenuSliders[];

// The running state of a page: where its fade animation has got to, the
// artwork it has loaded and which item the stick is on.
typedef struct menustate_s
{
	byte		reserved0[40];
	char*		pszCommand;

	// The fade walks from flFadeFrom to flFadeTo over flAnimTime seconds
	// while flAnimating is set.
	float		flAnimating;
	float		flFade;
	float		flFadeFrom;
	float		flFadeTo;
	float		flAnimStart;
	float		flAnimTime;
	float		flTime;
	float		flLastTime;

	// Artwork the page loaded, handed back when the page goes away
	char*		pTextureNames[MAX_MENU_TEXTURES];
	int			iTextures[MAX_MENU_TEXTURES];

	float		flScaleX;
	float		flScaleY;
	int			iVisible;
	int			iEnabled;

	int			iElementTexture;
	int			rgba[4];
	int			iVMUTexture;
	int			iBarneyLargeTexture;
	int			iBarneyTexture;
	int			iGordonLargeTexture;
	int			iGordonTexture;
	int			iControllerTexture;
	int			iControllerSmallTexture;
	int			iLinesTexture;

	int			iSoundBlocked;
	int			iTopItem;
	int			iSelected;
	int			iSelectedRow;
} menustate_t;

// One page of the front end.
class CMenu
{
public:
	static menupage_t* FindPage( char* pszName );
	static menuslider_t* FindSliderDef( int id );
	static menuoption_t* FindOptionDef( int id );
	static menuitemdef_t* FindItemDef( int id );
	CMenu( char* pszMenu );
	~CMenu( void );
	__forceinline void SetColor( int r, int g, int b, int a );

	void ExecuteCommand( char* pszCommand, int fade, int close );
	void Cancel( void );
	__forceinline int CountItems( void );
	__forceinline int SelectedCharacter( void );
	__forceinline void MoveSelection( int direction );
	void SelectPrevious( void );
	void SelectNext( void );
	void DrawControllerIconSmall( float x0, float y0, float x1, float y1 );
	void DrawControllerLines( float x0, float y0, float x1, float y1 );
	__forceinline void RenderControllerLines( float x0, float y0, float x1, float y1 );
	void DrawVMUIcon( float x0, float y0, float x1, float y1 );
	__forceinline void RenderVMUIcon( float x0, float y0, float x1, float y1 );
	void DrawGordonIconLarge( float x0, float y0, float x1, float y1 );
	__forceinline void RenderGordonIconLarge( float x0, float y0, float x1, float y1 );
	void DrawBarneyIconLarge( float x0, float y0, float x1, float y1 );
	__forceinline void RenderBarneyIconLarge( float x0, float y0, float x1, float y1 );
	void ClearTextures( void );
	void FreeTextures( void );
	int FindFreeTextureSlot( void );
	int FindTexture( char* pszName );
	int LoadMenuTexture( char* pszName );
	void DrawMenuElementTile( float x0, float y0, float x1, float y1 );
	__forceinline void DrawElementTile( float x0, float y0, float x1, float y1 );
	void DrawMenuElementTile2( float x0, float y0, float x1, float y1 );
	__forceinline void DrawElementTile2( float x0, float y0, float x1, float y1 );
	void DrawMenuElementBox( float x0, float y0, float x1, float y1 );
	void DrawMenuElementQuad( int orientation, float x0, float y0, float x1, float y1 );
	void DrawMenuElementQuad2( int corner, float x0, float y0, float x1, float y1 );
	void DrawControllerIcon( float x0, float y0, float x1, float y1 );
	void DrawGordonIcon( float x0, float y0, float x1, float y1 );
	void DrawBarneyIcon( float x0, float y0, float x1, float y1 );
	void FadeIn( void );
	void FadeOut( void );
	void AnimateLerp( void );
	void CheckCheatCode( char button );
	void Build( char* pszMenu );
	void Draw( void );
	void Input( void );

	char			m_szName[MAX_MENU_NAME];
	CMenuItemBase*	m_pItems[MAX_MENU_ITEMS];
	char			m_szCommand[MAX_MENU_COMMAND];
	menustate_t		m_state;
};

// Everything a page can put on itself answers to this. The page walks its
// items without caring what any of them actually are. The common header is 8 bytes.
class CMenuItemBase
{
public:
	__forceinline CMenuItemBase( void ) {}
	__forceinline CMenuItemBase( CMenu* pMenu ) { m_pMenu = pMenu; }

	virtual ~CMenuItemBase( void );
	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual void SetPos( float x, float y );
	virtual int  IsActive( void );
	CMenu*		m_pMenu;
};

// A plain line of text that runs its command when it is picked. Size: 60 bytes.
class CMenuTextItem : public CMenuItemBase
{
public:
	__forceinline CMenuTextItem( void ) {}
	CMenuTextItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y, int align );

	virtual ~CMenuTextItem( void );
	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual int  IsActive( void );

	char*		m_pszLabel;
	int			m_labelX;
	int			m_labelY;
	float		m_flLabelScale;
	float		m_flLabelAspect;
	char*		m_pszDescription;
	int			m_descX;
	int			m_descY;
	float		m_flDescScale;
	float		m_flDescAspect;
	char*		m_pszCommand;
	int			m_bEnabled;
	int			m_align;
};

// A line of text the stick runs past: it shows a label but never runs a
// command of its own.
class CMenuStaticItem : public CMenuTextItem
{
public:
	CMenuStaticItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y );

	virtual ~CMenuStaticItem( void );
	virtual int IsActive( void );
};

// The page title and the artwork that frames it.
class CMenuTitleItem : public CMenuItemBase
{
public:
	virtual ~CMenuTitleItem( void );
	CMenuTitleItem( CMenu* pMenu );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual int  IsActive( void );

	float		m_widthPeriod[2];
	float		m_xPeriod[2];
	float		m_alphaPeriod[2];
	float		m_widthPulse[2];
	float		m_width[2];
	float		m_xPulse[2];
	float		m_x[2];
	float		m_y[2];
	float		m_height[2];
	float		m_alphaPulse[2];
	float		m_alpha[2];
	int			m_texture[2];
};

// A line the stick runs past that only puts up the hint at the bottom of the
// screen.
class CMenuHintItem : public CMenuStaticItem
{
public:
	virtual ~CMenuHintItem( void );
	CMenuHintItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y );

	virtual void Draw( float flFade, qboolean bSelected );
};

// The way back, which every page but the main one carries along its bottom
// edge.
class CMenuReturnItem : public CMenuStaticItem
{
public:
	virtual ~CMenuReturnItem( void );
	CMenuReturnItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y );

	virtual void Draw( float flFade, qboolean bSelected );
};

// One unlocked passcode on the access page. These scroll under the three
// fixed lines above them, so they are told where to draw each frame.
class CMenuCodeTextItem : public CMenuTextItem
{
public:
	virtual ~CMenuCodeTextItem( void ) {}
	CMenuCodeTextItem( CMenu* pMenu, menuitemdef_t* pDef, int x, int y );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Up( void );
	virtual void Down( void );
	virtual void SetPos( float x, float y );
};

// One of the pictures a page hangs off the side of its list.
class CMenuIconItem : public CMenuItemBase
{
public:
	virtual ~CMenuIconItem( void ) {}
	CMenuIconItem( CMenu* pMenu, int id, int x, int y );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual int  IsActive( void );

	int			m_x;
	int			m_y;
	int			m_id;
	int			m_alpha;
};

// The full-screen picture a page sits on top of.
class CMenuPicItem : public CMenuItemBase
{
public:
	virtual int IsActive( void );
	virtual ~CMenuPicItem( void );
	CMenuPicItem( CMenu* pMenu, char* pszName );

	virtual void Draw( float flFade, qboolean bSelected );

	int			m_iTexture;
};

// The credits, which run themselves from the moment the page comes up.
class CMenuCreditsItem : public CMenuItemBase
{
public:
	void DrawLine( char* psz, float y, float flFade );
	virtual ~CMenuCreditsItem( void );
	CMenuCreditsItem( CMenu* pMenu );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Cancel( void );
	virtual int  IsActive( void );

	float		m_flStartTime;
	int			m_bRestart;
};

// The splash screen's timer: once it runs out the attract mode takes over.
class CMenuAttractItem : public CMenuItemBase
{
public:
	virtual ~CMenuAttractItem( void );
	CMenuAttractItem( CMenu* pMenu, float flDuration );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int  IsActive( void );

	float		m_flTimeout;
	int			m_bTaken;
};

// The save page's header. Building it kicks off the write of the slot the
// player is about to overwrite.
class CMenuSaveHeaderItem : public CMenuItemBase
{
public:
	virtual ~CMenuSaveHeaderItem( void );
	CMenuSaveHeaderItem( CMenu* pMenu );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int  IsActive( void );

};

// The "press any button" line on the splash screen.
class CMenuAnyKeyItem : public CMenuItemBase
{
public:
	virtual ~CMenuAnyKeyItem( void );
	CMenuAnyKeyItem( CMenu* pMenu );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int  IsActive( void );

};

// A setting the stick steps left and right through. The values are laid out
// once when the item is built, so drawing only has to pick one of them.
class CMenuOptionItem : public CMenuItemBase
{
public:
	virtual ~CMenuOptionItem( void );
	CMenuOptionItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int  IsActive( void );

	menuvalue_t*	m_pValues;
	char*			m_pszDescription;
	int				m_descX;
	int				m_descY;
	float			m_flDescScale;
	float			m_flDescAspect;
	int				m_nValues;
	int*			m_piValue;
	int*			m_piIndex;
};

// The stereo/mono setting, which starts out wherever the cvar left it.
class CMenuStereoItem : public CMenuOptionItem
{
public:
	virtual ~CMenuStereoItem( void ) {}
	CMenuStereoItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Left( void );
	virtual void Right( void );

	int			m_reserved0;
};

// One unlocked cheat on the codes page. It hands the game side a pointer to
// its toggle so the world can be kept in step with it.
class CMenuCheatItem : public CMenuOptionItem
{
public:
	virtual ~CMenuCheatItem( void ) {}
	virtual void SetPos( float x, float y );
	CMenuCheatItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );

	void		(*m_pfnBind)( int* pValue );
};

// A yes/no setting on the options pages, which starts out reading the cvar
// it controls.
class CMenuToggleItem : public CMenuOptionItem
{
public:
	virtual ~CMenuToggleItem( void ) {}
	CMenuToggleItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int id );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Left( void );
	virtual void Right( void );

	int			m_reserved;
	int			m_id;
};

// One of the three words the access code is spelled out of.
class CMenuWordItem : public CMenuItemBase
{
public:
	void UpdateResult( void );
	virtual ~CMenuWordItem( void );
	CMenuWordItem( CMenu* pMenu, menuoption_t* pOption, int x, int y, int* piValue );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int  IsActive( void );

	menuvalue_t*	m_pValues;
	char*			m_pszDescription;
	int				m_descX;
	int				m_descY;
	float			m_flDescScale;
	float			m_flDescAspect;
	int				m_nValues;
	int*			m_piValue;
	int*			m_piIndex;
};

// One row of the save/load list: the slot's picture, its name and the time
// it was written.
class CMenuSaveSlotItem : public CMenuOptionItem
{
public:
	static int BuildSaveFilename( qboolean bSaving, qboolean bNoSpace );
	static int AddSaveFile( char* pszName, char* pszDescription, int character, void* pUserData );
	static int CountSaveFiles( void );
	static void InitSaveList( qboolean bSaving );
	virtual ~CMenuSaveSlotItem( void ) {}
	CMenuSaveSlotItem( CMenu* pMenu, menuoption_t* pOption, int x, int iSlot, int id );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );

	int			m_reserved;
	int			m_slot;
	int			m_loaded;
	int			m_visibleRow;
	int			m_scrollTop;
	int			m_selectedFile;
	int			m_fileCount;
	int			m_scanPending;
	int			m_savePending;
	int			m_frame;
	int			m_noSpace;
	int			m_saved;
	int			m_mode;
	int			m_ready;
};

// A volume bar.
class CMenuVolumeSlider : public CMenuItemBase
{
public:
	static void PreviewMusicVolume( void );
	static void PreviewSuitVolume( void );
	virtual ~CMenuVolumeSlider( void );
	CMenuVolumeSlider( CMenu* pMenu, menuslider_t* pSlider, int x, int y, int id, int align );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int  IsActive( void );

	char*		m_pszLabel;
	int		m_x;
	int		m_y;
	float		m_flLabelScale;
	float		m_flLabelAspect;
	char*		m_pszDescription;
	int		m_descX;
	int		m_descY;
	float		m_flDescScale;
	float		m_flDescAspect;
	int		m_nSteps;
	int*		m_piToggle;
	int*		m_piValue;
	int		m_id;
};

// A stick sensitivity bar.
class CMenuSensitivitySlider : public CMenuVolumeSlider
{
public:
	virtual void Cancel( void );
	virtual ~CMenuSensitivitySlider( void );
	CMenuSensitivitySlider( CMenu* pMenu, menuslider_t* pSlider, int x, int y, int id );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Left( void );
	virtual void Right( void );
	int		m_sensitivityId;
};

// One of the ready-made control layouts.
class CMenuPresetItem : public CMenuTextItem
{
public:
	static char *LookupAlias( char *pszKey, char **ppAliases, int *pnAliases, qboolean bLong );
	virtual ~CMenuPresetItem( void );
	CMenuPresetItem( CMenu* pMenu, int preset, int x, int y );

	__forceinline void DrawLeftBinding( char *key, char *shiftedKey, int y, float flFade );
	__forceinline void DrawRightBinding( char *key, char *shiftedKey, int y, float flFade );
	int GetCalloutWidth( void );
	int GetLeftX( void );
	int GetRightX( void );
	void DrawCalloutBox( int x, int y, float flFade );
	__forceinline void DrawCalloutBackground( int x, int y, float flFade );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );

	char		m_szLabel[MAX_MENU_PRESET_LABEL];
	char		m_szDescription[MAX_MENU_PRESET_DESCRIPTION];
	char		m_szCommand[MAX_MENU_PRESET_COMMAND];
	char		m_preset;
	byte		m_pad[3];
	char**		m_ppAliases;
	int			m_nAliases;
	int			m_iAlias;
};

// The custom controls page: it takes the keyboard while it waits for the
// button the player wants to bind.
class CMenuBindItem : public CMenuItemBase
{
public:
	static char* GetShiftKey( int key );
	static int FindShiftKey( char* pszName );
	static int BuildControlList( qboolean bKeyboard, qboolean bJoystick );
	static __forceinline char * TranslateKeyName( char *pszKey );
	static char * GetKeyName( char *pszKey );
	virtual ~CMenuBindItem( void ) {}
	CMenuBindItem( CMenu* pMenu );

	virtual void Draw( float flFade, qboolean bSelected );
	virtual void Select( void );
	virtual void Cancel( void );
	virtual void Up( void );
	virtual void Down( void );
	virtual void Left( void );
	virtual void Right( void );
	virtual int  IsActive( void );

	int			m_nEntries;
	int			m_mode;
	int			m_selection;
	int			m_capturing;
	int			m_reserved18;
	int			m_leftValue;
	int			m_rightValue;
	int			m_reserved24;
	int			m_reserved28;
};

extern CMenu*	gpActiveMenu;
extern int		gfCreditsCode;
extern int		gfSecretCode;


#endif // MENU_H
