//=============================================================================//
// Purpose:	Distraction Half-Life 2 weapon baseclass
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "dhl/dhl_baseweapon.h"
#include "dhl/dhl_player_inc.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_te_legacytempents.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( DHLBaseWeapon, DT_DHLBaseWeapon )

BEGIN_NETWORK_TABLE( CDHLBaseWeapon, DT_DHLBaseWeapon )
	#ifdef CLIENT_DLL
		RecvPropBool( RECVINFO(m_bAkimbo) ),
		RecvPropInt( RECVINFO(m_iAkimboPos) ),
		RecvPropBool( RECVINFO(m_bShellEject) ),
	#else
		SendPropBool( SENDINFO(m_bAkimbo) ),
		SendPropInt( SENDINFO(m_iAkimboPos) ),
		SendPropBool( SENDINFO(m_bShellEject) ),
	#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CDHLBaseWeapon )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_dhl_base, CDHLBaseWeapon );

#ifndef CLIENT_DLL
BEGIN_DATADESC( CDHLBaseWeapon )
END_DATADESC()
#endif

//Constructor
CDHLBaseWeapon::CDHLBaseWeapon()
{
	m_bAkimbo = false;
	m_iAkimboPos = AKIMBOPOS_NONE;
	m_pOtherAkimbo = NULL;
	m_iInventoryValue = 0;
	m_iFireInputType = FIREINPUTTYPE_DEFAULT;
	m_bAllowPickup = true;
	m_bShellEject = false;

#ifdef CLIENT_DLL
	m_bOldShellEject = false;
#endif
	
	m_vecAccuracy = vec3_invalid;
	m_vecAccuracy2 = vec3_invalid;

	UseClientSideAnimation();
}

void CDHLBaseWeapon::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
}

//----------------------------------------------------------------------------------------
//Traditional recoil function - pushes up rather than just shaking in all directions
//----------------------------------------------------------------------------------------
void CDHLBaseWeapon::AddDHLViewKick( float flX, float flY )
{
#ifdef CLIENT_DLL
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
		return;

	flX *= 0.25f;
	flY *= 0.25f;

	QAngle recoil = QAngle( flX, flY, 0 );
	ToDHLPlayer(pPlayer)->GradVAngleChange( recoil, 0.1f );
#endif
}

void CDHLBaseWeapon::Drop( const Vector &vecVelocity )
{
	if ( GetWeaponFlags() & ITEM_FLAG_NODROP )
		return;

	BaseClass::Drop( vecVelocity );
}

bool CDHLBaseWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// cancel any reload in progress.
	m_bInReload = false;

	//HACK: Disable holster animations by not calling baseclass to fix bug with hidethink from CBaseCombatWeapon::Holster()
	return true;
}

const Vector& CDHLBaseWeapon::GetBulletSpread( void )
{
	static Vector cone;
	//Do we have a secondary accuracy value?
	if ( GetAccuracy( true ) != vec3_origin )
	{
		//Yes.  We should lerp between primary and secondary
		float ramp = SharedRandomFloat( "dhl_accuracy", 0.0f, 1.0f );
		VectorLerp( GetAccuracy(), GetAccuracy( true ), ramp, cone );
	}
	else
		//No.  Accuracy is always equal to the primary definition
		cone = GetAccuracy();

	//Modify
	ModAccuracy( cone );

	return cone;
}

bool CDHLBaseWeapon::Deploy( void )
{
	#ifdef CLIENT_DLL
		WeaponSound( DRAW );
	#endif
	return BaseClass::Deploy();
}

