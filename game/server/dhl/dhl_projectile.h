//=============================================================================//
// Purpose:	Distraction Half-Life 2 projectile entity
//
// Author: Skillet
//=============================================================================//
#ifndef DHL_PROJECTILE_H
#define DHL_PROJECTILE_H

class CDHLProjectile : public CBaseAnimating
{
public:
	DECLARE_CLASS( CDHLProjectile, CBaseAnimating );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CDHLProjectile();
	~CDHLProjectile();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Fire( Vector vecOrigin, Vector vecVelocity, int iDamage, CBaseCombatWeapon *pSrcWeapon, CBasePlayer *pShooter, int iAmmoType );
	virtual bool OnTouch( trace_t &touchtr, bool bDecalOnly = false, ITraceFilter* pTraceFilter = NULL );
	virtual void MoveProjectileToPosition( Vector& vecPos );
	virtual void PhysicsSimulate( void );
	virtual int UpdateTransmitState( void );

	//Name says it all - shared function so gravity can be in sync on client and server
	virtual void SetupGravity( void );

	CBaseCombatWeapon* m_pFiringWeapon;
	CBasePlayer* m_pShooter;

	CNetworkVector( m_vecProjectileOrigin );
	CNetworkVector( m_vecProjectileVelocity );
	CNetworkVar( unsigned char, m_iType ); //What kind of projectile?
	CNetworkVar( int, m_iOriginalDamage ); //How much damage is this projectile intended to do?
	CNetworkVar( int, m_iAmmoType );

private:

	bool CheckFriendlyFire( CBaseEntity* pEnt );

	CNetworkHandle( CBasePlayer, m_hShooter );

	Vector m_vecCurVelocity;
	Vector m_vecCurPosition;
	CBaseEntity* m_pSkipEnt;
	int m_iDamage;
	float m_flProjectileTimescale;
	float m_flRemoveAt;
	float m_flDistanceTravelled;
	int m_iTimesPenetrated;
};
#endif //DHL_PROJECTILE_H