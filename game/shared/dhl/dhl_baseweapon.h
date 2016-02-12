//=============================================================================//
// Purpose:	Distraction Half-Life 2 weapon baseclass
//
// Author: Skillet
//=============================================================================//
#ifndef DHL_BASEWEAPON_H
#define DHL_BASEWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_hl2mpbase.h"
#include "dhl/dhl_shareddefs.h"

#define AKIMBOPOS_NONE 0
#define AKIMBOPOS_RIGHT 1
#define AKIMBOPOS_LEFT 2

#define FIREINPUTTYPE_CANHOLD 0
#define FIREINPUTTYPE_MUSTRELEASE 1
#define FIREINPUTTYPE_DEFAULT FIREINPUTTYPE_MUSTRELEASE

#ifdef CLIENT_DLL
	#define CDHLBaseWeapon C_DHLBaseWeapon
	#include "input.h"
#endif

class CDHLBaseWeapon : public CWeaponHL2MPBase
{
public:
	DECLARE_CLASS( CDHLBaseWeapon, CWeaponHL2MPBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	#ifndef CLIENT_DLL
		DECLARE_DATADESC();
	#endif

	CDHLBaseWeapon();

	virtual void SecondaryAttack( void );
	virtual unsigned char GetProjectileType( void ) { return DHL_PROJECTILE_TYPE_BULLET; }
	virtual void AddDHLViewKick( float flX, float flY );
	virtual void Drop( const Vector &vecVelocity );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual const Vector& GetBulletSpread( void );
	virtual Vector GetBulletSpread( WeaponProficiency_t proficiency ) { return BaseClass::GetBulletSpread( proficiency ); }
	virtual float GetFireRate( void ) { return GetRateOfFire(); }
	virtual void AddViewKick( void ) { AddDHLViewKick( GetRecoilX(), GetRecoilY() ); }
	virtual bool Deploy( void );
	virtual void ItemPostFrame( void );
	virtual int ObjectCaps( void ) { return FCAP_IMPULSE_USE; } //Allow +use input to be processed.  Enough bullshit.
	virtual bool CanUse( void ) { return m_bAllowPickup; } //Called by player's Weapon_CanUse() to determine if player can pickup the weapon
	virtual bool DropsAmmoIndividually( void ) { return false; } //Drop each unit of ammo separately (knives and such)
	virtual bool AllowBumpAmmoPickup( void ) { return true; }

#ifndef CLIENT_DLL
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void SignalShellEject( void ) { m_bShellEject = !m_bShellEject; }
#else
	bool WantsShellEject( void ) { return m_bShellEject != m_bOldShellEject; }
	virtual void DoShellEject( void );
	void DoAnimationEvents( CStudioHdr *pStudioHdr );
#endif
	virtual int GetShellType( void ) { return 0; } //-1 = none, 0 = pistol, 1 = rifle, 2 = shotgun

	//These functions are in use
	virtual bool CanAkimbo( void ) { return false; } //GetWpnData().bCanAkimbo exists in script but akimbos have hardcoded dependencies, so ignore it
	virtual const char* GetAkimboEntClassname( void ) { return "AKIMBOWEP_UNDEF"; } //weapon_x should return "weapon_x_akimbo"
	virtual bool IsAkimboEnt( void ) { return false; } //weapon_x_akimbo should return true
	virtual const char* GetSingleEntClassname( void ) { return "SINGLEWEP_UNDEF"; } //weapon_x_akimbo should return "weapon_x"

		//Akimbo with multiple view models - cancelled idea with incomplete code
		//These functions should not be in use and do not work entirely
		bool NeedsFlip( void );
		bool IsAkimbo( void ) { return m_bAkimbo; }
		void StartAkimbo( CDHLBaseWeapon* pOtherAkimbo, int iAkimboPos ); //Set everything up for akimbo
		//Shutdown akimbo
		//NOTE: You only need to call this for ONE of the two paired akimbo weapons
		void EndAkimbo( void );
		void SetAkimbo( bool bAkimbo ) { m_bAkimbo = bAkimbo; }
		int GetAkimboPos( void ) { return m_iAkimboPos; }
		void SetAkimboPos( int iNewPos ) { m_iAkimboPos = iNewPos; }

	bool VisibleInWeaponSelection( void );

	//Scripting
	float	GetRecoilX( void ) const;
	float	GetRecoilY( void ) const;
	float	GetHeadDamageScalar( void ) const { return GetWpnData().flHeadDmgSclr; }
	float	GetBodyDamageScalar( void ) const { return GetWpnData().flBodyDmgSclr; }
	float	GetArmDamageScalar( void ) const { return GetWpnData().flArmDmgSclr; }
	float	GetLegDamageScalar( void ) const { return GetWpnData().flLegDmgSclr; }
	float	GetRateOfFire( void ) const;
	float	GetMeleeRange( void ) const;
	float	GetMeleeDamage( void ) const;
	float	GetMeleeHullW( void ) const;
	float	GetMeleeHullH( void ) const;
	Vector	GetAccuracy( bool bMax = false );
	float	GetMoveAccuracyScalar( void ) const { return GetWpnData().flMoveAccSclr; }

	//Called when the timescale changes
	void	TimescaleChanged( float flNewTimescale, float flOldTimescale );

	int m_iInventoryValue;
	CNetworkVar( bool, m_bAkimbo );
	CNetworkVar( int, m_iAkimboPos );
	CDHLBaseWeapon* m_pOtherAkimbo;
	bool m_bAllowPickup;

protected:
	virtual void ModAccuracy( Vector& accuracy );
	
	int m_iFireInputType;
	bool m_bCanPrimaryAttack;
	bool m_bCanSecondaryAttack;

private:
	Vector m_vecAccuracy;
	Vector m_vecAccuracy2;
	CNetworkVar( bool, m_bShellEject );
#ifdef CLIENT_DLL
	bool m_bOldShellEject;
#endif
};
#endif //DHL_BASEWEAPON_H