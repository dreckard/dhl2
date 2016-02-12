//=============================================================================//
// Purpose:	Distraction Half-Life 2 projectile entity
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "dhl_projectile.h"

#include "dhl/dhl_shared.h"
#include "hl2mp_gamerules.h"

LINK_ENTITY_TO_CLASS( dhl_projectile, CDHLProjectile );

BEGIN_DATADESC( CDHLProjectile )
END_DATADESC()

//Use _NOBASE to prevent inheriting CBaseAnimating/CBaseEntity send table data
IMPLEMENT_SERVERCLASS_ST_NOBASE( CDHLProjectile, DT_DHLProjectile )
	SendPropVector( SENDINFO(m_vecProjectileOrigin), -1,  SPROP_COORD ),
	SendPropVector( SENDINFO(m_vecProjectileVelocity) ),
	SendPropEHandle( SENDINFO(m_hShooter) ),
	SendPropInt( SENDINFO(m_iType) ),
	SendPropInt( SENDINFO(m_iOriginalDamage) ),
	SendPropInt( SENDINFO(m_iAmmoType) ),
END_SEND_TABLE()

CDHLProjectile::CDHLProjectile()
{
	m_pFiringWeapon = NULL;
	m_pShooter = NULL;
	m_pSkipEnt = NULL;
	m_flProjectileTimescale = 1.0f;
	m_iDamage = 0;
	m_iOriginalDamage = 0;
	m_vecCurVelocity = vec3_origin;
	m_vecCurPosition = vec3_origin;
	m_flRemoveAt = -1.0f;
}

CDHLProjectile::~CDHLProjectile()
{
}

int	CDHLProjectile::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}