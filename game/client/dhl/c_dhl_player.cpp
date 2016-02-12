//=============================================================================//
// Purpose:	Distraction Half-Life 2 player entity
//
// Author: Skillet
//=============================================================================//

#include "cbase.h"
#include "c_dhl_player.h"
#include "hl2mp_gamerules.h"
#include "dhl/c_dhl_projectile.h"
#include "TeamSelectionMenu.h"
#include "input.h"
#include "filesystem.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "ivieweffects.h"
#include "physpropclientside.h"
#include <string>
#include "engine/ivdebugoverlay.h"
#include "bone_setup.h"

#if defined( CDHL_Player )
#undef CDHL_Player
#endif

LINK_ENTITY_TO_CLASS( player, C_DHL_Player );

BEGIN_RECV_TABLE_NOBASE( C_DHL_Player, DT_DHLLocalPlayerExclusive )
	RecvPropInt( RECVINFO( m_iDHLArmor ) ),

	RecvPropBool( RECVINFO(m_bProneStandReady) ),
	RecvPropBool( RECVINFO(m_bIsBleeding) ),
	RecvPropBool( RECVINFO(m_bBandaging) ),
	RecvPropInt( RECVINFO(m_iSelectedInventoryValue) ),
	RecvPropBool( RECVINFO(m_bScoped) ),
	RecvPropInt( RECVINFO(m_iStylePoints) ),
	RecvPropInt( RECVINFO(m_iItemFlags) ),
	RecvPropBool( RECVINFO(m_bAutoReload) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_DHL_Player, DT_DHL_Player, CDHL_Player)
	RecvPropDataTable( "dhllocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_DHLLocalPlayerExclusive) ),

	RecvPropInt( RECVINFO( m_iStuntState ) ),
	RecvPropInt( RECVINFO( m_iStuntDir ) ),
	RecvPropBool( RECVINFO(m_bProne) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_DHL_Player )
	DEFINE_PRED_FIELD( m_iStuntState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iStuntDir, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bProne, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_bProneStandReady, FIELD_BOOLEAN ),
END_PREDICTION_DATA()

C_DHL_Player::C_DHL_Player()
{
	vecSavedVelocity = vec3_origin;
	m_bBandaging = false;
	m_bSpeedLocked = false;
	m_bNightvision = false;
	m_bWantsThirdPerson = false;
	m_bWantsNV = false;
	m_angGradVAngle = vec3_angle;
	m_flGradVAngleTime = 0.0f;
	m_flLastTimescaleChange = 0.0f;

	m_iItemFlags = 0;
	m_iSelectedInventoryValue = 0;
	m_bScoped = false;
	m_iStylePoints = 0;
	m_bAutoReload = true;

	m_bProne = false;
	m_iStuntState = STUNT_NONE;
	m_flProneStandTime = 0.0f;
	m_iStuntDir = 0;
	m_flLastDiveYaw = 0.0f;
}

C_DHL_Player::~C_DHL_Player()
{
}

void C_DHL_Player::Precache( void )
{
	//This function gets called for every player that joins...oh well
	PrecacheMaterial( "dhl_shaders/dhl_nightvision" );
	PrecacheMaterial( "dhl_shaders/dhl_grayscale" );
	PrecacheMaterial( "dhl_shaders/dhl_blur" );

	BaseClass::Precache();
}

void C_DHL_Player::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( IsLocalPlayer() && input->CAM_IsThirdPerson() )
	{
		OnLatchInterpolatedVariables( LATCH_ANIMATION_VAR );
	}
}

