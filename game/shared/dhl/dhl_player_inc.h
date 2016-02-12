//DHL - Skillet - This is a bit of a hack that makes life a little easier
//:)
#ifdef CLIENT_DLL
#define CDHL_Player C_DHL_Player
#include "dhl/c_dhl_player.h"
#else
#include "dhl/dhl_player.h"
#endif
#define DHLPLAYER ToDHLPlayer(this)
#define DHL_LOCALPLAYER ToDHLPlayer(C_BasePlayer::GetLocalPlayer())