/*

	Distraction HL's HUD VGUI Panel 3000 with X-engine zoomage functions ( extremely cheap tho :))

	O watch out there ya pirates, messy treasure awaits!

	Copyright (c) 2006 DHL team

*/

// bunch'a standard includes

#include "cbase.h"
#include "dhl_panel.h"
#include "vgui_controls/Frame.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
//#include <vgui_controls/ImagePanel.h> FU
#include "vgui_controls/BitmapImagePanel.h"

// need this to draw ammo
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hud_macros.h"

// for armor, not working now anyways
extern dhl_shared_armor;

#include "C_Point_Camera.h"

#include <client/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

// Using the standard library string.
#include <iostream>
#include <fstream>
#include <string.h>
#include <cstdlib>
#include "settings.h"

using namespace std;

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// TODO: Make this a HUD element probably. Or just keep it here.
class CHUD_Panel : public vgui::EditablePanel
//class CTopPanel : public vgui::Frame
{
	typedef vgui::EditablePanel BaseClass;

public:
	CHUD_Panel( vgui::VPANEL parent );
	~CHUD_Panel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		
		BaseClass::ApplySchemeSettings( pScheme );
	}

	// no background
	virtual void PaintBackground()
	{
		// draw no frame, only the ImagePanel
		/*
class Color
{
public:
	// constructors
	Color()
	{
		SetColor(0, 0, 0, 0);
	}
	Color(int r,int g,int b)
	{
		SetColor(r, g, b, 0);
	}
	Color(int r,int g,int b,int a)
	{
		SetColor(r, g, b, a);
	}
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	virtual void SetBgColor(Color color);
	virtual void SetFgColor(Color color);
	virtual Color GetBgColor();
	virtual Color GetFgColor();
		*/
		/*SetPaintBackgroundType( 0 );
		Color color(0, 0, 0, 0);
		SetBgColor(color);
		BaseClass::PaintBackground();*/
	}

	// checks if it should draw too
	void OnThink( void );

	// Checks and updates "stuff"
	void CheckHealthImage( C_BasePlayer *pPlayer, int iHealth );
	void CheckArmorImage( C_BasePlayer *pPlayer, int iArmor );
	void CheckAmmo( void );

	CHudNumericDisplay *PanelAmmo; 
private:
	
	int m_iOldHealth;
	int m_iOldArmor;

	// weapons
	int m_iOldAmmo;
	int m_iOldClips;

	// TODO: Move this to private.
	vgui::CBitmapImagePanel *ImagePanel; // sucky naming I know it...
	vgui::CBitmapImagePanel *ArmorPanel;
	
};

int ammopanel_xpos;
int ammopanel_ypos;
bool ammopanel_paintbackground;
int iDisableHUD = -1; //DHL - Skillet

void MoveAmmoPanelX( ConVar *var, const char *pOldString )
{
	ammopanel_xpos = var->GetInt();
}

void MoveAmmoPanelY( ConVar *var, const char *pOldString )
{
	ammopanel_ypos = var->GetInt();
}

void AmmoPanelDisable( ConVar *var, const char *pOldString )
{
	if(var->GetInt() == 1)
		ammopanel_paintbackground = true;
	else
		ammopanel_paintbackground = false;

	DevMsg("ammopanel paintbackground set to: %i.\n", ammopanel_paintbackground);
}

void CC_HUDDisable( ConVar *var, const char *pOldString )
{
	iDisableHUD = var->GetBool();
}


