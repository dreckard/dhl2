//=============================================================================//
// Purpose:	Distraction Half-Life 2 Akimbo Berettas
//
// Author: Skillet
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponBeretta_Akimbo C_WeaponBeretta_Akimbo
#endif

//-----------------------------------------------------------------------------
// CWeaponBeretta_Akimbo
//-----------------------------------------------------------------------------
class CWeaponBeretta_Akimbo : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponBeretta_Akimbo, CBaseHL2MPCombatWeapon );

	CWeaponBeretta_Akimbo(void);
	~CWeaponBeretta_Akimbo(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );
	void	DryFire( void );
	void	Precache( void );
	void	SetWeaponVisible( bool visible );
	bool	IsAkimboEnt( void ) { return true; }
	const char* GetSingleEntClassname( void ) { return "weapon_beretta"; }

#ifdef CLIENT_DLL
	int	DrawModel( int flags );
private:
	C_BaseAnimating* m_pLHModel;
public:
#endif

	void PrimaryAttack( void )
	{
		BaseClass::PrimaryAttack();
		#ifndef CLIENT_DLL
			SignalShellEject();
		#endif
	}
	Activity	GetPrimaryAttackActivity( void );

	virtual bool Reload( void );
	
	DECLARE_ACTTABLE();

private:
	CWeaponBeretta_Akimbo( const CWeaponBeretta_Akimbo & );
	bool m_bFireLeft;
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponBeretta_Akimbo, DT_WeaponBeretta_Akimbo )

BEGIN_NETWORK_TABLE( CWeaponBeretta_Akimbo, DT_WeaponBeretta_Akimbo )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponBeretta_Akimbo )
	DEFINE_FIELD( m_bFireLeft, FIELD_BOOLEAN ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_beretta_akimbo, CWeaponBeretta_Akimbo );
PRECACHE_WEAPON_REGISTER( weapon_beretta_akimbo );

acttable_t CWeaponBeretta_Akimbo::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_DHL_IDLE_AKIMBO,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_DHL_IDLE_CROUCH_AKIMBO,			false },

	{ ACT_MP_RUN,						ACT_DHL_RUN_AKIMBO,					false },
	{ ACT_MP_CROUCHWALK,				ACT_DHL_WALK_CROUCH_AKIMBO,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },

	{ ACT_MP_JUMP,						ACT_DHL_JUMP_AKIMBO,					false },

	{ ACT_DHL_DIVE_GENERIC,				ACT_DHL_DIVE_GENERIC_AKIMBO,			false },
	{ ACT_DHL_PRONE_GENERIC,			ACT_DHL_PRONE_GENERIC_AKIMBO,			false },
	{ ACT_DHL_PRONEMOVE_GENERIC,		ACT_DHL_PRONEMOVE_GENERIC_AKIMBO,		false },
};

IMPLEMENT_ACTTABLE( CWeaponBeretta_Akimbo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponBeretta_Akimbo::CWeaponBeretta_Akimbo( void )
{
	m_bFiresUnderwater = false;
	m_iInventoryValue = DHL_INV_VAL_HANDGUN;
	m_bFireLeft = true;

	#ifdef CLIENT_DLL
		m_pLHModel = NULL;
	#endif
}

CWeaponBeretta_Akimbo::~CWeaponBeretta_Akimbo( void )
{
#ifdef CLIENT_DLL
	delete m_pLHModel;
#endif
}

void CWeaponBeretta_Akimbo::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponBeretta_Akimbo::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;
	
	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}

	if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		DryFire();
	}
}

bool CWeaponBeretta_Akimbo::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
		m_bFireLeft = true;
	}
	return fRet;
}

Activity CWeaponBeretta_Akimbo::GetPrimaryAttackActivity( void )
{
	m_bFireLeft = !m_bFireLeft; //HACK: Hopefully this func only gets called on fully executed primary attack
	if ( m_bFireLeft )
		return ACT_VM_SECONDARYATTACK;
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponBeretta_Akimbo::Precache( void )
{
	PrecacheModel( "models/weapons/a_beretta/a_beretta_lh.mdl" );
	BaseClass::Precache();
}

void CWeaponBeretta_Akimbo::SetWeaponVisible( bool visible )
{
#ifdef CLIENT_DLL
	if ( m_pLHModel )
	{
		if ( visible )
			m_pLHModel->RemoveEffects( EF_NODRAW );
		else
			m_pLHModel->AddEffects( EF_NODRAW );
	}
#endif
	BaseClass::SetWeaponVisible( visible );
}

#ifdef CLIENT_DLL
int CWeaponBeretta_Akimbo::DrawModel( int flags )
{
	if ( !m_pLHModel )
	{
		m_pLHModel = new C_BaseAnimating;
		m_pLHModel->InitializeAsClientEntity( "models/weapons/a_beretta/a_beretta_lh.mdl", RENDER_GROUP_OPAQUE_ENTITY );
		m_pLHModel->AddEffects( EF_NODRAW );
		m_pLHModel->SetLocalOrigin( vec3_origin );
		m_pLHModel->SetSolidFlags( FSOLID_NOT_SOLID );
		m_pLHModel->AddEffects( EF_BONEMERGE ); //Automatically merges on parent's left hand bone because of identical naming in SMD
	}
	if ( m_pLHModel )
	{
		if ( GetOwner() && m_pLHModel->GetMoveParent() != GetOwner() )
		{
			m_pLHModel->SetParent( GetOwner() );
			m_pLHModel->RemoveEffects( EF_NODRAW );
		}
		else if ( !GetOwner() && m_pLHModel->GetMoveParent() )
		{
			m_pLHModel->SetParent( NULL );
			m_pLHModel->AddEffects( EF_NODRAW );
		}
	}
	return BaseClass::DrawModel( flags );
}
#endif