bool C_DHL_Player::ShouldDraw( void )
{
	//DHL - Skillet - Keep players from ghosting corners in third person
	//This is really shitty, should be redone.
	//Cut for now, causes random failures in third person
	//if ( !IsLocalPlayer() && input->CAM_IsThirdPerson() )
	//{
	//	CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
	//	if ( pPlayer )
	//	{
	//		trace_t tr;
	//		Vector vecStartPos;
	//		//QAngle angAngles;
	//		//if ( pPlayer->GetAttachment( pPlayer->LookupAttachment("eyes"), vecStartPos, angAngles ) )
	//		vecStartPos = pPlayer->EyePosition();
	//		{
	//			UTIL_TraceLine( vecStartPos, GetRenderOrigin(), MASK_OPAQUE, pPlayer, COLLISION_GROUP_NONE, &tr );
	//			if ( tr.DidHit() && tr.m_pEnt != this )
	//				return false;
	//		}
	//	}
	//}
	return BaseClass::ShouldDraw();
}
int C_DHL_Player::DrawModel( int flags )
{
	if ( !ShouldDraw() )
		return 0;

	return BaseClass::DrawModel( flags );
}

Activity C_DHL_Player::TranslateTeamActivity( Activity ActToTranslate )
{
	if ( GetTeamNumber() == TEAM_COMBINE )
		 return ActToTranslate;
	
	if ( ActToTranslate == ACT_RUN )
		 return ACT_RUN_AIM_AGITATED;

	if ( ActToTranslate == ACT_IDLE )
		 return ACT_IDLE_AIM_AGITATED;

	if ( ActToTranslate == ACT_WALK )
		 return ACT_WALK_AIM_AGITATED;

	return ActToTranslate;
}

//extern CMoveData *g_pMoveData; //A little sketchy but Valve does it too, so meh
void C_DHL_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	return BaseClass::SetAnimation( playerAnim );
	/*int animDesired = 0;

	float speed;

	speed = GetAbsVelocity().Length2D();

	
	// bool bRunning = true;

	//Revisit!
	//if ( ( m_nButtons & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) )
	//{
	//	if ( speed > 1.0f && speed < hl2_normspeed.GetFloat() - 20.0f )
	//	{
	//		bRunning = false;
	//	}
	//}

	if ( GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_HL2MP_RUN;

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if ( playerAnim == PLAYER_JUMP )
	{
		idealActivity = ACT_HL2MP_JUMP;
	}
	else if ( playerAnim == PLAYER_DIE )
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			return;
		}
	}
	else if ( playerAnim == PLAYER_ATTACK1 )
	{
		if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
		else
		{
			idealActivity = ACT_HL2MP_GESTURE_RANGE_ATTACK;
		}
	}
	else if ( playerAnim == PLAYER_RELOAD )
	{
		idealActivity = ACT_HL2MP_GESTURE_RELOAD;
	}
	else if ( playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK )
	{
		if ( !( GetFlags() & FL_ONGROUND ) && GetActivity( ) == ACT_HL2MP_JUMP )	// Still jumping
		{
			idealActivity = GetActivity( );
		}
		
		//else if ( GetWaterLevel() > 1 )
		//{
		//	if ( speed == 0 )
		//		idealActivity = ACT_HOVER;
		//	else
		//		idealActivity = ACT_SWIM;
		//}
		
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				if ( speed > 0 )
				{
					idealActivity = ACT_HL2MP_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE_CROUCH;
				}
			}
			else if ( m_bProne ) //Same as CMultiPlayerAnimState::HandleProne()
			{
				if ( GetStuntState() == STUNT_DIVE )
					idealActivity = ACT_DHL_DIVE_GENERIC;
				else if ( GetStuntState() == STUNT_PRONE )
				{
						if ( m_iStuntDir == STUNTDIR_FORWARDS && g_pMoveData->m_nPlayerHandle == this &&
							abs(g_pMoveData->m_flSideMove) >= DHL_WALK_SPEED * DHL_PRONESPEED_SCALAR )
								idealActivity = ACT_DHL_PRONEMOVE_GENERIC;
						else
							idealActivity = ACT_DHL_PRONE_GENERIC;
				}
			}
			else
			{
				if ( speed > 0 )
				{
					
					//if ( bRunning == false )
					//{
					//	idealActivity = ACT_WALK;
					//}
					//else
					
					{
						idealActivity = ACT_HL2MP_RUN;
					}
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE;
				}
			}
		}

		idealActivity = TranslateTeamActivity( idealActivity );
	}
	
	if ( idealActivity == ACT_HL2MP_GESTURE_RANGE_ATTACK )
	{
		#ifndef DISTRACTION
			RestartGesture( Weapon_TranslateActivity( idealActivity ) );

			// FIXME: this seems a bit wacked
			Weapon_SetActivity( Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );
		#endif

		return;
	}
	else if ( idealActivity == ACT_HL2MP_GESTURE_RELOAD )
	{
		#ifndef DISTRACTION
			RestartGesture( Weapon_TranslateActivity( idealActivity ) );
		#endif

		return;
	}
	else
	{
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( Weapon_TranslateActivity ( idealActivity ) );

		if (animDesired == -1)
		{
			animDesired = SelectWeightedSequence( idealActivity );

			if ( animDesired == -1 )
			{
				animDesired = 0;
			}
		}
	
		// Already using the desired animation?
		if ( GetSequence() == animDesired )
			return;

		m_flPlaybackRate = 1.0;
		ResetSequence( animDesired );
		SetCycle( 0 );
		return;
	}

	// Already using the desired animation?
	if ( GetSequence() == animDesired )
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	SetCycle( 0 );*/
}

