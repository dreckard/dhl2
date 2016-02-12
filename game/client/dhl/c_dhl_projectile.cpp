//=============================================================================//
// Purpose:	Distraction Half-Life 2 projectile entity
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "dhl/c_dhl_projectile.h"
#include "dhl/dhl_shared.h"

LINK_ENTITY_TO_CLASS( dhl_projectile, C_DHLProjectile );

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_DHLProjectile, DT_DHLProjectile, CDHLProjectile )
	RecvPropVector( RECVINFO(m_vecProjectileOrigin) ),
	RecvPropVector( RECVINFO(m_vecProjectileVelocity) ),
	RecvPropEHandle( RECVINFO(m_hShooter) ),
	RecvPropInt( RECVINFO(m_iType) ),
	RecvPropInt( RECVINFO(m_iOriginalDamage) ),
	RecvPropInt( RECVINFO(m_iAmmoType) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_DHLProjectile )
	DEFINE_PRED_FIELD( m_vecProjectileOrigin, FIELD_POSITION_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_vecProjectileVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hShooter, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_DHLProjectile::C_DHLProjectile( void )
{
	m_pFiringWeapon = NULL;
	m_pShooter = NULL;
	m_pSkipEnt = NULL;
	m_flProjectileTimescale = 1.0f;
	m_iDamage = 0;
	m_bCollided = false;
	m_RagdollHitList.RemoveAll();
	m_pTrail = NULL;
}

C_DHLProjectile::~C_DHLProjectile()
{
	if ( m_pTrail )
	{
		m_pTrail->Remove();
		m_pTrail = NULL;
	}
}

void C_DHLProjectile::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		QAngle angDir = vec3_angle;
		VectorAngles( m_vecProjectileVelocity, angDir );
		
		SetNetworkAngles( angDir );
		SetAbsAngles( angDir );
		SetLocalAngles( angDir );

		SetAbsVelocity( m_vecProjectileVelocity );
		SetLocalVelocity( m_vecProjectileVelocity );

		SetNetworkOrigin( m_vecProjectileOrigin );
		SetAbsOrigin( m_vecProjectileOrigin );
		SetLocalOrigin( m_vecProjectileOrigin );

		m_iDamage = m_iOriginalDamage;

		//Give bullets some spin
		if ( m_iType == DHL_PROJECTILE_TYPE_BULLET || m_iType == DHL_PROJECTILE_TYPE_PELLET )
			SetLocalAngularVelocity( QAngle( 0, 0, 2500 ) );
		if ( m_iType == DHL_PROJECTILE_TYPE_COMBATKNIFE )
			SetLocalAngularVelocity( QAngle( 2500, 0, 0 ) );

		UpdateModel();

		SetMoveType( MOVETYPE_CUSTOM );

		SetupGravity(); //Do gravity

		//Effects
		if ( m_iType == DHL_PROJECTILE_TYPE_BULLET || m_iType == DHL_PROJECTILE_TYPE_PELLET ) //Bullets get trails
		{
			Vector vecDir = m_vecProjectileVelocity;
			VectorNormalize( vecDir );
			QAngle trailAngles;
			VectorAngles( vecDir, trailAngles );

			m_pTrail = new C_BaseAnimating;
			m_pTrail->InitializeAsClientEntity( "models/bullet/bullet_trail.mdl", RENDER_GROUP_TRANSLUCENT_ENTITY );
			m_pTrail->SetLocalOrigin( m_vecProjectileOrigin );
			m_pTrail->SetLocalAngles( trailAngles );
			//m_pTrail = DHL_FX_BulletTrail( m_vecProjectileOrigin, vecDir, 1.0f, 180.0f, "effects/bullet_trail" ); 
		}
	}
}
