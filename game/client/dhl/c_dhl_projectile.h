//=============================================================================//
// Purpose:	Distraction Half-Life 2 projectile entity
//
// Author: Skillet
//=============================================================================//
#ifndef C_DHL_PROJECTILE_H
#define C_DHL_PROJECTILE_H
#ifdef _WIN32
#pragma once
#endif

class C_DHLProjectile : public C_BaseAnimating
{
	DECLARE_CLASS( C_DHLProjectile, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
public:
	
	C_DHLProjectile( void );
	~C_DHLProjectile();

	virtual RenderGroup_t GetRenderGroup( void )
	{
		return RENDER_GROUP_OPAQUE_ENTITY;
	}

	virtual bool ShouldPredict( void ) { return true; }

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void PhysicsSimulate( void ); //This isn't called by the game logic on the client, we call it manually from Simulate()
	virtual void Simulate( void ) { PhysicsSimulate(); }
	virtual void OnDataChanged( DataUpdateType_t updateType );

	virtual bool OnTouch( trace_t &touchtr, bool bDecalOnly = false, ITraceFilter* pTraceFilter = NULL );
	virtual void Fire( Vector vecOrigin, Vector vecVelocity, int iDamage, CBaseCombatWeapon *pSrcWeapon, CBasePlayer *pShooter, int iAmmoType );

	virtual void ReceiveMessage( int classID, bf_read &msg );

	//Called on creation
	virtual void UpdateModel( void );
	//Name says it all - shared function so gravity can be in sync on client and server
	virtual void SetupGravity( void );

	virtual void MoveProjectileToPosition( Vector& vecPos );

	CBaseCombatWeapon* m_pFiringWeapon;
	CBasePlayer* m_pShooter;

	Vector m_vecProjectileOrigin;
	Vector m_vecProjectileVelocity;
	unsigned char m_iType; //What kind of projectile?
	int m_iOriginalDamage; //How much damage is this projectile intended to do?
	int m_iAmmoType;

private:
	C_DHLProjectile( const C_DHLProjectile & ); // not defined, not accessible

	CUtlVector<CBaseEntity*> m_RagdollHitList;
	C_BaseAnimating* m_pTrail;

	CNetworkHandle( C_BasePlayer, m_hShooter );

	bool CheckFriendlyFire( CBaseEntity* pEnt );

	CBaseEntity* m_pSkipEnt;
	int m_iDamage;
	float m_flProjectileTimescale;
	float m_flDistanceTravelled;
	int m_iTimesPenetrated;
	bool m_bCollided;
};

#endif //C_DHL_PROJECTILE_H