Activity C_DHL_Player::Weapon_TranslateActivity( Activity baseAct, bool *pRequired )
{
	Activity translated = baseAct;

	CBaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( pWeapon )
	{
		translated = pWeapon->ActivityOverride( baseAct, pRequired );
	}
	else if (pRequired)
	{
		*pRequired = false;
	}

	return translated;
}

//Player is notified when view perspective is changed
void C_DHL_Player::PerspectiveChanged( int iType )
{
	switch ( iType )
	{
		case PCHANGE_TOTHIRDPERSON:
		{
			//Reset aim yaw pose param
			int upper_body_yaw = LookupPoseParameter( "aim_yaw" );
			if ( upper_body_yaw >= 0 )
				SetPoseParameter( upper_body_yaw, 0.0f );
			AddBaseAnimatingInterpolatedVars();
			return;
			break;
		}
		case PCHANGE_TOFIRSTPERSON:
		{
			RemoveBaseAnimatingInterpolatedVars();
			return;
			break;
		}
	}
}

void C_DHL_Player::ClientRespawn( void )
{
	m_pDeathViewEnt = NULL; //Reset override

	if ( m_bWantsThirdPerson )
	{
		engine->ClientCmd( "thirdperson \n");
		m_bWantsThirdPerson = false;
	}
	if ( m_bWantsNV && (m_iItemFlags & DHL_IFLAG_NIGHTVISION) )
	{
		EnableNightvision( true );
		m_bWantsNV = false;
	}
}

void C_DHL_Player::ClientDeath( void )
{
	//This function should only be called for the local player, but just in case...
	if ( !IsLocalPlayer() )
		return;

	//Fix ragdolls getting borked because of the stunt sequence
	if ( m_bProne )
	{
		MDLCACHE_CRITICAL_SECTION();
		ResetSequence( SelectWeightedSequence( ACT_DHL_PRONE_GENERIC ) );
	}

	//Gotta go to first person for the ragdoll death cam
	if ( input->CAM_IsThirdPerson() )
	{
		engine->ClientCmd( "firstperson \n");
		m_bWantsThirdPerson = true;
	}

	if ( GetNightvisionEnabled() )
	{
		EnableNightvision( false );
		m_bWantsNV = true;
	}
	vieweffects->ClearAllFades(); //No screenfades after death
}

bool C_DHL_Player::GetHeadGib( char* pDst, int iLen )
{
	std::string strHeadGib = modelinfo->GetModelName( GetModel() );
	int iSize = strHeadGib.length();
	strHeadGib.resize( iSize - 4 ); //Cut off .mdl
	strHeadGib.append( "_headgib.mdl" );
	if ( filesystem->FileExists( strHeadGib.c_str() ) )
	{
		Q_strncpy( pDst, strHeadGib.c_str(), iLen );
		return true;
	}
	return false;
}