void CDHLBaseWeapon::ItemPostFrame( void )
{
	//DHL - Copy pasted from CBaseCombatWeapon for editing
	CDHL_Player *pOwner = ToDHLPlayer( GetOwner() );
	if (!pOwner)
		return;

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = ( pOwner->m_nButtons & IN_ATTACK ) ? ( m_fFireDuration + gpGlobals->frametime ) : 0.0f;

	if ( UsesClipsForAmmo1() )
	{
		CheckReload();
	}

	bool bFired = false;

	// Secondary attack has priority
	if ((pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		if (UsesSecondaryAmmo() && pOwner->GetAmmoCount(m_iSecondaryAmmoType)<=0 )
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextSecondaryAttack = m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bAltFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		//else
		//DHL - Skillet
		else if ( m_iFireInputType == FIREINPUTTYPE_CANHOLD || pOwner->m_afButtonPressed & IN_ATTACK2 )
		{
			// FIXME: This isn't necessarily true if the weapon doesn't have a secondary fire!
			bFired = true;

			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if ( UsesClipsForAmmo2() )
			{
				// reload clip2 if empty
				if (m_iClip2 < 1)
				{
					pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}
	
	if ( !bFired && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if ( !IsMeleeWeapon() &&  
			(( UsesClipsForAmmo1() && m_iClip1 <= 0) || ( !UsesClipsForAmmo1() && pOwner->GetAmmoCount(m_iPrimaryAmmoType)<=0 )) )
		{
			HandleFireOnEmpty();
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw
			
			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ( ( pOwner->m_afButtonPressed & IN_ATTACK ) || ( pOwner->m_afButtonReleased & IN_ATTACK2 ) )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			//DHL - Skillet - Only attack if the key was PRESSED this frame for semi-automatics
			if ( m_iFireInputType == FIREINPUTTYPE_CANHOLD || pOwner->m_afButtonPressed & IN_ATTACK )
				PrimaryAttack();
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
		m_fFireDuration = 0.0f;
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if (!((pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_nButtons & IN_ATTACK2) || (pOwner->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
	
	/*if ( pOwner )
	{
		if ( CanPrimaryAttack() && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
			PrimaryAttack();
		if ( CanSecondaryAttack() && (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
			SecondaryAttack();

		if ( !m_bCanPrimaryAttack )
			if ( pOwner->m_afButtonReleased & IN_ATTACK )
			{
				m_bCanPrimaryAttack = true;
				if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
					m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
			}
		if ( !m_bCanSecondaryAttack )
			if ( pOwner->m_afButtonReleased & IN_ATTACK2 )
			{
				m_bCanSecondaryAttack = true;
				if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
					m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
			}
	}*/

	//BaseClass::ItemPostFrame();

	//Mixed Akimbo
	//if ( IsAkimbo() && pOwner->GetActiveWeapon() == this && m_pOtherAkimbo )
		//m_pOtherAkimbo->ItemPostFrame();
}

//Modify the accuracy cone for player movement and such
void CDHLBaseWeapon::ModAccuracy( Vector& accuracy )
{
	if ( GetOwner() && GetOwner()->GetAbsVelocity() != vec3_origin )
	{
		Vector vecVelocity = GetOwner()->GetAbsVelocity();
		vecVelocity.z = 0.0f; //Disregard z to facilitate those multi-level dives
		if ( vecVelocity.Length() > 100.0f )
		{
			float flFactor = (vecVelocity.Length() / 100.0f);
			flFactor *= GetMoveAccuracyScalar();

			//Accuracy should never improve from moving
			if ( flFactor < 1.0f )
				return;

			accuracy *= flFactor;
		}
	}
}

//
//Akimbo
//
bool CDHLBaseWeapon::NeedsFlip( void )
{
	if ( IsAkimbo() && GetAkimboPos() == AKIMBOPOS_LEFT )
		return true;
	return false;
}
void CDHLBaseWeapon::StartAkimbo( CDHLBaseWeapon* pOtherAkimbo, int iAkimboPos )
{
	m_bAkimbo = true;
	m_iAkimboPos = iAkimboPos;
	m_pOtherAkimbo = pOtherAkimbo;

	//Secondary view model
	if ( iAkimboPos == AKIMBOPOS_LEFT )
		SetViewModelIndex( 1 );
}

void CDHLBaseWeapon::EndAkimbo( void )
{
	m_bAkimbo = false;
	m_iAkimboPos = AKIMBOPOS_NONE;
	SetViewModelIndex( 0 );
	
	if ( m_pOtherAkimbo )
	{
		m_pOtherAkimbo->m_bAkimbo = false;
		m_pOtherAkimbo->m_iAkimboPos = AKIMBOPOS_NONE;
		m_pOtherAkimbo->SetViewModelIndex( 0 );
		m_pOtherAkimbo->m_pOtherAkimbo = NULL;
		m_pOtherAkimbo = NULL;
	}
}

bool CDHLBaseWeapon::VisibleInWeaponSelection( void )
{
	/*if ( IsAkimbo() && GetAkimboPos() == AKIMBOPOS_LEFT )
		return false;*/
	return BaseClass::VisibleInWeaponSelection();
}

#ifdef CLIENT_DLL
void CDHLBaseWeapon::DoShellEject( void )
{
	m_bOldShellEject = m_bShellEject;

	Vector attachOrigin;
	QAngle attachAngles;
	if ( GetAttachment( 2, attachOrigin, attachAngles ) )
	{
		attachAngles.y -= 90.0;
		tempents->EjectBrass( attachOrigin, attachAngles, GetAbsAngles(), GetShellType() );
	}
}
void CDHLBaseWeapon::DoAnimationEvents( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr )
		return;
	BaseClass::DoAnimationEvents( pStudioHdr );

	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if ( WantsShellEject() && pOwner && !pOwner->IsLocalPlayer() )
		DoShellEject();
}
#endif

//
//Scripting
//
float CDHLBaseWeapon::GetRecoilX( void ) const
{
	float fRecoilX;
	fRecoilX = random->RandomFloat( GetWpnData().fRecoilXMin, GetWpnData().fRecoilXMax );
	return fRecoilX;
}
float CDHLBaseWeapon::GetRecoilY( void ) const
{
	float fRecoilY;
	fRecoilY = random->RandomFloat( GetWpnData().fRecoilYMin, GetWpnData().fRecoilYMax );
	return fRecoilY;
}

//-----------------------------------------------------------------------------
// Formulas for conversion between units:
// RPM = (1/RoF)60
// ROF = 60/RPM
//-----------------------------------------------------------------------------
float CDHLBaseWeapon::GetRateOfFire( void ) const
{
	//Take input as rounds per minute, scale for time
	return ( 60 / (GetWpnData().fRoF) ) / DHLRules()->GetTimescale();
}

float CDHLBaseWeapon::GetMeleeRange( void ) const
{
	return GetWpnData().fMeleeRange;
}

float CDHLBaseWeapon::GetMeleeDamage( void ) const
{
	return GetWpnData().fMeleeDamage;
}

float CDHLBaseWeapon::GetMeleeHullW( void ) const
{
	return GetWpnData().fMeleeHullW;
}

float CDHLBaseWeapon::GetMeleeHullH( void ) const
{
	return GetWpnData().fMeleeHullH;
}

//-----------------------------------------------------------------------------
// Gets scripted accuracy value as cone vector
// bMax - Gets maximum value if true, does not apply to all weapons
//-----------------------------------------------------------------------------
Vector CDHLBaseWeapon::GetAccuracy( bool bMax )
{
	float flVal = 0.0f;
	if ( !bMax )
	{
		if ( m_vecAccuracy == vec3_invalid )
		{
			flVal = sin(DEG2RAD(GetWpnData().flAccuracy) / 2.0f);
			m_vecAccuracy = Vector( flVal, flVal, flVal );
		}
		return m_vecAccuracy;
	}
	else
	{
		if ( m_vecAccuracy2 == vec3_invalid )
		{
			flVal = sin(DEG2RAD(GetWpnData().flAccuracy2) / 2.0f);
			m_vecAccuracy2 = Vector( flVal, flVal, flVal );
		}
		return m_vecAccuracy2;
	}
}

float ConvertTimeRemaining( float flOldTimeRemaining, float flOldTimescale, float flNewTimescale )
{
	return flOldTimeRemaining / (flNewTimescale / flOldTimescale);
	//return ( flOldTimeRemaining * flNewTimescale ) / flOldTimescale;
}

//Called on timescale change
void CDHLBaseWeapon::TimescaleChanged( float flNewTimescale, float flOldTimescale )
{
#ifndef CLIENT_DLL //Server only, changes will be automagically networked to client
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		m_flNextPrimaryAttack = gpGlobals->curtime + ConvertTimeRemaining( m_flNextPrimaryAttack - gpGlobals->curtime, flOldTimescale, flNewTimescale );  //gpGlobals->curtime + ((m_flNextPrimaryAttack - gpGlobals->curtime) * flNewTimescale );
	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		m_flNextSecondaryAttack = gpGlobals->curtime + ConvertTimeRemaining( m_flNextSecondaryAttack - gpGlobals->curtime, flOldTimescale, flNewTimescale ); //gpGlobals->curtime + ( (m_flNextSecondaryAttack - gpGlobals->curtime) * flNewTimescale );

	if ( m_flTimeWeaponIdle > gpGlobals->curtime )
		m_flTimeWeaponIdle = gpGlobals->curtime + ConvertTimeRemaining( m_flTimeWeaponIdle - gpGlobals->curtime, flOldTimescale, flNewTimescale ); //gpGlobals->curtime + ((m_flTimeWeaponIdle - gpGlobals->curtime) * flNewTimescale);

	if ( GetOwner() && GetOwner()->GetActiveWeapon() == this )
	{
		if ( GetOwner()->m_flNextAttack > gpGlobals->curtime )
			GetOwner()->m_flNextAttack = gpGlobals->curtime + ConvertTimeRemaining( GetOwner()->m_flNextAttack - gpGlobals->curtime, flOldTimescale, flNewTimescale ); //gpGlobals->curtime + ( (GetOwner()->m_flNextAttack - gpGlobals->curtime) * flNewTimescale );
	}
#endif
}

#ifndef CLIENT_DLL
	//Called on +use input
	void CDHLBaseWeapon::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CDHL_Player* pDHLPlayer = ToDHLPlayer( pActivator );
		if ( pDHLPlayer )
		{
			//Same thing as CDHL_Player::BumpWeapon()
			if ( !AllowBumpAmmoPickup() && pDHLPlayer->Weapon_OwnsThisType( GetClassname() ) )
				if ( pDHLPlayer->Weapon_EquipAmmoOnly( this ) )
					if ( ( !HasPrimaryAmmo() && UsesClipsForAmmo1() ) && ( !HasSecondaryAmmo() || !UsesClipsForAmmo2() ) )
					{
						UTIL_Remove( this );
						return;
					}
		}

		BaseClass::Use( pActivator, pCaller, useType, value );
	}
#endif