ConVar dhl_panel_ammox("dhl_panel_ammox", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Panel testing and moving", MoveAmmoPanelX );
ConVar dhl_panel_ammoy("dhl_panel_ammoy", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Panel testing and moving", MoveAmmoPanelY );
ConVar panel_ammo_disable("panel_ammo_disable", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Panel testing and moving", AmmoPanelDisable );
ConVar dhl_hud_disable( "dhl_hud_disable", "0", FCVAR_ARCHIVE, "Enable/disable DHL HUD", CC_HUDDisable ); //DHL - Skillet


int armorpanel_xpos;
int armorpanel_ypos;

void ArmorPanelX( ConVar *var, const char *pOldString )
{
	armorpanel_xpos = var->GetInt();
}

void ArmorPanelY( ConVar *var, const char *pOldString )
{
	armorpanel_ypos = var->GetInt();
}

// armor panel
ConVar panel_armor_x("panel_armor_x", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Panel testing and moving", ArmorPanelX );
ConVar panel_armor_y("panel_armor_y", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Panel testing and moving", ArmorPanelY );

struct vidmode // videomode
{
	int w; // width of the screen
	int h; // height of the screen
};

/*
#include <fstream>
#include <string>

using namespace std;

#define SETTINGS_FILENAME	"settings.ini"

int		OpenSettingsFile(char *filename);
int		DoesSettingExist(char *filename, string str);

void	WriteSettingToFileNum(char *filename, string str, string num);
void	WriteSettingToFileStr(char *filename, string str, string str2);

int		ReadSettingNum(char *filename, char *str);
//char	*ReadSettingStr(char *filename, char *str);
string	ReadSettingStr(string filename, string str);

// TODO:
void	ChangeSettingNum(char *filename, string str, string num);
void	ChangeSettingStr(char *filename, string str, string str2);
// END TODO

//char	*StripQuotes(char *str);
string	StripQuotes(string str);

// for practical purposes
string	GetModDirectory(void);
string	FileInModDirectory(string filename);
string	fimd(string str);
void	BackupFile(string filename, string newfilename);
*/

/*int find(string str, string str1)
{
	int len = (int)str.size();

	for(int i=0; i<len; i++)
	{
		if(str[i] != str1[i])
			return FALSE;
	}

	return TRUE;
}*/

// returns the POSITION where the string is at.
int TestString(string filename, string str)
{
	int i = -1;
	char hudfile[256] = "";
	char oneline[256] = "";

	sprintf( hudfile, "%s/%s", engine->GetGameDirectory(), filename.c_str() );

	std::ifstream in(hudfile);
	if(!in)
		DevMsg("Couldn't find any %s.\n", filename);

	if(!in.is_open())
		DevMsg("Couldn't open %s.\n", filename);
	else
	{
		while(!in.eof())
		{
			in.getline(oneline, 256);
			i++;
			//if(find(str, oneline))
			if ( str.find( oneline ) != std::string::npos ) //This what you meant? - Skillet
			{
				DevMsg("Found the res.\n");
				return -1;
				//break;
			}
		}
	}

	in.close();
	DevMsg("Closed the filehandle.\n");
	return i; // return the pos
}

int RetrieveSetting(string filename, string str, int pos)
{

	// Don't wanna get the "res ..." part.
	pos++;

	// Same mess here
	char hudfile[256] = "";
	char oneline[256] = "";

	sprintf( hudfile, "%s/%s", engine->GetGameDirectory(), filename.c_str() );

	std::ifstream in(hudfile);
	if(!in)
		DevMsg("Couldn't find any %s.\n", filename);

	if(!in.is_open())
		DevMsg("Couldn't open %s.\n", filename);
	else
	{
		// Small thing here, jump to the pos
		for(int i=0; i<pos; i++)
		{
			in.getline(oneline, 256);
		}

		while(!in.eof())
		{
			in.getline(oneline, 256);
			//if(find(str, oneline))
			if ( str.find( oneline ) != std::string::npos ) //This what you meant? - Skillet
			{
				DevMsg("Found the res.\n");
				return -1;
				//break;
			}
		}
	}

	in.close();
	DevMsg("Closed the filehandle.\n");
//string a="321"
//int b = atoi(a.c_str());
}

void GetRes( ConVar *var, const char *pOldString )
{
	vidmode vmodeCurrent; // current video mode, e.g. 640x480
	//std::string line; // one line of text

	// Get current video mode
	vmodeCurrent.w = ScreenWidth();
	vmodeCurrent.h = ScreenHeight();

	// Display a developer message to the user with the current video mode.
	DevMsg("Current video mode: %ix%i.\n", vmodeCurrent.w, vmodeCurrent.h);

	// Now, tell the user if we are not having his/her resolution or not in a file called "hud.txt"
	char szCurrentRes[64];
	sprintf( szCurrentRes, "res %ix%i", vmodeCurrent.w, vmodeCurrent.h);

	// Parse the "hud.txt" file for the current resolution
	int pos = TestString("hud.txt", szCurrentRes);
}

ConVar getres("getres", "1.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Get resolution", GetRes );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHUD_Panel::CHUD_Panel( vgui::VPANEL parent ) : BaseClass( NULL, "CHUD_Panel" )
{
	SetParent( parent );
	SetProportional( true );
	SetVisible( true );
	//ActivateBuildMode();       // Activate build mode until we finish setting it up.
	SetScheme("ClientScheme");

	// Loading the .res file.
	LoadControlSettings( "resource/UI/TopPanel.res" );

	ImagePanel = new CBitmapImagePanel(this, "DHL_HP", "hud/health0");
	ImagePanel->SetBounds(130, 0, 200, 200);
	ImagePanel->setTexture( "hud/health0" );

	PanelAmmo = new CHudNumericDisplay(this, "DHL_Ammo");
	//PanelAmmo->SetParent(parent);
	PanelAmmo->SetBounds(0, 700, 200, 200);
	ammopanel_xpos = 110;
	ammopanel_ypos = 600;

	// TODO: Make a setting file for each resolution.
	//this->SetProportional(1);
	//int width, height;
//	GetDesktopResolution( width, height );
	
	m_iOldAmmo = -1;
	m_iOldClips = -1;

	ArmorPanel = new CBitmapImagePanel(this, "DHL_AP", "hud/armor/armor0");
	ArmorPanel->SetBounds(0, 0, 200, 200);
	ArmorPanel->setTexture( "hud/armor/armor0" );
	armorpanel_xpos = 0;
	armorpanel_ypos = 0;

	m_iOldHealth = 101;
	m_iOldArmor = -1; // todo

//	SetFillColor(Color( 0,0,0,0 ) );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHUD_Panel::~CHUD_Panel()
{
	// these are deleted anyway
	delete ImagePanel;
	delete ArmorPanel;
	delete PanelAmmo;
}

void CHUD_Panel::CheckAmmo()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_BaseCombatWeapon *wpn = GetActiveWeapon();

	// First check the current ammo, then clips left
	if ( !wpn || !pPlayer || !wpn->UsesPrimaryAmmo() )
	{
		PanelAmmo->SetPaintEnabled(false);
		PanelAmmo->SetPaintBackgroundEnabled(false);
		return;
	}

	PanelAmmo->SetPaintEnabled(true);

	//PanelAmmo->SetPaintEnabled(false);
	//PanelAmmo->SetPaintBackgroundEnabled(true);
	PanelAmmo->SetShouldDisplaySecondaryValue(true); // paint the clips left, also changed some stuff i hud_numericdisplay.h for this. imo should be done in a more elegant way.

	// get the ammo in our clip
	// CNetworkVar( int, m_iClip1 );				// number of shots left in the primary weapon clip, -1 it not used
	// CNetworkVar( int, m_iClip2 );				// number of shots left in the secondary weapon clip, -1 it not used
	
	// Need to check for ammo update!
	
	int ammo1 = wpn->Clip1();
	int ammo2;
	if (ammo1 < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = pPlayer->GetAmmoCount(wpn->GetPrimaryAmmoType());
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = pPlayer->GetAmmoCount(wpn->GetPrimaryAmmoType());
		//ammo2 /= pPlayer->GetAmmoCount(wpn->GetMaxClip1()); // hope this works.
	}

	m_iOldAmmo = ammo1;

	// no effects for now
	PanelAmmo->SetDisplayValue(ammo1);
	PanelAmmo->SetSecondaryValue(ammo2);
}

void CHUD_Panel::CheckHealthImage( C_BasePlayer *pPlayer, int iHealth )
{
	if(iHealth == m_iOldHealth)
		return; // no need to update

	// save the health in oldhealth
	m_iOldHealth = iHealth;

	DevMsg("DrawHealth.\n");

	std::string strImagePath = "";

	if(iHealth >= 90)
		strImagePath = "hud/health100";
	else if(iHealth >= 80)
		strImagePath = "hud/health90";
	else if(iHealth >= 70)
		strImagePath = "hud/health80";
	else if(iHealth >= 60)
		strImagePath = "hud/health70";
	else if(iHealth >= 50)
		strImagePath = "hud/health60";
	else if(iHealth >= 40)
		strImagePath = "hud/health50";
	else if(iHealth >= 30)
		strImagePath = "hud/health40";
	else if(iHealth >= 20)
		strImagePath = "hud/health30";
	else if(iHealth >= 10)
		strImagePath = "hud/health20";
	else //if(iHealth > 0)
		strImagePath = "hud/health10";

	// player is dead
	if(pPlayer->IsPlayerDead())
		strImagePath = "hud/health0";

	ImagePanel->setTexture( strImagePath.c_str() );
}

void CHUD_Panel::CheckArmorImage( C_BasePlayer *pPlayer, int iArmor )
{
	if(iArmor == m_iOldArmor)
		return; // no need to update

	// save it for later checks
	m_iOldArmor = iArmor;

	std::string strImagePath = "";

	DevMsg("DrawArmor.\n");

	if(iArmor >= 90)
		strImagePath = "hud/armor100";
	else if(iArmor >= 80)
		strImagePath = "hud/armor90";
	else if(iArmor >= 70)
		strImagePath = "hud/armor80";
	else if(iArmor >= 60)
		strImagePath = "hud/armor70";
	else if(iArmor >= 50)
		strImagePath = "hud/armor60";
	else if(iArmor >= 40)
		strImagePath = "hud/armor50";
	else if(iArmor >= 30)
		strImagePath = "hud/armor40";
	else if(iArmor >= 20)
		strImagePath = "hud/armor30";
	else if(iArmor >= 10)
		strImagePath = "hud/armor20";
	else if(iArmor > 0)
		strImagePath = "hud/armor10";
	else
		strImagePath = "hud/armor0";

	ImagePanel->setTexture( strImagePath.c_str() );
}

void CHUD_Panel::OnThink( void ) // did use OnTick() here before.
{
	if( !ImagePanel || !ArmorPanel || !PanelAmmo )
	{
		DevMsg("Something == FALSE. Terminating function 'OnThink()' in dhl_hud_panel.cpp.\n");
		return;
	}

	//DHL - Skillet
	if ( iDisableHUD != -1 )
	{
		if ( iDisableHUD == 1 )
		{
			ImagePanel->SetVisible( false );
			ArmorPanel->SetVisible( false );
			PanelAmmo->SetVisible( false );
		}
		if ( iDisableHUD == 0 )
		{
			ImagePanel->SetVisible( true );
			ArmorPanel->SetVisible( true );
			PanelAmmo->SetVisible( true );
		}
		iDisableHUD = -1;
	}

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
	{
		//DevMsg("pPlayer == NULL.\n");
		return;
	}
	PanelAmmo->SetPaintEnabled(true);
	ImagePanel->SetPaintEnabled(false);
	ArmorPanel->SetPaintEnabled(false);
	// update posititons
	PanelAmmo->SetPos( ammopanel_xpos, ammopanel_ypos );
	ArmorPanel->SetPos( armorpanel_xpos, armorpanel_ypos );
	//PanelAmmo->SetPaintBackgroundEnabled( ammopanel_paintbackground );

//	ImagePanel->SetPaintBackground( false);
//	ArmorPanel->SetFillColor(Color( 0,0,0,0 ) );

	// check health
	int iHealth = pPlayer->GetHealth();
	CheckHealthImage( pPlayer, iHealth );

	// check ammo
	CheckAmmo();

	// check armor/battery
	CheckArmorImage( pPlayer, 0 ); // right now disable armor, just draw 0
}

// Class
// creates the HUD
class CHUD : public CDHL_Panel
{
private:
	CHUD_Panel *hudPanel;
	vgui::VPANEL m_hParent;

public:
	CHUD( void )
	{
		hudPanel = NULL;
	}

	void Create( vgui::VPANEL parent )
	{
		// Create immediately
		hudPanel = new CHUD_Panel(parent);
	}

	void Destroy( void )
	{
		if ( hudPanel )
		{
			hudPanel->SetParent( (vgui::Panel *)NULL );
			delete hudPanel;
		}
	}

};

static CHUD g_hud;
// dhlPanel should be loaded in vgui_int.cpp
CDHL_Panel *dhlPanel = ( CDHL_Panel * )&g_hud;