bool C_DHL_Player::GetBodyGib( char* pDst, int iLen )
{
	std::string strBodyGib = modelinfo->GetModelName( GetModel() );
	int iSize = strBodyGib.length();
	strBodyGib.resize( iSize - 4 ); //Cut off .mdl
	strBodyGib.append( "_bodygib.mdl" );
	if ( filesystem->FileExists( strBodyGib.c_str() ) )
	{
		Q_strncpy( pDst, strBodyGib.c_str(), iLen );
		return true;
	}
	return false;
}

C_HL2MPRagdoll* C_DHL_Player::GetHL2MPRagdoll( void )
{
	return (C_HL2MPRagdoll*)m_hRagdoll.Get();
}

//Returns true on success
bool C_DHL_Player::HideRagdoll( void )
{
	if ( m_hRagdoll.Get() )
	{
		C_HL2MPRagdoll *pRagdoll = (C_HL2MPRagdoll*)m_hRagdoll.Get();
		pRagdoll->AddEffects( EF_NODRAW );
		//Freeze it too
		pRagdoll->VPhysicsGetObject()->EnableCollisions( false );
		pRagdoll->VPhysicsGetObject()->EnableMotion( false );
		return true;
	}
	return false;
}

bool C_DHL_Player::SetRagdollModel( const char* szMdlName )
{
	if ( m_hRagdoll.Get() )
	{
		C_HL2MPRagdoll *pRagdoll = (C_HL2MPRagdoll*)m_hRagdoll.Get();
		return pRagdoll->SetModel( szMdlName );
	}
	return false;
}

#ifdef _DEBUG
	CON_COMMAND( showbonepos, "Shows local player bone pos for index. Uses ragdoll if dead." )
	{
		if (args.ArgC() == 1)
			return;

		C_DHL_Player* pPlayer = ToDHLPlayer(C_BasePlayer::GetLocalPlayer());

		if ( pPlayer )
		{
			C_BaseAnimating* pTargetEnt = pPlayer->IsAlive() ? static_cast<C_BaseAnimating*>(pPlayer) : pPlayer->GetHL2MPRagdoll();
			CStudioHdr* pModel = pTargetEnt->GetModelPtr();

			int iBone = atoi(args.Arg(1));

			mstudiobone_t* pBone = pModel->pBone(iBone);
			if ( !pBone )
			{
				Warning( "Invalid bone: %i\n", iBone );
				return;
			}
			Vector vecPos = vec3_origin;
			QAngle angAng = vec3_angle;
			pTargetEnt->GetBonePosition( iBone, vecPos, angAng );
			debugoverlay->AddLineOverlay( pPlayer->GetAbsOrigin(), vecPos,
				255, 0, 0, false, 10.0f );
			Msg( "Showing bone: %s\n", pBone->pszName() );
		}
	}
#endif

ConVar dhl_debuggibbing( "dhl_debuggibbing", "0", FCVAR_CHEAT );
ConVar dhl_gibstay( "dhl_gibstay", "30.0f", FCVAR_ARCHIVE );
void DHLPlayerGibCallback( const CEffectData &data )
{
	float flGibStay = dhl_gibstay.GetFloat();
	if ( dhl_gibstay.GetFloat() < 1.0f ) //No gibs if this is too small
		return;
	if ( flGibStay > 1000.0f ) //Nothing too ridiculous
		flGibStay = 1000.0f;

	C_DHL_Player* pPlayer = static_cast<C_DHL_Player*>(data.GetEntity());
	if ( !pPlayer )
		return;

	char szHeadGib[256];
	char szBodyGib[256];

	if ( !pPlayer->GetHeadGib( szHeadGib, 256 ) || !pPlayer->GetBodyGib( szBodyGib, 256 ) )
		return;

	C_HL2MPRagdoll *pOrigRagdoll = pPlayer->GetHL2MPRagdoll();
	if ( !pOrigRagdoll ) //There should always be a ragdoll already for us to rip data from
		return;

	//Head first
	//C_BaseEntity* pHead = C_Gib::CreateClientsideGib( szHeadGib, data.m_vOrigin, pOrigRagdoll->GetRagdollVelocity(), AngularImpulse( 0, 0, 0 ), dhl_gibstay.GetFloat() );
	C_PhysPropClientside* pHead = C_PhysPropClientside::CreateNew();
	pHead->SetModelName( szHeadGib );
	pHead->SetInteraction( PROPINTER_WORLD_BLOODSPLAT );
	pHead->SetAbsOrigin( pPlayer->GetAbsOrigin() + VEC_VIEW );
	pHead->SetAbsAngles( pPlayer->GetRenderAngles() );
	if ( pHead && pHead->Initialize() )
	{
		IPhysicsObject* pGibObj = pHead->VPhysicsGetObject();
		if ( pGibObj )
		{
			//pGibObj->SetPosition( data.m_vOrigin, pPlayer->GetRenderAngles(), true );
			Vector vel = pOrigRagdoll->GetRagdollVelocity();
			pGibObj->SetVelocity( &vel, NULL );
		}

		pHead->StartFadeOut( flGibStay );
		pPlayer->m_pDeathViewEnt = pHead;
		pPlayer->m_flDeathViewEntEndTime = gpGlobals->curtime + flGibStay;
	}
	else if ( pHead )
		pHead->Release();

	C_HL2MPRagdoll *pRagdoll = new C_HL2MPRagdoll;
	pRagdoll->SetAbsOrigin( pPlayer->GetAbsOrigin() );
	pRagdoll->SetAbsAngles( pPlayer->GetRenderAngles() );
	pRagdoll->InitializeAsClientEntity( szBodyGib, RENDER_GROUP_OPAQUE_ENTITY );

	//SetAbsVelocity does nothing here for some reason, see below for the IPhysicsObject version of this
	//pRagdoll->SetAbsVelocity( pOrigRagdoll->GetRagdollVelocity() );

	pRagdoll->m_nRenderFX = kRenderFxRagdoll;

	//pRagdoll->Interp_Copy( pPlayer );
	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t playerBones[MAXSTUDIOBONES];
	matrix3x4_t gibBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;
	pRagdoll->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, gibBones, boneDt );
	pOrigRagdoll->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, playerBones, boneDt );

	//Map from the original ragdoll to the gib based on bone names
	for ( int i = 0; i < pRagdoll->GetModelPtr()->numbones(); i++ )
	{
		mstudiobone_t* pDstBone = pRagdoll->GetModelPtr()->pBone(i);
		if ( !pDstBone )
			break;
		const char* szBoneName = pDstBone->pszName();
		int iSrcBone = Studio_BoneIndexByName( pOrigRagdoll->GetModelPtr(), szBoneName );
		if ( iSrcBone > -1 )
			gibBones[i] = playerBones[iSrcBone];
	}

	pRagdoll->InitAsClientRagdoll( boneDelta0, boneDelta1, gibBones, boneDt );
	pRagdoll->AddEffects( EF_NODRAW );
	pRagdoll->m_bTempND = true;

	if ( dhl_debuggibbing.GetBool() )
	{
		int iPlrBones = pOrigRagdoll->GetModelPtr()->numbones();
		int iGibBones = pRagdoll->GetModelPtr()->numbones();

		if ( iPlrBones != iGibBones )
			Warning( "Gib has %i bones, player ragdoll has %i\n", iGibBones, iPlrBones );

		CStudioHdr* pHdr = pOrigRagdoll->GetModelPtr();
		for ( int i = 0; i < max(iPlrBones,iGibBones); i++ )
		{
			const char* plrbone = i < iPlrBones ? pHdr->pBone(i)->pszName() : "none";
			const char* gibbone = i < iGibBones ? pRagdoll->GetModelPtr()->pBone(i)->pszName() : "none";
			if ( Q_strcmp( plrbone, gibbone ) )
				Warning( "Bone at index %i differs:\n  Gib: %s; Player: %s\n", i, gibbone, plrbone );
			else
			{
				//pRagdoll->SetParent( pOrigRagdoll );
				//pRagdoll->AddEffects( EF_BONEMERGE );
				//pRagdoll->m_bBoneMergeOnce = true;
				/*Vector vecPos = vec3_origin;
				matrix3x4_t boneToWorld;
				pOrigRagdoll->GetBoneTransform( i, boneToWorld );
				MatrixPosition( boneToWorld, vecPos );

				pRagdoll->GetBoneTransform( i, boneToWorld );
				matrix3x4_t worldToBone;
				MatrixInvert( boneToWorld, worldToBone );

				vecPos.z += 50.0f;
				MatrixPosition( worldToBone, vecPos );
				pRagdoll->GetBoneForWrite(i)->pos = vecPos;*/
			}
		}
	}

	IRagdoll* pNewRagdoll = pRagdoll->GetIRagdoll();
	if ( pNewRagdoll )
	{
		Vector vel = pOrigRagdoll->GetRagdollVelocity();
		pNewRagdoll->GetElement( 0 )->SetVelocity( &vel, NULL );
	}

	//Horrible idea?
	/*CStudioHdr* pHdr = pOrigRagdoll->GetModelPtr();
	for ( int i = 0; i < pHdr->numbones(); i++ )
	{
		mstudiobone_t* bone = pHdr->pBone(i);
		int iMatchingBone = Studio_BoneIndexByName( pRagdoll->GetModelPtr(), bone->pszName() );
		if ( iMatchingBone > -1 )
		{
			pRagdoll->GetModelPtr()->pBone( iMatchingBone )->pos = bone->pos;
		}
	}*/

	/*IRagdoll* origRagdoll = pOrigRagdoll->GetIRagdoll();
	CRagdoll* newRagdoll = static_cast<CRagdoll*>(pRagdoll->GetIRagdoll());
	for ( int i = 0; i < 24; i++ )
	{
		IPhysicsObject* pObj = origRagdoll->GetElement( i );
		if ( !pObj )
			break;
		Vector pos = vec3_origin;
		QAngle angles = vec3_angle;
		pObj->GetPosition( &pos, &angles );

		//char Command[128];
		//Q_snprintf( Command, sizeof( Command ), "drawcross %f %f %f \n", pos.x, pos.y, pos.z );
		//engine->ClientCmd( Command );

		//if ( i > 0 )
		//	newRagdoll->GetRagdoll()->list[i].pConstraint->Deactivate();

		//newRagdoll->GetElement( i )->SetVelocity( &vec3_origin, &vec3_origin );
		newRagdoll->GetElement( i )->SetPosition( pos, vec3_angle, true );
		//newRagdoll->GetRagdoll()->list[i].pConstraint->Activate();
	}*/

	pRagdoll->SetNextClientThink( gpGlobals->curtime + flGibStay ); //Removal time
	pPlayer->HideRagdoll();
}
DECLARE_CLIENT_EFFECT( "DHLPlayerGib", DHLPlayerGibCallback );

void C_DHL_Player::CC_ToggleNightvision( void )
{
	C_DHL_Player* pPlayer = ToDHLPlayer(CBasePlayer::GetLocalPlayer());
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	//Always allow them to disable it in case of goofy bugs
	if ( pPlayer->m_iItemFlags & DHL_IFLAG_NIGHTVISION || pPlayer->GetNightvisionEnabled() )
		pPlayer->EnableNightvision( !pPlayer->GetNightvisionEnabled() );
}
ConCommand CC_ToggleNightvision( "togglenv", C_DHL_Player::CC_ToggleNightvision, "Toggles nightvision (if goggles possessed)." );